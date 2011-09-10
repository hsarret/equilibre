#ifndef OPENEQ_SCENE_VIEWPORT_H
#define OPENEQ_SCENE_VIEWPORT_H

#include "Platform.h"
#include <QGLWidget>
#include <QTime>
#include "Vertex.h"

class QTimer;
class QPainter;
class QGLFormat;
class Scene;
class RenderState;

class SceneViewport : public QGLWidget
{
    Q_OBJECT

public:
    SceneViewport(Scene *scene, RenderState *state, QWidget *parent = 0);
    virtual ~SceneViewport();

    void setAnimation(bool enabled);
    bool showFps() const;

public slots:
    void setShowFps(bool show);

protected:
    virtual void initializeGL();
    virtual void resizeGL(int width, int height);
    virtual void paintGL();
    virtual void paintEvent(QPaintEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

private slots:
    void updateFPS();

private:
    void paintFPS(QPainter *p, float fps);
    void startFPS();
    void updateAnimationState();
    void toggleAnimation();

    Scene *m_scene;
    RenderState *m_state;
    QTimer *m_renderTimer;
    bool m_animate;

    // FPS settings
    QTimer *m_fpsTimer;
    QTime m_start;
    uint m_frames;
    float m_lastFPS;
};

#endif
