#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSize>
#include <QPoint>
#include <QByteArray>

namespace cutie {
enum class MemorySize;
enum class CpuType;
}

// Application configuration using QSettings
// Stores user preferences and emulator settings
class AppConfig : public QObject
{
    Q_OBJECT

public:
    // Get singleton instance
    static AppConfig& instance();

    // Prevent copy
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    // Emulator settings
    cutie::MemorySize memorySize() const;
    void setMemorySize(cutie::MemorySize size);

    cutie::CpuType cpuType() const;
    void setCpuType(cutie::CpuType type);

    uint32_t audioSampleRate() const;
    void setAudioSampleRate(uint32_t rate);

    // ROM paths
    QString systemRomPath() const;
    void setSystemRomPath(const QString& path);

    QString lastCartridgeDir() const;
    void setLastCartridgeDir(const QString& dir);

    // Recent files (most recent first)
    QStringList recentCartridges() const;
    void addRecentCartridge(const QString& path);
    void clearRecentCartridges();

    // Window state
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& geometry);

    QByteArray windowState() const;
    void setWindowState(const QByteArray& state);

    // Display settings
    bool maintainAspectRatio() const;
    void setMaintainAspectRatio(bool maintain);

    bool smoothScaling() const;
    void setSmoothScaling(bool smooth);

    // Sync settings to disk immediately
    void sync();

signals:
    void memorySizeChanged(cutie::MemorySize size);
    void cpuTypeChanged(cutie::CpuType type);
    void audioSampleRateChanged(uint32_t rate);

private:
    AppConfig();
    ~AppConfig() override = default;

    static constexpr int MAX_RECENT_FILES = 10;
};

#endif // APPCONFIG_H
