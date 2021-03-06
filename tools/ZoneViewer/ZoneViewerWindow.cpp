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

#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include "ZoneViewerWindow.h"
#include "EQuilibre/Render/SceneViewport.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Render/Scene.h"
#include "EQuilibre/Game/Game.h"
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/WLDActor.h"
#include "EQuilibre/Game/WLDSkeleton.h"
#include "EQuilibre/Game/Zone.h"
#include "EQuilibre/Game/SoundTrigger.h"

ZoneViewerWindow::ZoneViewerWindow(RenderContext *renderCtx, QWidget *parent) : QMainWindow(parent)
{
    m_scene = new ZoneScene(renderCtx);
    m_renderCtx = renderCtx;
    setWindowTitle("EQuilibre Zone Viewer");
    m_viewport = new SceneViewport(m_scene, renderCtx);
    setCentralWidget(m_viewport);
    initMenus();
}

ZoneViewerWindow::~ZoneViewerWindow()
{
    delete m_viewport;
    m_scene->game()->clear(m_renderCtx);
    delete m_scene;
}

ZoneScene * ZoneViewerWindow::scene() const
{
    return m_scene;
}

void ZoneViewerWindow::initMenus()
{
    QMenu *fileMenu = new QMenu(this);
    fileMenu->setTitle("&File");

    QAction *openAction = new QAction("&Open S3D archive...", this);
    openAction->setShortcut(QKeySequence::Open);
    QAction *clearAction = new QAction("&Clear", this);
    QAction *selectDirAction = new QAction("Select Asset Directory...", this);

    QAction *quitAction = new QAction("&Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);

    fileMenu->addAction(openAction);
    fileMenu->addAction(clearAction);
    fileMenu->addAction(selectDirAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    QMenu *renderMenu = new QMenu(this);
    renderMenu->setTitle("&Render");

    m_noLightingAction = new QAction("No Lighting", this);
    m_bakedLightingAction = new QAction("Baked Lighting", this);
    m_debugVertexColorAction = new QAction("Show Vertex Color", this);
    m_debugTextureFactorAction = new QAction("Show Texture Blend Factor", this);
    m_debugDiffuseAction = new QAction("Show Diffuse Factor", this);
    m_noLightingAction->setCheckable(true);
    m_bakedLightingAction->setCheckable(true);
    m_debugVertexColorAction->setCheckable(true);
    m_debugTextureFactorAction->setCheckable(true);
    m_debugDiffuseAction->setCheckable(true);
    m_bakedLightingAction->setEnabled(false);
    m_debugDiffuseAction->setEnabled(false);
    QActionGroup *lightingActions = new QActionGroup(this);
    lightingActions->addAction(m_noLightingAction);
    lightingActions->addAction(m_bakedLightingAction);
    lightingActions->addAction(m_debugVertexColorAction);
    lightingActions->addAction(m_debugTextureFactorAction);
    lightingActions->addAction(m_debugDiffuseAction);

    m_showFpsAction = new QAction("Show Stats", this);
    m_showFpsAction->setCheckable(true);
    m_showZoneAction = new QAction("Show Zone", this);
    m_showZoneAction->setCheckable(true);
    m_showZoneAction->setChecked(m_scene->game()->showZone());
    m_showZoneObjectsAction = new QAction("Show Zone Objects", this);
    m_showZoneObjectsAction->setCheckable(true);
    m_showFogAction = new QAction("Show Fog", this);
    m_showFogAction->setCheckable(true);
    m_showFogAction->setChecked(m_scene->game()->showFog());
    m_showZoneObjectsAction->setChecked(m_scene->game()->showObjects());
    m_cullZoneObjectsAction = new QAction("Frustum Culling of Zone Objects", this);
    m_cullZoneObjectsAction->setCheckable(true);
    m_cullZoneObjectsAction->setChecked(m_scene->game()->cullObjects());
    m_showSoundTriggersAction = new QAction("Show Sound Triggers", this);
    m_showSoundTriggersAction->setCheckable(true);

    renderMenu->addAction(m_noLightingAction);
    renderMenu->addAction(m_bakedLightingAction);
    renderMenu->addAction(m_debugVertexColorAction);
    renderMenu->addAction(m_debugTextureFactorAction);
    renderMenu->addAction(m_debugDiffuseAction);
    renderMenu->addSeparator();
    renderMenu->addAction(m_showFpsAction);
    renderMenu->addAction(m_showZoneAction);
    renderMenu->addAction(m_showZoneObjectsAction);
    renderMenu->addAction(m_cullZoneObjectsAction);
    renderMenu->addAction(m_showFogAction);
    renderMenu->addAction(m_showSoundTriggersAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(renderMenu);

    updateMenus();

    connect(openAction, SIGNAL(triggered()), this, SLOT(openArchive()));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clearZone()));
    connect(selectDirAction, SIGNAL(triggered()), this, SLOT(selectAssetDir()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    connect(m_noLightingAction, SIGNAL(triggered()), this, SLOT(setNoLighting()));
    connect(m_bakedLightingAction, SIGNAL(triggered()), this, SLOT(setBakedLighting()));
    connect(m_debugVertexColorAction, SIGNAL(triggered()), this, SLOT(setDebugVertexColor()));
    connect(m_debugTextureFactorAction, SIGNAL(triggered()), this, SLOT(setDebugTextureFactor()));
    connect(m_debugDiffuseAction, SIGNAL(triggered()), this, SLOT(setDebugDiffuse()));
    connect(m_showFpsAction, SIGNAL(toggled(bool)), m_viewport, SLOT(setShowStats(bool)));
    connect(m_showZoneAction, SIGNAL(toggled(bool)), m_scene, SLOT(showZone(bool)));
    connect(m_showZoneObjectsAction, SIGNAL(toggled(bool)), m_scene, SLOT(showZoneObjects(bool)));
    connect(m_showFogAction, SIGNAL(toggled(bool)), m_scene, SLOT(showFog(bool)));
    connect(m_cullZoneObjectsAction, SIGNAL(toggled(bool)), m_scene, SLOT(setFrustumCulling(bool)));
    connect(m_showSoundTriggersAction, SIGNAL(toggled(bool)), m_scene, SLOT(showSoundTriggers(bool)));
}

void ZoneViewerWindow::openArchive()
{
    QString filePath;
    // OpenGL rendering interferes wtih QFileDialog
    m_viewport->setAnimation(false);
    filePath = QFileDialog::getOpenFileName(0, "Select a S3D file to open",
        m_scene->assetPath(), "S3D Archive (*.s3d)");
    if(!filePath.isEmpty())
    {
        QFileInfo info(filePath);
        loadZone(info.absolutePath(), info.baseName());
        m_viewport->setFocus();
    }
    m_viewport->setAnimation(true);
}

void ZoneViewerWindow::clearZone()
{
    m_scene->game()->clearZone(m_renderCtx);
}

void ZoneViewerWindow::selectAssetDir()
{
    QFileDialog fd;
    fd.setFileMode(QFileDialog::Directory);
    fd.setDirectory(m_scene->assetPath());
    fd.setWindowTitle("Select an asset directory");
    // OpenGL rendering interferes wtih QFileDialog
    m_viewport->setAnimation(false);
    if((fd.exec() == QDialog::Accepted) && (fd.selectedFiles().count() > 0))
        m_scene->setAssetPath(fd.selectedFiles().value(0));
    m_viewport->setAnimation(true);
}

bool ZoneViewerWindow::loadZone(QString path, QString name)
{
    Game *game = m_scene->game();
    m_viewport->makeCurrent();
    game->clearZone(m_renderCtx);
    if(!game->loadZone(path, name))
    {
        return false;
    }
    WLDModel *playerModel = game->findCharacter("ELM", m_renderCtx);
    game->player()->setModel(playerModel);
    return true;
}

void ZoneViewerWindow::updateMenus()
{
    switch(m_scene->lightingMode())
    {
    default:
    case RenderProgram::NoLighting:
        m_noLightingAction->setChecked(true);
        break;
    case RenderProgram::BakedLighting:
        m_bakedLightingAction->setChecked(true);
        break;
    case RenderProgram::DebugVertexColor:
        m_debugVertexColorAction->setChecked(true);
        break;
    case RenderProgram::DebugTextureFactor:
        m_debugTextureFactorAction->setChecked(true);
        break;
    case RenderProgram::DebugDiffuse:
        m_debugDiffuseAction->setChecked(true);
        break;
    }

    m_showFpsAction->setChecked(m_viewport->showStats());
}

void ZoneViewerWindow::setNoLighting()
{
    m_scene->setLightingMode(RenderProgram::NoLighting);
    updateMenus();
}

void ZoneViewerWindow::setBakedLighting()
{
    m_scene->setLightingMode(RenderProgram::BakedLighting);
    updateMenus();
}

void ZoneViewerWindow::setDebugVertexColor()
{
    m_scene->setLightingMode(RenderProgram::DebugVertexColor);
    updateMenus();
}

void ZoneViewerWindow::setDebugTextureFactor()
{
    m_scene->setLightingMode(RenderProgram::DebugTextureFactor);
    updateMenus();
}

void ZoneViewerWindow::setDebugDiffuse()
{
    m_scene->setLightingMode(RenderProgram::DebugDiffuse);
    updateMenus();
}

////////////////////////////////////////////////////////////////////////////////

ZoneScene::ZoneScene(RenderContext *renderCtx) : Scene(renderCtx)
{
    m_game = new Game();
    m_program = renderCtx->programByID(RenderContext::BasicShader);
    m_lightingMode = (int)RenderProgram::NoLighting;
    m_rotState.last = vec3();
    m_rotState.active = false;
    m_lastTimestamp = 0.0;
}

ZoneScene::~ZoneScene()
{
    delete m_game;
}

Game * ZoneScene::game() const
{
    return m_game;
}

int ZoneScene::lightingMode() const
{
    return m_lightingMode;
}

void ZoneScene::setLightingMode(int newMode)
{
    m_lightingMode = newMode;
}

void ZoneScene::showZone(bool show)
{
    m_game->setShowZone(show);
}

void ZoneScene::showZoneObjects(bool show)
{
    m_game->setShowObjects(show);
}

void ZoneScene::showFog(bool show)
{
    m_game->setShowFog(show);
}

void ZoneScene::setFrustumCulling(bool enabled)
{
    m_game->setCullObjects(enabled);
}

void ZoneScene::showSoundTriggers(bool show)
{
    m_game->setShowSoundTriggers(show);
}

void ZoneScene::init()
{
    m_lastTimestamp = 0.0;
    m_renderCtx->viewFrustum().setFarPlane(2000.0);
}

void ZoneScene::update(double timestamp)
{
    double sinceLastUpdate = timestamp - m_lastTimestamp;
    m_game->update(m_renderCtx, timestamp, sinceLastUpdate);
    m_lastTimestamp = timestamp;
}

void ZoneScene::draw()
{
    Zone *zone = m_game->zone();
    vec4 clearColor = zone ? zone->info().fogColor : vec4(0.4, 0.4, 0.6, 0.1);
    if(m_renderCtx->beginFrame(clearColor))
    {
        clearLog();
        drawFrame();
    }
    m_renderCtx->endFrame();
}

void ZoneScene::drawFrame()
{
    Zone *zone = m_game->zone();
    if(!zone)
        return;
    
    vec3 playerPos = m_game->player()->location();
    log(QString("%1 %2 %3")
        .arg(playerPos.x, 0, 'f', 2)
        .arg(playerPos.y, 0, 'f', 2)
        .arg(playerPos.z, 0, 'f', 2));
    Frustum &viewFrustum = m_renderCtx->viewFrustum();
    m_renderCtx->matrix(RenderContext::Projection) = viewFrustum.projection();
    m_program->setLightingMode((RenderProgram::LightingMode)m_lightingMode);
    zone->draw(m_renderCtx, m_program);
    
    ZoneTerrain *terrain = zone->terrain();
    if(terrain)
    {
        uint32_t currentRegion = terrain->currentRegion();
        if(currentRegion)
            log(QString("Current region: %1").arg(currentRegion));
    }
    
    if(m_game->showSoundTriggers())
    {
        QVector<SoundTrigger *> triggers;
        zone->currentSoundTriggers(triggers);
        foreach(SoundTrigger *trigger, triggers)
        {
            SoundEntry e = trigger->entry();
            log(QString("Sound Trigger %1 (%2-%3)").arg(e.ID).arg(e.SoundID1).arg(e.SoundID2));
        }
    }
}

void ZoneScene::keyPressEvent(QKeyEvent *e)
{
    Zone *zone = m_game->zone();
    WLDCharActor *player = zone ? zone->player() : NULL;
    int newMovementX = 0;
    int newMovementY = 0;
    int key = e->key();
    if(key == Qt::Key_A)
    {
        newMovementX = 1;
    }
    else if(key == Qt::Key_D)
    {
        newMovementX = -1;
    }
    else if(key == Qt::Key_W)
    {
        newMovementY = 1;
    }
    else if(key == Qt::Key_S)
    {
        newMovementY = -1;
    }
    else if(e->modifiers() & Qt::ShiftModifier)
    {
        if(player)
        {
            bool isWalking = (player->moveMode() == WLDCharActor::Walking);
            player->setMoveMode(isWalking ? WLDCharActor::Running
                                          : WLDCharActor::Walking);
        }
    }
    
    if(player && newMovementX)
        player->setMovementX(newMovementX);
    if(player && newMovementY)
        player->setMovementY(newMovementY);
}

void ZoneScene::keyReleaseEvent(QKeyEvent *e)
{
    Zone *zone = m_game->zone();
    WLDCharActor *player = zone ? zone->player() : NULL;
    bool releaseX = false;
    bool releaseY = false;
    int key = e->key();
    if((key == Qt::Key_A) || (key == Qt::Key_D))
    {
        releaseX = !e->isAutoRepeat();
    }
    else if((key == Qt::Key_W) || (key == Qt::Key_S))
    {
        releaseY = !e->isAutoRepeat();
    }
    else if(key == Qt::Key_E)
    {
        if(player)
            player->jump();
    }
    else if(key == Qt::Key_Space)
    {
        if(m_game->frustumIsFrozen())
            m_game->unFreezeFrustum();
        else
            m_game->freezeFrustum(m_renderCtx);
    }
    else if(key == Qt::Key_G)
    {
        m_game->setApplyGravity(!m_game->applyGravity());
    }
    
    if(player && releaseX)
        player->setMovementX(0);
    if(player && releaseY)
        player->setMovementY(0);
}

void ZoneScene::mouseMoveEvent(QMouseEvent *e)
{
    if(m_rotState.active)
    {
        int dx = m_rotState.x0 - e->x();
        int dy = m_rotState.y0 - e->y();
        float newX = (m_rotState.last.x - (dy * 1.0));
        float newZ = (m_rotState.last.z - (dx * 1.0));
        newX = qMin(qMax(newX, -85.0f), 85.0f);
        m_game->player()->setLookOrientX(newX);
        m_game->player()->setLookOrientZ(newZ);
    }
}

void ZoneScene::mousePressEvent(QMouseEvent *e)
{
    if(e->button() & Qt::RightButton)
    {
        m_rotState.active = true;
        m_rotState.x0 = e->x();
        m_rotState.y0 = e->y();
        m_rotState.last = m_game->player()->lookOrient();
    }
}

void ZoneScene::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() & Qt::RightButton)
        m_rotState.active = false;
}

void ZoneScene::wheelEvent(QWheelEvent *e)
{
    float distance = m_game->player()->cameraDistance();
    float delta = (e->delta() * 0.01);
    distance = qMax(distance - delta, 0.0f);
    m_game->player()->setCameraDistance(distance);
 }
