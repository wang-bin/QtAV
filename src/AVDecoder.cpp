#include <QtAV/AVDecoder.h>
#include <private/AVDecoder_p.h>

namespace QtAV {
AVDecoder::AVDecoder()
    :d_ptr(new AVDecoderPrivate())
{
}

AVDecoder::AVDecoder(AVDecoderPrivate &d)
    :d_ptr(&d)
{

}

AVDecoder::~AVDecoder()
{
    if (d_ptr) {
        delete d_ptr;
        d_ptr = 0;
    }
}

void AVDecoder::setCodecContext(AVCodecContext *codecCtx)
{
    d_ptr->codec_ctx = codecCtx;
}

AVCodecContext* AVDecoder::codecContext() const
{
    return d_ptr->codec_ctx;
}

bool AVDecoder::decode(const QByteArray &encoded)
{
    d_ptr->decoded = encoded;
    return true;
}

QByteArray AVDecoder::data() const
{
    return d_ptr->decoded;
}

}
