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

#ifndef QTAV_VIDEODECODERFFMPEGHW_P_H
#define QTAV_VIDEODECODERFFMPEGHW_P_H

#include "QtAV/private/VideoDecoderFFmpeg_p.h"

namespace QtAV {

class Q_AV_EXPORT VideoDecoderFFmpegHWPrivate : public VideoDecoderFFmpegPrivate
{
public:
    VideoDecoderFFmpegHWPrivate()
        : VideoDecoderFFmpegPrivate()
        , va_pixfmt(QTAV_PIX_FMT_C(NONE))
    {
        get_format = 0;
        get_buffer = 0;
        release_buffer = 0;
        reget_buffer = 0;
        //get_buffer2 = 0;
        // subclass setup va_pixfmt here
    }

    virtual ~VideoDecoderFFmpegHWPrivate() {}
    void restore() {
        codec_ctx->pix_fmt = pixfmt;
        codec_ctx->opaque = 0;
        codec_ctx->get_format = get_format;
        codec_ctx->get_buffer = get_buffer;
        codec_ctx->release_buffer = release_buffer;
        codec_ctx->reget_buffer = reget_buffer;
        //codec_ctx->get_buffer2 = get_buffer2;
    }

    virtual bool setup(void **hwctx, AVPixelFormat *chroma, int w, int h) = 0;

    AVPixelFormat getFormat(struct AVCodecContext *p_context, const AVPixelFormat *pi_fmt);
    virtual bool getBuffer(void **opaque, uint8_t **data) = 0;
    virtual void releaseBuffer(void *opaque, uint8_t *data) = 0;

    AVPixelFormat va_pixfmt;
    AVPixelFormat pixfmt; //store old one
    //store old values because it does not own AVCodecContext
    AVPixelFormat (*get_format)(struct AVCodecContext *s, const AVPixelFormat * fmt);
    int (*get_buffer)(struct AVCodecContext *c, AVFrame *pic);
    void (*release_buffer)(struct AVCodecContext *c, AVFrame *pic);
    int (*reget_buffer)(struct AVCodecContext *c, AVFrame *pic);
    //int (*get_buffer2)(struct AVCodecContext *s, AVFrame *frame, int flags);

    QString description;
};

} //namespace QtAV

#endif // QTAV_VideoDecoderFFmpegHW_P_H
