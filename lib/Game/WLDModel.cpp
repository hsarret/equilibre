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

#include <QImage>
#include <QRegExp>
#include "EQuilibre/Game/WLDModel.h"
#include "EQuilibre/Game/Fragments.h"
#include "EQuilibre/Game/WLDSkeleton.h"
#include "EQuilibre/Game/PFSArchive.h"
#include "EQuilibre/Render/RenderContext.h"
#include "EQuilibre/Render/RenderProgram.h"
#include "EQuilibre/Render/Material.h"

using namespace std;

WLDModel::WLDModel(PFSArchive *archive)
{
    m_buffer = 0;
    m_skel = 0;
    m_skin = 0;
    m_skin = newSkin("00");
    m_palette = new WLDMaterialPalette(archive);
    m_materials = NULL;
}

WLDModel::~WLDModel()
{
    delete m_materials;
    delete m_palette;
    foreach(WLDModelSkin *skin, m_skins)
        delete skin;
    foreach(WLDMesh *mesh, m_meshes)
        delete mesh;
}

MeshBuffer * WLDModel::buffer() const
{
    return m_buffer;
}

void WLDModel::setBuffer(MeshBuffer *newBuffer)
{
    m_buffer = newBuffer;
}

WLDSkeleton * WLDModel::skeleton() const
{
    return m_skel;
}

WLDModelSkin * WLDModel::skin() const
{
    return m_skin;
}

void WLDModel::setSkeleton(WLDSkeleton *skeleton)
{
    m_skel = skeleton;
}

WLDMaterialPalette * WLDModel::palette() const
{
    return m_palette;
}

MaterialArray * WLDModel::materials() const
{
    return m_materials;
}

void WLDModel::setMaterials(MaterialArray *newMaterials)
{
    m_materials = newMaterials;
}

const QMap<QString, WLDModelSkin *> & WLDModel::skins() const
{
    return m_skins;
}

WLDModelSkin * WLDModel::newSkin(QString name)
{
    WLDModelSkin *skin = new WLDModelSkin(name, this);
    m_skins.insert(name, skin);
    return skin;
}

const QList<WLDMesh *> & WLDModel::meshes() const
{
    return m_meshes;
}

QList<MeshDefFragment *> WLDModel::listMeshes(ActorDefFragment *def)
{
    QList<MeshDefFragment *> meshes;
    if(!def)
        return meshes;
    foreach(WLDFragment *modelFrag, def->m_models)
    {
        MeshFragment *meshFrag = modelFrag->cast<MeshFragment>();
        if(meshFrag)
            meshes.append(meshFrag->m_def);
        HierSpriteFragment *skelFrag = modelFrag->cast<HierSpriteFragment>();
        if(skelFrag)
        {
            foreach(MeshFragment *meshFrag2, skelFrag->m_def->m_meshes)
            {
                if(meshFrag2->m_def)
                   meshes.append(meshFrag2->m_def);
            }
        }
    }
    return meshes;
}

////////////////////////////////////////////////////////////////////////////////

WLDMesh::WLDMesh(MeshDefFragment *meshDef, uint32_t partID)
{
    m_partID = partID;
    m_meshDef = meshDef;
    m_data = NULL;
    m_palette = NULL;
    m_boundsAA.low = meshDef->m_boundsAA.low + meshDef->m_center;
    m_boundsAA.high = meshDef->m_boundsAA.high + meshDef->m_center;
}

WLDMesh::~WLDMesh()
{
    delete m_palette;
}

MeshData * WLDMesh::data() const
{
    return m_data;
}

void WLDMesh::setData(MeshData *mesh)
{
    m_data = mesh;
}

MeshDefFragment * WLDMesh::def() const
{
    return m_meshDef;
}

WLDMaterialPalette * WLDMesh::palette() const
{
    return m_palette;
}

uint32_t WLDMesh::partID() const
{
    return m_partID;
}

const AABox & WLDMesh::boundsAA() const
{
    return m_boundsAA;
}

void WLDMesh::importPalette(PFSArchive *archive)
{
    m_palette = new WLDMaterialPalette(archive);
    m_palette->setDef(m_meshDef->m_palette);
    m_palette->createSlots();
}

