#include "settingsdialog.h"
#include "appconfig.h"
#include "cutie/emulator.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QGroupBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setMinimumWidth(400);

    auto* mainLayout = new QVBoxLayout(this);

    auto* tabs = new QTabWidget(this);
    createCpuTab(tabs);
    createAudioTab(tabs);
    createDisplayTab(tabs);

    mainLayout->addWidget(tabs);

    // Dialog buttons
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    loadSettings();
}

void SettingsDialog::createCpuTab(QTabWidget* tabs)
{
    auto* tab = new QWidget();
    auto* layout = new QVBoxLayout(tab);

    // CPU type group
    auto* cpuGroup = new QGroupBox(tr("Processor"), tab);
    auto* cpuLayout = new QFormLayout(cpuGroup);

    m_cpuTypeCombo = new QComboBox(cpuGroup);
    m_cpuTypeCombo->addItem(tr("Motorola MC6809"), static_cast<int>(cutie::CpuType::MC6809));
    m_cpuTypeCombo->addItem(tr("Hitachi HD6309"), static_cast<int>(cutie::CpuType::HD6309));
    cpuLayout->addRow(tr("CPU Type:"), m_cpuTypeCombo);

    layout->addWidget(cpuGroup);

    // Memory group
    auto* memGroup = new QGroupBox(tr("Memory"), tab);
    auto* memLayout = new QFormLayout(memGroup);

    m_memorySizeCombo = new QComboBox(memGroup);
    m_memorySizeCombo->addItem(tr("128 KB"), static_cast<int>(cutie::MemorySize::Mem128K));
    m_memorySizeCombo->addItem(tr("512 KB"), static_cast<int>(cutie::MemorySize::Mem512K));
    m_memorySizeCombo->addItem(tr("2 MB"), static_cast<int>(cutie::MemorySize::Mem2M));
    m_memorySizeCombo->addItem(tr("8 MB"), static_cast<int>(cutie::MemorySize::Mem8M));
    memLayout->addRow(tr("RAM Size:"), m_memorySizeCombo);

    layout->addWidget(memGroup);

    // Note about restart requirement
    auto* noteLabel = new QLabel(
        tr("<i>Note: CPU and memory changes require a restart to take effect.</i>"),
        tab);
    noteLabel->setWordWrap(true);
    layout->addWidget(noteLabel);

    layout->addStretch();

    tabs->addTab(tab, tr("CPU"));
}

void SettingsDialog::createAudioTab(QTabWidget* tabs)
{
    auto* tab = new QWidget();
    auto* layout = new QVBoxLayout(tab);

    auto* audioGroup = new QGroupBox(tr("Audio Output"), tab);
    auto* audioLayout = new QFormLayout(audioGroup);

    m_sampleRateCombo = new QComboBox(audioGroup);
    m_sampleRateCombo->addItem(tr("22050 Hz"), 22050);
    m_sampleRateCombo->addItem(tr("44100 Hz (Recommended)"), 44100);
    m_sampleRateCombo->addItem(tr("48000 Hz"), 48000);
    audioLayout->addRow(tr("Sample Rate:"), m_sampleRateCombo);

    layout->addWidget(audioGroup);
    layout->addStretch();

    tabs->addTab(tab, tr("Audio"));
}

void SettingsDialog::createDisplayTab(QTabWidget* tabs)
{
    auto* tab = new QWidget();
    auto* layout = new QVBoxLayout(tab);

    auto* displayGroup = new QGroupBox(tr("Display Options"), tab);
    auto* displayLayout = new QVBoxLayout(displayGroup);

    m_maintainAspectCheck = new QCheckBox(tr("Maintain 4:3 aspect ratio"), displayGroup);
    displayLayout->addWidget(m_maintainAspectCheck);

    m_smoothScalingCheck = new QCheckBox(tr("Smooth scaling (bilinear filtering)"), displayGroup);
    displayLayout->addWidget(m_smoothScalingCheck);

    layout->addWidget(displayGroup);
    layout->addStretch();

    tabs->addTab(tab, tr("Display"));
}

void SettingsDialog::loadSettings()
{
    AppConfig& config = AppConfig::instance();

    // CPU settings
    int cpuTypeIdx = m_cpuTypeCombo->findData(static_cast<int>(config.cpuType()));
    if (cpuTypeIdx >= 0) {
        m_cpuTypeCombo->setCurrentIndex(cpuTypeIdx);
    }

    int memSizeIdx = m_memorySizeCombo->findData(static_cast<int>(config.memorySize()));
    if (memSizeIdx >= 0) {
        m_memorySizeCombo->setCurrentIndex(memSizeIdx);
    }

    // Audio settings
    int sampleRateIdx = m_sampleRateCombo->findData(config.audioSampleRate());
    if (sampleRateIdx >= 0) {
        m_sampleRateCombo->setCurrentIndex(sampleRateIdx);
    }

    // Display settings
    m_maintainAspectCheck->setChecked(config.maintainAspectRatio());
    m_smoothScalingCheck->setChecked(config.smoothScaling());
}

void SettingsDialog::saveSettings()
{
    AppConfig& config = AppConfig::instance();

    // CPU settings
    auto newCpuType = static_cast<cutie::CpuType>(m_cpuTypeCombo->currentData().toInt());
    auto newMemSize = static_cast<cutie::MemorySize>(m_memorySizeCombo->currentData().toInt());

    if (newCpuType != config.cpuType()) {
        config.setCpuType(newCpuType);
        m_settingsChanged = true;
    }

    if (newMemSize != config.memorySize()) {
        config.setMemorySize(newMemSize);
        m_settingsChanged = true;
    }

    // Audio settings
    uint32_t newSampleRate = m_sampleRateCombo->currentData().toUInt();
    if (newSampleRate != config.audioSampleRate()) {
        config.setAudioSampleRate(newSampleRate);
        m_settingsChanged = true;
    }

    // Display settings
    bool newMaintainAspect = m_maintainAspectCheck->isChecked();
    bool newSmoothScaling = m_smoothScalingCheck->isChecked();

    if (newMaintainAspect != config.maintainAspectRatio()) {
        config.setMaintainAspectRatio(newMaintainAspect);
        m_settingsChanged = true;
    }

    if (newSmoothScaling != config.smoothScaling()) {
        config.setSmoothScaling(newSmoothScaling);
        m_settingsChanged = true;
    }

    config.sync();
}

void SettingsDialog::accept()
{
    saveSettings();
    QDialog::accept();
}
