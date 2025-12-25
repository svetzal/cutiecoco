// CutieCoCo Qt Application
// A cross-platform Tandy CoCo 3 Emulator

#include <QApplication>
#include <QDir>
#include <filesystem>
#include "mainwindow.h"
#include "dream/stubs.h"

// Determine the path to system ROMs based on platform
static std::filesystem::path getSystemRomPath()
{
    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MACOS
    // macOS app bundle: Resources is ../Resources relative to MacOS
    QDir dir(appDir);
    dir.cdUp();  // From MacOS to Contents
    return std::filesystem::path(dir.filePath("Resources/system-roms").toStdString());
#else
    // Linux and Windows: system-roms next to executable
    return std::filesystem::path(appDir.toStdString()) / "system-roms";
#endif
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("CutieCoCo");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("CutieCoCo");

    // Set the system ROM path before creating the main window
    SetSystemRomPath(getSystemRomPath());

    MainWindow window;
    window.show();

    return app.exec();
}
