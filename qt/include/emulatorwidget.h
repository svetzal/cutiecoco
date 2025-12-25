#ifndef EMULATORWIDGET_H
#define EMULATORWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class EmulatorWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit EmulatorWidget(QWidget *parent = nullptr);
    ~EmulatorWidget() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    // Framebuffer for emulator output
    QImage m_framebuffer;
    GLuint m_texture = 0;
};

#endif // EMULATORWIDGET_H
