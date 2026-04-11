#include <iostream>
#include <windows.h>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include "FAT32.h"
using namespace std;

// Helper: đọc 2 byte little-endian
static unsigned int readUint16(const unsigned char *p)
{
    return (unsigned int)(p[0] | (p[1] << 8));
}

// Helper: đọc 4 byte little-endian
static unsigned int readUint32(const unsigned char *p)
{
    return (unsigned int)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

// Helper: cắt trailing space
static string trimRightSpace(const string &s)
{
    int end = (int)s.size() - 1;
    while (end >= 0 && s[end] == ' ')
        --end;
    return s.substr(0, end + 1);
}

// Helper: uppercase string
static string toUpperString(string s)
{
    for (char &c : s)
        c = (char)toupper((unsigned char)c);
    return s;
}

void FAT32::setDrive(const string &driveLetter)
{
    if (driveLetter.size() >= 2 && driveLetter[1] == ':')
    {
        drivePath = "\\\\.\\" + driveLetter.substr(0, 2);
    }
    else
    {
        drivePath = driveLetter;
    }
}

bool FAT32::readSector(int sectorIndex, unsigned char *buffer)
{
    HANDLE hDrive = CreateFileA(
        drivePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (hDrive == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER offset;
    offset.QuadPart = (LONGLONG)sectorIndex * boot.bytesPerSector;

    if (!SetFilePointerEx(hDrive, offset, NULL, FILE_BEGIN))
    {
        CloseHandle(hDrive);
        return false;
    }

    DWORD bytesRead = 0;
    BOOL ok = ReadFile(hDrive, buffer, boot.bytesPerSector, &bytesRead, NULL);
    CloseHandle(hDrive);
    return ok && bytesRead == (DWORD)boot.bytesPerSector;
}

void FAT32::readBootSector()
{
    if (drivePath.empty())
        throw runtime_error("Chua set drive. Hay goi setDrive() truoc");

    unsigned char buffer[512] = {0};

    HANDLE hDrive = CreateFileA(
        drivePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (hDrive == INVALID_HANDLE_VALUE)
        throw runtime_error("Khong mo duoc o dia");

    DWORD bytesRead = 0;
    SetFilePointer(hDrive, 0, NULL, FILE_BEGIN);
    BOOL ok = ReadFile(hDrive, buffer, 512, &bytesRead, NULL);
    CloseHandle(hDrive);

    if (!ok || bytesRead != 512)
        throw runtime_error("Khong doc duoc Boot Sector");
    if (buffer[510] != 0x55 || buffer[511] != 0xAA)
        throw runtime_error("Boot Sector khong hop le");

    boot.bytesPerSector = readUint16(buffer + 0x0B);
    boot.sectorsPerCluster = buffer[0x0D];
    boot.reservedSectors = readUint16(buffer + 0x0E);
    boot.numFAT = buffer[0x10];
    boot.totalSectors = (int)readUint32(buffer + 0x20);
    boot.sectorsPerFAT = (int)readUint32(buffer + 0x24);
    boot.rootCluster = (int)readUint32(buffer + 0x2C);
    boot.rootDirSectors = 0;
}

BootSector FAT32::getBootSector() { return boot; }

int FAT32::clusterToSector(int cluster)
{
    int firstDataSector = boot.reservedSectors + boot.numFAT * boot.sectorsPerFAT;
    return firstDataSector + (cluster - 2) * boot.sectorsPerCluster;
}

int FAT32::getFATEntry(int cluster)
{
    int byteOffset = cluster * 4;
    int fatSector = boot.reservedSectors + byteOffset / boot.bytesPerSector;
    int offsetInSect = byteOffset % boot.bytesPerSector;

    vector<unsigned char> buf(boot.bytesPerSector);
    if (!readSector(fatSector, buf.data()))
        return 0x0FFFFFFF;

    unsigned int val = readUint32(buf.data() + offsetInSect);
    return (int)(val & 0x0FFFFFFF);
}

vector<int> FAT32::readClusterChain(int firstCluster)
{
    vector<int> chain;
    int current = firstCluster;

    while (current >= 2 && current < 0x0FFFFFF8)
    {
        chain.push_back(current);
        current = getFATEntry(current);
    }
    return chain;
}

string FAT32::readFileContent(const vector<int> &chain, int fileSize)
{
    string content;
    content.reserve(fileSize);

    vector<unsigned char> sectorBuf(boot.bytesPerSector);

    for (int cluster : chain)
    {
        int startSector = clusterToSector(cluster);
        for (int s = 0; s < boot.sectorsPerCluster; s++)
        {
            if ((int)content.size() >= fileSize)
                goto done;
            if (!readSector(startSector + s, sectorBuf.data()))
                break;
            for (int b = 0; b < boot.bytesPerSector; b++)
            {
                if ((int)content.size() >= fileSize)
                    goto done;
                content += (char)sectorBuf[b];
            }
        }
    }
done:
    return content;
}

void FAT32::parseDateField(unsigned short raw, int &day, int &month, int &year)
{
    day = raw & 0x1F;
    month = (raw >> 5) & 0x0F;
    year = ((raw >> 9) & 0x7F) + 1980;
}

void FAT32::parseTimeField(unsigned short raw, int &hour, int &minute, int &second)
{
    second = (raw & 0x1F) * 2;
    minute = (raw >> 5) & 0x3F;
    hour = (raw >> 11) & 0x1F;
}

void FAT32::parseDirectory(int startCluster, const string &currentPath)
{
    vector<int> dirChain = readClusterChain(startCluster);
    vector<unsigned char> sectorBuf(boot.bytesPerSector);

    bool done = false;

    for (int cluster : dirChain)
    {
        if (done)
            break;
        int startSector = clusterToSector(cluster);

        for (int s = 0; s < boot.sectorsPerCluster && !done; s++)
        {
            if (!readSector(startSector + s, sectorBuf.data()))
                continue;

            int entriesPerSector = boot.bytesPerSector / 32;

            for (int e = 0; e < entriesPerSector && !done; e++)
            {
                const unsigned char *entry = sectorBuf.data() + e * 32;

                if (entry[0] == 0x00)
                {
                    done = true;
                    break;
                } // hết thư mục
                if (entry[0] == 0xE5)
                    continue; // đã xóa

                unsigned char attr = entry[0x0B];
                if (attr == 0x0F)
                    continue; // Long File Name

                // Đọc tên 8.3
                string name8 = "";
                for (int i = 0; i < 8; i++)
                    name8 += (char)entry[i];
                name8 = trimRightSpace(name8);

                string ext3 = "";
                for (int i = 8; i < 11; i++)
                    ext3 += (char)entry[i];
                ext3 = trimRightSpace(ext3);

                // Lấy firstCluster từ HIGH(0x14) và LOW(0x1A)
                unsigned int clusterHigh = readUint16(entry + 0x14);
                unsigned int clusterLow = readUint16(entry + 0x1A);
                int firstCluster = (int)((clusterHigh << 16) | clusterLow);

                // === Thư mục con ===
                if (attr == 0x10)
                {
                    if (name8 != "." && name8 != "..")
                    {
                        parseDirectory(firstCluster, currentPath + "/" + name8);
                    }
                    continue;
                }

                // === File thường: chỉ lấy .TXT ===
                if (attr == 0x20)
                {
                    string extUpper = toUpperString(ext3);
                    if (extUpper != "TXT")
                        continue;

                    TxtFile f;
                    f.name = name8;
                    f.extension = extUpper;
                    f.fullPath = currentPath + "/" + name8 + ".TXT";
                    f.size = (int)readUint32(entry + 0x1C);

                    unsigned short cTime = (unsigned short)readUint16(entry + 0x0E);
                    unsigned short cDate = (unsigned short)readUint16(entry + 0x10);
                    parseTimeField(cTime, f.createdHour, f.createdMinute, f.createdSecond);
                    parseDateField(cDate, f.createdDay, f.createdMonth, f.createdYear);

                    unsigned short mTime = (unsigned short)readUint16(entry + 0x16);
                    unsigned short mDate = (unsigned short)readUint16(entry + 0x18);
                    parseTimeField(mTime, f.modifiedHour, f.modifiedMinute, f.modifiedSecond);
                    parseDateField(mDate, f.modifiedDay, f.modifiedMonth, f.modifiedYear);

                    f.firstCluster = firstCluster;
                    f.clusterChain = readClusterChain(firstCluster);
                    f.content = readFileContent(f.clusterChain, f.size);

                    files.push_back(f);
                }
            }
        }
    }
}

void FAT32::scanAllTxtFiles()
{
    files.clear();
    parseDirectory(boot.rootCluster);
}

vector<TxtFile> FAT32::getTxtFiles() { return files; }
TxtFile FAT32::getFile(int index) { return files[index]; }