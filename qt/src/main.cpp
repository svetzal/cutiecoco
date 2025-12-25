// DREAM-VCC Qt Application
// CoCo 3 Emulator

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DREAM-VCC");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("DREAM");

    MainWindow window;
    window.show();

    return app.exec();
}
