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
    setWindowTitle("DREAM-VCC - CoCo 3 Emulator");
    setCentralWidget(m_emulatorWidget);

    createMenus();
    createStatusBar();

    // Set default size (CoCo 3 native resolution scaled 2x)
    resize(640, 480);

    // Start emulation when window is shown
    QTimer::singleShot(0, this, [this]() {
        m_emulatorWidget->startEmulation();
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
