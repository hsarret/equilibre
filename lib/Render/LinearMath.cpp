#include <cmath>
#include <QMatrix4x4>
#include "OpenEQ/Render/LinearMath.h"

using namespace std;

bool fequal(double a, double b)
{
    return fabs(a - b) < 1e-16;
}

vec3 vec3::normalized() const
{
    float w = (float)sqrt(x * x + y * y + z * z);
    return vec3(x / w, y / w, z / w);
}

float vec3::dot(const vec3 &a, const vec3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec3::cross(const vec3 &u, const vec3 &v)
{
    vec3 n;
    n.x = (u.y * v.z - u.z * v.y);
    n.y = (u.z * v.x - u.x * v.z);
    n.z = (u.x * v.y - u.y * v.x);
    return n;
}

vec3 vec3::normal(const vec3 &a, const vec3 &b, const vec3 &c)
{
    // the cross-product of AB and AC is the normal of ABC
    return cross(b - a, c - a).normalized();
}

vec3 operator-(const vec3 &a)
{
    return vec3(-a.x, -a.y, -a.z);
}

vec3 operator+(const vec3 &a, const vec3 &b)
{
    vec3 u;
    u.x = a.x + b.x;
    u.y = a.y + b.y;
    u.z = a.z + b.z;
    return u;
}

vec3 operator-(const vec3 &a, const vec3 &b)
{
    vec3 u;
    u.x = a.x - b.x;
    u.y = a.y - b.y;
    u.z = a.z - b.z;
    return u;
}

vec3 operator*(const vec3 &a, float scalar)
{
    vec3 u;
    u.x = a.x * scalar;
    u.y = a.y * scalar;
    u.z = a.z * scalar;
    return u;
}

float vec4::dot(const vec4 &a, const vec4 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

////////////////////////////////////////////////////////////////////////////////

matrix4::matrix4()
{
    clear();
}

matrix4::matrix4(const QMatrix4x4 &m)
{
    const qreal *md = m.constData();
    for(int i = 0; i < 4; i++, md += 4)
        r[i] = vec4(md[0], md[1], md[2], md[3]);
}

const vec4 *matrix4::data() const
{
    return r;
}

vec3 matrix4::map(const vec3 &v) const
{
    vec4 v4(v.x, v.y, v.z, 1.0);
    float x = vec4::dot(r[0], v4);
    float y = vec4::dot(r[1], v4);
    float z = vec4::dot(r[2], v4);
    float w = vec4::dot(r[3], v4);
    return vec3(x / w, y / w, z / w);
}

void matrix4::clear()
{
    for(int i = 0; i < 4; i++)
        r[i] = vec4();
}

void matrix4::setIdentity()
{
    r[0] = vec4(1.0, 0.0, 0.0, 0.0);
    r[1] = vec4(0.0, 1.0, 0.0, 0.0);
    r[2] = vec4(0.0, 0.0, 1.0, 0.0);
    r[3] = vec4(0.0, 0.0, 0.0, 1.0);
}

matrix4 matrix4::translate(float dx, float dy, float dz)
{
    matrix4 m;
    m.setIdentity();
    m.r[3] = vec4(dx, dy, dz, 1.0);
    return m;
}

matrix4 matrix4::rotate(float angle, float x, float y, float z)
{
    float theta = angle / 180.0 * M_PI;
    float c = cos(theta);
    float s = sin(theta);
    float t = 1 - c;
    matrix4 m;
    m.r[0] = vec4(t * x * x + c, t * x * y + s * z, t * x * z - s * y, 0.0);
    m.r[1] = vec4(t * x * y - s * z, t * y * y + c, t * y * z + s * x, 0.0);
    m.r[2] = vec4(t * x * z + s * y, t * y * z - s * x, t * z * z + c, 0.0);
    m.r[3] = vec4(0.0, 0.0, 0.0, 1.0);
    return m;
}

matrix4 matrix4::scale(float sx, float sy, float sz)
{
    matrix4 m;
    m.r[0] = vec4(sx, 0.0, 0.0, 0.0);
    m.r[1] = vec4(0.0, sy, 0.0, 0.0);
    m.r[2] = vec4(0.0, 0.0, sz, 0.0);
    m.r[3] = vec4(0.0, 0.0, 0.0, 1.0);
    return m;
}

// The following three functions have been adapted from Qt:
// http://qt.gitorious.org/qt/qt/blobs/raw/4.7/src/gui/math3d/qmatrix4x4.h
// http://qt.gitorious.org/qt/qt/blobs/raw/4.7/src/gui/math3d/qmatrix4x4.cpp

matrix4 matrix4::perspective(float angle, float aspect, float nearPlane, float farPlane)
{
    matrix4 m;
    if (nearPlane == farPlane || aspect == 0.0)
        return m;
    float radians = (angle / 2.0) * M_PI / 180.0;
    float sine = sin(radians);
    if(fequal(sine, 0.0))
        return m;
    float cotan = cos(radians) / sine;
    float clip = farPlane - nearPlane;
    m.r[0] = vec4(cotan / aspect, 0.0, 0.0, 0.0);
    m.r[1] = vec4(0.0, cotan, 0.0, 0.0);
    m.r[2] = vec4(0.0, 0.0, -(nearPlane + farPlane) / clip, -1.0);
    m.r[3] = vec4(0.0, 0.0, -(2.0 * nearPlane * farPlane) / clip, 0.0);
    return m;
}

matrix4 matrix4::ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    matrix4 m;
    if (left == right || bottom == top || nearPlane == farPlane)
        return m;
    float width = right - left;
    float invheight = top - bottom;
    float clip = farPlane - nearPlane;
    m.r[0] = vec4(2.0 / width, 0.0, 0.0, 0.0);
    m.r[1] = vec4(0.0, 2.0 / invheight, 0.0, 0.0);
    m.r[2] = vec4(0.0, 0.0, -2.0 / clip, 0.0);
    m.r[3] = vec4(-(left + right) / width, -(top + bottom) / invheight, -(nearPlane + farPlane) / clip, 1.0);
    return m;
}

matrix4 matrix4::operator*(const matrix4 &b)
{
    matrix4 m;
    m.r[0].x = r[0].x * b.r[0].x + r[1].x * b.r[0].y + r[2].x * b.r[0].z + r[3].x * b.r[0].w;
    m.r[0].y = r[0].y * b.r[0].x + r[1].y * b.r[0].y + r[2].y * b.r[0].z + r[3].y * b.r[0].w;
    m.r[0].z = r[0].z * b.r[0].x + r[1].z * b.r[0].y + r[2].z * b.r[0].z + r[3].z * b.r[0].w;
    m.r[0].w = r[0].w * b.r[0].x + r[1].w * b.r[0].y + r[2].w * b.r[0].z + r[3].w * b.r[0].w;
    
    m.r[1].x = r[0].x * b.r[1].x + r[1].x * b.r[1].y + r[2].x * b.r[1].z + r[3].x * b.r[1].w;
    m.r[1].y = r[0].y * b.r[1].x + r[1].y * b.r[1].y + r[2].y * b.r[1].z + r[3].y * b.r[1].w;
    m.r[1].z = r[0].z * b.r[1].x + r[1].z * b.r[1].y + r[2].z * b.r[1].z + r[3].z * b.r[1].w;
    m.r[1].w = r[0].w * b.r[1].x + r[1].w * b.r[1].y + r[2].w * b.r[1].z + r[3].w * b.r[1].w;
    
    m.r[2].x = r[0].x * b.r[2].x + r[1].x * b.r[3].x + r[2].x * b.r[2].z + r[3].x * b.r[2].w;
    m.r[2].y = r[0].y * b.r[2].x + r[1].y * b.r[2].y + r[2].y * b.r[2].z + r[3].y * b.r[2].w;
    m.r[2].z = r[0].z * b.r[2].x + r[1].z * b.r[2].y + r[2].z * b.r[2].z + r[3].z * b.r[2].w;
    m.r[2].w = r[0].w * b.r[2].x + r[1].w * b.r[2].y + r[2].w * b.r[2].z + r[3].w * b.r[2].w;
    
    m.r[3].x = r[0].x * b.r[3].x + r[1].x * b.r[3].y + r[2].x * b.r[3].z + r[3].x * b.r[3].w;
    m.r[3].y = r[0].y * b.r[3].x + r[1].y * b.r[3].y + r[2].y * b.r[3].z + r[3].y * b.r[3].w;
    m.r[3].z = r[0].z * b.r[3].x + r[1].z * b.r[3].y + r[2].z * b.r[3].z + r[3].z * b.r[3].w;
    m.r[3].w = r[0].w * b.r[3].x + r[1].w * b.r[3].y + r[2].w * b.r[3].z + r[3].w * b.r[3].w;
    return m;
}

matrix4 matrix4::lookAt(vec3 eye, vec3 center, vec3 up)
{
    vec3 forward = (center - eye).normalized();
    vec3 side = vec3::cross(forward, up).normalized();
    up = vec3::cross(side, forward);

    matrix4 m;
    m.r[0] = vec4(side.x, up.x, -forward.x, 0.0);
    m.r[1] = vec4(side.y, up.y, -forward.y, 0.0);
    m.r[2] = vec4(side.z, up.z, -forward.z, 0.0);
    m.r[3] = vec4(0.0, 0.0, 0.0, 1.0);
    return m * matrix4::translate(-eye.x, -eye.y, -eye.z);
}

////////////////////////////////////////////////////////////////////////////////

BoneTransform::BoneTransform()
{
}

BoneTransform::BoneTransform(const vec4 &loc, const vec4 &rot)
{
    location = QVector4D(loc.x, loc.y, loc.z, loc.w);
    rotation = QQuaternion(rot.w, rot.x, rot.y, rot.z);
}

vec3 BoneTransform::map(const vec3 &v)
{
    QVector3D v2(v.x, v.y, v.z);
    v2 = rotation.rotatedVector(v2) + location.toVector3D();
    return vec3(v2.x(), v2.y(), v2.z());
}

QVector4D BoneTransform::map(const QVector4D &v)
{
    return QVector4D(rotation.rotatedVector(v.toVector3D()) + location.toVector3D(), 1.0);
}

BoneTransform BoneTransform::interpolate(BoneTransform a, BoneTransform b, double f)
{
    BoneTransform c;
    c.rotation = QQuaternion::slerp(a.rotation, b.rotation, f);
    c.location = (a.location * (1.0 - f)) + (b.location * f);
    return c;
}

void BoneTransform::toDualQuaternion(vec4 &d0, vec4 &d1) const
{
    const QVector4D &tran(location);
    d0.x = rotation.x();
    d0.y = rotation.y();
    d0.z = rotation.z();
    d0.w = rotation.scalar();
    d1.x = 0.5f * (tran.x() * d0.w + tran.y() * d0.z - tran.z() * d0.y);
    d1.y = 0.5f * (-tran.x() * d0.z + tran.y() * d0.w + tran.z() * d0.x);
    d1.z = 0.5f * (tran.x() * d0.y - tran.y() * d0.x + tran.z() * d0.w);
    d1.w = -0.5f * (tran.x() * d0.x + tran.y() * d0.y + tran.z() * d0.z);
}
