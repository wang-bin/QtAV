#include <QtAV/AudioDecoder.h>

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
//
bool AudioDecoder::decode(const QByteArray &encoded)
{
    int bytes_consumed = 0, frameFinished;

    AVPacket packet;
    av_new_packet(&packet, encoded.size());
    memcpy(packet.data, encoded.data(), encoded.size() );
//TODO: use AVPacket directly instead of QAVPacket?
    bytes_consumed = avcodec_decode_audio4(codec_ctx, frame_, &frameFinished, &packet);
    av_free_packet(&packet);
    if (frameFinished) {
        const int samples_with_channels = frame_->nb_samples * codec_ctx->channels;
        decoded.resize(samples_with_channels * sizeof(float));
        float *decoded_data = (float*)decoded.data();
        switch (codec_ctx->sample_fmt) {
        case AV_SAMPLE_FMT_U8:
        {    uint8_t *data = (uint8_t*)*frame_->data;
            for (int i = 0; i < samples_with_channels; i++)
                decoded_data[i] = (data[i] - 0x7F) / 128.0f;
            break;}
        case AV_SAMPLE_FMT_S16:
        {    int16_t *data = (int16_t*)*frame_->data;
            for (int i = 0; i < samples_with_channels; i++)
                decoded_data[i] = data[i] / 32768.0f;
            break;}
        case AV_SAMPLE_FMT_S32:
        {    int32_t *data = (int32_t*)*frame_->data;
            for (int i = 0; i < samples_with_channels; i++)
                decoded_data[i] = data[i] / 2147483648.0f;
            break;}
        case AV_SAMPLE_FMT_FLT:
        {    memcpy(decoded_data, *frame_->data, decoded.size());
            break;}
        case AV_SAMPLE_FMT_DBL:
        {    double *data = ( double * )*frame_->data;
            for (int i = 0; i < samples_with_channels; i++)
                decoded_data[ i ] = data[ i ];
            break;}
        default:
            decoded.clear();
            break;
        }
/*
        if ( pts )
            clock = pts;
        else
            pts = clock;
        clock += (double)frame->nb_samples / (double)codec_ctx->sample_rate;
*/

    }
}
}
