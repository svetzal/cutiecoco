#include "emulatorwidget.h"
#include "dream/emulation.h"
#include "dream/keyboard.h"

#include <QKeyEvent>
#include <QOpenGLContext>
#include <optional>
#include <unordered_map>

namespace {
    // CoCo 3 aspect ratio (640x480 = 4:3)
    constexpr float ASPECT_RATIO = 4.0f / 3.0f;

    // Map Qt key codes to CoCo keys
    // This provides a "natural" mapping where keys match their keycap labels
    std::optional<dream::CocoKey> mapQtKeyToCoco(int qtKey)
    {
        using K = dream::CocoKey;

        switch (qtKey) {
            // Letters
            case Qt::Key_A: return K::A;
            case Qt::Key_B: return K::B;
            case Qt::Key_C: return K::C;
            case Qt::Key_D: return K::D;
            case Qt::Key_E: return K::E;
            case Qt::Key_F: return K::F;
            case Qt::Key_G: return K::G;
            case Qt::Key_H: return K::H;
            case Qt::Key_I: return K::I;
            case Qt::Key_J: return K::J;
            case Qt::Key_K: return K::K;
            case Qt::Key_L: return K::L;
            case Qt::Key_M: return K::M;
            case Qt::Key_N: return K::N;
            case Qt::Key_O: return K::O;
            case Qt::Key_P: return K::P;
            case Qt::Key_Q: return K::Q;
            case Qt::Key_R: return K::R;
            case Qt::Key_S: return K::S;
            case Qt::Key_T: return K::T;
            case Qt::Key_U: return K::U;
            case Qt::Key_V: return K::V;
            case Qt::Key_W: return K::W;
            case Qt::Key_X: return K::X;
            case Qt::Key_Y: return K::Y;
            case Qt::Key_Z: return K::Z;

            // Numbers
            case Qt::Key_0: return K::Key0;
            case Qt::Key_1: return K::Key1;
            case Qt::Key_2: return K::Key2;
            case Qt::Key_3: return K::Key3;
            case Qt::Key_4: return K::Key4;
            case Qt::Key_5: return K::Key5;
            case Qt::Key_6: return K::Key6;
            case Qt::Key_7: return K::Key7;
            case Qt::Key_8: return K::Key8;
            case Qt::Key_9: return K::Key9;

            // Arrow keys
            case Qt::Key_Up: return K::Up;
            case Qt::Key_Down: return K::Down;
            case Qt::Key_Left: return K::Left;
            case Qt::Key_Right: return K::Right;

            // Special keys
            case Qt::Key_Return:
            case Qt::Key_Enter: return K::Enter;
            case Qt::Key_Space: return K::Space;
            case Qt::Key_Shift: return K::Shift;
            case Qt::Key_Control: return K::Ctrl;
            case Qt::Key_Alt: return K::Alt;
            case Qt::Key_Escape: return K::Break;
            case Qt::Key_Backspace: return K::Left;  // Backspace acts as left arrow
            case Qt::Key_Home: return K::Clear;

            // Punctuation
            case Qt::Key_At: return K::At;
            case Qt::Key_Colon: return K::Colon;
            case Qt::Key_Semicolon: return K::Semicolon;
            case Qt::Key_Comma: return K::Comma;
            case Qt::Key_Minus: return K::Minus;
            case Qt::Key_Period: return K::Period;
            case Qt::Key_Slash: return K::Slash;

            // Function keys
            case Qt::Key_F1: return K::F1;
            case Qt::Key_F2: return K::F2;

            default: return std::nullopt;
        }
    }
}

EmulatorWidget::EmulatorWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_framebuffer(dream::FRAMEBUFFER_WIDTH, dream::FRAMEBUFFER_HEIGHT, QImage::Format_RGBA8888)
    , m_emulation(std::make_unique<dream::EmulationThread>())
    , m_pendingFrame(dream::FRAMEBUFFER_WIDTH * dream::FRAMEBUFFER_HEIGHT * 4)
    , m_refreshTimer(new QTimer(this))
{
    // Enable keyboard focus
    setFocusPolicy(Qt::StrongFocus);

    // Initialize framebuffer with black
    m_framebuffer.fill(Qt::black);

    // Connect frame ready signal to slot for thread-safe UI updates
    connect(this, &EmulatorWidget::frameReady, this, &EmulatorWidget::onFrameReady, Qt::QueuedConnection);

    // Set up refresh timer for display updates
    // We use a timer to decouple the emulation frame rate from display refresh
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        update();
    });
    m_refreshTimer->setInterval(16); // ~60 Hz display refresh
}

