#ifndef OPENEQ_SHADER_PROGRAM_GL2_H
#define OPENEQ_SHADER_PROGRAM_GL2_H

#include <vector>
#include <QString>
#include "OpenEQ/Render/Platform.h"
#include "OpenEQ/Render/Vertex.h"

const int MAX_TRANSFORMS = 256;
const int A_POSITION = 0;
const int A_NORMAL = 1;
const int A_TEX_COORDS = 2;
const int A_BONE_INDEX = 3;
const int A_MODEL_VIEW_0 = 4;
const int A_MAX = A_MODEL_VIEW_0;

const int U_MODELVIEW_MATRIX = 0;
const int U_PROJECTION_MATRIX = 1;
const int U_MAT_AMBIENT = 2;
const int U_MAT_DIFFUSE = 3;
const int U_MAT_HAS_TEXTURE = 4;
const int U_MAT_TEXTURE = 5;
const int U_MAX = U_MAT_TEXTURE;

class RenderStateGL2;
class BoneTransform;
class Material;
class MaterialMap;

class MeshDataGL2
{
public:
    void clear();
    
    const MeshBuffer *meshBuf;
    const BoneTransform *bones;
    uint32_t boneCount;
    MaterialMap *materials;
    const uint32_t *indices;
    bool haveIndices;
    bool pending;
};

class ShaderProgramGL2
{
public:
    ShaderProgramGL2(RenderStateGL2 *state);
    virtual ~ShaderProgramGL2();

    bool loaded() const;
    bool current() const;
    uint32_t program() const;
    int drawCalls() const;
    int textureBinds() const;
    void resetFrameStats();

    bool load(QString vertexFile, QString fragmentFile);

    virtual bool init();
    
    virtual void beginDrawMesh(const MeshBuffer *meshBuf, MaterialMap *materials,
                               const BoneTransform *bones, int boneCount);
    virtual void drawMeshBatch(const matrix4 *mvMatrices, uint32_t instances);
    virtual void endDrawMesh();

    void setModelViewMatrix(const matrix4 &modelView);
    void setProjectionMatrix(const matrix4 &projection);
    void setMatrices(const matrix4 &modelView, const matrix4 &projection);
    void setBoneTransforms(const BoneTransform *transforms, int count);

    void enableVertexAttribute(int attr, int index = 0);
    void disableVertexAttribute(int attr, int index = 0);
    void uploadVertexAttributes(const MeshBuffer *meshBuf);

protected:
    bool compileProgram(QString vertexFile, QString fragmentFile);
    static uint32_t loadShader(QString path, uint32_t type);
    void beginApplyMaterial(MaterialMap *map, Material *m);
    void endApplyMaterial(MaterialMap *map, Material *m);
    void drawMaterialGroup(const MaterialGroup &mg);
    virtual void beginSkinMesh();
    virtual void endSkinMesh();

    RenderStateGL2 *m_state;
    uint32_t m_vertexShader;
    uint32_t m_fragmentShader;
    uint32_t m_program;
    int m_attr[A_MAX+1];
    int m_uniform[U_MAX+1];
    vec4 *m_bones;
    MeshDataGL2 m_meshData;
    int m_drawCalls;
    int m_textureBinds;
};

class UniformSkinningProgram : public ShaderProgramGL2
{
public:
    UniformSkinningProgram(RenderStateGL2 *state);
    virtual bool init();
    virtual void beginSkinMesh();
    virtual void endSkinMesh();

private:
    int m_bonesLoc;
};

class TextureSkinningProgram : public ShaderProgramGL2
{
public:
    TextureSkinningProgram(RenderStateGL2 *state);
    virtual ~TextureSkinningProgram();
    virtual bool init();
    virtual void beginSkinMesh();
    virtual void endSkinMesh();

private:
    int m_bonesLoc;
    uint32_t m_boneTexture;
};

#endif