MeshData * WLDMesh::importFrom(MeshBuffer *meshBuf, uint32_t paletteOffset)
{
    MeshData *meshData = importMaterialGroups(meshBuf, paletteOffset);
    importVertexData(meshBuf, meshData->vertexSegment);
    importIndexData(meshBuf, meshData->indexSegment, meshData->vertexSegment,
                    0, (uint32_t)m_meshDef->m_indices.count());
    return meshData;
}

void WLDMesh::importVertexData(MeshBuffer *buffer, BufferSegment &dataLoc)
{
    // Update the location of the mesh in the buffer.
    QVector<Vertex> &vertices(buffer->vertices);
    uint32_t vertexCount = (uint32_t)m_meshDef->m_vertices.count();
    uint32_t vertexIndex = vertices.count();
    dataLoc.offset = vertexIndex;
    dataLoc.count = vertexCount;
    dataLoc.elementSize = sizeof(Vertex);
    
    // Load vertices, texCoords, normals, faces.
    bool hasColors = m_meshDef->m_colors.count() > 0;
    uint32_t defaultColorABGR = 0xbfffffff; // A=0.75, B=1, G=1, R=1
    for(uint32_t i = 0; i < vertexCount; i++)
    {
        Vertex v;
        v.position = m_meshDef->m_vertices.value(i) + m_meshDef->m_center;
        v.normal = m_meshDef->m_normals.value(i);
        v.texCoords = vec3(m_meshDef->m_texCoords.value(i), 0.0f);
        v.color = hasColors ? m_meshDef->m_colors.value(i) : defaultColorABGR;
        v.bone = 0;
        vertices.append(v);
    }
    
    // Load bone indices.
    foreach(vec2us g, m_meshDef->m_vertexPieces)
    {
        uint16_t count = g.first, pieceID = g.second;
        for(uint32_t i = 0; i < count; i++, vertexIndex++)
            vertices[vertexIndex].bone = pieceID;
    }
}

void WLDMesh::importIndexData(MeshBuffer *buffer, BufferSegment &indexLoc,
                                   const BufferSegment &dataLoc, uint32_t offset, uint32_t count)
{
    QVector<uint32_t> &indices = buffer->indices;
    indexLoc.offset = indices.count();
    indexLoc.count = count;
    indexLoc.elementSize = sizeof(uint32_t);
    for(uint32_t i = 0; i < count; i++)
        indices.push_back(m_meshDef->m_indices[i + offset] + dataLoc.offset);
}

MeshData * WLDMesh::importMaterialGroups(MeshBuffer *buffer, uint32_t paletteOffset)
{
    // Load material groups.
    uint32_t meshOffset = 0;
    m_data = buffer->createMesh(m_meshDef->m_polygonsByTex.count());
    for(int i = 0; i < m_meshDef->m_polygonsByTex.count(); i++)
    {
        vec2us g = m_meshDef->m_polygonsByTex[i];
        uint32_t vertexCount = g.first * 3;
        MaterialGroup &mg(m_data->matGroups[i]);
        mg.id = m_partID;
        mg.offset = meshOffset;
        mg.count = vertexCount;
        mg.matID = paletteOffset + g.second;
        meshOffset += vertexCount;
    }
    return m_data;
}

static bool materialGroupLessThan(const MaterialGroup &a, const MaterialGroup &b)
{
    return a.matID < b.matID;
}

