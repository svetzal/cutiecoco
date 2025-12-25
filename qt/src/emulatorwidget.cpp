#include "emulatorwidget.h"

#include <QKeyEvent>
#include <QOpenGLContext>

EmulatorWidget::EmulatorWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_framebuffer(640, 480, QImage::Format_RGB32)
{
    // Enable keyboard focus
    setFocusPolicy(Qt::StrongFocus);

    // Initialize framebuffer with a test pattern (green screen for CoCo)
    m_framebuffer.fill(QColor(0, 255, 0));
}

EmulatorWidget::~EmulatorWidget()
{
    makeCurrent();
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
    }
    doneCurrent();
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
                 0, GL_BGRA, GL_UNSIGNED_BYTE, m_framebuffer.constBits());

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
