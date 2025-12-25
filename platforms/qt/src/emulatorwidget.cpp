#include "emulatorwidget.h"
#include "qtaudiooutput.h"
#include "cutie/emulator.h"
#include "cutie/keyboard.h"

#include <QKeyEvent>
#include <QOpenGLContext>
#include <QDateTime>
#include <optional>
#include <unordered_map>
#include <cstring>

namespace {
    // CoCo 3 aspect ratio (640x480 = 4:3)
    constexpr float ASPECT_RATIO = 4.0f / 3.0f;

    // A CoCo key combination (primary key + optional shift)
    struct CocoKeyCombo {
        cutie::CocoKey key;
        bool withShift;
    };

    // Map printable characters to CoCo key combinations
    // This handles the translation between PC keyboard layout and CoCo layout
    // For example: PC " (Shift+') -> CoCo Shift+2
    std::optional<CocoKeyCombo> mapCharToCoco(QChar ch)
    {
        using K = cutie::CocoKey;

        // Lowercase letters -> just the letter key
        if (ch >= 'a' && ch <= 'z') {
            return CocoKeyCombo{static_cast<K>(static_cast<int>(K::A) + (ch.unicode() - 'a')), false};
        }

        // Uppercase letters -> letter key + shift
        if (ch >= 'A' && ch <= 'Z') {
            return CocoKeyCombo{static_cast<K>(static_cast<int>(K::A) + (ch.unicode() - 'A')), true};
        }

        // Numbers
        if (ch >= '0' && ch <= '9') {
            return CocoKeyCombo{static_cast<K>(static_cast<int>(K::Key0) + (ch.unicode() - '0')), false};
        }

        // CoCo shifted number keys produce different symbols than PC
        switch (ch.unicode()) {
            // Basic punctuation (unshifted on CoCo)
            case '@': return CocoKeyCombo{K::At, false};
            case ':': return CocoKeyCombo{K::Colon, false};
            case ';': return CocoKeyCombo{K::Semicolon, false};
            case ',': return CocoKeyCombo{K::Comma, false};
            case '-': return CocoKeyCombo{K::Minus, false};
            case '.': return CocoKeyCombo{K::Period, false};
            case '/': return CocoKeyCombo{K::Slash, false};
            case ' ': return CocoKeyCombo{K::Space, false};

            // Shifted punctuation on CoCo
            case '!': return CocoKeyCombo{K::Key1, true};   // Shift+1
            case '"': return CocoKeyCombo{K::Key2, true};   // Shift+2
            case '#': return CocoKeyCombo{K::Key3, true};   // Shift+3
            case '$': return CocoKeyCombo{K::Key4, true};   // Shift+4
            case '%': return CocoKeyCombo{K::Key5, true};   // Shift+5
            case '&': return CocoKeyCombo{K::Key6, true};   // Shift+6
            case '\'': return CocoKeyCombo{K::Key7, true};  // Shift+7 (apostrophe)
            case '(': return CocoKeyCombo{K::Key8, true};   // Shift+8
            case ')': return CocoKeyCombo{K::Key9, true};   // Shift+9
            case '*': return CocoKeyCombo{K::Colon, true};  // Shift+:
            case '+': return CocoKeyCombo{K::Semicolon, true}; // Shift+;
            case '<': return CocoKeyCombo{K::Comma, true};  // Shift+,
            case '=': return CocoKeyCombo{K::Minus, true};  // Shift+-
            case '>': return CocoKeyCombo{K::Period, true}; // Shift+.
            case '?': return CocoKeyCombo{K::Slash, true};  // Shift+/

            default: return std::nullopt;
        }
    }

    // Map Qt key codes to CoCo keys for non-printable keys
    // (arrows, function keys, modifiers, etc.)
    // Returns the CoCo key, or std::nullopt if no mapping exists
    std::optional<cutie::CocoKey> mapQtKeyToCoco(int qtKey)
    {
        using K = cutie::CocoKey;

        switch (qtKey) {
            // Arrow keys
            case Qt::Key_Up: return K::Up;
            case Qt::Key_Down: return K::Down;
            case Qt::Key_Left: return K::Left;
            case Qt::Key_Right: return K::Right;

            // Special keys
            case Qt::Key_Return:
            case Qt::Key_Enter: return K::Enter;
            case Qt::Key_Shift: return K::Shift;
#ifdef Q_OS_MACOS
            // On macOS, physical Control key sends Qt::Key_Meta (due to Qt's swap)
            case Qt::Key_Meta: return K::Ctrl;
#else
            case Qt::Key_Control: return K::Ctrl;
#endif
            case Qt::Key_Alt: return K::Alt;
            case Qt::Key_Escape: return K::Break;
            case Qt::Key_Backspace: return K::Left;  // Backspace acts as left arrow
            case Qt::Key_Home: return K::Clear;

            // Function keys
            case Qt::Key_F1: return K::F1;
            case Qt::Key_F2: return K::F2;

            default: return std::nullopt;
        }
    }

