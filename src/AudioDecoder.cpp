/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QtAV/AudioDecoder.h>
#include <private/AVDecoder_p.h>
#include <QtAV/QtAV_Compat.h>

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
    DPTR_D(AudioDecoder);
    AVPacket packet;
    av_new_packet(&packet, encoded.size());
    memcpy(packet.data, encoded.data(), encoded.size());
//TODO: use AVPacket directly instead of Packet?
    int ret = avcodec_decode_audio4(d.codec_ctx, d.frame, &d.got_frame_ptr, &packet);
    av_free_packet(&packet);
    if (ret < 0) {
        qWarning("[AudioDecoder] %s", av_err2str(ret));
        return false;
    }
    if (!d.got_frame_ptr) {
        qWarning("[AudioDecoder] got_frame_ptr=false");
        return false;
    }
    int samples_with_channels = d.frame->nb_samples * d.codec_ctx->channels;
    int samples_with_channels_half = samples_with_channels/2;
    d.decoded.resize(samples_with_channels * sizeof(float));
    float *decoded_data = (float*)d.decoded.data();
     //TODO: use bit. SIMD, hwa
    switch (d.codec_ctx->sample_fmt) {
    case AV_SAMPLE_FMT_U8:
    {
        uint8_t *data = (uint8_t*)*d.frame->data;
        static const float kInt8_inv = 1.0f/128.0f;
        for (int i = 0; i < samples_with_channels_half; i++) {
            decoded_data[i] = (data[i] - 0x7F) * kInt8_inv;
            decoded_data[samples_with_channels - i] = (data[samples_with_channels - i] - 0x7F) * kInt8_inv;
        }
        break;
    }
    case AV_SAMPLE_FMT_S16:
    {
        int16_t *data = (int16_t*)*d.frame->data;
        static const float kInt16_inv = 1.0f/32768.0f;
        for (int i = 0; i < samples_with_channels_half; i++) {
            decoded_data[i] = data[i] * kInt16_inv;
            decoded_data[samples_with_channels - i] = data[samples_with_channels - i] * kInt16_inv;
        }
        break;
    }
    case AV_SAMPLE_FMT_S32:
    {
        int32_t *data = (int32_t*)*d.frame->data;
        static const float kInt64_inv = 1.0f/2147483648.0f;
        for (int i = 0; i < samples_with_channels_half; i++) {
            decoded_data[i] = data[i] * kInt64_inv;
            decoded_data[samples_with_channels - i] = data[samples_with_channels - i] * kInt64_inv;
        }
        break;
    }
    case AV_SAMPLE_FMT_FLT:
    //case AV_SAMPLE_FMT_FLTP:
    {
        memcpy(decoded_data, *d.frame->data, d.decoded.size());
        break;
    }
    case AV_SAMPLE_FMT_DBL:
    {
        double *data = (double*)*d.frame->data;
        for (int i = 0; i < samples_with_channels_half; i++) {
            decoded_data[i] = data[i];
            decoded_data[samples_with_channels - i] = data[samples_with_channels - i];
        }
        break;
    }
    default: //TODO: planar format
        static bool sWarn_a_fmt = true; //FIXME: no warning when replay. warn only once
        if (sWarn_a_fmt) {
            qWarning("Unsupported audio format: %d", d.codec_ctx->sample_fmt);
            sWarn_a_fmt = false;
        }
        d.decoded.clear();
        break;
    }
/*
    if ( pts )
        clock = pts;
    else
        pts = clock;
    clock += (double)d.frame->nb_samples / (double)d.codec_ctx->sample_rate;
*/
    return !d.decoded.isEmpty();
}

} //namespace QtAV
