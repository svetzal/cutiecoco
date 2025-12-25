#include "mainwindow.h"
#include "emulatorwidget.h"

#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_emulatorWidget(new EmulatorWidget(this))
{
    setWindowTitle("DREAM-VCC - CoCo 3 Emulator");
    setCentralWidget(m_emulatorWidget);

    createMenus();
    createStatusBar();

    // Set default size (CoCo 3 native resolution scaled 2x)
    resize(640, 480);
}

MainWindow::~MainWindow() = default;

void MainWindow::createMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, qApp, &QApplication::quit);

    // Emulator menu
    QMenu *emulatorMenu = menuBar()->addMenu(tr("&Emulator"));
    emulatorMenu->addAction(tr("&Reset"), []() {
        // TODO: Implement reset
    });

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), [this]() {
        // TODO: Show about dialog
    });
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}
