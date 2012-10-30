#ifndef QAVDECODER_H
#define QAVDECODER_H

#include <qbytearray.h>
#include <QtAV/QtAV_Global.h>

struct AVCodecContext;
struct AVFrame;

namespace QtAV {
class AVDecoderPrivate;
class Q_EXPORT AVDecoder
{
public:
    AVDecoder();
    virtual ~AVDecoder();
    void setCodecContext(AVCodecContext* codecCtx); //protected
    AVCodecContext* codecContext() const;

    virtual bool decode(const QByteArray& encoded) = 0; //decode AVPacket?
    QByteArray data() const; //decoded data

protected:
    AVDecoder(AVDecoderPrivate& d);

    Q_DECLARE_PRIVATE(AVDecoder)
    AVDecoderPrivate *d_ptr;
};
}

#endif // QAVDECODER_H
