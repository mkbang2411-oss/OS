#include "GUI.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QPen>
#include <QBrush>

// ============================================================
// GanttWidget
// ============================================================

GanttWidget::GanttWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(250);
}

void GanttWidget::setData(const SimulationResult &result)
{
    data = result;
    hasData = true;

    int totalTime = 0;
    if (!data.timeline.empty())
    {
        totalTime = data.timeline.back().end;
    }

    int width = MARGIN_LEFT + qMax(800, totalTime * 60);
    setMinimumWidth(width);
    update();
}

QColor GanttWidget::colorForPID(const QString &pid)
{
    uint h = qHash(pid);
    return QColor::fromHsv(h % 360, 180, 220);
}

int GanttWidget::timeToX(int time, int totalTime, int drawWidth)
{
    if (totalTime <= 0)
        return MARGIN_LEFT;
    double pixelsPerUnit = qMax(20.0, (double)drawWidth / totalTime);
    return MARGIN_LEFT + static_cast<int>(time * pixelsPerUnit);
}

void GanttWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (!hasData || data.timeline.empty())
    {
        painter.drawText(rect(), Qt::AlignCenter, "No Gantt data");
        return;
    }

    int totalTime = data.timeline.back().end;

    // Lấy danh sách PID theo thứ tự xuất hiện (không trùng)
    QStringList pidList;
    for (const auto &entry : data.timeline)
    {
        QString pid = QString::fromStdString(entry.pid);
        if (!pidList.contains(pid))
            pidList << pid;
    }

    int drawWidth = width() - MARGIN_LEFT - 40;
    int yStart = 20;

    // Vẽ từng hàng PID
    for (int i = 0; i < pidList.size(); ++i)
    {
        int y = yStart + i * ROW_HEIGHT;

        // Nhãn PID bên trái
        painter.setPen(Qt::black);
        painter.drawText(5, y + 25, pidList[i]);

        // Vẽ các block thời gian
        for (const auto &entry : data.timeline)
        {
            QString pid = QString::fromStdString(entry.pid);
            if (pid != pidList[i])
                continue;

            int x1 = timeToX(entry.start, totalTime, drawWidth);
            int x2 = timeToX(entry.end, totalTime, drawWidth);
            QRect r(x1, y, x2 - x1, 30);

            painter.fillRect(r, colorForPID(pid));
            painter.setPen(Qt::darkGray);
            painter.drawRect(r);

            // Tên PID + khoảng thời gian bên trong block
            painter.setPen(Qt::black);
            painter.drawText(r, Qt::AlignCenter,
                             pid + QString(" [%1-%2]").arg(entry.start).arg(entry.end));
        }
    }

    // Trục thời gian phía dưới
    int axisY = yStart + pidList.size() * ROW_HEIGHT + 10;
    painter.setPen(Qt::black);
    painter.drawLine(MARGIN_LEFT, axisY, MARGIN_LEFT + drawWidth, axisY);

    for (int t = 0; t <= totalTime; t += TICK_INTERVAL)
    {
        int x = timeToX(t, totalTime, drawWidth);
        painter.drawLine(x, axisY, x, axisY + 5);
        painter.drawText(x - 5, axisY + 20, QString::number(t));
    }
}

// ============================================================
// GUI
// ============================================================

GUI::GUI(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();

    // Tự động detect USB/thẻ nhớ đang cắm
    refreshDrives();

    connect(refreshBtn, &QPushButton::clicked, this, &GUI::refreshDrives);
    connect(loadBtn, &QPushButton::clicked, this, &GUI::onLoadClicked);
    connect(fileList, &QListWidget::itemClicked, this, &GUI::onFileSelected);

    setWindowTitle("FAT32 USB Reader - OS Project 02");
    resize(1100, 700);
}

