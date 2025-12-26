#include "appconfig.h"
#include "cutie/emulator.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>

namespace {
    // Settings keys
    const QString KEY_MEMORY_SIZE = QStringLiteral("emulator/memorySize");
    const QString KEY_CPU_TYPE = QStringLiteral("emulator/cpuType");
    const QString KEY_AUDIO_SAMPLE_RATE = QStringLiteral("emulator/audioSampleRate");
    const QString KEY_SYSTEM_ROM_PATH = QStringLiteral("paths/systemRom");
    const QString KEY_LAST_CARTRIDGE_DIR = QStringLiteral("paths/lastCartridgeDir");
    const QString KEY_RECENT_CARTRIDGES = QStringLiteral("recentFiles/cartridges");
    const QString KEY_WINDOW_GEOMETRY = QStringLiteral("window/geometry");
    const QString KEY_WINDOW_STATE = QStringLiteral("window/state");
    const QString KEY_MAINTAIN_ASPECT = QStringLiteral("display/maintainAspectRatio");
    const QString KEY_SMOOTH_SCALING = QStringLiteral("display/smoothScaling");

    // Convert MemorySize enum to/from int for storage
    int memorySizeToInt(cutie::MemorySize size)
    {
        switch (size) {
            case cutie::MemorySize::Mem128K: return 128;
            case cutie::MemorySize::Mem512K: return 512;
            case cutie::MemorySize::Mem2M: return 2048;
            case cutie::MemorySize::Mem8M: return 8192;
            default: return 512;
        }
    }

    cutie::MemorySize intToMemorySize(int kb)
    {
        switch (kb) {
            case 128: return cutie::MemorySize::Mem128K;
            case 512: return cutie::MemorySize::Mem512K;
            case 2048: return cutie::MemorySize::Mem2M;
            case 8192: return cutie::MemorySize::Mem8M;
            default: return cutie::MemorySize::Mem512K;
        }
    }

    // Convert CpuType enum to/from string for storage
    QString cpuTypeToString(cutie::CpuType type)
    {
        switch (type) {
            case cutie::CpuType::MC6809: return QStringLiteral("6809");
            case cutie::CpuType::HD6309: return QStringLiteral("6309");
            default: return QStringLiteral("6809");
        }
    }

    cutie::CpuType stringToCpuType(const QString& str)
    {
        if (str == QStringLiteral("6309")) {
            return cutie::CpuType::HD6309;
        }
        return cutie::CpuType::MC6809;
    }
}

AppConfig::AppConfig()
    : QObject(nullptr)
{
}

AppConfig& AppConfig::instance()
{
    static AppConfig config;
    return config;
}

cutie::MemorySize AppConfig::memorySize() const
{
    QSettings settings;
    int kb = settings.value(KEY_MEMORY_SIZE, 512).toInt();
    return intToMemorySize(kb);
}

void AppConfig::setMemorySize(cutie::MemorySize size)
{
    QSettings settings;
    settings.setValue(KEY_MEMORY_SIZE, memorySizeToInt(size));
    emit memorySizeChanged(size);
}

cutie::CpuType AppConfig::cpuType() const
{
    QSettings settings;
    QString str = settings.value(KEY_CPU_TYPE, QStringLiteral("6809")).toString();
    return stringToCpuType(str);
}

void AppConfig::setCpuType(cutie::CpuType type)
{
    QSettings settings;
    settings.setValue(KEY_CPU_TYPE, cpuTypeToString(type));
    emit cpuTypeChanged(type);
}

uint32_t AppConfig::audioSampleRate() const
{
    QSettings settings;
    return settings.value(KEY_AUDIO_SAMPLE_RATE, 44100).toUInt();
}

void AppConfig::setAudioSampleRate(uint32_t rate)
{
    QSettings settings;
    settings.setValue(KEY_AUDIO_SAMPLE_RATE, rate);
    emit audioSampleRateChanged(rate);
}

QString AppConfig::systemRomPath() const
{
    QSettings settings;
    return settings.value(KEY_SYSTEM_ROM_PATH).toString();
}

void AppConfig::setSystemRomPath(const QString& path)
{
    QSettings settings;
    settings.setValue(KEY_SYSTEM_ROM_PATH, path);
}

QString AppConfig::lastCartridgeDir() const
{
    QSettings settings;
    QString dir = settings.value(KEY_LAST_CARTRIDGE_DIR).toString();
    if (dir.isEmpty()) {
        // Default to documents folder
        dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    return dir;
}

void AppConfig::setLastCartridgeDir(const QString& dir)
{
    QSettings settings;
    settings.setValue(KEY_LAST_CARTRIDGE_DIR, dir);
}

QStringList AppConfig::recentCartridges() const
{
    QSettings settings;
    return settings.value(KEY_RECENT_CARTRIDGES).toStringList();
}

void AppConfig::addRecentCartridge(const QString& path)
{
    QSettings settings;
    QStringList recent = settings.value(KEY_RECENT_CARTRIDGES).toStringList();

    // Remove if already exists (will re-add at front)
    recent.removeAll(path);

    // Add to front
    recent.prepend(path);

    // Limit size
    while (recent.size() > MAX_RECENT_FILES) {
        recent.removeLast();
    }

    settings.setValue(KEY_RECENT_CARTRIDGES, recent);
}

void AppConfig::clearRecentCartridges()
{
    QSettings settings;
    settings.remove(KEY_RECENT_CARTRIDGES);
}

QByteArray AppConfig::windowGeometry() const
{
    QSettings settings;
    return settings.value(KEY_WINDOW_GEOMETRY).toByteArray();
}

void AppConfig::setWindowGeometry(const QByteArray& geometry)
{
    QSettings settings;
    settings.setValue(KEY_WINDOW_GEOMETRY, geometry);
}

QByteArray AppConfig::windowState() const
{
    QSettings settings;
    return settings.value(KEY_WINDOW_STATE).toByteArray();
}

void AppConfig::setWindowState(const QByteArray& state)
{
    QSettings settings;
    settings.setValue(KEY_WINDOW_STATE, state);
}

bool AppConfig::maintainAspectRatio() const
{
    QSettings settings;
    return settings.value(KEY_MAINTAIN_ASPECT, true).toBool();
}

void AppConfig::setMaintainAspectRatio(bool maintain)
{
    QSettings settings;
    settings.setValue(KEY_MAINTAIN_ASPECT, maintain);
}

bool AppConfig::smoothScaling() const
{
    QSettings settings;
    return settings.value(KEY_SMOOTH_SCALING, true).toBool();
}

void AppConfig::setSmoothScaling(bool smooth)
{
    QSettings settings;
    settings.setValue(KEY_SMOOTH_SCALING, smooth);
}

void AppConfig::sync()
{
    QSettings settings;
    settings.sync();
}
