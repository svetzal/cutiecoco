#include "mainwindow.h"
#include "emulatorwidget.h"
#include "cutie/emulator.h"

#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QApplication>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_emulatorWidget(new EmulatorWidget(this))
{
    setWindowTitle("CutieCoCo - CoCo 3 Emulator");
    setCentralWidget(m_emulatorWidget);

    createMenus();
    createStatusBar();

    // Size window to fit the emulator widget at native resolution
    // plus room for menu bar and status bar
    adjustSize();

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

    // Cartridge submenu
    fileMenu->addAction(tr("&Insert Cartridge..."), [this]() {
        // Pause emulation while loading
        bool wasPaused = m_emulatorWidget->isEmulationPaused();
        if (!wasPaused) {
            m_emulatorWidget->pauseEmulation();
        }

        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Insert Cartridge"),
            QString(),
            tr("ROM Files (*.rom *.ccc *.pak);;All Files (*)"));

        if (!fileName.isEmpty()) {
            auto* emu = m_emulatorWidget->emulator();
            if (emu) {
                if (emu->loadCartridge(fileName.toStdString())) {
                    statusBar()->showMessage(
                        tr("Loaded: %1").arg(QString::fromStdString(emu->getCartridgeName())),
                        3000);
                } else {
                    QMessageBox::warning(this, tr("Load Error"),
                        tr("Failed to load cartridge:\n%1").arg(
                            QString::fromStdString(emu->getLastError())));
                }
            }
        }

        // Resume emulation if it wasn't paused before
        if (!wasPaused) {
            m_emulatorWidget->resumeEmulation();
        }
    });

    fileMenu->addAction(tr("&Eject Cartridge"), [this]() {
        auto* emu = m_emulatorWidget->emulator();
        if (emu && emu->hasCartridge()) {
            emu->ejectCartridge();
            statusBar()->showMessage(tr("Cartridge ejected"), 2000);
        }
    });

    fileMenu->addSeparator();
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