    // Check if a Qt key event should be ignored (system modifiers, etc.)
    bool shouldIgnoreKeyEvent(const QKeyEvent* event)
    {
        int key = event->key();

#ifdef Q_OS_MACOS
        // On macOS, Qt swaps Control and Meta by default:
        // - Physical Command key → Qt::Key_Control
        // - Physical Control key → Qt::Key_Meta
        // We want to ignore Command (used for macOS shortcuts)
        // The physical Control key (Qt::Key_Meta) will map to CoCo Ctrl
        if (key == Qt::Key_Control) {
            return true;  // This is Command key on macOS - ignore it
        }
#endif

        // Ignore other system modifiers that don't map to CoCo keys
        switch (key) {
            case Qt::Key_Super_L:
            case Qt::Key_Super_R:
            case Qt::Key_Hyper_L:
            case Qt::Key_Hyper_R:
            case Qt::Key_CapsLock:
            case Qt::Key_NumLock:
            case Qt::Key_ScrollLock:
                return true;
            default:
                return false;
        }
    }
}

namespace {
    // Framebuffer dimensions (must match emulator output)
    constexpr int FRAMEBUFFER_WIDTH = 640;
    constexpr int FRAMEBUFFER_HEIGHT = 480;

    // Target frame interval in milliseconds (~59.923 Hz)
    constexpr int FRAME_INTERVAL_MS = 16;  // ~62.5 Hz, close enough for display
}

