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
#include <QScrollArea>

#include "../backend/FAT32.h"
#include "../backend/Scheduler.h"

// ============================================================
// GanttWidget - ve bieu do Gantt bang QPainter
// Dat trong QScrollArea de cuon ngang khi bieu do dai.
// ============================================================
class GanttWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GanttWidget(QWidget *parent = nullptr);

    /*
     * setData: nap ket qua tu Scheduler va goi update() de ve lai.
     * Sau khi goi xong, setMinimumWidth() theo totalTime de scrollArea hoat dong.
     */
    void setData(const SimulationResult &result);

protected:
    /*
     * paintEvent: Qt tu goi khi can ve lai.
     *
     * Cach ve:
     *   1. totalTime = timeline.back().end  (hoac 0 neu rong).
     *   2. Lay danh sach PID duy nhat -> moi PID 1 dong ngang.
     *   3. Moi GanttEntry: ve QRect tu timeToX(start) den timeToX(end),
     *      chieu cao ROW_HEIGHT = 40px, fillRect bang colorForPID(pid).
     *   4. Ve ten PID va khoang thoi gian ben trong hinh chu nhat.
     *   5. Ve truc thoi gian phia duoi: mot moc moi TICK_INTERVAL don vi.
     *   6. Ve nhan Queue ID phia trai.
     */
    void paintEvent(QPaintEvent *event) override;

private:
    SimulationResult data;
    bool hasData = false;

    static const int ROW_HEIGHT = 40;    // Chieu cao moi hang PID
    static const int MARGIN_LEFT = 60;   // Rong lề trái (de hien nhan queue)
    static const int MARGIN_BOTTOM = 30; // Rong le duoi (truc thoi gian)
    static const int TICK_INTERVAL = 1;  // Moc tren truc thoi gian (don vi)

    /*
     * colorForPID: tra ve QColor khac nhau cho moi PID.
     * Dung hash de map PID -> mau, dam bao cung PID cung mau.
     */
    QColor colorForPID(const QString &pid);

    /*
     * timeToX: chuyen doi thoi gian thanh toa do X tren widget.
     *   x = MARGIN_LEFT + time * pixelsPerUnit
     *   pixelsPerUnit = drawWidth / totalTime  (toi thieu 20px/don vi)
     */
    int timeToX(int time, int totalTime, int drawWidth);
};

// ============================================================
// GUI - cua so chinh
//
// Layout:
//   [ComboBox chon o dia "E:", "F:", ...]  [Nut "Load"]
//   ┌──────────────┬────────────────────────────────────────┐
//   │ QListWidget  │ QTabWidget                             │
//   │ (danh sach   │  Tab 0: Boot Sector (QTableWidget)     │
//   │  file .txt,  │  Tab 1: File Info + Process Table      │
//   │  hien        │  Tab 2: Gantt Chart (QScrollArea)      │
//   │  fullPath)   │                                        │
//   └──────────────┴────────────────────────────────────────┘
// ============================================================
class GUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit GUI(QWidget *parent = nullptr);

private slots:
    /*
     * onLoadClicked:
     *   1. Lay letter tu driveCombo (e.g. "E:").
     *   2. fat.setDrive("E:").
     *   3. fat.readBootSector() -> displayBootSector(fat.getBootSector()).
     *   4. fat.scanAllTxtFiles().
     *   5. Xoa fileList, them fullPath cua tung TxtFile vao.
     *   6. Hien thong bao so file tim duoc.
     */
    void onLoadClicked();

    /*
     * onFileSelected:
     *   1. Lay index tu currentRow() cua fileList.
     *   2. TxtFile f = fat.getFile(index).
     *   3. displayFileInfo(f) -> dien Tab 1.
     *   4. runAndDisplayGantt(f) -> dien Tab 2.
     *   5. tabWidget->setCurrentIndex(1).
     */
    void onFileSelected(QListWidgetItem *item);

