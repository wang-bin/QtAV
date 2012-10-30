#include <QtAV/AudioDecoder.h>
#include <private/AVDecoder_p.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif //__cplusplus

namespace QtAV {

class AudioDecoderPrivate : public AVDecoderPrivate
{
public:
    //AudioDecoderPrivate();
};

AudioDecoder::AudioDecoder()
    :AVDecoder(*new AudioDecoderPrivate)
{

}

//
bool AudioDecoder::decode(const QByteArray &encoded)
{
    AVPacket packet;
    av_new_packet(&packet, encoded.size());
    memcpy(packet.data, encoded.data(), encoded.size());
//TODO: use AVPacket directly instead of QAVPacket?
    int ret = avcodec_decode_audio4(d_ptr->codec_ctx, d_ptr->frame, &d_ptr->got_frame_ptr, &packet);
    av_free_packet(&packet);
    if (ret < 0) {
        qDebug("[AudioDecoder] %s", av_err2str(ret));
        return 0;
    }
    if (!d_ptr->got_frame_ptr) {
        return false;
    }
    const int samples_with_channels = d_ptr->frame->nb_samples * d_ptr->codec_ctx->channels;
    d_ptr->decoded.resize(samples_with_channels * sizeof(float));
    float *decoded_data = (float*)d_ptr->decoded.data();
    switch (d_ptr->codec_ctx->sample_fmt) {
    case AV_SAMPLE_FMT_U8:
    {    uint8_t *data = (uint8_t*)*d_ptr->frame->data;
        for (int i = 0; i < samples_with_channels; i++)
            decoded_data[i] = (data[i] - 0x7F) / 128.0f;
        break;}
    case AV_SAMPLE_FMT_S16:
    {    int16_t *data = (int16_t*)*d_ptr->frame->data;
        for (int i = 0; i < samples_with_channels; i++)
            decoded_data[i] = data[i] / 32768.0f;
        break;}
    case AV_SAMPLE_FMT_S32:
    {    int32_t *data = (int32_t*)*d_ptr->frame->data;
        for (int i = 0; i < samples_with_channels; i++)
            decoded_data[i] = data[i] / 2147483648.0f;
        break;}
    case AV_SAMPLE_FMT_FLT:
    {    memcpy(decoded_data, *d_ptr->frame->data, d_ptr->decoded.size());
        break;}
    case AV_SAMPLE_FMT_DBL:
    {    double *data = ( double * )*d_ptr->frame->data;
        for (int i = 0; i < samples_with_channels; i++)
            decoded_data[ i ] = data[ i ];
        break;}
    default:
        d_ptr->decoded.clear();
        break;
    }
/*
    if ( pts )
        clock = pts;
    else
        pts = clock;
    clock += (double)d_ptr->frame->nb_samples / (double)d_ptr->codec_ctx->sample_rate;
*/
    return true;
}

}
