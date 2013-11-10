/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/VideoDecoderFFmpeg.h"
#include "private/VideoDecoder_p.h"
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include "prepost.h"

namespace QtAV {


extern VideoDecoderId VideoDecoderId_FFmpeg;
FACTORY_REGISTER_ID_AUTO(VideoDecoder, FFmpeg, "FFmpeg")

void RegisterVideoDecoderFFmpeg_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoDecoder, FFmpeg, "FFmpeg")
}


class VideoDecoderFFmpegPrivate : public VideoDecoderPrivate
{
public:
    VideoDecoderFFmpegPrivate():
        VideoDecoderPrivate()
    {
    }
    virtual ~VideoDecoderFFmpegPrivate() {
    }
};

VideoDecoderFFmpeg::VideoDecoderFFmpeg():
    VideoDecoder(*new VideoDecoderFFmpegPrivate())
{
}

VideoDecoderFFmpeg::~VideoDecoderFFmpeg()
{
}

bool VideoDecoderFFmpeg::decode(const QByteArray &encoded)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoderFFmpeg);
    AVPacket packet;
    av_new_packet(&packet, encoded.size());
    memcpy(packet.data, encoded.data(), encoded.size());
//TODO: use AVPacket directly instead of Packet?
    //AVStream *stream = format_context->streams[stream_idx];

    //TODO: some decoders might in addition need other fields like flags&AV_PKT_FLAG_KEY
    int ret = avcodec_decode_video2(d.codec_ctx, d.frame, &d.got_frame_ptr, &packet);
    //TODO: decoded format is YUV420P, YUV422P?
    av_free_packet(&packet);
    if (ret < 0) {
        qWarning("[VideoDecoder] %s", av_err2str(ret));
        return false;
    }
    if (!d.got_frame_ptr) {
        qWarning("no frame could be decompressed: %s", av_err2str(ret));
        return true;
    }

    if (d.width <= 0 || d.height <= 0) {
        qDebug("decoded video size not seted. use original size [%d x %d]"
            , d.codec_ctx->width, d.codec_ctx->height);
        if (!d.codec_ctx->width || !d.codec_ctx->height)
            return false;
        resizeVideoFrame(d.codec_ctx->width, d.codec_ctx->height);
    }

    d.video_frame = VideoFrame(d.codec_ctx->width, d.codec_ctx->height, VideoFormat((int)d.codec_ctx->pix_fmt));
    d.video_frame.setBits(d.frame->data);
    d.video_frame.setBytesPerLine(d.frame->linesize);
    return true;

}

void VideoDecoderFFmpeg::resizeVideoFrame(int width, int height)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.width = width;
    d.height = height;
}

} //namespace QtAV
