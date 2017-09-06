#include "VUMeterFilter.h"
#include <QtAV/AudioFrame.h>
#include <cmath>

VUMeterFilter::VUMeterFilter(QObject *parent)
    : AudioFilter(parent)
    , mLeft(0)
    , mRight(0)
{
}

void VUMeterFilter::process(Statistics *statistics, AudioFrame *frame)
{
    if (!frame)
        return;
    const AudioFormat& af = frame->format();
    int step = frame->channelCount();
    const quint8* data[2];
    data[0] = frame->constBits(0);
    if (frame->planeCount() > 0) {
        step = 1;
        data[1] = frame->constBits(1);
    } else {
        data[1] = data[0] + step*af.sampleSize();
    }
    const int s = frame->samplesPerChannel();
    const int r = 1;
    float level[2];
    if (af.isFloat()) {
        float max[2];
        max[0] = max[1] = 0;
        const float *p0 = (float*)data[0];
        const float *p1 = (float*)data[1];
        for (int i = 0; i < s; i+=step) {
            max[0] = qMax(max[0], qAbs(p0[i]));
            max[1] = qMax(max[1], qAbs(p1[i]));
        }
        level[0] = 20.0f * log10(max[0]);
        level[1] = 20.0f * log10(max[1]);
    } else if (!af.isUnsigned()) {
        const int b = af.sampleSize();
        if (b == 2) {
            qint16 max[2];
            max[0] = max[1] = 0;
            const qint16 *p0 = (qint16*)data[0];
            const qint16 *p1 = (qint16*)data[1];
            for (int i = 0; i < s; i+=step) {
                max[0] = qMax(max[0], qAbs(p0[i]));
                max[1] = qMax(max[1], qAbs(p1[i]));
            }
            level[0] = 20.0f * log10((double)max[0]/(double)INT16_MAX);
            level[1] = 20.0f * log10((double)max[1]/(double)INT16_MAX);
        }
    }
    if (!qFuzzyCompare(level[0], mLeft)) {
        mLeft = level[0];
        Q_EMIT leftLevelChanged(mLeft);
    }
    if (!qFuzzyCompare(level[1], mRight)) {
        mRight = level[1];
        Q_EMIT leftLevelChanged(mRight);
    }
    //qDebug("db: %d --- %d", mLeft, mRight);
}
