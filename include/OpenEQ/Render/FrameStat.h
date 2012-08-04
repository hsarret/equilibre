#ifndef OPENEQ_STATISTIC_H
#define OPENEQ_STATISTIC_H

#include <QString>
#include <QVector>
#include "OpenEQ/Render/Platform.h"

typedef uint32_t gpu_timer_t;

class RENDER_DLL FrameStat
{
public:
    enum TimerType
    {
        WallTime,
        CPUTime,
        GPUTime
    };

    FrameStat(QString name, int samples, TimerType type);
    virtual ~FrameStat();

    QString name() const;
    float average() const;
    TimerType type() const;

    float current() const;
    void setCurrent(float s);
    void beginTime();
    void endTime();
    void clear();
    void next();

private:
    QString m_name;
    QVector<float> m_samples;
    TimerType m_timerType;
    bool m_pendingGpuQuery;
    gpu_timer_t m_timer;
    int m_current;
    int m_count;
    double m_startTime;
};

#endif
