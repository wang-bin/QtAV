#include <QtAV/AVDecoder.h>

namespace QtAV {
AVDecoder::AVDecoder()
    :codec_ctx(0),frame_(0)
{
}

AVDecoder::~AVDecoder()
{
}

void AVDecoder::setCodecContext(AVCodecContext *codecCtx)
{
    codec_ctx = codecCtx;
}

AVCodecContext* AVDecoder::codecContext() const
{
    return codec_ctx;
}

void AVDecoder::setFrame(AVFrame *frame)
{
    frame_ = frame;
}

AVFrame* AVDecoder::frame() const
{
    return frame_;
}

bool AVDecoder::decode(const QByteArray &encoded)
{
    decoded = encoded;
    return true;
}

QByteArray AVDecoder::data() const
{
    return decoded;
}

}
