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

#include <QDir>
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
    if (!data.timeline.empty()) {
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
    if (totalTime <= 0) return MARGIN_LEFT;
    double pixelsPerUnit = qMax(20.0, (double)drawWidth / totalTime);
    return MARGIN_LEFT + static_cast<int>(time * pixelsPerUnit);
}

void GanttWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (!hasData || data.timeline.empty()) {
        painter.drawText(rect(), Qt::AlignCenter, "No Gantt data");
        return;
    }

    int totalTime = data.timeline.back().end;

    QStringList pidList;
    for (const auto &entry : data.timeline) {
        if (!pidList.contains(QString::fromStdString(entry.pid))) {
            pidList << QString::fromStdString(entry.pid);
        }
    }

    int drawWidth = width() - MARGIN_LEFT - 40;
    int yStart = 20;

    for (int i = 0; i < pidList.size(); ++i) {
        int y = yStart + i * ROW_HEIGHT;
        painter.drawText(10, y + 25, pidList[i]);

        for (const auto &entry : data.timeline) {
            QString pid = QString::fromStdString(entry.pid);
            if (pid != pidList[i]) continue;

            int x1 = timeToX(entry.start, totalTime, drawWidth);
            int x2 = timeToX(entry.end, totalTime, drawWidth);
            QRect r(x1, y, x2 - x1, 30);

            painter.fillRect(r, colorForPID(pid));
            painter.drawRect(r);
            painter.drawText(r, Qt::AlignCenter,
                             pid + QString(" [%1-%2]").arg(entry.start).arg(entry.end));
        }
    }

    int axisY = yStart + pidList.size() * ROW_HEIGHT + 20;
    painter.drawLine(MARGIN_LEFT, axisY, MARGIN_LEFT + drawWidth, axisY);

    for (int t = 0; t <= totalTime; t += TICK_INTERVAL) {
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

#ifdef _WIN32
    driveCombo->clear();
    driveCombo->addItem("C:");
    driveCombo->addItem("D:");
#else
    driveCombo->addItem("E:");
#endif

    connect(loadBtn, &QPushButton::clicked, this, &GUI::onLoadClicked);
    connect(fileList, &QListWidget::itemClicked, this, &GUI::onFileSelected);

    setWindowTitle("FAT32 USB Reader");
    resize(1100, 700);
}

void GUI::setupUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Top bar
    QHBoxLayout *topLayout = new QHBoxLayout;
    QLabel *driveLabel = new QLabel("USB Drive:");
    driveCombo = new QComboBox;
    loadBtn = new QPushButton("Load");

    topLayout->addWidget(driveLabel);
    topLayout->addWidget(driveCombo);
    topLayout->addWidget(loadBtn);
    topLayout->addStretch();

    // Main area
    QHBoxLayout *contentLayout = new QHBoxLayout;

    fileList = new QListWidget;
    fileList->setMinimumWidth(250);

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
        "Root Cluster / Root Dir",
        "Total Sectors"
    };

    for (int i = 0; i < fieldNames.size(); ++i) {
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
    lblSize = new QLabel("Size: ");

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

    layout->addWidget(lblName);
    layout->addWidget(lblCreated);
    layout->addWidget(lblSize);
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

    lblAvgTurnaround = new QLabel("Avg Turnaround: -");
    lblAvgWaiting = new QLabel("Avg Waiting: -");

    layout->addWidget(ganttScrollArea);
    layout->addWidget(lblAvgTurnaround);
    layout->addWidget(lblAvgWaiting);

    return tab;
}

void GUI::displayBootSector(const BootSector &bs)
{
    // Đổi tên field nếu FAT32.h của bạn đặt tên khác
    bootTable->item(0, 1)->setText(QString::number(bs.bytesPerSector));
    bootTable->item(1, 1)->setText(QString::number(bs.sectorsPerCluster));
    bootTable->item(2, 1)->setText(QString::number(bs.reservedSectors));
    bootTable->item(3, 1)->setText(QString::number(bs.numFAT));
    bootTable->item(4, 1)->setText(QString::number(bs.sectorsPerFAT));
    bootTable->item(5, 1)->setText(QString::number(bs.rootCluster)); // hoặc rootDirSectors
    bootTable->item(6, 1)->setText(QString::number(bs.totalSectors));
}

void GUI::displayFileInfo(const TxtFile &file)
{
    lblName->setText("Name: " + QString::fromStdString(file.name));
    lblCreated->setText("Created: " + QString::number(file.createdDay));
    lblSize->setText("Size: " + QString::number(file.size) + " bytes");

    processTable->clearContents();
    processTable->setRowCount(0);

    std::vector<CPUQueue> queues;
    std::vector<Process> processes;

    Scheduler::parseInputFromString(file.content, queues, processes);

    processTable->setRowCount((int)processes.size());

    for (int i = 0; i < (int)processes.size(); ++i) {
        const auto &p = processes[i];
        const auto &q = queues[p.queueIndex];

        processTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(p.pid)));
        processTable->setItem(i, 1, new QTableWidgetItem(QString::number(p.arrivalTime)));
        processTable->setItem(i, 2, new QTableWidgetItem(QString::number(p.burstTime)));
        processTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(q.qid)));
        processTable->setItem(i, 4, new QTableWidgetItem(QString::number(q.timeSlice)));
        processTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(q.algorithmName)));
    }
}

void GUI::runAndDisplayGantt(const TxtFile &file)
{
    std::vector<CPUQueue> queues;
    std::vector<Process> processes;

    Scheduler::parseInputFromString(file.content, queues, processes);
    SimulationResult result = Scheduler::run(std::move(queues), std::move(processes));

    ganttWidget->setData(result);

    lblAvgTurnaround->setText(QString("Avg Turnaround: %1").arg(result.avgTurnaround, 0, 'f', 2));
    lblAvgWaiting->setText(QString("Avg Waiting: %1").arg(result.avgWaiting, 0, 'f', 2));
}

void GUI::onLoadClicked()
{
    QString drive = driveCombo->currentText().trimmed();
    if (drive.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a drive.");
        return;
    }

    try {
        fat.setDrive(drive.toStdString());
        QMessageBox::information(this, "Debug", "setDrive OK");

        fat.readBootSector();
        QMessageBox::information(this, "Debug", "readBootSector OK");
        
        displayBootSector(fat.getBootSector());
        QMessageBox::information(this, "Debug", "displayBootSector OK");

        fat.scanAllTxtFiles();
        QMessageBox::information(this, "Debug", "scanAllTxtFiles OK");

        fileList->clear();

        auto files = fat.getTxtFiles();
        int count = static_cast<int>(files.size());

        for (const auto &f : files) {
            fileList->addItem(QString::fromStdString(f.fullPath));
        }

        QMessageBox::information(this, "Done",
                                 QString("Loaded %1 file(s) from %2").arg(count).arg(drive));
        tabWidget->setCurrentIndex(0);
    }
    catch (const std::exception &e) {
        QMessageBox::critical(this, "Error", e.what());
    }
    catch (...) {
        QMessageBox::critical(this, "Error", "Failed to load USB drive.");
    }
}

void GUI::onFileSelected(QListWidgetItem *item)
{
    Q_UNUSED(item);

    int index = fileList->currentRow();
    if (index < 0) return;

    TxtFile file = fat.getFile(index);

    displayFileInfo(file);
    runAndDisplayGantt(file);

    tabWidget->setCurrentIndex(1);
}