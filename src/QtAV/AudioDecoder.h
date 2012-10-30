#ifndef QAVAUDIODECODER_H
#define QAVAUDIODECODER_H

#include <QtAV/AVDecoder.h>

namespace QtAV {
class AudioDecoderPrivate;
class Q_EXPORT AudioDecoder : public AVDecoder
{
public:
    AudioDecoder();
    virtual bool decode(const QByteArray &encoded);

private:
    Q_DECLARE_PRIVATE(AudioDecoder)
};
}

#endif // QAVAUDIODECODER_H
