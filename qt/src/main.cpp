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
    app.setApplicationVersion("0.2.0");
    app.setOrganizationName("CutieCoCo");

    // Set the system ROM path before creating the main window
    auto romPath = getSystemRomPath();
    fprintf(stderr, "ROM path: %s\n", romPath.string().c_str());
    SetSystemRomPath(romPath);

    fprintf(stderr, "Creating main window...\n");
    MainWindow window;
    fprintf(stderr, "Showing window...\n");
    window.show();

    fprintf(stderr, "Entering event loop...\n");
    return app.exec();
}
