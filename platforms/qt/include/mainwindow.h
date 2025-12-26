#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QAction;
class QMenu;
class EmulatorWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void createMenus();
    void createStatusBar();
    void updateRecentFilesMenu();
    void loadCartridge(const QString& path);
    void restoreWindowState();
    void saveWindowState();

    EmulatorWidget *m_emulatorWidget;
    QAction *m_pauseAction = nullptr;
    QMenu *m_recentFilesMenu = nullptr;
};

#endif // MAINWINDOW_H
