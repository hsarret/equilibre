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

#ifndef EQUILIBRE_WLD_MODEL_H
#define EQUILIBRE_WLD_MODEL_H

#include <QList>
#include <QMap>
#include "EQuilibre/Render/Platform.h"
#include "EQuilibre/Render/Vertex.h"
#include "EQuilibre/Render/Geometry.h"

class MeshDefFragment;
class HierSpriteDefFragment;
class MaterialDefFragment;
class MaterialPaletteFragment;
class ActorDefFragment;
class Material;
class MaterialMap;
class PFSArchive;
class RenderContext;
class RenderProgram;
class MeshData;
class MeshBuffer;
class WLDMesh;
class WLDModelSkin;
class WLDSkeleton;
class BoneTransform;
class WLDAnimation;
class WLDMaterialPalette;

/*!
  \brief Describes a model (such as an object or a character) that can be rendered.
  */
class GAME_DLL WLDModel
{
public:
    WLDModel(PFSArchive *archive);
    virtual ~WLDModel();

    static QList<MeshDefFragment *> listMeshes(ActorDefFragment *def);

    MeshBuffer * buffer() const;
    void setBuffer(MeshBuffer *newBuffer);

    WLDSkeleton *skeleton() const;
    void setSkeleton(WLDSkeleton *skeleton);

    WLDMaterialPalette * palette() const;

    MaterialMap *materials() const;
    void setMaterials(MaterialMap *newMaterials);

    WLDModelSkin *skin() const;
    const QMap<QString, WLDModelSkin *> & skins() const;

    WLDModelSkin * newSkin(QString name);

private:
    friend class WLDModelSkin;
    MeshBuffer *m_buffer;
    WLDSkeleton *m_skel;
    WLDModelSkin *m_skin;
    QMap<QString, WLDModelSkin *> m_skins;
    QList<WLDMesh *> m_meshes;
    WLDMaterialPalette *m_palette;
    MaterialMap *m_materials;
};

/*!
  \brief Describes part of a model.
  */
class GAME_DLL WLDMesh
{
public:
    WLDMesh(MeshDefFragment *meshDef, uint32_t partID);
    virtual ~WLDMesh();

    MeshData * data() const;
    void setData(MeshData *data);
    MeshDefFragment *def() const;
    WLDMaterialPalette * palette() const;
    uint32_t partID() const;
    const AABox & boundsAA() const;

    void importPalette(PFSArchive *archive);
    MeshData * importFrom(MeshBuffer *meshBuf, uint32_t paletteOffset = 0);
    static MeshBuffer *combine(const QVector<WLDMesh *> &meshes);

private:
    void importVertexData(MeshBuffer *buffer, BufferSegment &dataLoc);
    void importIndexData(MeshBuffer *buffer, BufferSegment &indexLoc,
                         const BufferSegment &dataLoc, uint32_t offset, uint32_t count);
    MeshData * importMaterialGroups(MeshBuffer *buffer, uint32_t paletteOffset);
    
    uint32_t m_partID;
    MeshData *m_data;
    MeshDefFragment *m_meshDef;
    WLDMaterialPalette *m_palette;
    AABox m_boundsAA;
};

class GAME_DLL WLDMaterial
{
public:
    WLDMaterial();

    // Index of the material in the material map.
    uint32_t index() const;
    void setIndex(uint32_t index);
    
    MaterialDefFragment *def();
    void setDef(MaterialDefFragment *matDef);
    
    Material *material() const;
    void setMaterial(Material *material);

    const static uint32_t INVALID_INDEX = (uint32_t)-1;

private:
    MaterialDefFragment *m_def;
    Material *m_mat;
    uint32_t m_index;
};

/*!
  \brief Defines a set of possible materials that can be used interchangeably.
  */
class GAME_DLL WLDMaterialSlot
{
public:
    WLDMaterialSlot(QString matName);

    void addSkinMaterial(uint32_t skinID, MaterialDefFragment *matDef);

    // Name of the slot. PC models use standardized slot names.
    QString slotName;
    // Default material for the slot (e.g. referred to by the MaterialPaletteFragment).
    WLDMaterial baseMat;
    // Alternative materials for the slot, one for each alternative skin.
    // The material at index zero is for skin one and so on.
    std::vector<WLDMaterial> skinMats;
    // Index of the first texture in this slot's texture list.
    // The textures are stored sequentially with increasing skin ID.
    uint32_t offset;
    bool visible;
};

/*!
  \brief Defines a set of materials (e.g.\ textures) that can be used for a model.
  The palette contains all materials that can be used with the model.
  */
class GAME_DLL WLDMaterialPalette
{
public:
    WLDMaterialPalette(PFSArchive *archive);
    virtual ~WLDMaterialPalette();

    MaterialPaletteFragment *def() const;
    void setDef(MaterialPaletteFragment *newDef);

    uint32_t mapOffset() const;
    void setMapOffset(uint32_t offset);
    
    std::vector<WLDMaterialSlot *> & materialSlots();
    WLDMaterialSlot * slotByName(const QString &name) const;
    
    void createSlots(bool addMatDefs = true);
    void addMeshMaterials(MeshDefFragment *meshDef, uint32_t skinID);
    void exportTo(MaterialMap *map);

    /*!
      \brief Return the canonical name of a material. This strips out the skin ID.
      */
    static QString materialName(QString defName);
    static QString materialName(MaterialDefFragment *def);
    static uint32_t materialHash(QString matName);

    static bool explodeName(QString defName, QString &charName,
                            QString &palName, QString &partName);
    static bool explodeName(MaterialDefFragment *def, QString &charName,
                            QString &palName, QString &partName);

private:
    Material * loadMaterial(MaterialDefFragment *frag);
    bool exportMaterial(WLDMaterial &wldMat, MaterialMap *map, uint32_t &pos);

    MaterialPaletteFragment *m_def;
    std::vector<WLDMaterialSlot *> m_materialSlots;
    uint32_t m_mapOffset;
    PFSArchive *m_archive;
};

/*!
  \brief Describes one way an actor can be rendered. A skin can use alternative
  materials and meshes (e.g.\ for a character's head).
  */
class GAME_DLL WLDModelSkin
{
public:
    WLDModelSkin(QString name, WLDModel *model);
    virtual ~WLDModelSkin();

    QString name() const;
    
    const AABox & boundsAA() const;
    
    const QList<WLDMesh *> & parts() const;

    void addPart(MeshDefFragment *frag);
    void replacePart(WLDMesh *basePart, MeshDefFragment *frag);

    static bool explodeMeshName(QString defName, QString &actorName,
                                QString &meshName, QString &skinName);

    void draw(RenderProgram *prog, const BoneTransform *bones = 0, uint32_t boneCount = 0);

private:
    void updateBounds();
    
    QString m_name;
    WLDModel *m_model;
    QList<WLDMesh *> m_parts;
    AABox m_boundsAA;
};

#endif