MeshBuffer * WLDMesh::combine(const QVector<WLDMesh *> &meshes)
{
    // import each part (vertices and material groups) into a single vertex group
    MeshBuffer *meshBuf = new MeshBuffer();
    foreach(WLDMesh *mesh, meshes)
    {
        MeshData *meshData = mesh->importMaterialGroups(meshBuf, 0);
        meshBuf->addMaterialGroups(meshData);
        mesh->importVertexData(meshBuf, meshData->vertexSegment);
    }

    // sort the polygons per material and import indices
    qSort(meshBuf->matGroups.begin(), meshBuf->matGroups.end(), materialGroupLessThan);
    uint32_t indiceOffset = 0;
    for(int i = 0; i < meshBuf->matGroups.count(); i++)
    {
        MaterialGroup &mg(meshBuf->matGroups[i]);
        WLDMesh *mesh = meshes[mg.id];
        mesh->importIndexData(meshBuf, mesh->data()->indexSegment, mesh->data()->vertexSegment,
                              mg.offset, mg.count);
        mg.offset = indiceOffset;
        indiceOffset += mg.count;
    }

    // merge material groups with common material
    QVector<MaterialGroup> newGroups;
    MaterialGroup group;
    group.id = meshBuf->matGroups[0].id;
    group.offset = 0;
    group.count = 0;
    group.matID = meshBuf->matGroups[0].matID;
    for(int i = 0; i < meshBuf->matGroups.count(); i++)
    {
        MaterialGroup &mg(meshBuf->matGroups[i]);
        if(mg.matID != group.matID)
        {
            // new material - output the current group
            newGroups.append(group);
            group.id = mg.id;
            group.offset += group.count;
            group.count = 0;
            group.matID = mg.matID;
        }
        group.count += mg.count;
    }
    newGroups.append(group);
    meshBuf->matGroups = newGroups;
    return meshBuf;
}

////////////////////////////////////////////////////////////////////////////////

WLDMaterialPalette::WLDMaterialPalette(PFSArchive *archive)
{
    m_archive = archive;
    m_def = NULL;
    m_arrayOffset = 0;
}

WLDMaterialPalette::~WLDMaterialPalette()
{
    std::vector<WLDMaterialSlot *>::iterator i, e = m_materialSlots.end();
    for(i = m_materialSlots.begin(); i != e; i++)
        delete *i;
}

MaterialPaletteFragment * WLDMaterialPalette::def() const
{
    return m_def;
}

void WLDMaterialPalette::setDef(MaterialPaletteFragment *newDef)
{
    m_def = newDef;
}

uint32_t WLDMaterialPalette::arrayOffset() const
{
    return m_arrayOffset;
}

void WLDMaterialPalette::setArrayOffset(uint32_t offset)
{
    m_arrayOffset = offset;
}

std::vector<WLDMaterialSlot *> & WLDMaterialPalette::materialSlots()
{
    return m_materialSlots;
}

void WLDMaterialPalette::createSlots(bool addMatDefs)
{
    if(!m_def)
        return;
    for(uint32_t i = 0; i < m_def->m_materials.size(); i++)
    {
        MaterialDefFragment *matDef = m_def->m_materials[i];
        WLDMaterialSlot *slot = new WLDMaterialSlot(matDef->name());
        if(addMatDefs)
            slot->addSkinMaterial(0, matDef);
        slot->visible = (matDef->m_renderMode != 0);
        m_materialSlots.push_back(slot);
    }
}

WLDMaterialSlot * WLDMaterialPalette::slotByName(const QString &name) const
{
    for(uint32_t i = 0; i < m_materialSlots.size(); i++)
    {
        WLDMaterialSlot *slot = m_materialSlots[i];
        if(slot->slotName == name)
            return slot;
    }
    return NULL;
}

void WLDMaterialPalette::addMeshMaterials(MeshDefFragment *meshDef, uint32_t skinID)
{
    QVector<vec2us> &texMap = meshDef->m_polygonsByTex;
    for(uint32_t i = 0; i < texMap.size(); i++)
    {
        uint32_t slotID = texMap[i].second;
        WLDMaterialSlot *slot = m_materialSlots[slotID];
        slot->addSkinMaterial(skinID, m_def->m_materials[slotID]);
    }
}

bool WLDMaterialPalette::explodeName(QString defName, QString &charName,
                        QString &palName, QString &partName)
{
    // e.g. defName == 'ORCCH0201_MDF'
    // 'ORC' : character
    // 'CH' : piece (part 1)
    // '02' : palette ID
    // '01' : piece (part 2)
    static QRegExp r("^\\w{5}\\d{4}_MDF$");
    if(r.exactMatch(defName))
    {
        charName = defName.left(3);
        palName = defName.mid(5, 2);
        partName = defName.mid(3, 2) + defName.mid(7, 2);
        return true;
    }
    return false;
}

