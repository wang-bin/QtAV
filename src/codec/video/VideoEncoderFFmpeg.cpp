/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/VideoEncoder.h"
#include "QtAV/private/AVEncoder_p.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/mkid.h"
#include "QtAV/private/prepost.h"
#include "QtAV/version.h"
#include "utils/Logger.h"

/*!
 * options (properties) are from libavcodec/options_table.h
 * enum name here must convert to lower case to fit the names in avcodec. done in AVEncoder.setOptions()
 * Don't use lower case here because the value name may be "default" in avcodec which is a keyword of C++
 */

namespace QtAV {

class VideoEncoderFFmpegPrivate;
class VideoEncoderFFmpeg Q_DECL_FINAL: public VideoEncoder
{
    DPTR_DECLARE_PRIVATE(VideoEncoderFFmpeg)
public:
    VideoEncoderFFmpeg();
    VideoEncoderId id() const Q_DECL_OVERRIDE;
    bool encode(const VideoFrame &frame) Q_DECL_OVERRIDE;
};

static const VideoEncoderId VideoEncoderId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
FACTORY_REGISTER_ID_AUTO(VideoEncoder, FFmpeg, "FFmpeg")

void RegisterVideoEncoderFFmpeg_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoEncoder, FFmpeg, "FFmpeg")
}

class VideoEncoderFFmpegPrivate Q_DECL_FINAL: public VideoEncoderPrivate
{
public:
    VideoEncoderFFmpegPrivate()
        : VideoEncoderPrivate()
    {
        avcodec_register_all();
        // NULL: codec-specific defaults won't be initialized, which may result in suboptimal default settings (this is important mainly for encoders, e.g. libx264).
        avctx = avcodec_alloc_context3(NULL);
    }
    bool open() Q_DECL_OVERRIDE;
    bool close() Q_DECL_OVERRIDE;
};

bool VideoEncoderFFmpegPrivate::open()
{
    if (codec_name.isEmpty()) {
        // copy ctx from muxer
        AVCodec *codec = avcodec_find_decoder(avctx->codec_id);
        AV_ENSURE_OK(avcodec_open2(avctx, codec, &dict), false);
        return true;
    }
    AVCodec *codec = avcodec_find_encoder_by_name(codec_name.toUtf8().constData());
    if (!codec) {
        qWarning() << "Can not find encoder for codec " << codec_name;
        return false;
    }
    if (avctx) {
        avcodec_free_context(&avctx);
        avctx = 0;
    }
    avctx = avcodec_alloc_context3(codec);
    applyOptionsForContext();
    AV_ENSURE_OK(avcodec_open2(avctx, codec, &dict), false);
    return true;
}

bool VideoEncoderFFmpegPrivate::close()
{
    AV_ENSURE_OK(avcodec_close(avctx), false);
    return true;
}


VideoEncoderFFmpeg::VideoEncoderFFmpeg()
    : VideoEncoder(*new VideoEncoderFFmpegPrivate())
{
}

VideoEncoderId VideoEncoderFFmpeg::id() const
{
    return VideoEncoderId_FFmpeg;
}

bool VideoEncoderFFmpeg::encode(const VideoFrame &frame)
{
    DPTR_D(VideoEncoderFFmpeg);
    AVFrame *f = av_frame_alloc();
    f->format = frame.format().pixelFormatFFmpeg();
    f->width = frame.width();
    f->height = frame.height();
    // pts is set in muxer
    const int nb_planes = frame.planeCount();
    for (int i = 0; i < nb_planes; ++i) {
        f->linesize[i] = frame.bytesPerLine(i);
        f->data[i] = (uint8_t*)frame.bits(i);
    }
    AVPacket pkt;
    av_init_packet(&pkt);
    int got_packet = 0;
    int ret = avcodec_encode_video2(d.avctx, &pkt, f, &got_packet);
    av_frame_free(&f);
    if (ret < 0) {
        return false; //false
    }
    if (!got_packet) {
        return false; //false
    }
    d.packet = Packet::fromAVPacket(&pkt, 0);
    return true;
}

} //namespace QtAV
