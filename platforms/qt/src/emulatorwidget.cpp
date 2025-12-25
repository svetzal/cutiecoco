#include "emulatorwidget.h"
#include "cutie/emulation.h"
#include "cutie/keyboard.h"

#include <QKeyEvent>
#include <QOpenGLContext>
#include <optional>
#include <unordered_map>

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

EmulatorWidget::EmulatorWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_framebuffer(cutie::FRAMEBUFFER_WIDTH, cutie::FRAMEBUFFER_HEIGHT, QImage::Format_RGBA8888)
    , m_emulation(std::make_unique<cutie::EmulationThread>())
    , m_pendingFrame(cutie::FRAMEBUFFER_WIDTH * cutie::FRAMEBUFFER_HEIGHT * 4)
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

QSize EmulatorWidget::sizeHint() const
{
    // Return the native framebuffer size as the preferred size
    return QSize(cutie::FRAMEBUFFER_WIDTH, cutie::FRAMEBUFFER_HEIGHT);
}

QSize EmulatorWidget::minimumSizeHint() const
{
    // Minimum size is half the native resolution
    return QSize(cutie::FRAMEBUFFER_WIDTH / 2, cutie::FRAMEBUFFER_HEIGHT / 2);
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
