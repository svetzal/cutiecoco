// CutieCoCo Qt Application
// A cross-platform Tandy CoCo 3 Emulator

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("CutieCoCo");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("CutieCoCo");

    MainWindow window;
    window.show();

    return app.exec();
}
