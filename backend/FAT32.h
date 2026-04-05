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
 *   0x14-15 : First Cluster HIGH (2 byte)
 *   0x16-17 : Last Write Time
 *   0x18-19 : Last Write Date
 *   0x1A-1B : First Cluster LOW  (2 byte)
 *   0x1C-1F : File Size (4 byte, little-endian)
 *
 * First Cluster = (HIGH << 16) | LOW
 *
 * LUU Y: Khong dung fopen/ifstream/filesystem.
 * Doc bang CreateFile + SetFilePointerEx + ReadFile (Windows API).
 */

// ============================================================
// BootSector - cac truong can thiet tu Boot Record
// (Function 1 hien thi bang nay)
// ============================================================
struct BootSector
{
    int bytesPerSector;    // offset 0x0B, 2 byte
    int sectorsPerCluster; // offset 0x0D, 1 byte
    int reservedSectors;   // offset 0x0E, 2 byte  (so sector vung Boot)
    int numFAT;            // offset 0x10, 1 byte
    int sectorsPerFAT;     // offset 0x24, 4 byte  (FAT32 dung 0x24)
    int rootDirSectors;    // FAT32 = 0 (RDET nam trong Data Region)
    int totalSectors;      // offset 0x20, 4 byte
    int rootCluster;       // offset 0x2C, 4 byte  (thuong = 2)
};

// ============================================================
// TxtFile - thong tin 1 file .txt tim duoc tren dia
// (Function 2 liet ke, Function 3 hien thi chi tiet)
// ============================================================
struct TxtFile
{
    // --- Ten file ---
    string name;      // 8 ky tu (bo trailing space)
    string extension; // 3 ky tu (luon la "TXT")
    string fullPath;  // Duong dan day du ke ca thu muc (e.g. "/FOLDER/FILE.TXT")
                      // Dung de hien thi o danh sach (Function 2)

    // --- Kich thuoc ---
    int size; // byte, doc tu offset 0x1C

    // --- Ngay/gio TAO (Created): offset 0x0E (time) + 0x10 (date) ---
    int createdHour;
    int createdMinute;
    int createdSecond; // = field_value * 2

    int createdDay;
    int createdMonth;
    int createdYear; // = field_value + 1980

    // --- Ngay/gio SUA CUOI (Last Write): offset 0x16 (time) + 0x18 (date) ---
    int modifiedHour;
    int modifiedMinute;
    int modifiedSecond;

    int modifiedDay;
    int modifiedMonth;
    int modifiedYear;

    // --- Cluster ---
    int firstCluster;         // (HIGH << 16) | LOW
    vector<int> clusterChain; // Chuoi cluster doc tu bang FAT
    string content;           // Noi dung file (doc du 'size' byte)
};

// ============================================================
// FAT32 - doc cau truc FAT32 tren o nho ngoai
// ============================================================
class FAT32
{
private:
    string drivePath; // e.g. "\\\\.\\E:"
    BootSector boot;
    vector<TxtFile> files;

    /*
     * readSector: doc sector thu 'sectorIndex' vao buffer.
     *   - Mo o dia bang CreateFile(drivePath, GENERIC_READ, FILE_SHARE_READ, ...)
     *   - Tinh offset = (LONGLONG)sectorIndex * bytesPerSector
     *   - SetFilePointerEx(hDrive, offset, NULL, FILE_BEGIN)
     *   - ReadFile(hDrive, buffer, bytesPerSector, &bytesRead, NULL)
     *   - Tra ve false neu loi.
     * buffer phai co kich thuoc >= bytesPerSector (thuong 512).
     */
    bool readSector(int sectorIndex, unsigned char *buffer);

    /*
     * clusterToSector: tinh LBA cua cluster.
     *   firstDataSector = reservedSectors + numFAT * sectorsPerFAT
     *   return firstDataSector + (cluster - 2) * sectorsPerCluster
     */
    int clusterToSector(int cluster);

