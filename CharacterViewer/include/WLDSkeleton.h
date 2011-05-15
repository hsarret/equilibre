#ifndef OPENEQ_WLD_SKELETON_H
#define OPENEQ_WLD_SKELETON_H

#include <QObject>
#include <QMap>
#include <QVector3D>
#include <QQuaternion>
#include "Platform.h"
#include "Vertex.h"
#include "WLDFragment.h"

class HierSpriteDefFragment;
class TrackDefFragment;
class TrackFragment;
class MeshFragment;
class WLDAnimation;

class SkeletonNode
{
public:
    WLDFragmentRef name;
    uint32_t flags;
    TrackFragment *track;
    MeshFragment *mesh;
    QVector<uint32_t> children;
};

class BoneTransform
{
public:
    QVector3D location;
    QQuaternion rotation;

    inline vec3 map(const vec3 &v)
    {
        QVector3D v2(v.x, v.y, v.z);
        v2 = rotation.rotatedVector(v2) + location;
        return vec3(v2.x(), v2.y(), v2.z());
    }

    inline QVector3D map(const QVector3D &v)
    {
        return rotation.rotatedVector(v) + location;
    }
};

/*!
  \brief Holds information about a model's skeleton, used for animation.
  */
class WLDSkeleton : public QObject
{
public:
    WLDSkeleton(HierSpriteDefFragment *def, QObject *parent = 0);

    WLDAnimation *pose() const;
    const QVector<SkeletonNode> &tree() const;

    void addTrack(QString animName, TrackDefFragment *track);

private:
    HierSpriteDefFragment *m_def;
    QMap<QString, WLDAnimation *> m_animations;
    WLDAnimation *m_pose;
};

/*!
  \brief Describes one way of animating a model's skeleton.
  */
class WLDAnimation : public QObject
{
public:
    WLDAnimation(QString name, QVector<TrackDefFragment *> tracks, WLDSkeleton *skel,
                 QObject *parent = 0);

    QString name() const;

    void replaceTrack(TrackDefFragment *track);
    WLDAnimation * copy(QString newName, QObject *parent = 0) const;
    QVector<BoneTransform> transformations(uint32_t frame = 0) const;

private:
    void transformPiece(QVector<BoneTransform> &transforms, const QVector<SkeletonNode> &tree,
        uint32_t pieceID, uint32_t frameIndex, BoneTransform parentTrans) const;

    QString m_name;
    QVector<TrackDefFragment *> m_tracks;
    WLDSkeleton *m_skel;
    uint32_t m_frameCount;
};

#endif