void GUI::setupUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // ── Top bar: chọn ổ đĩa + nút Refresh + nút Load ──
    QHBoxLayout *topLayout = new QHBoxLayout;
    QLabel *driveLabel = new QLabel("USB Drive:");
    driveCombo = new QComboBox;
    refreshBtn = new QPushButton("Refresh");
    loadBtn = new QPushButton("Load");

    topLayout->addWidget(driveLabel);
    topLayout->addWidget(driveCombo);
    topLayout->addWidget(refreshBtn);
    topLayout->addWidget(loadBtn);
    topLayout->addStretch();

    // ── Content: danh sách file (trái) + tabs (phải) ──
    QHBoxLayout *contentLayout = new QHBoxLayout;

    fileList = new QListWidget;
    fileList->setMinimumWidth(250);
    fileList->setMaximumWidth(300);

    tabWidget = new QTabWidget;
    tabWidget->addTab(createBootSectorTab(), "Boot Sector");
    tabWidget->addTab(createFileInfoTab(), "File Info");
    tabWidget->addTab(createGanttTab(), "Gantt Chart");

    contentLayout->addWidget(fileList, 1);
    contentLayout->addWidget(tabWidget, 3);

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(contentLayout);
}

QWidget *GUI::createBootSectorTab()
{
    QWidget *tab = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(tab);

    bootTable = new QTableWidget(7, 2);
    bootTable->setHorizontalHeaderLabels(QStringList() << "Field" << "Value");
    bootTable->verticalHeader()->setVisible(false);
    bootTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    bootTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList fieldNames = {
        "Bytes per Sector",
        "Sectors per Cluster",
        "Reserved Sectors",
        "Number of FATs",
        "Sectors per FAT",
        "Root Dir Sectors",
        "Total Sectors"};

    for (int i = 0; i < fieldNames.size(); ++i)
    {
        bootTable->setItem(i, 0, new QTableWidgetItem(fieldNames[i]));
        bootTable->setItem(i, 1, new QTableWidgetItem("-"));
    }

    layout->addWidget(bootTable);
    return tab;
}

QWidget *GUI::createFileInfoTab()
{
    QWidget *tab = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(tab);

    lblName = new QLabel("Name: ");
    lblCreated = new QLabel("Created: ");
    lblModified = new QLabel("Modified: ");
    lblSize = new QLabel("Size: ");

    // Style cho các label thông tin
    for (QLabel *lbl : {lblName, lblCreated, lblModified, lblSize})
    {
        lbl->setStyleSheet("font-size: 13px; padding: 2px;");
    }

    processTable = new QTableWidget(0, 6);
    processTable->setHorizontalHeaderLabels(QStringList()
                                            << "Process ID"
                                            << "Arrival Time"
                                            << "CPU Burst Time"
                                            << "Priority Queue ID"
                                            << "Time Slice"
                                            << "Algorithm");
    processTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    processTable->verticalHeader()->setVisible(false);
    processTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    processTable->setAlternatingRowColors(true);

    layout->addWidget(lblName);
    layout->addWidget(lblCreated);
    layout->addWidget(lblModified);
    layout->addWidget(lblSize);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("Process List:"));
    layout->addWidget(processTable);

    return tab;
}

QWidget *GUI::createGanttTab()
{
    QWidget *tab = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(tab);

    ganttScrollArea = new QScrollArea;
    ganttWidget = new GanttWidget;
    ganttScrollArea->setWidget(ganttWidget);
    ganttScrollArea->setWidgetResizable(false);
    ganttScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    lblAvgTurnaround = new QLabel("Avg Turnaround: -");
    lblAvgWaiting = new QLabel("Avg Waiting: -");

    lblAvgTurnaround->setStyleSheet("font-size: 13px; font-weight: bold;");
    lblAvgWaiting->setStyleSheet("font-size: 13px; font-weight: bold;");

    // Bảng kết quả từng process (Completion, Turnaround, Waiting)
    resultTable = new QTableWidget(0, 5);
    resultTable->setHorizontalHeaderLabels(QStringList()
                                           << "Process ID"
                                           << "Arrival"
                                           << "Burst"
                                           << "Turnaround"
                                           << "Waiting");
    resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    resultTable->verticalHeader()->setVisible(false);
    resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultTable->setAlternatingRowColors(true);
    resultTable->setMaximumHeight(200);

    layout->addWidget(ganttScrollArea, 2);
    layout->addWidget(resultTable, 1);
    layout->addWidget(lblAvgTurnaround);
    layout->addWidget(lblAvgWaiting);

    return tab;
}