EmulatorWidget::~EmulatorWidget()
{
    stopEmulation();

    makeCurrent();
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
    }
    doneCurrent();
}

void EmulatorWidget::startEmulation()
{
    if (m_emulation->isRunning()) {
        return;
    }

    // Start the emulation thread with a callback that signals the main thread
    m_emulation->start([this](const uint8_t* pixels, int width, int height) {
        onEmulatorFrame(pixels, width, height);
    });

    // Start the display refresh timer
    m_refreshTimer->start();
}

void EmulatorWidget::stopEmulation()
{
    m_refreshTimer->stop();
    m_emulation->stop();
}

void EmulatorWidget::pauseEmulation()
{
    m_emulation->pause();
}

void EmulatorWidget::resumeEmulation()
{
    m_emulation->resume();
}

void EmulatorWidget::resetEmulation()
{
    m_emulation->reset();
}

bool EmulatorWidget::isEmulationRunning() const
{
    return m_emulation->isRunning();
}

bool EmulatorWidget::isEmulationPaused() const
{
    return m_emulation->isPaused();
}

float EmulatorWidget::getFps() const
{
    return m_emulation->getFps();
}

void EmulatorWidget::onEmulatorFrame(const uint8_t* pixels, int width, int height)
{
    // Called from emulation thread - copy frame data and signal main thread
    {
        std::lock_guard<std::mutex> lock(m_framebufferMutex);
        size_t size = width * height * 4;
        if (m_pendingFrame.size() != size) {
            m_pendingFrame.resize(size);
        }
        std::memcpy(m_pendingFrame.data(), pixels, size);
        m_frameUpdated = true;
    }

    // Signal the main thread that a frame is ready
    emit frameReady();
}

void EmulatorWidget::onFrameReady()
{
    // Called on main thread - update framebuffer from pending data
    std::lock_guard<std::mutex> lock(m_framebufferMutex);
    if (m_frameUpdated) {
        // Copy pending frame to QImage framebuffer
        std::memcpy(m_framebuffer.bits(), m_pendingFrame.data(), m_pendingFrame.size());
        m_frameUpdated = false;
    }
}

void EmulatorWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Create texture for framebuffer
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void EmulatorWidget::resizeGL(int w, int h)
{
    // Calculate aspect-ratio-preserving viewport
    float windowAspect = static_cast<float>(w) / static_cast<float>(h);

    if (windowAspect > ASPECT_RATIO) {
        // Window is wider than content - pillarbox (bars on sides)
        m_viewportH = h;
        m_viewportW = static_cast<int>(h * ASPECT_RATIO);
        m_viewportX = (w - m_viewportW) / 2;
        m_viewportY = 0;
    } else {
        // Window is taller than content - letterbox (bars on top/bottom)
        m_viewportW = w;
        m_viewportH = static_cast<int>(w / ASPECT_RATIO);
        m_viewportX = 0;
        m_viewportY = (h - m_viewportH) / 2;
    }

    // Set the viewport for rendering
    glViewport(m_viewportX, m_viewportY, m_viewportW, m_viewportH);
}

void EmulatorWidget::paintGL()
{
    // Clear the entire window (including letterbox/pillarbox areas)
    glViewport(0, 0, width(), height());
    glClear(GL_COLOR_BUFFER_BIT);

    // Set viewport for content with aspect ratio preservation
    glViewport(m_viewportX, m_viewportY, m_viewportW, m_viewportH);

    // Upload framebuffer to texture
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 m_framebuffer.width(), m_framebuffer.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, m_framebuffer.constBits());

    // Enable texturing and draw fullscreen quad
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 1, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void EmulatorWidget::keyPressEvent(QKeyEvent *event)
{
    // Ignore auto-repeat events for key down
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }

    auto cocoKey = mapQtKeyToCoco(event->key());
    if (cocoKey) {
        dream::getKeyboard().keyDown(*cocoKey);
        event->accept();
    } else {
        QOpenGLWidget::keyPressEvent(event);
    }
}

void EmulatorWidget::keyReleaseEvent(QKeyEvent *event)
{
    // Ignore auto-repeat events for key up
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }

    auto cocoKey = mapQtKeyToCoco(event->key());
    if (cocoKey) {
        dream::getKeyboard().keyUp(*cocoKey);
        event->accept();
    } else {
        QOpenGLWidget::keyReleaseEvent(event);
    }
}
