#ifndef EMULATORWIDGET_H
#define EMULATORWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace dream {
class EmulationThread;
}

class EmulatorWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit EmulatorWidget(QWidget *parent = nullptr);
    ~EmulatorWidget() override;

    void startEmulation();
    void stopEmulation();
    void pauseEmulation();
    void resumeEmulation();
    void resetEmulation();

    bool isEmulationRunning() const;
    bool isEmulationPaused() const;
    float getFps() const;

    // Size hint for layout
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void frameReady();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onFrameReady();

private:
    void onEmulatorFrame(const uint8_t* pixels, int width, int height);

    // Framebuffer for emulator output
    QImage m_framebuffer;
    GLuint m_texture = 0;

    // Emulation thread
    std::unique_ptr<dream::EmulationThread> m_emulation;

    // Thread-safe framebuffer update
    std::mutex m_framebufferMutex;
    std::vector<uint8_t> m_pendingFrame;
    bool m_frameUpdated = false;

    // Refresh timer for display updates
    QTimer* m_refreshTimer;

    // Viewport for aspect-ratio-correct rendering
    int m_viewportX = 0;
    int m_viewportY = 0;
    int m_viewportW = 0;
    int m_viewportH = 0;

    // Device pixel dimensions (physical pixels on HiDPI displays)
    int m_deviceWidth = 0;
    int m_deviceHeight = 0;

    // Track active character-based key presses
    // Maps Qt key code to the CoCo keys we pressed (key + whether shift was added)
    struct ActiveKeyInfo {
        int cocoKey;      // dream::CocoKey value
        bool addedShift;  // Did we add shift for this key?
    };
    std::unordered_map<int, ActiveKeyInfo> m_activeCharKeys;
};

#endif // EMULATORWIDGET_H