EmulatorWidget::EmulatorWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_framebuffer(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, QImage::Format_RGBA8888)
    , m_emulationTimer(new QTimer(this))
{
    // Enable keyboard focus
    setFocusPolicy(Qt::StrongFocus);

    // Initialize framebuffer with black
    m_framebuffer.fill(Qt::black);

    // Create emulator with default config
    cutie::EmulatorConfig config;
    m_emulator = cutie::CocoEmulator::create(config);

    // Set up emulation timer
    connect(m_emulationTimer, &QTimer::timeout, this, &EmulatorWidget::onEmulationTick);
    m_emulationTimer->setInterval(FRAME_INTERVAL_MS);
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

QSize EmulatorWidget::sizeHint() const
{
    // Return the native framebuffer size as the preferred size
    return QSize(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
}

QSize EmulatorWidget::minimumSizeHint() const
{
    // Minimum size is half the native resolution
    return QSize(FRAMEBUFFER_WIDTH / 2, FRAMEBUFFER_HEIGHT / 2);
}

void EmulatorWidget::startEmulation()
{
    if (m_running) {
        return;
    }

    // Initialize emulator if not ready
    if (!m_emulator->isReady()) {
        if (!m_emulator->init()) {
            // TODO: Handle initialization error
            return;
        }
    }

    // Initialize audio output
    if (!m_audioOutput) {
        m_audioOutput = std::make_unique<QtAudioOutput>();
    }
    auto audioInfo = m_emulator->getAudioInfo();
    if (audioInfo.sampleRate > 0) {
        m_audioOutput->init(audioInfo.sampleRate);
    }

    m_running = true;
    m_paused = false;
    m_frameCount = 0;
    m_fpsStartTime = QDateTime::currentMSecsSinceEpoch();

    // Start the emulation timer
    m_emulationTimer->start();
}

void EmulatorWidget::stopEmulation()
{
    m_emulationTimer->stop();
    m_running = false;

    if (m_audioOutput) {
        m_audioOutput->shutdown();
    }

    if (m_emulator) {
        m_emulator->shutdown();
    }
}

void EmulatorWidget::pauseEmulation()
{
    m_paused = true;
}

void EmulatorWidget::resumeEmulation()
{
    m_paused = false;
    m_fpsStartTime = QDateTime::currentMSecsSinceEpoch();
    m_frameCount = 0;
}

void EmulatorWidget::resetEmulation()
{
    if (m_emulator) {
        m_emulator->reset();
    }
}

bool EmulatorWidget::isEmulationRunning() const
{
    return m_running;
}

bool EmulatorWidget::isEmulationPaused() const
{
    return m_paused;
}

float EmulatorWidget::getFps() const
{
    return m_fps;
}

void EmulatorWidget::onEmulationTick()
{
    if (!m_running || m_paused || !m_emulator) {
        return;
    }

    // Run one frame of emulation
    m_emulator->runFrame();

    // Get the framebuffer and copy to our QImage
    auto [pixels, size] = m_emulator->getFramebuffer();
    if (pixels && size > 0) {
        std::memcpy(m_framebuffer.bits(), pixels, size);
    }

    // Submit audio samples to the output
    if (m_audioOutput) {
        auto [samples, count] = m_emulator->getAudioSamples();
        if (samples && count > 0) {
            m_audioOutput->submitSamples(samples, count);
        }
    }

    // Update display
    update();

    // Calculate FPS
    ++m_frameCount;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = now - m_fpsStartTime;
    if (elapsed >= 1000) {
        m_fps = static_cast<float>(m_frameCount) * 1000.0f / static_cast<float>(elapsed);
        m_frameCount = 0;
        m_fpsStartTime = now;
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
    // Qt passes logical pixels to resizeGL, but OpenGL needs device pixels
    // Multiply by devicePixelRatio to get actual framebuffer dimensions
    qreal dpr = devicePixelRatio();
    m_deviceWidth = static_cast<int>(w * dpr);
    m_deviceHeight = static_cast<int>(h * dpr);

    // Calculate aspect-ratio-preserving viewport in device pixels
    float windowAspect = static_cast<float>(m_deviceWidth) / static_cast<float>(m_deviceHeight);

    if (windowAspect > ASPECT_RATIO) {
        // Window is wider than content - pillarbox (bars on sides)
        m_viewportH = m_deviceHeight;
        m_viewportW = static_cast<int>(m_deviceHeight * ASPECT_RATIO);
        m_viewportX = (m_deviceWidth - m_viewportW) / 2;
        m_viewportY = 0;
    } else {
        // Window is taller than content - letterbox (bars on top/bottom)
        m_viewportW = m_deviceWidth;
        m_viewportH = static_cast<int>(m_deviceWidth / ASPECT_RATIO);
        m_viewportX = 0;
        m_viewportY = (m_deviceHeight - m_viewportH) / 2;
    }

    // Set the viewport for rendering
    glViewport(m_viewportX, m_viewportY, m_viewportW, m_viewportH);
}

void EmulatorWidget::paintGL()
{
    // Clear the entire window (including letterbox/pillarbox areas)
    // Use device pixels (physical pixels on HiDPI displays)
    glViewport(0, 0, m_deviceWidth, m_deviceHeight);
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

    auto& kb = cutie::getKeyboard();

    // Ignore system modifiers that don't map to CoCo keys (e.g., Command on macOS)
    if (shouldIgnoreKeyEvent(event)) {
        event->ignore();
        return;
    }

    // First, try non-printable key mapping (arrows, modifiers, function keys)
    auto cocoKey = mapQtKeyToCoco(event->key());
    if (cocoKey) {
        kb.keyDown(*cocoKey);
        event->accept();
        return;
    }

    // Try character-based mapping for printable characters
    QString text = event->text();
    if (!text.isEmpty()) {
        auto combo = mapCharToCoco(text[0]);
        if (combo) {
            // Track what we're pressing so we can release it properly
            ActiveKeyInfo info;
            info.cocoKey = static_cast<int>(combo->key);
            info.addedShift = combo->withShift;

            // Press shift first if needed
            if (combo->withShift) {
                kb.keyDown(cutie::CocoKey::Shift);
            }
            kb.keyDown(combo->key);

            m_activeCharKeys[event->key()] = info;
            event->accept();
            return;
        }
    }

    QOpenGLWidget::keyPressEvent(event);
}

void EmulatorWidget::keyReleaseEvent(QKeyEvent *event)
{
    // Ignore auto-repeat events for key up
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }

    auto& kb = cutie::getKeyboard();

    // Ignore system modifiers that don't map to CoCo keys (e.g., Command on macOS)
    if (shouldIgnoreKeyEvent(event)) {
        event->ignore();
        return;
    }

    // First, try non-printable key mapping
    auto cocoKey = mapQtKeyToCoco(event->key());
    if (cocoKey) {
        kb.keyUp(*cocoKey);
        event->accept();
        return;
    }

    // Check if this was a character-based key press
    auto it = m_activeCharKeys.find(event->key());
    if (it != m_activeCharKeys.end()) {
        // Release the key
        kb.keyUp(static_cast<cutie::CocoKey>(it->second.cocoKey));

        // Release shift if we added it
        if (it->second.addedShift) {
            kb.keyUp(cutie::CocoKey::Shift);
        }

        m_activeCharKeys.erase(it);
        event->accept();
        return;
    }

    QOpenGLWidget::keyReleaseEvent(event);
}
