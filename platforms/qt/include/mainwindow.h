#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QAction;
class EmulatorWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void createMenus();
    void createStatusBar();

    EmulatorWidget *m_emulatorWidget;
    QAction *m_pauseAction = nullptr;
};

#endif // MAINWINDOW_H
