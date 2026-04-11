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

#ifdef _WIN32
#include <windows.h>
#endif

#include "../backend/FAT32.h"
#include "../backend/Scheduler.h"

class GanttWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GanttWidget(QWidget *parent = nullptr);

    void setData(const SimulationResult &result);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    SimulationResult data;
    bool hasData = false;

    static const int ROW_HEIGHT = 40;    // Chieu cao moi hang PID
    static const int MARGIN_LEFT = 60;   // Rong lề trái (de hien nhan queue)
    static const int MARGIN_BOTTOM = 30; // Rong le duoi (truc thoi gian)
    static const int TICK_INTERVAL = 1;  // Moc tren truc thoi gian (don vi)

    QColor colorForPID(const QString &pid);

    int timeToX(int time, int totalTime, int drawWidth);
};

class GUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit GUI(QWidget *parent = nullptr);

private slots:

    void onLoadClicked();

    void onFileSelected(QListWidgetItem *item);

private:
    FAT32 fat;

    // --- Top bar ---
    QComboBox *driveCombo;   // Ky tu o dia detect tu dong
    QPushButton *loadBtn;    // "Load"
    QPushButton *refreshBtn; // "Refresh" - quet lai danh sach o dia

    // --- Left panel ---
    QListWidget *fileList; // Hien fullPath cua tung TxtFile

    // --- Right panel ---
    QTabWidget *tabWidget;

    // ---- Tab 0: Boot Sector ----
    QTableWidget *bootTable;

    // ---- Tab 1: File Info ----
    QLabel *lblName;            // "Name: FILENAME.TXT"
    QLabel *lblCreated;         // "Created: DD/MM/YYYY  HH:MM:SS"
    QLabel *lblModified;        // "Modified: DD/MM/YYYY  HH:MM:SS"
    QLabel *lblSize;            // "Size: 1234 bytes"
    QTableWidget *processTable; // Bang thong tin cac process doc tu file

    // ---- Tab 2: Gantt Chart ----
    QScrollArea *ganttScrollArea;
    GanttWidget *ganttWidget;
    QTableWidget *resultTable;

    // ---- Thanh trang thai ket qua (phia duoi Tab 2) ----
    QLabel *lblAvgTurnaround;
    QLabel *lblAvgWaiting;

    // --- Ham khoi tao giao dien ---
    void setupUI();
    void refreshDrives();
    QWidget *createBootSectorTab();
    QWidget *createFileInfoTab();
    QWidget *createGanttTab();

    void displayBootSector(const BootSector &bs);
    void displayFileInfo(const TxtFile &file);
    void runAndDisplayGantt(const TxtFile &file);
};