bool WLDMaterialPalette::explodeName(MaterialDefFragment *def, QString &charName,
                        QString &palName, QString &partName)
{
    return explodeName(def->name(), charName, palName, partName);
}

QString WLDMaterialPalette::materialName(QString defName)
{
    QString charName, palName, partName;
    if(explodeName(defName, charName, palName, partName))
        return charName + "00" + partName;
    else
        return defName.replace("_MDF", "");
}

QString WLDMaterialPalette::materialName(MaterialDefFragment *def)
{
    return materialName(def->name());
}

uint32_t WLDMaterialPalette::materialHash(QString matName)
{
    uint32_t hash = 2166136261u;
    for(int i = 0; i < matName.length(); i++)
    {
        hash ^= matName[i].toLatin1();
        hash *= 16777619u;
    }
    return hash;
}

bool WLDMaterialPalette::exportMaterial(WLDMaterial &wldMat, MaterialArray *array, uint32_t &pos)
{
    // Don't export invisible materials.
    MaterialDefFragment *matDef = wldMat.def();
    Material *mat = NULL;
    if(matDef && (matDef->m_renderMode != 0))
        mat = loadMaterial(matDef);
    wldMat.setMaterial(mat ? mat : NULL);
    wldMat.setIndex(mat ? pos : WLDMaterial::INVALID_INDEX);
    array->setMaterial(pos, mat);
    pos++;
    return (mat != NULL);
}

static uint32_t findSkinIndex(WLDMaterialSlot *slot, uint32_t skinID)
{
    // See if the slot has a material with the specified skin ID.
    const WLDMaterial *mat = slot->material(skinID);
    if(mat && mat->isValid())
        return mat->index();
    
    // Otherwise find the first valid material in the slot.
    skinID = 0;
    mat = slot->material(skinID);
    while(mat)
    {
        if(mat && mat->isValid())
            return mat->index();
        mat = slot->material(skinID++);
    }
    return -1;
}

void WLDMaterialPalette::makeSkinMap(uint32_t skinID, vector<uint32_t> &slotIndices) const
{
    size_t endSkinID = qMin(m_materialSlots.size(), slotIndices.size());
    for(size_t i = 0; i < endSkinID; i++)
    {
        WLDMaterialSlot *slot = m_materialSlots[i];
        slotIndices[i] = findSkinIndex(slot, skinID);
    }
}

void WLDMaterialPalette::makeSkinMap(const vector<uint32_t> &skinIDs, vector<uint32_t> &slotIndices) const
{
    size_t endSkinID = qMin(m_materialSlots.size(), skinIDs.size());
    Q_ASSERT(slotIndices.size() >= skinIDs.size());
    for(size_t i = 0; i < endSkinID; i++)
    {
        uint32_t skinID = skinIDs[i];
        WLDMaterialSlot *slot = m_materialSlots[i];
        slotIndices[i] = findSkinIndex(slot, skinID);
    }
}

void WLDMaterialPalette::exportTo(MaterialArray *array)
{
    // Materials are exported to the array in skin-major order. This mean we
    // export every slot of the base skin, then every slot of the first
    // alternative skin and so on.

    // Keep track of the index of the first material of this palette into the array.
    uint32_t pos = array->materials().size();
    m_arrayOffset = pos;

    // Export the base skin's materials.
    uint32_t maxSkinID = 0;
    for(uint32_t j = 0; j < m_materialSlots.size(); j++)
    {
        WLDMaterialSlot *slot = m_materialSlots[j];
        exportMaterial(slot->baseMat, array, pos);
        maxSkinID = qMax(maxSkinID, (uint32_t)slot->skinMats.size());
    }

    // Export other skins' materials.
    for(uint32_t i = 0; i < maxSkinID; i++)
    {
        for(uint32_t j = 0; j < m_materialSlots.size(); j++)
        {
            WLDMaterialSlot *slot = m_materialSlots[j];
            if(i < slot->skinMats.size())
                exportMaterial(slot->skinMats[i], array, pos);
        }
    }
}

