#ifndef OPENEQ_VERTEX_H
#define OPENEQ_VERTEX_H

#include <inttypes.h>
#include <QString>
#include <QVector>

bool fequal(double a, double b);

class Material;
class WLDMaterialPalette;

class vec2
{
public:
    float x;
    float y;

    inline vec2()
    {
        x = y = 0.0;
    }

    inline vec2(float x, float y)
    {
        this->x = x;
        this->y = y;
    }
};

class vec3
{
public:
    float x;
    float y;
    float z;

    inline vec3()
    {
        x = y = z = 0.0;
    }

    inline vec3(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    static vec3 normal(const vec3 &a, const vec3 &b, const vec3 &c);
};

vec3 operator+(const vec3 &a, const vec3 &b);
vec3 operator-(const vec3 &a, const vec3 &b);

class vec4
{
public:
    float x;
    float y;
    float z;
    float w;

    inline vec4()
    {
        x = y = z = w = 0.0;
    }

    inline vec4(float x, float y, float z, float w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }
};

class matrix4
{
public:
    float d[16];

    matrix4();

    vec3 map(const vec3 &v) const;
    vec3 mapNormal(const vec3 &v) const;

    void clear();
    void setIdentity();

    void dump() const;

    static matrix4 translate(float dx, float dy, float dz);
    static matrix4 rotate(float angle, float rx, float ry, float rz);
    static matrix4 scale(float sx, float sy, float sz);
    static matrix4 perspective(float angle, float aspect, float nearPlane, float farPlane);
    static matrix4 ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane);
};

matrix4 operator*(const matrix4 &a, const matrix4 &b);

class VertexData
{
public:
    vec3 position;
    vec3 normal;
    vec2 texCoords;
    uint32_t bone;
    uint32_t padding[3]; // align on 16-bytes boundaries
};

class MaterialGroup
{
public:
    uint32_t id;
    uint32_t offset;
    uint32_t count;
    QString matName;
    WLDMaterialPalette *palette;
};

class VertexGroup
{
public:
    VertexGroup(uint32_t mode, uint32_t count);
    VertexGroup(uint32_t mode, const std::vector<VertexData> &data);
    virtual ~VertexGroup();

    uint32_t mode;
    uint32_t count;
    VertexData *data;
    QVector<uint32_t> indices;
    QVector<MaterialGroup> matGroups;
    uint32_t buffer;
};

#endif
