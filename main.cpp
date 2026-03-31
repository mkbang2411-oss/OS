#include <QApplication>
#include "frontend/GUI.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    GUI window;
    window.setWindowTitle("FAT32 Reader - OS Project 02");
    window.resize(1000, 650);
    window.show();

    return app.exec();
}