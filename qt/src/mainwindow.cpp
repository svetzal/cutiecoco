#include "mainwindow.h"
#include "emulatorwidget.h"

#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QApplication>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_emulatorWidget(new EmulatorWidget(this))
{
    fprintf(stderr, "MainWindow: setting up...\n");
    setWindowTitle("CutieCoCo - CoCo 3 Emulator");
    setCentralWidget(m_emulatorWidget);

    createMenus();
    createStatusBar();

    // Size window to fit the emulator widget at native resolution
    // plus room for menu bar and status bar
    adjustSize();
    fprintf(stderr, "MainWindow: setup complete, scheduling emulation start...\n");

    // Start emulation when window is shown
    QTimer::singleShot(0, this, [this]() {
        fprintf(stderr, "MainWindow: starting emulation...\n");
        m_emulatorWidget->startEmulation();
        fprintf(stderr, "MainWindow: emulation started\n");
    });

    // Update FPS in status bar periodically
    auto fpsTimer = new QTimer(this);
    connect(fpsTimer, &QTimer::timeout, this, [this]() {
        if (m_emulatorWidget->isEmulationRunning()) {
            float fps = m_emulatorWidget->getFps();
            statusBar()->showMessage(QString("FPS: %1").arg(fps, 0, 'f', 1));
        }
    });
    fpsTimer->start(500);
}

MainWindow::~MainWindow()
{
    m_emulatorWidget->stopEmulation();
}

void MainWindow::createMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, qApp, &QApplication::quit);

    // Emulator menu
    QMenu *emulatorMenu = menuBar()->addMenu(tr("&Emulator"));
    emulatorMenu->addAction(tr("&Reset"), [this]() {
        m_emulatorWidget->resetEmulation();
    });

    m_pauseAction = emulatorMenu->addAction(tr("&Pause"), [this]() {
        if (m_emulatorWidget->isEmulationPaused()) {
            m_emulatorWidget->resumeEmulation();
            m_pauseAction->setText(tr("&Pause"));
        } else {
            m_emulatorWidget->pauseEmulation();
            m_pauseAction->setText(tr("&Resume"));
        }
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