// ──────────────────────────────────────────
// displayBootSector
// ──────────────────────────────────────────
void GUI::displayBootSector(const BootSector &bs)
{
    bootTable->item(0, 1)->setText(QString::number(bs.bytesPerSector));
    bootTable->item(1, 1)->setText(QString::number(bs.sectorsPerCluster));
    bootTable->item(2, 1)->setText(QString::number(bs.reservedSectors));
    bootTable->item(3, 1)->setText(QString::number(bs.numFAT));
    bootTable->item(4, 1)->setText(QString::number(bs.sectorsPerFAT));
    bootTable->item(5, 1)->setText(QString::number(bs.rootDirSectors));
    bootTable->item(6, 1)->setText(QString::number(bs.totalSectors));
}

// ──────────────────────────────────────────
// displayFileInfo
// ──────────────────────────────────────────
void GUI::displayFileInfo(const TxtFile &file)
{
    // Tên file đầy đủ
    lblName->setText("Name: " + QString::fromStdString(file.name + "." + file.extension));

    // Ngày giờ tạo — format DD/MM/YYYY  HH:MM:SS
    lblCreated->setText(QString("Created:  %1/%2/%3  %4:%5:%6")
                            .arg(file.createdDay, 2, 10, QChar('0'))
                            .arg(file.createdMonth, 2, 10, QChar('0'))
                            .arg(file.createdYear)
                            .arg(file.createdHour, 2, 10, QChar('0'))
                            .arg(file.createdMinute, 2, 10, QChar('0'))
                            .arg(file.createdSecond, 2, 10, QChar('0')));

    // Ngày giờ sửa cuối
    lblModified->setText(QString("Modified: %1/%2/%3  %4:%5:%6")
                             .arg(file.modifiedDay, 2, 10, QChar('0'))
                             .arg(file.modifiedMonth, 2, 10, QChar('0'))
                             .arg(file.modifiedYear)
                             .arg(file.modifiedHour, 2, 10, QChar('0'))
                             .arg(file.modifiedMinute, 2, 10, QChar('0'))
                             .arg(file.modifiedSecond, 2, 10, QChar('0')));

    // Kích thước
    lblSize->setText("Size: " + QString::number(file.size) + " bytes");

    // ── Parse nội dung file → queues + processes ──
    std::vector<CPUQueue> queues;
    std::vector<Process> processes;

    processTable->clearContents();
    processTable->setRowCount(0);

    if (!Scheduler::parseInputFromString(file.content, queues, processes))
    {
        QMessageBox::warning(this, "Parse Error",
                             "Không đọc được thông tin scheduling từ file này.\n"
                             "Kiểm tra lại format file theo Lab 01.");
        return;
    }

    processTable->setRowCount((int)processes.size());

    for (int i = 0; i < (int)processes.size(); ++i)
    {
        const auto &p = processes[i];
        const auto &q = queues[p.queueIndex];

        auto item = [](const QString &s)
        {
            auto *it = new QTableWidgetItem(s);
            it->setTextAlignment(Qt::AlignCenter);
            return it;
        };

        processTable->setItem(i, 0, item(QString::fromStdString(p.pid)));
        processTable->setItem(i, 1, item(QString::number(p.arrivalTime)));
        processTable->setItem(i, 2, item(QString::number(p.burstTime)));
        processTable->setItem(i, 3, item(QString::fromStdString(q.qid)));
        processTable->setItem(i, 4, item(QString::number(q.timeSlice)));
        processTable->setItem(i, 5, item(QString::fromStdString(q.algorithmName)));
    }
}

