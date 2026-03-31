#pragma once
#include <QMainWindow>
#include <QListWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QWidget>
#include <QPainter>

#include "../backend/FAT32.h"
#include "../backend/Scheduler.h"

// ============================================================
// GanttWidget - ve so do Gantt bang QPainter
// ============================================================
class GanttWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GanttWidget(QWidget *parent = nullptr);

    // Nap ket qua tu Scheduler va goi update() de ve lai
    void setData(const SimulationResult &result);

protected:
    /*
     * paintEvent: Qt tu goi khi can ve lai
     * Cach ve:
     *   1. Tinh totalTime = timeline cuoi .end
     *   2. Moi GanttEntry: ve hinh chu nhat tu timeToX(start) den timeToX(end)
     *      Chieu cao moi hang = 40px, moi PID 1 hang
     *   3. Ve nhan ten process ben trong hinh chu nhat
     *   4. Ve truc thoi gian phia duoi voi cac moc so
     */
    void paintEvent(QPaintEvent *event) override;

private:
    SimulationResult data;
    bool hasData = false;

    QColor colorForPID(const QString &pid); // tra mau khac nhau theo PID
    int timeToX(int time, int totalTime, int drawWidth, int marginLeft);
};

// ============================================================
// GUI - cua so chinh
// Layout:
//   [ComboBox chon o dia]  [Nut Load]
//   ┌──────────────┬──────────────────────────────────────┐
//   │ QListWidget  │ QTabWidget                           │
//   │ (danh sach   │   Tab 1: Boot Sector (QTableWidget)  │
//   │  file .txt)  │   Tab 2: Thong tin file + noi dung  │
//   │              │   Tab 3: Gantt Chart                 │
//   └──────────────┴──────────────────────────────────────┘
// ============================================================
class GUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit GUI(QWidget *parent = nullptr);

private slots:
    /*
     * onLoadClicked:
     *   1. Lay ten o dia tu driveCombo (e.g. "E:")
     *   2. fat.setDrive("\\\\.\\E:")
     *   3. fat.readBootSector() -> displayBootSector()
     *   4. fat.scanAllTxtFiles() -> hien ten file len fileList
     */
    void onLoadClicked();

    /*
     * onFileSelected:
     *   1. Lay index tu fileList
     *   2. fat.getFile(index) -> displayFileInfo()
     *   3. Chay Scheduler::parseInputFromString(file.content, ...)
     *      -> Scheduler::run(...) -> ganttWidget->setData(result)
     *   4. tabWidget->setCurrentIndex(1) de hien tab File Info
     */
    void onFileSelected(QListWidgetItem *item);

private:
    FAT32 fat;

    // --- Top bar ---
    QComboBox *driveCombo;
    QPushButton *loadBtn;

    // --- Left panel ---
    QListWidget *fileList;

    // --- Right panel ---
    QTabWidget *tabWidget;

    // Tab 1: Boot Sector
    QTableWidget *bootTable;
    /*
     * Cau truc bang (2 cot: "Truong", "Gia tri"):
     *   Bytes per Sector        | ...
     *   Sectors per Cluster     | ...
     *   Reserved Sectors        | ...
     *   Number of FATs          | ...
     *   Sectors per FAT         | ...
     *   Root Dir Sectors        | ...
     *   Total Sectors           | ...
     */

    // Tab 2: Thong tin file
    QLabel *lblName;             // "Filename: ABCDE.TXT"
    QLabel *lblSize;             // "Size: 1234 bytes"
    QLabel *lblCreated;          // "Created: DD/MM/YYYY HH:MM:SS"
    QLabel *lblModified;         // "Modified: DD/MM/YYYY HH:MM:SS"
    QPlainTextEdit *contentView; // Hien thi noi dung file (read-only)
    /*
     * contentView->setReadOnly(true)
     * contentView->setPlainText(QString::fromStdString(file.content))
     */

    // Tab 3: Gantt Chart
    GanttWidget *ganttWidget;

    // --- Ham khoi tao ---
    void setupUI();
    QWidget *createBootSectorTab();
    QWidget *createFileInfoTab();
    QWidget *createGanttTab();

    // --- Ham hien thi du lieu ---
    void displayBootSector(const BootSector &bs);
    void displayFileInfo(const TxtFile &file);
    void runAndDisplayGantt(const TxtFile &file);
};