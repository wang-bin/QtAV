#ifndef QAVDECODER_H
#define QAVDECODER_H

#include <qbytearray.h>
#include <QtAV/QtAV_Global.h>

struct AVCodecContext;
struct AVFrame;

namespace QtAV {
class Q_EXPORT AVDecoder
{
public:
    AVDecoder();
    virtual ~AVDecoder();
    void setCodecContext(AVCodecContext* codecCtx); //protected
    AVCodecContext* codecContext() const;

    void setFrame(AVFrame* frame);
    AVFrame* frame() const;

    virtual bool decode(const QByteArray& encoded) = 0; //decode AVPacket?
    QByteArray data() const; //decoded data

protected:
    AVCodecContext *codec_ctx; //set once and not change
    AVFrame *frame_; //set once and not change
    QByteArray decoded;
};
}

#endif // QAVDECODER_H
