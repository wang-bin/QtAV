/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2014 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/private/VideoDecoderFFmpegHW_p.h"
#include "utils/Logger.h"

namespace QtAV {

static AVPixelFormat ffmpeg_get_va_format(struct AVCodecContext *c, const AVPixelFormat * ff)
{
    VideoDecoderFFmpegHWPrivate *va = (VideoDecoderFFmpegHWPrivate*)c->opaque;
    return va->getFormat(c, ff);
}

#if QTAV_VA_REF

typedef struct ffmpeg_va_ref_t {
    VideoDecoderFFmpegHWPrivate *va;
    void *opaque; //va surface from AVFrame.opaque
} ffmpeg_va_ref_t;

static void ffmpeg_release_va_buffer2(void *opaque, uint8_t *data)
{
    ffmpeg_va_ref_t *ref = (ffmpeg_va_ref_t*)opaque;
    ref->va->releaseBuffer(ref->opaque, data);
    delete ref;
}

static int ffmpeg_get_va_buffer2(struct AVCodecContext *ctx, AVFrame *frame, int flags)
{
    Q_UNUSED(flags);
    for (unsigned i = 0; i < AV_NUM_DATA_POINTERS; i++) {
        frame->data[i] = NULL;
        frame->linesize[i] = 0;
        frame->buf[i] = NULL;
    }
    //frame->reordered_opaque = ctx->reordered_opaque; //?? xbmc
    VideoDecoderFFmpegHWPrivate *va = (VideoDecoderFFmpegHWPrivate*)ctx->opaque;
    /* hwaccel_context is not present in old ffmpeg version */
    // not coded_width. assume coded_width is 6 aligned of width
    if (!va->setup(&ctx->hwaccel_context, ctx->width, ctx->height)) {
        qWarning("va Setup failed");
        return -1;
    }
    if (!va->getBuffer(&frame->opaque, &frame->data[0])) {
        qWarning("va->getBuffer failed");
        return -1;
    }
    ffmpeg_va_ref_t *ref = new ffmpeg_va_ref_t;
    ref->va = va;
    ref->opaque = frame->opaque;
    /* data[0] must be non-NULL for libavcodec internal checks. data[3] actually contains the format-specific surface handle. */
    frame->data[3] = frame->data[0];
    frame->buf[0] = av_buffer_create(frame->data[0], 0, ffmpeg_release_va_buffer2, ref, 0);
    if (Q_UNLIKELY(!frame->buf[0])) {
        ffmpeg_release_va_buffer2(ref, frame->data[0]);
        return -1;
    }
    Q_ASSERT(frame->data[0] != NULL); // FIXME: VDA may crash in debug mode
    return 0;
}
#else

static int ffmpeg_get_va_buffer(struct AVCodecContext *c, AVFrame *ff)//vlc_va_t *external, AVFrame *ff)
{
    VideoDecoderFFmpegHWPrivate *va = (VideoDecoderFFmpegHWPrivate*)c->opaque;
    //ff->reordered_opaque = c->reordered_opaque; //TODO: dxva?
    ff->opaque = 0;
#if ! LIBAVCODEC_VERSION_CHECK(54, 34, 0, 79, 101)
    ff->pkt_pts = c->pkt ? c->pkt->pts : AV_NOPTS_VALUE;
#endif
#if LIBAVCODEC_VERSION_MAJOR < 54
    ff->age = 256*256*256*64;
#endif
    /* hwaccel_context is not present in old ffmpeg version */
    // not coded_width. assume coded_width is 6 aligned of width
    if (!va->setup(&c->hwaccel_context, c->width, c->height)) {
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
    memset(ff->linesize, 0, sizeof(ff->linesize));
}
#endif //QTAV_VA_REF

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
        if (vaPixelFormat() != pi_fmt[i])
            continue;

        /* We try to call vlc_va_Setup when possible to detect errors when
         * possible (later is too late) */
        if (p_context->width > 0 && p_context->height > 0
         && !setup(&p_context->hwaccel_context, p_context->width, p_context->height)) {
            qWarning("acceleration setup failure");
            break;
        }

        qDebug("Using %s for hardware decoding.", qPrintable(description));

        /* FIXME this will disable direct rendering
         * even if a new pixel format is renegotiated
         */
        //p_sys->b_direct_rendering = false;
        p_context->draw_horiz_band = NULL;
        return pi_fmt[i];
    }

    close();
    //vlc_va_Delete(p_va);
end:
    qWarning("acceleration not available" );
    /* Fallback to default behaviour */
    return avcodec_default_get_format(p_context, pi_fmt);
}

VideoDecoderFFmpegHW::VideoDecoderFFmpegHW(VideoDecoderFFmpegHWPrivate &d):
    VideoDecoder(d)
{
}

bool VideoDecoderFFmpegHW::prepare()
{
    DPTR_D(VideoDecoderFFmpegHW);
    if (!d.codec_ctx) {
        qWarning("call this after AVCodecContext is set!");
        return false;
    }
    //TODO: setup profile in open. vlc/modules/hw/vdpau/avcodec.c, xbmc/xbmc/cores/dvdplayer/DVDCodecs/Video/VDPAU.cpp
    if (d.codec_ctx->codec_id == QTAV_CODEC_ID(H264)) {
        // check Hi10p. NO HW support now
        switch (d.codec_ctx->profile) {
        //case FF_PROFILE_H264_HIGH: //VDP_DECODER_PROFILE_H264_HIGH
        case FF_PROFILE_H264_HIGH_10:
        case FF_PROFILE_H264_HIGH_10_INTRA:
        case FF_PROFILE_H264_HIGH_422:
        case FF_PROFILE_H264_HIGH_422_INTRA:
        //case FF_PROFILE_H264_HIGH_444: //ignored by xbmc
        case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
        case FF_PROFILE_H264_HIGH_444_INTRA:
        case FF_PROFILE_H264_CAVLC_444:
            return false;
        default:
            break;
        }
    }
    //TODO: neccesary?
#if 0
    if (!d.setup(&d.codec_ctx->hwaccel_context, d.codec_ctx->width, d.codec_ctx->height)) {
        qWarning("Setup vaapi failed.");
        return false;
    }
#endif
    d.codec_ctx->opaque = &d; //is it ok?

    d.pixfmt = d.codec_ctx->pix_fmt;
    d.get_format = d.codec_ctx->get_format;
#if QTAV_VA_REF
    d.get_buffer2 = d.codec_ctx->get_buffer2;
#else
    d.get_buffer = d.codec_ctx->get_buffer;
    d.reget_buffer = d.codec_ctx->reget_buffer;
    d.release_buffer = d.codec_ctx->release_buffer;
#endif //QTAV_VA_REF
    d.codec_ctx->get_format = ffmpeg_get_va_format;
#if QTAV_VA_REF
    d.codec_ctx->get_buffer2 = ffmpeg_get_va_buffer2;
#else
    // TODO: FF_API_GET_BUFFER
    d.codec_ctx->get_buffer = ffmpeg_get_va_buffer;//ffmpeg_GetFrameBuf;
    d.codec_ctx->reget_buffer = avcodec_default_reget_buffer;
    d.codec_ctx->release_buffer = ffmpeg_release_va_buffer;//ffmpeg_ReleaseFrameBuf;
#endif //QTAV_VA_REF
    return true;
}

} //namespace QtAV
