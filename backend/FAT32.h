#pragma once
#include <string>
#include <vector>
using namespace std;

/*
 * OFFSET THAM KHAO - DIRECTORY ENTRY (moi entry = 32 byte):
 *   0x00-07 : Ten file (8 byte, padding bang 0x20)
 *   0x08-0A : Extension (3 byte)
 *   0x0B    : Attribute - 0x10 = DIRECTORY, 0x20 = ARCHIVE (file thuong)
 *   0x0E-0F : Created Time  (bit 15-11=gio, 10-5=phut, 4-0=giay/2)
 *   0x10-11 : Created Date  (bit 15-9=nam-1980, 8-5=thang, 4-0=ngay)
 *   0x16-17 : Last Write Time
 *   0x18-19 : Last Write Date
 *   0x14-15 : First Cluster HIGH (2 byte)
 *   0x1A-1B : First Cluster LOW  (2 byte)
 *   0x1C-1F : File Size (4 byte, little-endian)
 *
 * First Cluster = (HIGH << 16) | LOW
 */

struct BootSector
{
    int bytesPerSector;    // offset 0x0B, 2 byte
    int sectorsPerCluster; // offset 0x0D, 1 byte
    int reservedSectors;   // offset 0x0E, 2 byte  (so sector vung Boot)
    int numFAT;            // offset 0x10, 1 byte
    int sectorsPerFAT;     // offset 0x24, 4 byte  (FAT32 dung 0x24, khong phai 0x16)
    int rootDirSectors;    // FAT32 = 0 (RDET nam trong Data Region, khong co vung rieng)
    int totalSectors;      // offset 0x20, 4 byte
    int rootCluster;       // offset 0x2C, 4 byte  (cluster dau RDET, thuong = 2)
};

struct TxtFile
{
    // Ten file
    string name;      // 8 ky tu (bo trailing space)
    string extension; // 3 ky tu (e.g. "TXT")

    // Kich thuoc
    int size; // byte, doc tu offset 0x1C

    // Ngay/gio TAO (Created): offset 0x0E + 0x10
    int createdHour;
    int createdMinute;
    int createdSecond; // = field_value * 2
    int createdDay;
    int createdMonth;
    int createdYear; // = field_value + 1980

    // Ngay/gio SUA CUOI (Last Write): offset 0x16 + 0x18
    int modifiedHour;
    int modifiedMinute;
    int modifiedSecond;
    int modifiedDay;
    int modifiedMonth;
    int modifiedYear;

    // Cluster
    int firstCluster;         // (HIGH << 16) | LOW
    vector<int> clusterChain; // chuoi cluster doc tu bang FAT
    string content;           // noi dung file (doc du size byte)
};

class FAT32
{
private:
    string drivePath;
    BootSector boot;
    vector<TxtFile> files;

    /*
     * readSector: doc sector thu 'sectorIndex' vao buffer 512 byte
     * Dung CreateFile("\\\\.\\E:") + SetFilePointerEx + ReadFile
     * Offset (byte) = sectorIndex * bytesPerSector
     */
    bool readSector(int sectorIndex, unsigned char *buffer);

    /*
     * clusterToSector: tinh sector tuyet doi cua cluster
     * firstDataSector = reservedSectors + numFAT * sectorsPerFAT
     * ket qua      = firstDataSector + (cluster - 2) * sectorsPerCluster
     */
    int clusterToSector(int cluster);

    /*
     * getFATEntry: doc gia tri 4 byte tai vi tri cluster trong vung FAT
     * Byte offset trong FAT = cluster * 4
     * => sector chua no   = reservedSectors + (cluster * 4) / bytesPerSector
     * => offset trong sector = (cluster * 4) % bytesPerSector
     * Tra ve (value & 0x0FFFFFFF)
     * Neu >= 0x0FFFFFF8 thi la cluster cuoi chuoi
     */
    int getFATEntry(int cluster);

    /*
     * readClusterChain: loop getFATEntry tu firstCluster
     * den khi gap cluster cuoi hoac cluster = 0
     */
    vector<int> readClusterChain(int firstCluster);

    /*
     * readFileContent: doc toan bo cluster chain
     * Moi cluster = sectorsPerCluster * bytesPerSector byte
     * Chi giu 'fileSize' byte dau, bo phan du o cluster cuoi
     */
    string readFileContent(const vector<int> &chain, int fileSize);

    /*
     * parseDirectory: duyet entry trong directory bat dau tu startCluster
     * Moi entry = 32 byte, moi sector chua (bytesPerSector / 32) entry
     * - entry[0] == 0x00 : het (break)
     * - entry[0] == 0xE5 : da xoa (skip)
     * - attribute == 0x10 && ten != "." && ten != ".." : folder -> de quy
     * - attribute == 0x20 && extension == "TXT" : doc thong tin + content -> push vao files
     */
    void parseDirectory(int startCluster);

public:
    void setDrive(const string &path);

    void readBootSector(); // doc sector 0, parse cac truong
    BootSector getBootSector();

    void scanAllTxtFiles(); // goi parseDirectory(boot.rootCluster)
    vector<TxtFile> getTxtFiles();
    TxtFile getFile(int index);
};