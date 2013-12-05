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

#ifndef QTAV_VIDEODECODERFFMPEGHW_P_H
#define QTAV_VIDEODECODERFFMPEGHW_P_H

#include "private/VideoDecoderFFmpeg_p.h"

namespace QtAV {

class Q_AV_EXPORT VideoDecoderFFmpegHWPrivate : public VideoDecoderFFmpegPrivate
{
public:
    VideoDecoderFFmpegHWPrivate()
        : VideoDecoderFFmpegPrivate()
        , va_pixfmt(QTAV_PIX_FMT_C(NONE))
    {
        // subclass setup va_pixfmt here
    }

    virtual ~VideoDecoderFFmpegHWPrivate() {}
    virtual bool setup(void **hwctx, AVPixelFormat *chroma, int w, int h) = 0;

    AVPixelFormat getFormat(struct AVCodecContext *p_context, const AVPixelFormat *pi_fmt);
    virtual bool getBuffer(void **opaque, uint8_t **data) = 0;
    virtual void releaseBuffer(void *opaque, uint8_t *data) = 0;

    AVPixelFormat va_pixfmt;
    QString description;
};

} //namespace QtAV

#endif // QTAV_VideoDecoderFFmpegHW_P_H
