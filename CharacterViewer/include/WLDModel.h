#ifndef OPENEQ_WLD_MODEL_H
#define OPENEQ_WLD_MODEL_H

#include <QObject>
#include <QList>
#include <QMap>
#include "Platform.h"
#include "RenderState.h"

class Mesh;
class MeshFragment;
class MaterialDefFragment;
class Material;

/*!
  \brief Describes a model (such as an object or a character) that can be rendered.
  */
class WLDModel : public QObject, StateObject
{
public:
    WLDModel(RenderState *state, QString path, QObject *parent = 0);
    virtual ~WLDModel();

    void addMesh(MeshFragment *frag);
    void draw();

private:
    void importVertexGroups(MeshFragment *frag, Mesh *m);
    Material * importMaterial(MaterialDefFragment *frag);

    QList<Mesh *> m_meshes;
    QList<MeshFragment *> m_meshFrags;
    QMap<QString, Material *> m_materials;
    QString m_path;
};

#endif
