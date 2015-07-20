/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "VideoDecoderFFmpegHW.h"
#include "VideoDecoderFFmpegHW_p.h"
#include <algorithm>
#include "utils/Logger.h"
#ifndef Q_UNLIKELY
#define Q_UNLIKELY(x) (!!(x))
#endif
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
    // not coded_width. assume coded_width is 6 aligned of width. ??
    if (!va->setup(ctx)) {
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
#if !AV_MODULE_CHECK(LIBAVCODEC, 54, 34, 0, 79, 101)
    ff->pkt_pts = c->pkt ? c->pkt->pts : AV_NOPTS_VALUE;
#endif
#if LIBAVCODEC_VERSION_MAJOR < 54
    ff->age = 256*256*256*64;
#endif
    /* hwaccel_context is not present in old ffmpeg version */
    // not coded_width. assume coded_width is 6 aligned of width. ??
    if (!va->setup(c)) {
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


bool VideoDecoderFFmpegHWPrivate::prepare()
{
    //// From vlc begin
    codec_ctx->thread_safe_callbacks = true; //?
#pragma warning(disable:4065) //vc: switch has default but no case
    switch (codec_ctx->codec_id) {
# if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 1, 0))
        /// tested libav-9.x + va-api. If remove this code:  Bug detected, please report the issue. Context scratch buffers could not be allocated due to unknown size
        case QTAV_CODEC_ID(H264):
        case QTAV_CODEC_ID(VC1):
        case QTAV_CODEC_ID(WMV3):
            codec_ctx->thread_type &= ~FF_THREAD_FRAME;
# endif
        default:
            break;
    }
    //// From vlc end
    //TODO: neccesary?
#if 0
    if (!setup(codec_ctx)) {
        qWarning("Setup va failed.");
        return false;
    }
#endif
    codec_ctx->opaque = this; //is it ok?

    pixfmt = codec_ctx->pix_fmt;
    get_format = codec_ctx->get_format;
#if QTAV_VA_REF
    get_buffer2 = codec_ctx->get_buffer2;
#else
    get_buffer = codec_ctx->get_buffer;
    reget_buffer = codec_ctx->reget_buffer;
    release_buffer = codec_ctx->release_buffer;
#endif //QTAV_VA_REF
    codec_ctx->get_format = ffmpeg_get_va_format;
#if QTAV_VA_REF
    codec_ctx->get_buffer2 = ffmpeg_get_va_buffer2;
#else
    // TODO: FF_API_GET_BUFFER
    codec_ctx->get_buffer = ffmpeg_get_va_buffer;//ffmpeg_GetFrameBuf;
    codec_ctx->reget_buffer = avcodec_default_reget_buffer;
    codec_ctx->release_buffer = ffmpeg_release_va_buffer;//ffmpeg_ReleaseFrameBuf;
#endif //QTAV_VA_REF
    return true;
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
        if (vaPixelFormat() != pi_fmt[i])
            continue;

        /* We try to call vlc_va_Setup when possible to detect errors when
         * possible (later is too late) */
        if (p_context->width > 0 && p_context->height > 0
         && !setup(p_context)) {
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

int VideoDecoderFFmpegHWPrivate::codedWidth(AVCodecContext *avctx) const
{
    if (avctx->coded_width > 0)
        return avctx->coded_width;
    return avctx->width;
}

int VideoDecoderFFmpegHWPrivate::codedHeight(AVCodecContext *avctx) const
{
    if (avctx->coded_height > 0)
        return avctx->coded_height;
    return avctx->height;
}

bool VideoDecoderFFmpegHWPrivate::initUSWC(int lineSize)
{
    if (copy_mode != VideoDecoderFFmpegHW::OptimizedCopy)
        return false;
    return gpu_mem.initCache(lineSize);
}

void VideoDecoderFFmpegHWPrivate::releaseUSWC()
{
    if (copy_mode == VideoDecoderFFmpegHW::OptimizedCopy)
        gpu_mem.cleanCache();
}

VideoDecoderFFmpegHW::VideoDecoderFFmpegHW(VideoDecoderFFmpegHWPrivate &d):
    VideoDecoderFFmpegBase(d)
{
    setProperty("detail_copyMode", tr("ZeroCopy: fastest. Direct rendering without data copy between CPU and GPU") + ". " + tr("Not implemented for all codecs")
                          + "\n" + tr("LazyCopy: no explicitly additional copy") + ". " + tr("Not implemented for all codecs")
                          + "\n" + tr("OptimizedCopy: copy from USWC memory optimized by SSE4.1")
                          + "\n" + tr("GenericCopy: slowest. Generic cpu copy"));
}

void VideoDecoderFFmpegHW::setCopyMode(CopyMode value)
{
    DPTR_D(VideoDecoderFFmpegHW);
    if (d.copy_mode == value)
        return;
    d_func().copy_mode = value;
    emit copyModeChanged();
}

VideoDecoderFFmpegHW::CopyMode VideoDecoderFFmpegHW::copyMode() const
{
    return d_func().copy_mode;
}

VideoFrame VideoDecoderFFmpegHW::copyToFrame(const VideoFormat& fmt, int surface_h, quint8 *src[], int pitch[], bool swapUV)
{
    DPTR_D(VideoDecoderFFmpegHW);
    Q_ASSERT_X(src[0] && pitch[0] > 0, "VideoDecoderFFmpegHW::copyToFrame", "src[0] and pitch[0] must be set");
    const int nb_planes = fmt.planeCount();
    const int chroma_pitch = nb_planes > 1 ? fmt.bytesPerLine(pitch[0], 1) : 0;
    const int chroma_h = fmt.chromaHeight(surface_h);
    int h[] = { surface_h, 0, 0};
    for (int i = 1; i < nb_planes; ++i) {
        h[i] = chroma_h;
        // set chroma address and pitch if not set
        if (pitch[i] <= 0)
            pitch[i] = chroma_pitch;
        if (!src[i])
            src[i] = src[i-1] + pitch[i-1]*h[i-1];
    }
    if (swapUV) {
        std::swap(src[1], src[2]);
        std::swap(pitch[1], pitch[2]);
    }
    VideoFrame frame;
    if (copyMode() == VideoDecoderFFmpegHW::OptimizedCopy && d.gpu_mem.isReady()) {
        int yuv_size = 0;
        for (int i = 0; i < nb_planes; ++i) {
            yuv_size += pitch[i]*h[i];
        }
        // additional 15 bytes to ensure 16 bytes aligned
        QByteArray buf(15 + yuv_size, 0);
        const int offset_16 = (16 - ((uintptr_t)buf.data() & 0x0f)) & 0x0f;
        // plane 1, 2... is aligned?
        uchar* plane_ptr = (uchar*)buf.data() + offset_16;
        QVector<uchar*> dst(nb_planes, 0);
        for (int i = 0; i < nb_planes; ++i) {
            dst[i] = plane_ptr;
            // TODO: add VideoFormat::planeWidth/Height() ?
            // pitch instead of surface_width
            plane_ptr += pitch[i] * h[i];
            d.gpu_mem.copyFrame(src[i], dst[i], pitch[i], h[i], pitch[i]);
        }
        frame = VideoFrame(buf, width(), height(), fmt);
        frame.setBits(dst);
        frame.setBytesPerLine(pitch);
    } else {
        frame = VideoFrame(width(), height(), fmt);
        frame.setBits(src);
        frame.setBytesPerLine(pitch);
        // TODO: why clone is faster()?
        // TODO: buffer pool and create VideoFrame when needed to avoid copy? also for other va
        frame = frame.clone();
    }
    frame.setTimestamp(double(d.frame->pkt_pts)/1000.0);
    d.updateColorDetails(&frame);
    return frame;
}

} //namespace QtAV
