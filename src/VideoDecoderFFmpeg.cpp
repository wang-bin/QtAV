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
#include <QtAV/ImageConverterTypes.h>
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include "prepost.h"

namespace QtAV {

#define PIX_FMT PIX_FMT_RGB32 //PIX_FMT_YUV420P

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
        conv = ImageConverterFactory::create(ImageConverterId_FF); //TODO: set in AVPlayer
        conv->setOutFormat(PIX_FMT);
    }
    virtual ~VideoDecoderFFmpegPrivate() {
        if (conv) {
            delete conv;
            conv = 0;
        }
    }
    ImageConverter* conv;
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
    d.conv->setInFormat(d.codec_ctx->pix_fmt);
    d.conv->setInSize(d.codec_ctx->width, d.codec_ctx->height);
    if (d.width <= 0 || d.height <= 0) {
        qDebug("decoded video size not seted. use original size [%d x %d]"
            , d.codec_ctx->width, d.codec_ctx->height);
        if (!d.codec_ctx->width || !d.codec_ctx->height)
            return false;
        resizeVideoFrame(d.codec_ctx->width, d.codec_ctx->height);
    }
    //If not YUV420P or ImageConverter supported format pair, convert to YUV420P first. or directly convert to RGB?(no hwa)
    //TODO: move convertion out. decoder only do some decoding
    //if not yuv420p or conv supported convertion pair(in/out), convert to yuv420p first using ff, then use other yuv2rgb converter
    if (!d.conv->convert(d.frame->data, d.frame->linesize))
        return false;
    d.decoded = d.conv->outData();
    return true;

}

void VideoDecoderFFmpeg::resizeVideoFrame(int width, int height)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.conv->setOutSize(width, height);
    d.width = width;
    d.height = height;
}

} //namespace QtAV
