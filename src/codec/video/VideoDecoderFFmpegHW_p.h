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

#ifndef QTAV_VIDEODECODERFFMPEGHW_P_H
#define QTAV_VIDEODECODERFFMPEGHW_P_H

#include "VideoDecoderFFmpegHW.h"
#include "utils/GPUMemCopy.h"

/*!
   QTAV_VA_REF: use AVCodecContext.get_buffer2 instead of old callbacks. In order to avoid compile warnings, now disable old
   callbacks if possible. maybe we can also do a runtime check and enable old callbacks
 */
#define QTAV_VA_REF (LIBAVCODEC_VERSION_MAJOR >= 55)

namespace QtAV {

class VideoDecoderFFmpegHWPrivate : public VideoDecoderFFmpegBasePrivate
{
public:
    VideoDecoderFFmpegHWPrivate()
        : VideoDecoderFFmpegBasePrivate()
        , copy_mode(VideoDecoderFFmpegHW::OptimizedCopy)
    {
        get_format = 0;
        get_buffer = 0;
        release_buffer = 0;
        reget_buffer = 0;
        get_buffer2 = 0;
    }
    virtual ~VideoDecoderFFmpegHWPrivate() {} //ctx is 0 now
    void restore() {
        codec_ctx->pix_fmt = pixfmt;
        codec_ctx->opaque = 0;
        codec_ctx->get_format = get_format;
#if QTAV_VA_REF
        codec_ctx->get_buffer2 = get_buffer2;
#else
        codec_ctx->get_buffer = get_buffer;
        codec_ctx->release_buffer = release_buffer;
        codec_ctx->reget_buffer = reget_buffer;
#endif //QTAV_VA_REF
    }

    virtual bool setup(AVCodecContext* avctx) = 0;

    AVPixelFormat getFormat(struct AVCodecContext *p_context, const AVPixelFormat *pi_fmt);
    virtual bool getBuffer(void **opaque, uint8_t **data) = 0;
    virtual void releaseBuffer(void *opaque, uint8_t *data) = 0;
    virtual AVPixelFormat vaPixelFormat() const = 0;

    int codedWidth(AVCodecContext *avctx) const;  //TODO: virtual int surfaceWidth(AVCodecContext*) const;
    int codedHeight(AVCodecContext *avctx) const;
    bool initUSWC(int lineSize);
    void releaseUSWC();

    AVPixelFormat pixfmt; //store old one
    //store old values because it does not own AVCodecContext
    AVPixelFormat (*get_format)(struct AVCodecContext *s, const AVPixelFormat * fmt);
    int (*get_buffer)(struct AVCodecContext *c, AVFrame *pic);
    void (*release_buffer)(struct AVCodecContext *c, AVFrame *pic);
    int (*reget_buffer)(struct AVCodecContext *c, AVFrame *pic);
    int (*get_buffer2)(struct AVCodecContext *s, AVFrame *frame, int flags);

    QString description;
    // false for not intel gpu. my test result is intel gpu is supper fast and lower cpu usage if use optimized uswc copy. but nv is worse.
    // TODO: flag enable, disable, auto
    VideoDecoderFFmpegHW::CopyMode copy_mode;
    GPUMemCopy gpu_mem;
};

} //namespace QtAV

#endif // QTAV_VideoDecoderFFmpegHW_P_H
