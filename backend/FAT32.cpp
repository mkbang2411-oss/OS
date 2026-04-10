#include <iostream>
#include <windows.h>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include "FAT32.h"
using namespace std;
static unsigned int readUnit16(const unsigned char* p){// vì fat32 chỉ lưu bằng 2 byte nên dùng short
    return (unsigned int) (p[0]| (p[1]<<8)); // p[0] là byte thấp p[1] là byte cao, p[1] dịch sang trái 8 bit ghép với p[0]
}
static unsigned int readUnit32(const unsigned char* p){
    return (unsigned int) (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3] << 24));
}
static string trimRightSpace (const string& s){
    int end = (int) s.size()-1;
    while (end >= 0 && s[end] == ' '){
        --end;
    }
    return s.substr(0, end+1);
}
static string toUpperString(string s){
    for (char&c : s){
        c = (char) toupper((unsigned char)c);
    }
    return s;
}
void FAT32::setDrive(const string& driveLetter){
    if (driveLetter.size()>=2 && driveLetter[1] == ':'){
        drivePath = "\\\\.\\" + driveLetter.substr(0,2); 
    }
    else {
        drivePath = driveLetter;
    }
}
bool FAT32::readSector(int sectorIndex, unsigned char* buffer){
    HANDLE hDrive = CreateFileA(//mở ổ đĩa
        drivePath.c_str(),//lấy chuỗi
        GENERIC_READ,//mở để đọc
        FILE_SHARE_READ | FILE_SHARE_WRITE,// cho phép tiến trình khác đọc | ghi cùng lúc
        NULL,//không dùng thuộc tính bảo mật riêng
        OPEN_EXISTING,//mở cái đang tồn tại, không tạo mới
        0,//không có cờ đặc biệt
        NULL//
    );
    if (hDrive == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER offset;//vị trí byte trong ổ đĩa
    offset.QuadPart=(LONGLONG) sectorIndex*boot.bytesPerSector;
    if (!SetFilePointerEx(hDrive, offset, NULL, FILE_BEGIN)){
        CloseHandle(hDrive);
        return false;
    }
    DWORD bytesRead = 0;
    BOOL ok = ReadFile(hDrive, buffer, boot.bytesPerSector,&bytesRead, NULL);
    //đọc từ hrive ghi vào buffer, đọc boot.bytesPerSector ghi vào bytesRead
    CloseHandle(hDrive);
    return ok&& bytesRead == (DWORD)boot.bytesPerSector; // trả về true khi readfile đọc thành công và đock đủ đúng số byte của 1 sector
}
void FAT32::readBootSector(){
    if (drivePath.empty()) {
        throw runtime_error("Chua set drive. Hay goi setDrive() truoc");
    }
    unsigned char buffer[512] = {0}; // khi đọc boot sector, chưa biết bytesPerSector, FAT32 thường là 512
    HANDLE hDrive = CreateFileA(//mở ổ đĩa
        drivePath.c_str(),//lấy chuỗi
        GENERIC_READ,//mở để đọc
        FILE_SHARE_READ | FILE_SHARE_WRITE,// cho phép tiến trình khác đọc | ghi cùng lúc
        NULL,//không dùng thuộc tính bảo mật riêng
        OPEN_EXISTING,//mở cái đang tồn tại, không tạo mới
        0,//không có cờ đặc biệt
        NULL//
    );
    if (hDrive == INVALID_HANDLE_VALUE) {
        throw runtime_error("Khong mo duoc o dia");
    }
    DWORD bytesRead = 0;
    SetFilePointer(hDrive, 0 , NULL, FILE_BEGIN);
    const DWORD bootSectorSize = 512;
    BOOL ok = ReadFile(hDrive, buffer, boot.bytesPerSector,&bytesRead, NULL);
    //đọc từ hrive ghi vào buffer, đọc boot.bytesPerSector ghi vào bytesRead
    CloseHandle(hDrive);
    if (!ok || bytesRead!= bootSectorSize){
        throw runtime_error("Khong doc doc Boost Sector");
    }
    if (buffer[510] != 0x55 || buffer[511] != 0xAA)
        throw runtime_error("Boot Sector khong hop le");
    boot.bytesPerSector = readUnit16(buffer + 0x0B); // 1 sector có bao nhiêu byte
    boot.sectorsPerCluster = buffer[0x0D]; // một cluster có bao nhiêu byte
    boot.reservedSectors = readUnit16(buffer + 0x0E);// số sector dành cho boot sector và vùng system
    boot.numFAT = buffer[0x10]; //số bảng fat
    boot.totalSectors = (int) readUnit32(buffer + 0x20);// tổng số sector của usb
    boot.sectorsPerFAT = (int) readUnit32(buffer + 0x24);//mỗi bảng fat chiếm bao nhiêu sector
    boot.rootCluster = (int) readUnit32 (buffer + 0x2C);// cluster bắt đầu của thư mục gôc
    boot.rootDirSectors = 0;// fat 32 không có vùng root directory riêng
}
BootSector FAT32:: getBootSector(){
    return boot;
}
int FAT32:: clusterToSector(int cluster){ // đổi từ cluster sang sector
    int firstDataSector = boot.reservedSectors + boot.numFAT * boot.sectorsPerFAT;// data bắt đầu từ sector?
    return firstDataSector + (cluster-2) * boot.sectorsPerCluster;
}
// KB
void FAT32::parseDirectory(int startCluster, const std::string &currentPath)
{
}
void FAT32::scanAllTxtFiles()
{
    files.clear();
    parseDirectory(boot.rootCluster);
}

vector<TxtFile> FAT32::getTxtFiles()
{
    return files;
}

TxtFile FAT32::getFile(int index)
{
    return files[index];
}