#ifndef EMULATORWIDGET_H
#define EMULATORWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <memory>
#include <unordered_map>

namespace cutie {
class CocoEmulator;
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

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onEmulationTick();

private:
    // Framebuffer for emulator output
    QImage m_framebuffer;
    GLuint m_texture = 0;

    // Emulator instance
    std::unique_ptr<cutie::CocoEmulator> m_emulator;

    // Emulation timer (calls runFrame at ~60 Hz)
    QTimer* m_emulationTimer;

    // Emulation state
    bool m_running = false;
    bool m_paused = false;

    // FPS tracking
    int m_frameCount = 0;
    qint64 m_fpsStartTime = 0;
    float m_fps = 0.0f;

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
        int cocoKey;      // cutie::CocoKey value
        bool addedShift;  // Did we add shift for this key?
    };
    std::unordered_map<int, ActiveKeyInfo> m_activeCharKeys;
};

#endif // EMULATORWIDGET_H
