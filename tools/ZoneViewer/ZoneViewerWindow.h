// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef EQUILIBRE_ZONE_VIEWER_WINDOW_H
#define EQUILIBRE_ZONE_VIEWER_WINDOW_H

#include <QMainWindow>
#include "EQuilibre/Render/Scene.h"
#include "EQuilibre/Render/Vertex.h"

class QComboBox;
class QVBoxLayout;
class QAction;
class ZoneScene;
class RenderContext;
class SceneViewport;
class WLDSkeleton;
class Game;
class Zone;
class RenderProgram;

class ZoneViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    ZoneViewerWindow(RenderContext *renderCtx, QWidget *parent = 0);
    ~ZoneViewerWindow();

    ZoneScene * scene() const;

    bool loadZone(QString path, QString name);

private slots:
    void openArchive();
    void clearZone();
    void selectAssetDir();
    void setNoLighting();
    void setBakedLighting();
    void setDebugVertexColor();
    void setDebugTextureFactor();
    void setDebugDiffuse();

private:
    void initMenus();
    void updateMenus();

    SceneViewport *m_viewport;
    ZoneScene *m_scene;
    RenderContext *m_renderCtx;
    QAction *m_noLightingAction;
    QAction *m_bakedLightingAction;
    QAction *m_debugVertexColorAction;
    QAction *m_debugTextureFactorAction;
    QAction *m_debugDiffuseAction;
    QAction *m_showFpsAction;
    QAction *m_showZoneAction;
    QAction *m_showZoneObjectsAction;
    QAction *m_showFogAction;
    QAction *m_cullZoneObjectsAction;
    QAction *m_showSoundTriggersAction;
};

class ZoneScene : public Scene
{
    Q_OBJECT

public:
    ZoneScene(RenderContext *renderCtx);
    virtual ~ZoneScene();

    Game * game() const;
    int lightingMode() const;
    void setLightingMode(int newMode);

    virtual void init();
    virtual void update(double timestamp);
    virtual void draw();
    
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

public slots:
    void showZone(bool show);
    void showZoneObjects(bool show);
    void showFog(bool show);
    void setFrustumCulling(bool enabled);
    void showSoundTriggers(bool show);

private:
    void drawFrame();
    
    double m_lastTimestamp;
    Game *m_game;
    MouseState m_rotState;
    RenderProgram *m_program;
    int m_lightingMode;
};

#endif