private:
    FAT32 fat;

    // --- Top bar ---
    QComboBox *driveCombo; // Cac ky tu o dia: "C:", "D:", "E:", "F:", "G:"
    QPushButton *loadBtn;  // "Load"

    // --- Left panel ---
    QListWidget *fileList; // Hien fullPath cua tung TxtFile

    // --- Right panel ---
    QTabWidget *tabWidget;

    // ---- Tab 0: Boot Sector ----
    QTableWidget *bootTable;
    /*
     * 7 dong, 2 cot ("Truong", "Gia tri"):
     *   Bytes per Sector      | ...
     *   Sectors per Cluster   | ...
     *   Reserved Sectors      | ...
     *   Number of FATs        | ...
     *   Sectors per FAT       | ...
     *   Root Dir Sectors      | ...
     *   Total Sectors         | ...
     */

    // ---- Tab 1: File Info ----
    QLabel *lblName;            // "Name: FILENAME.TXT"
    QLabel *lblCreated;         // "Created: DD/MM/YYYY  HH:MM:SS"
    QLabel *lblSize;            // "Size: 1234 bytes"
    QTableWidget *processTable; // Bang thong tin cac process doc tu file
    /*
     * processTable co 6 cot (theo Function 3):
     *   "Process ID" | "Arrival Time" | "CPU Burst Time"
     *   | "Priority Queue ID" | "Time Slice" | "Algorithm"
     *
     * So dong = so process doc duoc tu noi dung file.
     * Moi dong dien tu 1 Process + CPUQueue tuong ung.
     *
     * "Priority Queue ID" = CPUQueue.qid (e.g. "Q1")
     * "Time Slice"        = CPUQueue.timeSlice
     * "Algorithm"         = CPUQueue.algorithmName
     */

    // ---- Tab 2: Gantt Chart ----
    QScrollArea *ganttScrollArea; // Cho phep cuon ngang khi bieu do dai
    GanttWidget *ganttWidget;     // Widget ve bieu do

    // ---- Thanh trang thai ket qua (phia duoi Tab 2) ----
    QLabel *lblAvgTurnaround; // "Avg Turnaround: X.XX"
    QLabel *lblAvgWaiting;    // "Avg Waiting: X.XX"

    // --- Ham khoi tao giao dien ---
    void setupUI();
    QWidget *createBootSectorTab(); // Tao widget cho Tab 0
    QWidget *createFileInfoTab();   // Tao widget cho Tab 1
    QWidget *createGanttTab();      // Tao widget cho Tab 2

    // --- Ham hien thi du lieu ---

    /*
     * displayBootSector: dien bootTable voi 7 truong tu BootSector.
     * Dung setItem(row, 0, new QTableWidgetItem(ten))
     *      setItem(row, 1, new QTableWidgetItem(QString::number(gia_tri)))
     */
    void displayBootSector(const BootSector &bs);

    /*
     * displayFileInfo: dien cac label va processTable tu TxtFile.
     *   1. lblName->setText(...)
     *   2. lblCreated->setText(...)
     *   3. lblSize->setText(...)
     *   4. Parse file.content bang Scheduler::parseInputFromString(...)
     *      de lay queues va processes.
     *   5. processTable->setRowCount(processes.size()).
     *   6. Moi process: dien 6 cot (lay timeSlice va algorithm tu queues[p.queueIndex]).
     */
    void displayFileInfo(const TxtFile &file);

    /*
     * runAndDisplayGantt: chay scheduling va cap nhat GanttWidget.
     *   1. Parse lai tu file.content -> queues + processes.
     *   2. SimulationResult r = Scheduler::run(move(queues), move(processes)).
     *   3. ganttWidget->setData(r).
     *   4. lblAvgTurnaround / lblAvgWaiting cap nhat.
     *   5. Dat lai minimumWidth cho ganttWidget de scrollArea hoat dong.
     */
    void runAndDisplayGantt(const TxtFile &file);
};