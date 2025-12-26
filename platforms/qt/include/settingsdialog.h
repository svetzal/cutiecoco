#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QComboBox;
class QCheckBox;
class QTabWidget;

// Settings dialog with tabs for CPU, Audio, and Display settings
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // Returns true if settings were changed
    bool settingsChanged() const { return m_settingsChanged; }

private slots:
    void accept() override;

private:
    void createCpuTab(QTabWidget* tabs);
    void createAudioTab(QTabWidget* tabs);
    void createDisplayTab(QTabWidget* tabs);

    void loadSettings();
    void saveSettings();

    // CPU settings
    QComboBox* m_cpuTypeCombo = nullptr;
    QComboBox* m_memorySizeCombo = nullptr;

    // Audio settings
    QComboBox* m_sampleRateCombo = nullptr;

    // Display settings
    QCheckBox* m_maintainAspectCheck = nullptr;
    QCheckBox* m_smoothScalingCheck = nullptr;

    bool m_settingsChanged = false;
};

#endif // SETTINGSDIALOG_H
