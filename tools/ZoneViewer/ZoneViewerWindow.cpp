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
    QAction *selectDirAction = new QAction("Select Asset Directory...", this);

    QAction *quitAction = new QAction("&Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);

    fileMenu->addAction(openAction);
    fileMenu->addAction(selectDirAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    QMenu *renderMenu = new QMenu();
    renderMenu->setTitle("&Render");

    m_noLightingAction = new QAction("No Lighting", this);
    m_bakedLightingAction = new QAction("Baked Lighting", this);
    m_debugVertexColorAction = new QAction("Show Vertex Color", this);
    m_debugDiffuseAction = new QAction("Show Diffuse Factor", this);
    m_debugTextureFactorAction = new QAction("Show Texture Blend Factor", this);
    m_noLightingAction->setCheckable(true);
    m_bakedLightingAction->setCheckable(true);
    m_debugVertexColorAction->setCheckable(true);
    m_debugDiffuseAction->setCheckable(true);
    m_debugTextureFactorAction->setCheckable(true);
    
    QActionGroup *lightingActions = new QActionGroup(this);
    lightingActions->addAction(m_noLightingAction);
    lightingActions->addAction(m_bakedLightingAction);
    lightingActions->addAction(m_debugVertexColorAction);
    lightingActions->addAction(m_debugDiffuseAction);
    lightingActions->addAction(m_debugTextureFactorAction);

    m_showFpsAction = new QAction("Show Stats", this);
    m_showFpsAction->setCheckable(true);
    m_showZoneAction = new QAction("Show Zone", this);
    m_showZoneAction->setCheckable(true);
    m_showZoneAction->setChecked(m_scene->zone()->showZone());
    m_showZoneObjectsAction = new QAction("Show Zone Objects", this);
    m_showZoneObjectsAction->setCheckable(true);
    m_showZoneObjectsAction->setChecked(m_scene->zone()->showObjects());
    m_cullZoneObjectsAction = new QAction("Frustum Culling of Zone Objects", this);
    m_cullZoneObjectsAction->setCheckable(true);
    m_cullZoneObjectsAction->setChecked(m_scene->zone()->cullObjects());
    m_showSoundTriggersAction = new QAction("Show Sound Triggers", this);
    m_showSoundTriggersAction->setCheckable(true);

    renderMenu->addAction(m_noLightingAction);
    renderMenu->addAction(m_bakedLightingAction);
    renderMenu->addAction(m_debugVertexColorAction);
    renderMenu->addAction(m_debugDiffuseAction);
    renderMenu->addAction(m_debugTextureFactorAction);
    renderMenu->addSeparator();
    renderMenu->addAction(m_showFpsAction);
    renderMenu->addAction(m_showZoneAction);
    renderMenu->addAction(m_showZoneObjectsAction);
    renderMenu->addAction(m_cullZoneObjectsAction);
    renderMenu->addAction(m_showSoundTriggersAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(renderMenu);

    updateMenus();

    connect(openAction, SIGNAL(triggered()), this, SLOT(openArchive()));
    connect(selectDirAction, SIGNAL(triggered()), this, SLOT(selectAssetDir()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    connect(m_noLightingAction, SIGNAL(triggered()), this, SLOT(setNoLighting()));
    connect(m_bakedLightingAction, SIGNAL(triggered()), this, SLOT(setBakedLighting()));
    connect(m_debugVertexColorAction, SIGNAL(triggered()), this, SLOT(setDebugVertexColor()));
    connect(m_debugDiffuseAction, SIGNAL(triggered()), this, SLOT(setDebugDiffuse()));
    connect(m_debugTextureFactorAction, SIGNAL(triggered()), this, SLOT(setDebugTextureFactor()));
    connect(m_showFpsAction, SIGNAL(toggled(bool)), m_viewport, SLOT(setShowStats(bool)));
    connect(m_showZoneAction, SIGNAL(toggled(bool)), m_scene, SLOT(showZone(bool)));
    connect(m_showZoneObjectsAction, SIGNAL(toggled(bool)), m_scene, SLOT(showZoneObjects(bool)));
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
    m_viewport->makeCurrent();
    m_scene->zone()->clear(m_renderCtx);
    return m_scene->zone()->load(path, name);
}

bool ZoneViewerWindow::loadCharacters(QString archivePath)
{
    m_viewport->makeCurrent();
    return m_scene->zone()->loadCharacters(archivePath);
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
    m_zone = new Zone(this);
    m_program = renderCtx->programByID(RenderContext::BasicShader);
    m_lightingMode = (int)RenderProgram::NoLighting;
    m_rotState.last = vec3();
    m_rotState.active = false;
}

Zone * ZoneScene::zone() const
{
    return m_zone;
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
    m_zone->setShowZone(show);
}

void ZoneScene::showZoneObjects(bool show)
{
    m_zone->setShowObjects(show);
}

void ZoneScene::setFrustumCulling(bool enabled)
{
    m_zone->setCullObjects(enabled);
}

void ZoneScene::showSoundTriggers(bool show)
{
    m_zone->setShowSoundTriggers(show);
}

void ZoneScene::init()
{
    m_started = currentTime();
    m_renderCtx->viewFrustum().setFarPlane(1000.0);
}

void ZoneScene::draw()
{
    if(m_renderCtx->beginFrame(m_zone->fogParams().color))
    {
        clearLog();
        drawFrame();
    }
    m_renderCtx->endFrame();
}

void ZoneScene::drawFrame()
{
    vec3 camPos = m_zone->playerPos() + m_zone->cameraPos();
    log(QString("%1 %2 %3\n")
        .arg(camPos.x, 0, 'f', 2)
        .arg(camPos.y, 0, 'f', 2)
        .arg(camPos.z, 0, 'f', 2));
    m_program->setLightingMode((RenderProgram::LightingMode)m_lightingMode);
    m_zone->draw(m_renderCtx);
    
    if(m_zone->showSoundTriggers())
    {
        QVector<SoundTrigger *> triggers;
        m_zone->currentSoundTriggers(triggers);
        foreach(SoundTrigger *trigger, triggers)
        {
            SoundEntry e = trigger->entry();
            log(QString("Sound Trigger %1 (%2-%3)").arg(e.ID).arg(e.SoundID1).arg(e.SoundID2));
        }
    }
}

void ZoneScene::keyReleaseEvent(QKeyEvent *e)
{
    float dist = 10.0;
    int key = e->key();
    if(key == Qt::Key_Q)
        m_zone->step(0.0, dist, 0.0);
    else if(key == Qt::Key_D)
        m_zone->step(0.0, -dist, 0.0);
    else if(key == Qt::Key_Z)
        m_zone->step(dist, 0.0, 0.0);
    else if(key == Qt::Key_S)
        m_zone->step(-dist, 0.0, 0.0);
    else if(key == Qt::Key_Space)
    {
        if(m_zone->frustumIsFrozen())
            m_zone->unFreezeFrustum();
        else
            m_zone->freezeFrustum(m_renderCtx);
    }
}

void ZoneScene::mouseMoveEvent(QMouseEvent *e)
{
    if(m_rotState.active)
    {
        int dx = m_rotState.x0 - e->x();
        int dy = m_rotState.y0 - e->y();
        vec3 camOrient = m_zone->cameraOrient();
        camOrient.x = (m_rotState.last.x - (dy * 1.0));
        m_zone->setCameraOrient(camOrient);
        m_zone->setPlayerOrient(m_rotState.last.z - (dx * 1.0));
    }
}

void ZoneScene::mousePressEvent(QMouseEvent *e)
{
    if(e->button() & Qt::RightButton)
    {
        m_rotState.active = true;
        m_rotState.x0 = e->x();
        m_rotState.y0 = e->y();
        m_rotState.last = vec3(0.0, 0.0, m_zone->playerOrient()) + m_zone->cameraOrient();
    }
}

void ZoneScene::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() & Qt::RightButton)
        m_rotState.active = false;
}
