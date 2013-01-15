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

#include <QtAV/VideoDecoder.h>
#include <private/AVDecoder_p.h>
#include <QtAV/ImageConverterFF.h>
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include <QtCore/QSize>

#if CONFIG_EZX
#define PIX_FMT PIX_FMT_BGR565
#else
#define PIX_FMT PIX_FMT_RGB32 //PIX_FMT_YUV420P
#endif //CONFIG_EZX

namespace QtAV {

class VideoDecoderPrivate : public AVDecoderPrivate
{
public:
    VideoDecoderPrivate():width(0),height(0)
    {
        conv = new ImageConverterFF(); //TODO: set in AVPlayer
        conv->setOutFormat(PIX_FMT_RGB32);
    }
    ~VideoDecoderPrivate() {
        if (conv) {
            delete conv;
            conv = 0;
        }
    }

    int width, height;
    ImageConverter* conv;
};

VideoDecoder::VideoDecoder()
    :AVDecoder(*new VideoDecoderPrivate())
{
}
//TODO: use ipp, cuda decode and yuv functions. is sws_scale necessary?
bool VideoDecoder::decode(const QByteArray &encoded)
{
    DPTR_D(VideoDecoder);

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
        return false;
    }

    if (d.width <= 0 || d.height <= 0) {
        qDebug("decoded video size not seted. use original size [%d x %d]"
            , d.codec_ctx->width, d.codec_ctx->height);
        if (!d.codec_ctx->width || !d.codec_ctx->height)
            return false;
        resizeVideo(d.codec_ctx->width, d.codec_ctx->height);
    }
    //If not YUV420P or ImageConverter supported format pair, convert to YUV420P first. or directly convert to RGB?(no hwa)
    //TODO: move convertion out. decoder only do some decoding
    //if not yuv420p or conv supported convertion pair(in/out), convert to yuv420p first using ff, then use other yuv2rgb converter
    d.conv->setInFormat(d.codec_ctx->pix_fmt);
    d.conv->setInSize(d.codec_ctx->width, d.codec_ctx->height);
    d.conv->convert(d.frame->data, d.frame->linesize);
    d.decoded = d.conv->outData();
    return true;
}

void VideoDecoder::resizeVideo(const QSize &size)
{
    resizeVideo(size.width(), size.height());
}

void VideoDecoder::resizeVideo(int width, int height)
{
    if (width == 0 || height == 0)
        return;
    DPTR_D(VideoDecoder);
    d.conv->setOutSize(width, height);
    d.width = width;
    d.height = height;
}

int VideoDecoder::width() const
{
    return d_func().width;
}

int VideoDecoder::height() const
{
    return d_func().height;
}
} //namespace QtAV
