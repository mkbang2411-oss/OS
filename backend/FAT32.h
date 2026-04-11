#pragma once
#include <string>
#include <vector>
using namespace std;

struct BootSector
{
    int bytesPerSector = 512;
    int sectorsPerCluster = 0;
    int reservedSectors = 0;
    int numFAT = 0;
    int sectorsPerFAT;
    int rootDirSectors;
    int totalSectors = 0;
    int rootCluster = 0;
};

// TxtFile - thong tin 1 file .txt tim duoc tren dia
struct TxtFile
{
    // --- Ten file ---
    string name;
    string extension;
    string fullPath;

    // --- Kich thuoc ---
    int size;

    // --- Ngay/gio TAO (Created): offset 0x0E (time) + 0x10 (date) ---
    int createdHour;
    int createdMinute;
    int createdSecond;

    int createdDay;
    int createdMonth;
    int createdYear;

    // --- Ngay/gio SUA CUOI (Last Write): offset 0x16 (time) + 0x18 (date) ---
    int modifiedHour;
    int modifiedMinute;
    int modifiedSecond;

    int modifiedDay;
    int modifiedMonth;
    int modifiedYear;

    // --- Cluster ---
    int firstCluster;
    vector<int> clusterChain;
    string content;
};

// FAT32 - doc cau truc FAT32 tren o nho ngoai
class FAT32
{
private:
    string drivePath;
    BootSector boot;
    vector<TxtFile> files;

    bool readSector(int sectorIndex, unsigned char *buffer);

    int clusterToSector(int cluster);

    int getFATEntry(int cluster);

    vector<int> readClusterChain(int firstCluster);

    string readFileContent(const vector<int> &chain, int fileSize);

    void parseDirectory(int startCluster, const string &currentPath = "");

    void parseDateField(unsigned short raw, int &day, int &month, int &year);

    void parseTimeField(unsigned short raw, int &hour, int &minute, int &second);

public:
    void setDrive(const string &driveLetter);

    void readBootSector();

    BootSector getBootSector();

    void scanAllTxtFiles();

    vector<TxtFile> getTxtFiles();

    TxtFile getFile(int index);
};