Material * WLDMaterialPalette::loadMaterial(MaterialDefFragment *frag)
{
    if(!frag)
        return 0;
    SpriteFragment *sprite = frag->m_sprite;
    if(!sprite)
        return 0;
    SpriteDefFragment *spriteDef = sprite->m_def;
    if(!spriteDef)
        return 0;
    BitmapNameFragment *bmp = spriteDef->m_bitmaps.value(0);
    if(!bmp)
        return 0;

    bool opaque = true;
    bool dds = false;
    QImage img;
    if(m_archive)
    {
        // XXX case-insensitive lookup
        QByteArray data = m_archive->unpackFile(bmp->m_fileName.toLower());
        if(!img.loadFromData(data))
        {
            Material::loadTextureDDS(data.constData(), data.length(), img);
            dds = true;
        }
    }
    
    if(frag->m_renderMode & MaterialDefFragment::USER_DEFINED)
    {
        uint32_t renderMode = (frag->m_renderMode & ~MaterialDefFragment::USER_DEFINED);
        if(renderMode == 0x01)
        {
            // normal rendering
        }
        else if(renderMode == 0x13)
        {
            // masked texture
            if(img.colorCount() > 0)
            {
                // replace the mask color by a transparent color in the table
                QVector<QRgb> colors = img.colorTable();
                uint8_t maskColor = 0;
                colors[maskColor] = qRgba(0, 0, 0, 0);
                img.setColorTable(colors);
            }
            opaque = false;
        }
        else if(renderMode == 0x17)
        {
            // semi-transparent (e.g. the sleeper, wasp, bixie)
            // depends on how dark/light the color is
            if(img.colorCount() > 0)
            {
                QVector<QRgb> colors = img.colorTable();
                for(int i = 0; i < colors.count(); i++)
                {
                    QRgb c = colors[i];
                    int alpha = (qRed(c) + qGreen(c) + qBlue(c)) / 3;
                    colors[i] = qRgba(qRed(c), qGreen(c), qBlue(c), alpha);
                }
                img.setColorTable(colors);
            }
            opaque = false;
        }
        else if(renderMode == 0x05)
        {
            // semi-transparent (water elemental, air elemental, ghost wolf)
            if(img.colorCount() > 0)
            {
                QVector<QRgb> colors = img.colorTable();
                int alpha = 127; // arbitrary value XXX find the real value
                for(int i = 0; i < colors.count(); i++)
                {
                    QRgb c = colors[i];
                    colors[i] = qRgba(qRed(c), qGreen(c), qBlue(c), alpha);
                }
                img.setColorTable(colors);
            }
            opaque = false;
        }
        else
        {
            qDebug("Unknown render mode %x", renderMode);
        }
    }

    Material *mat = new Material();
    mat->setOpaque(opaque);
    mat->setImage(img);
    mat->setOrigin(dds ? Material::LowerLeft : Material::UpperLeft);
    return mat;
}

///////////////////////////////////////////////////////////////////////////////

WLDMaterial::WLDMaterial()
{
    m_mat = NULL;
    m_def = NULL;
    m_index = INVALID_INDEX;
}

bool WLDMaterial::isValid() const
{
    return m_def && (m_index != INVALID_INDEX);
}

uint32_t WLDMaterial::index() const
{
    return m_index;
}

void WLDMaterial::setIndex(uint32_t index)
{
    m_index = index;
}

MaterialDefFragment * WLDMaterial::def()
{
    return m_def;
}

void WLDMaterial::setDef(MaterialDefFragment *matDef)
{
    if(matDef->handled())
        return;
    if((m_def != NULL) && (m_def != matDef))
        qDebug("warning: duplicated material definitions '%s' (fragments %d and %d)",
               m_def->name().toLatin1().constData(), m_def->ID(), matDef->ID());
    m_def = matDef;
    matDef->setHandled(true);
}

Material * WLDMaterial::material() const
{
    return m_mat;
}

void WLDMaterial::setMaterial(Material *material)
{
    m_mat = material;
}

