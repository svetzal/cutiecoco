#include "mainwindow.h"
#include "emulatorwidget.h"
#include "appconfig.h"
#include "settingsdialog.h"
#include "aboutdialog.h"
#include "cutie/emulator.h"

#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QApplication>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_emulatorWidget(new EmulatorWidget(this))
{
    setWindowTitle("CutieCoCo - CoCo 3 Emulator");
    setCentralWidget(m_emulatorWidget);

    createMenus();
    createStatusBar();

    // Restore window state or size to fit the emulator widget
    restoreWindowState();

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

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveWindowState();
    QMainWindow::closeEvent(event);
}

void MainWindow::createMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    // Insert cartridge action
    fileMenu->addAction(tr("&Insert Cartridge..."), [this]() {
        // Pause emulation while loading
        bool wasPaused = m_emulatorWidget->isEmulationPaused();
        if (!wasPaused) {
            m_emulatorWidget->pauseEmulation();
        }

        QString startDir = AppConfig::instance().lastCartridgeDir();
        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Insert Cartridge"),
            startDir,
            tr("ROM Files (*.rom *.ccc *.pak);;All Files (*)"));

        if (!fileName.isEmpty()) {
            // Remember directory
            QFileInfo fi(fileName);
            AppConfig::instance().setLastCartridgeDir(fi.absolutePath());

            loadCartridge(fileName);
        }

        // Resume emulation if it wasn't paused before
        if (!wasPaused) {
            m_emulatorWidget->resumeEmulation();
        }
    });

    // Recent files submenu
    m_recentFilesMenu = fileMenu->addMenu(tr("Recent &Cartridges"));
    updateRecentFilesMenu();

    fileMenu->addAction(tr("&Eject Cartridge"), [this]() {
        auto* emu = m_emulatorWidget->emulator();
        if (emu && emu->hasCartridge()) {
            emu->ejectCartridge();
            statusBar()->showMessage(tr("Cartridge ejected"), 2000);
        }
    });

    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, qApp, &QApplication::quit);

    // Edit menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(tr("&Settings..."), QKeySequence::Preferences, [this]() {
        SettingsDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted && dialog.settingsChanged()) {
            // Notify user that restart may be needed
            QMessageBox::information(this, tr("Settings Changed"),
                tr("Some settings may require restarting the emulator to take effect."));
        }
    });

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
        AboutDialog dialog(this);
        dialog.exec();
    });
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::updateRecentFilesMenu()
{
    m_recentFilesMenu->clear();

    QStringList recentFiles = AppConfig::instance().recentCartridges();
    if (recentFiles.isEmpty()) {
        QAction* noRecent = m_recentFilesMenu->addAction(tr("(No recent files)"));
        noRecent->setEnabled(false);
    } else {
        for (const QString& path : recentFiles) {
            QFileInfo fi(path);
            QString displayName = fi.fileName();
            QAction* action = m_recentFilesMenu->addAction(displayName);
            action->setData(path);
            action->setToolTip(path);
            connect(action, &QAction::triggered, this, [this, path]() {
                loadCartridge(path);
            });
        }

        m_recentFilesMenu->addSeparator();
        m_recentFilesMenu->addAction(tr("Clear Recent Files"), [this]() {
            AppConfig::instance().clearRecentCartridges();
            updateRecentFilesMenu();
        });
    }
}

void MainWindow::loadCartridge(const QString& path)
{
    auto* emu = m_emulatorWidget->emulator();
    if (!emu) return;

    if (emu->loadCartridge(path.toStdString())) {
        // Add to recent files
        AppConfig::instance().addRecentCartridge(path);
        updateRecentFilesMenu();

        statusBar()->showMessage(
            tr("Loaded: %1").arg(QString::fromStdString(emu->getCartridgeName())),
            3000);
    } else {
        QMessageBox::warning(this, tr("Load Error"),
            tr("Failed to load cartridge:\n%1").arg(
                QString::fromStdString(emu->getLastError())));
    }
}

void MainWindow::restoreWindowState()
{
    QByteArray geometry = AppConfig::instance().windowGeometry();
    QByteArray state = AppConfig::instance().windowState();

    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    } else {
        // Default: size to fit emulator widget
        adjustSize();
    }

    if (!state.isEmpty()) {
        QMainWindow::restoreState(state);
    }
}

void MainWindow::saveWindowState()
{
    AppConfig::instance().setWindowGeometry(saveGeometry());
    AppConfig::instance().setWindowState(QMainWindow::saveState());
    AppConfig::instance().sync();
}