    /*
     * getFATEntry: doc gia tri FAT32 tai vi tri 'cluster'.
     *   byteOffset = cluster * 4
     *   fatSector  = reservedSectors + byteOffset / bytesPerSector
     *   offsetInSector = byteOffset % bytesPerSector
     *   Doc 4 byte tai vi tri do, mask 0x0FFFFFFF.
     *   Neu ket qua >= 0x0FFFFFF8 thi la cluster cuoi chuoi (EOC).
     */
    int getFATEntry(int cluster);

    /*
     * readClusterChain: xay dung danh sach cluster tu firstCluster.
     *   Loop: them cluster vao ket qua, next = getFATEntry(cluster).
     *   Dung khi next >= 0x0FFFFFF8 hoac next == 0.
     */
    vector<int> readClusterChain(int firstCluster);

    /*
     * readFileContent: doc noi dung file tu cluster chain.
     *   Moi cluster: doc sectorsPerCluster sector lien tiep.
     *   Moi sector: readSector -> them vao string.
     *   Chi giu 'fileSize' byte dau tien (cat phan du o cluster cuoi).
     *   Tra ve string chua noi dung.
     */
    string readFileContent(const vector<int> &chain, int fileSize);

    /*
     * parseDirectory: duyet tat ca entry trong 1 directory.
     *   startCluster: cluster dau tien cua directory (RDET: boot.rootCluster).
     *   currentPath:  duong dan hien tai (de xay dung fullPath cho TxtFile).
     *
     *   Moi sector chua (bytesPerSector / 32) entry (moi entry = 32 byte).
     *   Xu ly tung entry:
     *     - entry[0] == 0x00  : het directory (break).
     *     - entry[0] == 0xE5  : entry da xoa (skip).
     *     - entry[0x0B] == 0x0F : Long File Name entry (skip).
     *     - attribute == 0x10 && ten != "." && ten != ".." : thu muc con
     *         -> de quy parseDirectory(firstCluster, currentPath + "/" + name).
     *     - attribute == 0x20 && extension == "TXT"
     *         -> doc cac truong vao TxtFile, doc content, push vao files.
     */
    void parseDirectory(int startCluster, const string &currentPath = "");

    /*
     * parseDateField: giai ma 2 byte Date (FAT format) thanh day/month/year.
     *   bit 15-9 : year offset (+ 1980)
     *   bit  8-5 : month
     *   bit  4-0 : day
     */
    void parseDateField(unsigned short raw, int &day, int &month, int &year);

    /*
     * parseTimeField: giai ma 2 byte Time (FAT format) thanh hour/min/sec.
     *   bit 15-11 : hour
     *   bit  10-5 : minute
     *   bit   4-0 : second/2 (nhan 2 de ra giay that)
     */
    void parseTimeField(unsigned short raw, int &hour, int &minute, int &second);

public:
    /*
     * setDrive: dat duong dan o dia.
     *   path vi du: "E:" -> doi thanh "\\\\.\\E:" o ben trong.
     */
    void setDrive(const string &driveLetter);

    /*
     * readBootSector: doc sector 0, parse vao struct boot.
     * Can goi truoc scanAllTxtFiles() va getBootSector().
     */
    void readBootSector();

    /*
     * getBootSector: tra ve struct BootSector sau khi doc.
     */
    BootSector getBootSector();

    /*
     * scanAllTxtFiles: xoa mang files, goi parseDirectory(boot.rootCluster).
     * Can goi readBootSector() truoc.
     */
    void scanAllTxtFiles();

    /*
     * getTxtFiles: tra ve danh sach tat ca file .txt da scan.
     */
    vector<TxtFile> getTxtFiles();

    /*
     * getFile: tra ve TxtFile tai vi tri index (0-based).
     * Khong kiem tra gioi han - caller tu bao dam index hop le.
     */
    TxtFile getFile(int index);
};