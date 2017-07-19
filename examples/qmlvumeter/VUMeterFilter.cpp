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
    const uchar* data = frame->constBits();
    const int s = frame->samplesPerChannel();
    const int cn = frame->channelCount();
    const int r = 1;
    float level[2];
    const AudioFormat& af = frame->format();
    if (af.isFloat()) {
        float max[2];
        max[0] = max[1] = 0;
        const float *p = (float*)data;
        for (int i = 0; i < s; i+=cn) {
            max[0] = qMax(max[0], qAbs(p[i]));
            max[1] = qMax(max[1], qAbs(p[i+r]));
        }
        level[0] = 20.0f * log10(max[0]);
        level[1] = 20.0f * log10(max[1]);
    } else if (!af.isUnsigned()) {
        const int b = af.sampleSize();
        if (b == 2) {
            qint16 max[2];
            max[0] = max[1] = 0;
            const qint16 *p = (qint16*)data;
            for (int i = 0; i < s; i+=cn) {
                max[0] = qMax(max[0], qAbs(p[i]));
                max[1] = qMax(max[1], qAbs(p[i+r]));
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
}
