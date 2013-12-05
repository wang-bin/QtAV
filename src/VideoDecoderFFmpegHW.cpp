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

#include "QtAV/VideoDecoderFFmpegHW.h"
#include "private/VideoDecoderFFmpegHW_p.h"

namespace QtAV {


static AVPixelFormat ffmpeg_get_va_format(struct AVCodecContext *c, const AVPixelFormat * ff)
{
    VideoDecoderFFmpegHWPrivate *va = (VideoDecoderFFmpegHWPrivate*)c->opaque;
    return va->getFormat(c, ff);
}

static int ffmpeg_get_va_buffer(struct AVCodecContext *c, AVFrame *ff)//vlc_va_t *external, AVFrame *ff)
{
    VideoDecoderFFmpegHWPrivate *va = (VideoDecoderFFmpegHWPrivate*)c->opaque;
    ff->opaque = 0;
    /* hwaccel_context is not present in old ffmpeg version */
    if (!va->setup(&c->hwaccel_context, &c->pix_fmt, c->coded_width, c->coded_height)) {
        qWarning("va Setup failed");
        return -1;
    }
    if (!va->getBuffer(&ff->opaque, &ff->data[0]))
        return -1;

    //ffmpeg_va_GetFrameBuf
    ff->data[3] = ff->data[0];
    ff->type = FF_BUFFER_TYPE_USER;
    return 0;
}

static void ffmpeg_release_va_buffer(struct AVCodecContext *c, AVFrame *ff)
{
    VideoDecoderFFmpegHWPrivate *va = (VideoDecoderFFmpegHWPrivate*)c->opaque;
    va->releaseBuffer(ff->opaque, ff->data[0]);
    memset(ff->data, 0, sizeof(ff->data));
}

AVPixelFormat VideoDecoderFFmpegHWPrivate::getFormat(struct AVCodecContext *p_context, const AVPixelFormat *pi_fmt)
{
    bool can_hwaccel = false;
    for (size_t i = 0; pi_fmt[i] != QTAV_PIX_FMT_C(NONE); i++) {
        const AVPixFmtDescriptor *dsc = av_pix_fmt_desc_get(pi_fmt[i]);
        if (dsc == NULL)
            continue;
        bool hwaccel = (dsc->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0;

        qDebug("available %sware decoder output format %d (%s)",
                 hwaccel ? "hard" : "soft", pi_fmt[i], dsc->name);
        if (hwaccel)
            can_hwaccel = true;
    }

    if (!can_hwaccel)
        goto end;

    /* Profile and level information is needed now.
     * TODO: avoid code duplication with avcodec.c */
#if 0
    if (p_context->profile != FF_PROFILE_UNKNOWN)
        p_dec->fmt_in.i_profile = p_context->profile;
    if (p_context->level != FF_LEVEL_UNKNOWN)
        p_dec->fmt_in.i_level = p_context->level;
#endif
    for (size_t i = 0; pi_fmt[i] != PIX_FMT_NONE; i++) {
        if (va_pixfmt != pi_fmt[i])
            continue;

        /* We try to call vlc_va_Setup when possible to detect errors when
         * possible (later is too late) */
        if (p_context->width > 0 && p_context->height > 0
         && !setup(&p_context->hwaccel_context, &p_context->pix_fmt, p_context->width, p_context->height)) {
            qWarning("acceleration setup failure");
            break;
        }

        qDebug("Using %s for hardware decoding.", qPrintable(description));

        /* FIXME this will disable direct rendering
         * even if a new pixel format is renegotiated
         */
        //p_sys->b_direct_rendering = false;
        //p_sys->p_va = p_va;
        p_context->draw_horiz_band = NULL;
        return pi_fmt[i];
    }

    close();
    //vlc_va_Delete(p_va);

end:
    /* Fallback to default behaviour */
    //p_sys->p_va = NULL;
    return avcodec_default_get_format(p_context, pi_fmt);
}

VideoDecoderFFmpegHW::VideoDecoderFFmpegHW(VideoDecoderFFmpegHWPrivate &d):
    VideoDecoderFFmpeg(d)
{
}

VideoDecoderFFmpegHW::~VideoDecoderFFmpegHW()
{
}

bool VideoDecoderFFmpegHW::prepare()
{
    DPTR_D(VideoDecoderFFmpegHW);
    if (!d.codec_ctx) {
        qWarning("call this after AVCodecContext is set!");
        return false;
    }
    //TODO: neccesary?
#if 0
    if (!d.setup(&d.codec_ctx->hwaccel_context, &d.codec_ctx->pix_fmt, d.codec_ctx->width, d.codec_ctx->height)) {
        qWarning("Setup vaapi failed.");
        return false;
    }
#endif
    d.codec_ctx->opaque = &d; //is it ok?
    d.codec_ctx->get_format = ffmpeg_get_va_format;
//#if LIBAVCODEC_VERSION_MAJOR >= 55
    //d.codec_ctx->get_buffer2 = ffmpeg_get_va_frame;
//#else
    d.codec_ctx->get_buffer = ffmpeg_get_va_buffer;//ffmpeg_GetFrameBuf;
    d.codec_ctx->reget_buffer = avcodec_default_reget_buffer;
    d.codec_ctx->release_buffer = ffmpeg_release_va_buffer;//ffmpeg_ReleaseFrameBuf;
//#endif
    return true;
}


bool VideoDecoderFFmpegHW::decode(const QByteArray &encoded)
{
    if (!isAvailable())
        return false;
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
        return false; //FIXME
    }
    // TODO: wait key frame?
    if (!d.codec_ctx->width || !d.codec_ctx->height)
        return false;
    d.width = d.codec_ctx->width;
    d.height = d.codec_ctx->height;
    return true;
}

} //namespace QtAV
