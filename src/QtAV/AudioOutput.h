#ifndef QAVAUDIOWRITER_H
#define QAVAUDIOWRITER_H

#include <QtAV/AVOutput.h>

struct PaStreamParameters;
typedef void PaStream;

namespace QtAV {

class Q_EXPORT AudioOutput : public AVOutput
{
public:
    AudioOutput();
    virtual ~AudioOutput();

    void setSampleRate(int rate);
    void setChannels(int channels);
    int write(const QByteArray& data);

    virtual bool open();
    virtual bool close();

protected:
    PaStreamParameters *outputParameters;
    PaStream *stream;
    int sample_rate;
    double outputLatency;
#ifdef Q_OS_LINUX
    bool autoFindMultichannelDevice;
#endif
};
}

#endif // QAVAUDIOWRITER_H
