#include "emulatorwidget.h"
#include "dream/emulation.h"

#include <QKeyEvent>
#include <QOpenGLContext>

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
    glViewport(0, 0, w, h);
}

void EmulatorWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

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
    // TODO: Forward to emulator keyboard handler
    QOpenGLWidget::keyPressEvent(event);
}

void EmulatorWidget::keyReleaseEvent(QKeyEvent *event)
{
    // TODO: Forward to emulator keyboard handler
    QOpenGLWidget::keyReleaseEvent(event);
}
