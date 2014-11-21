/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include "QtAV/AudioDecoder.h"
#include "QtAV/private/AVDecoder_p.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/AudioResampler.h"
#include "QtAV/AudioResamplerTypes.h"
#include "utils/Logger.h"

namespace QtAV {

class AudioDecoderPrivate : public AVDecoderPrivate
{
public:
    AudioDecoderPrivate()
        : AVDecoderPrivate()
      , resampler(0)
    {
        resampler = AudioResamplerFactory::create(AudioResamplerId_FF);
        if (!resampler)
            resampler = AudioResamplerFactory::create(AudioResamplerId_Libav);
        if (resampler)
            resampler->setOutSampleFormat(AV_SAMPLE_FMT_FLT);
    }
    virtual ~AudioDecoderPrivate() {
        if (resampler) {
            delete resampler;
            resampler = 0;
        }
    }

    AudioResampler *resampler;
};

AudioDecoder::AudioDecoder()
    :AVDecoder(*new AudioDecoderPrivate)
{
}

bool AudioDecoder::prepare()
{
    DPTR_D(AudioDecoder);
    if (!d.codec_ctx)
        return false;
    if (!d.resampler)
        return true;
    d.resampler->setInChannelLayout(d.codec_ctx->channel_layout);
    d.resampler->setInChannels(d.codec_ctx->channels);
    d.resampler->setInSampleFormat(d.codec_ctx->sample_fmt);
    d.resampler->setInSampleRate(d.codec_ctx->sample_rate);
    d.resampler->prepare();
    return true;
}

bool AudioDecoder::decode(const Packet &packet)
{
    if (!isAvailable())
        return false;
    DPTR_D(AudioDecoder);
    // const AVPacket*: ffmpeg >= 1.0. no libav
    int ret = avcodec_decode_audio4(d.codec_ctx, d.frame, &d.got_frame_ptr, (AVPacket*)packet.asAVPacket());
    d.undecoded_size = qMin(packet.data.size() - ret, packet.data.size());
    if (ret == AVERROR(EAGAIN)) {
        return false;
    }
    if (ret < 0) {
        qWarning("[AudioDecoder] %s", av_err2str(ret));
        return false;
    }
    if (!d.got_frame_ptr) {
        qWarning("[AudioDecoder] got_frame_ptr=false. decoded: %d, un: %d", ret, d.undecoded_size);
        return true;
    }
    d.resampler->setInSampesPerChannel(d.frame->nb_samples);
    if (!d.resampler->convert((const quint8**)d.frame->extended_data)) {
        return false;
    }
    d.decoded = d.resampler->outData();
    return true;
    return !d.decoded.isEmpty();
}

//
bool AudioDecoder::decode(const QByteArray &encoded)
{
    if (!isAvailable())
        return false;
    DPTR_D(AudioDecoder);
    AVPacket packet;
#if NO_PADDING_DATA
    /*!
      larger than the actual read bytes because some optimized bitstream readers read 32 or 64 bits at once and could read over the end.
      The end of the input buffer avpkt->data should be set to 0 to ensure that no overreading happens for damaged MPEG streams
     */
    // auto released by av_buffer_default_free
    av_new_packet(&packet, encoded.size());
    memcpy(packet.data, encoded.data(), encoded.size());
#else
    av_init_packet(&packet);
    packet.size = encoded.size();
    packet.data = (uint8_t*)encoded.constData();
#endif //NO_PADDING_DATA
    int ret = avcodec_decode_audio4(d.codec_ctx, d.frame, &d.got_frame_ptr, &packet);
    d.undecoded_size = qMin(encoded.size() - ret, encoded.size());
    av_free_packet(&packet);
    if (ret == AVERROR(EAGAIN)) {
        return false;
    }
    if (ret < 0) {
        qWarning("[AudioDecoder] %s", av_err2str(ret));
        return false;
    }
    if (!d.got_frame_ptr) {
        qWarning("[AudioDecoder] got_frame_ptr=false. decoded: %d, un: %d", ret, d.undecoded_size);
        return true;
    }
#if !QTAV_HAVE(SWRESAMPLE) && !QTAV_HAVE(AVRESAMPLE)
    int samples_with_channels = d.frame->nb_samples * d.codec_ctx->channels;
    int samples_with_channels_half = samples_with_channels/2;
    d.decoded.resize(samples_with_channels * sizeof(float));
    float *decoded_data = (float*)d.decoded.data();
    static const float kInt8_inv = 1.0f/128.0f;
    static const float kInt16_inv = 1.0f/32768.0f;
    static const float kInt32_inv = 1.0f/2147483648.0f;
    //TODO: hwa
    //https://code.google.com/p/lavfilters/source/browse/decoder/LAVAudio/LAVAudio.cpp
    switch (d.codec_ctx->sample_fmt) {
    case AV_SAMPLE_FMT_U8:
    {
        uint8_t *data = (uint8_t*)*d.frame->data;
        for (int i = 0; i < samples_with_channels_half; i++) {
            decoded_data[i] = (data[i] - 0x7F) * kInt8_inv;
            decoded_data[samples_with_channels - i] = (data[samples_with_channels - i] - 0x7F) * kInt8_inv;
        }
    }
        break;
    case AV_SAMPLE_FMT_S16:
    {
        int16_t *data = (int16_t*)*d.frame->data;
        for (int i = 0; i < samples_with_channels_half; i++) {
            decoded_data[i] = data[i] * kInt16_inv;
            decoded_data[samples_with_channels - i] = data[samples_with_channels - i] * kInt16_inv;
        }
    }
        break;
    case AV_SAMPLE_FMT_S32:
    {
        int32_t *data = (int32_t*)*d.frame->data;
        for (int i = 0; i < samples_with_channels_half; i++) {
            decoded_data[i] = data[i] * kInt32_inv;
            decoded_data[samples_with_channels - i] = data[samples_with_channels - i] * kInt32_inv;
        }
    }
        break;
    case AV_SAMPLE_FMT_FLT:
        memcpy(decoded_data, *d.frame->data, d.decoded.size());
        break;
    case AV_SAMPLE_FMT_DBL:
    {
        double *data = (double*)*d.frame->data;
        for (int i = 0; i < samples_with_channels_half; i++) {
            decoded_data[i] = data[i];
            decoded_data[samples_with_channels - i] = data[samples_with_channels - i];
        }
    }
        break;
    case AV_SAMPLE_FMT_U8P:
    {
        uint8_t **data = (uint8_t**)d.frame->extended_data;
        for (int i = 0; i < d.frame->nb_samples; ++i) {
            for (int ch = 0; ch < d.codec_ctx->channels; ++ch) {
                *decoded_data++ = (data[ch][i] - 0x7F) * kInt8_inv;
            }
        }
    }
        break;
    case AV_SAMPLE_FMT_S16P:
    {
        uint16_t **data = (uint16_t**)d.frame->extended_data;
        for (int i = 0; i < d.frame->nb_samples; ++i) {
            for (int ch = 0; ch < d.codec_ctx->channels; ++ch) {
                *decoded_data++ = data[ch][i] * kInt16_inv;
            }
        }
    }
        break;
    case AV_SAMPLE_FMT_S32P:
    {
        uint32_t **data = (uint32_t**)d.frame->extended_data;
        for (int i = 0; i < d.frame->nb_samples; ++i) {
            for (int ch = 0; ch < d.codec_ctx->channels; ++ch) {
                *decoded_data++ = data[ch][i] * kInt32_inv;
            }
        }
    }
        break;
    case AV_SAMPLE_FMT_FLTP:
    {
        float **data = (float**)d.frame->extended_data;
        for (int i = 0; i < d.frame->nb_samples; ++i) {
            for (int ch = 0; ch < d.codec_ctx->channels; ++ch) {
                *decoded_data++ = data[ch][i];
            }
        }
    }
        break;
    case AV_SAMPLE_FMT_DBLP:
    {
        double **data = (double**)d.frame->extended_data;
        for (int i = 0; i < d.frame->nb_samples; ++i) {
            for (int ch = 0; ch < d.codec_ctx->channels; ++ch) {
                *decoded_data++ = data[ch][i];
            }
        }
    }
        break;
    default:
        static bool sWarn_a_fmt = true; //FIXME: no warning when replay. warn only once
        if (sWarn_a_fmt) {
            qWarning("Unsupported audio format: %d", d.codec_ctx->sample_fmt);
            sWarn_a_fmt = false;
        }
        d.decoded.clear();
        break;
    }
#else
    d.resampler->setInSampesPerChannel(d.frame->nb_samples);
    if (!d.resampler->convert((const quint8**)d.frame->extended_data)) {
        return false;
    }
    d.decoded = d.resampler->outData();
    return true;
#endif //!(QTAV_HAVE(SWRESAMPLE) && !QTAV_HAVE(AVRESAMPLE))
    return !d.decoded.isEmpty();
}

AudioResampler* AudioDecoder::resampler()
{
    return d_func().resampler;
}

} //namespace QtAV
