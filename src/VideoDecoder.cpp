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
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include <QtCore/QSize>

namespace QtAV {

class VideoDecoderPrivate : public AVDecoderPrivate
{
public:
    VideoDecoderPrivate():sws_ctx(0),width(0),height(0),pix_fmt(PIX_FMT)
    {}
    SwsContext *sws_ctx;
    int width, height;
    enum PixelFormat pix_fmt;
    AVPicture picture;
};

VideoDecoder::VideoDecoder()
    :AVDecoder(*new VideoDecoderPrivate())
{
}

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
    av_free_packet(&packet);
    if (ret < 0) {
        qDebug("[VideoDecoder] %s", av_err2str(ret));
        return false;
    }
    if (!d.got_frame_ptr) {
        qDebug("no frame could be decompressed");
        return false;
    }
    /*
    d.sws_ctx = sws_getContext(
            codecCtx->width, //int srcW,
            codecCtx->height, //int srcH,
            codecCtx->pix_fmt, //enum PixelFormat srcFormat,
            d.width, //int dstW,
            d.height, //int dstH,
            d.pix_fmt, //enum PixelFormat dstFormat,
            SWS_BICUBIC, //int flags,
            NULL, //SwsFilter *srcFilter,
            NULL, //SwsFilter *dstFilter,
            NULL  //const double *param
            );
    */

    if (d.width <= 0 || d.height <= 0) {
        qDebug("decoded video size not seted. use original size [%d x %d]"
            , d.codec_ctx->width, d.codec_ctx->height);
        if (!d.codec_ctx->width || !d.codec_ctx->height)
            return false;
        resizeVideo(d.codec_ctx->width, d.codec_ctx->height);
    }
    d.sws_ctx = sws_getCachedContext(d.sws_ctx
            , d.codec_ctx->width, d.codec_ctx->height, d.codec_ctx->pix_fmt
            , d.width, d.height, d.pix_fmt
            , (d.width == d.codec_ctx->width && d.height == d.codec_ctx->height) ? SWS_POINT : SWS_FAST_BILINEAR //SWS_BICUBIC
            , NULL, NULL, NULL
            );
    
    int v_scale_result = sws_scale(
            d.sws_ctx,
            d.frame->data,
            d.frame->linesize,
            0,
            d.codec_ctx->height,
            d.picture.data,
            d.picture.linesize
            );
    Q_UNUSED(v_scale_result);
    //sws_freeContext(v_sws_ctx);
    if (d.frame->interlaced_frame) //?
        avpicture_deinterlace(&d.picture, &d.picture, d.pix_fmt, d.width, d.height);
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

    int bytes = avpicture_get_size(PIX_FMT, width, height);
    if(d.decoded.size() < bytes) {
        d.decoded.resize(bytes);
    }
    //picture的数据按PIX_FMT格式自动"关联"到 data
    avpicture_fill(
            &d.picture,
            reinterpret_cast<uint8_t*>(d.decoded.data()),
            d.pix_fmt,
            width,
            height
            );

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