///////////////////////////////////////////////////////////////////////////////

WLDMaterialSlot::WLDMaterialSlot(QString matName)
{
    QString charName, palName;
    if(!WLDMaterialPalette::explodeName(matName, charName, palName, slotName))
        slotName = matName.replace("_MDF", "");
    offset = 0;
    visible = false;
}

const WLDMaterial * WLDMaterialSlot::material(uint32_t skinID) const
{
    if(skinID == 0)
        return &baseMat;
    else if(skinID <= skinMats.size())
        return &skinMats[skinID - 1];
    else
        return NULL;
}

void WLDMaterialSlot::addSkinMaterial(uint32_t skinID, MaterialDefFragment *matDef)
{
    if(skinID == 0)
    {
        // Skin zero is the base skin.
        baseMat.setDef(matDef);
    }
    else
    {
        if(skinID > skinMats.size())
            skinMats.resize(skinID);
        skinMats[skinID-1].setDef(matDef);
    }
}

////////////////////////////////////////////////////////////////////////////////

WLDModelSkin::WLDModelSkin(QString name, WLDModel *model)
{
    m_name = name;
    m_model = model;
    WLDModelSkin *defaultSkin = model->skin();
    if(defaultSkin)
    {
        foreach(WLDMesh *part, defaultSkin->parts())
            m_parts.append(part);
        updateBounds();
    }
}

WLDModelSkin::~WLDModelSkin()
{
}

QString WLDModelSkin::name() const
{
    return m_name;
}

const AABox & WLDModelSkin::boundsAA() const
{
    return m_boundsAA;
}

const QList<WLDMesh *> & WLDModelSkin::parts() const
{
    return m_parts;
}

void WLDModelSkin::addPart(MeshDefFragment *frag)
{
    if(!frag)
        return;
    uint32_t partID = m_parts.count();
    WLDMesh *meshPart = new WLDMesh(frag, partID);
    m_parts.append(meshPart);
    m_model->m_meshes.append(meshPart);
    frag->setHandled(true);
    updateBounds();
}

void WLDModelSkin::replacePart(WLDMesh *basePart, MeshDefFragment *frag)
{
    uint32_t partID = basePart->partID();
    if(partID >= m_parts.size())
        return;
    if((basePart == m_parts[partID]) && (basePart->def() != frag))
    {
        WLDMesh *meshPart = new WLDMesh(frag, partID);
        m_parts[partID] = meshPart;
        m_model->m_meshes.append(meshPart);
    }
    updateBounds();
}

bool WLDModelSkin::explodeMeshName(QString defName, QString &actorName,
                            QString &meshName, QString &skinName)
{
    // e.g. defName == 'ELEHE00_DMSPRITEDEF'
    // 'ELE' : character
    // 'HE' : mesh
    // '00' : skin ID
    static QRegExp r("^(\\w{3})(.*)(\\d{2})_DMSPRITEDEF$");
    if(r.exactMatch(defName))
    {
        actorName = r.cap(1);
        meshName = r.cap(2);
        skinName = r.cap(3);
        return true;
    }
    return false;
}

void WLDModelSkin::updateBounds()
{
    if(m_parts.count() == 0)
    {
        m_boundsAA = AABox();
    }
    else
    {
        m_boundsAA = m_parts[0]->boundsAA();
        for(int i = 1; i < m_parts.count(); i++)
        {
            // XXX sort out skinning
            m_boundsAA.extendTo(m_parts[i]->boundsAA());
        }
    }
}

void WLDModelSkin::draw(RenderProgram *prog, const BoneTransform *bones, uint32_t boneCount)
{
    MeshBuffer *meshBuf = m_model->buffer();
    if(!meshBuf)
        return;

    // Gather material groups from all the mesh parts we want to draw.
    meshBuf->matGroups.clear();
    foreach(WLDMesh *mesh, m_parts)
        meshBuf->addMaterialGroups(mesh->data());

    // Draw all the material groups in one draw call.
    prog->beginDrawMesh(meshBuf, m_model->materials(), bones, boneCount);
    prog->drawMesh();
    prog->endDrawMesh();
}