// ──────────────────────────────────────────
// runAndDisplayGantt
// ──────────────────────────────────────────
void GUI::runAndDisplayGantt(const TxtFile &file)
{
    std::vector<CPUQueue> queues;
    std::vector<Process> processes;

    if (!Scheduler::parseInputFromString(file.content, queues, processes))
        return;

    SimulationResult result = Scheduler::run(std::move(queues), std::move(processes));

    // Cập nhật Gantt widget
    ganttWidget->setData(result);

    // Cập nhật bảng kết quả từng process
    resultTable->setRowCount((int)result.processes.size());
    for (int i = 0; i < (int)result.processes.size(); ++i)
    {
        const auto &p = result.processes[i];

        auto item = [](const QString &s)
        {
            auto *it = new QTableWidgetItem(s);
            it->setTextAlignment(Qt::AlignCenter);
            return it;
        };

        resultTable->setItem(i, 0, item(QString::fromStdString(p.pid)));
        resultTable->setItem(i, 1, item(QString::number(p.arrivalTime)));
        resultTable->setItem(i, 2, item(QString::number(p.burstTime)));
        resultTable->setItem(i, 3, item(QString::number(p.turnaroundTime)));
        resultTable->setItem(i, 4, item(QString::number(p.waitingTime)));
    }

    // Avg metrics
    lblAvgTurnaround->setText(QString("Avg Turnaround: %1").arg(result.avgTurnaround, 0, 'f', 2));
    lblAvgWaiting->setText(QString("Avg Waiting:    %1").arg(result.avgWaiting, 0, 'f', 2));
}

// ──────────────────────────────────────────
// onLoadClicked
// ──────────────────────────────────────────
void GUI::onLoadClicked()
{
    QString drive = driveCombo->currentText().trimmed();
    if (drive.isEmpty())
    {
        QMessageBox::warning(this, "Error", "Please select a drive.");
        return;
    }

    try
    {
        fat.setDrive(drive.toStdString());
        fat.readBootSector();
        displayBootSector(fat.getBootSector());

        fat.scanAllTxtFiles();

        fileList->clear();
        auto files = fat.getTxtFiles();

        for (const auto &f : files)
        {
            fileList->addItem(QString::fromStdString(f.fullPath));
        }

        QMessageBox::information(this, "Done",
                                 QString("Loaded %1 .txt file(s) from %2").arg((int)files.size()).arg(drive));

        tabWidget->setCurrentIndex(0);
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "Error", e.what());
    }
    catch (...)
    {
        QMessageBox::critical(this, "Error", "Failed to read drive.");
    }
}

// ──────────────────────────────────────────
// onFileSelected
// ──────────────────────────────────────────
void GUI::onFileSelected(QListWidgetItem *item)
{
    Q_UNUSED(item);

    int index = fileList->currentRow();
    if (index < 0)
        return;

    TxtFile file = fat.getFile(index);

    displayFileInfo(file);
    runAndDisplayGantt(file);

    tabWidget->setCurrentIndex(1);
}

void GUI::refreshDrives()
{
    driveCombo->clear();

#ifdef _WIN32
    DWORD drives = GetLogicalDrives(); // mỗi bit = 1 ổ đĩa (bit0=A, bit1=B, bit2=C...)
    QStringList removable;
    QStringList allDrives;

    for (int i = 0; i < 26; i++)
    {
        if (!(drives & (1 << i)))
            continue; // ổ không tồn tại
        if (i < 2)
            continue; // bỏ A: B:

        QString letter = QString(QChar('A' + i)) + ":";
        QString path = letter + "\\";

        UINT type = GetDriveTypeA(path.toStdString().c_str());
        if (type == DRIVE_REMOVABLE)
        {
            removable << letter; // USB hoặc thẻ nhớ
        }
        else
        {
            allDrives << letter;
        }
    }

    // Ưu tiên hiện removable — nếu không có thì hiện tất cả
    QStringList &list = removable.isEmpty() ? allDrives : removable;
    for (const QString &d : list)
        driveCombo->addItem(d);

#else
    driveCombo->addItem("E:");
#endif
}