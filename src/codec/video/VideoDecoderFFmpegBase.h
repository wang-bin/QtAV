/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#ifndef QTAV_VIDEODECODERFFMPEGBASE_H
#define QTAV_VIDEODECODERFFMPEGBASE_H

#include "QtAV/VideoDecoder.h"
#include "QtAV/private/AVDecoder_p.h"
#include "QtAV/private/AVCompat.h"

namespace QtAV {

class VideoDecoderFFmpegBasePrivate;
class VideoDecoderFFmpegBase : public VideoDecoder
{
    Q_DISABLE_COPY(VideoDecoderFFmpegBase)
    DPTR_DECLARE_PRIVATE(VideoDecoderFFmpegBase)
public:
    virtual bool decode(const Packet& packet) Q_DECL_OVERRIDE;
    virtual VideoFrame frame() Q_DECL_OVERRIDE;
protected:
    VideoDecoderFFmpegBase(VideoDecoderFFmpegBasePrivate &d);
private:
    VideoDecoderFFmpegBase(); //it's a base class
};

class VideoDecoderFFmpegBasePrivate : public VideoDecoderPrivate
{
public:
    VideoDecoderFFmpegBasePrivate()
        : VideoDecoderPrivate()
        , frame(0)
        , width(0)
        , height(0)
    {
#if !AVCODEC_STATIC_REGISTER
        avcodec_register_all();
#endif
        frame = av_frame_alloc();
    }
    virtual ~VideoDecoderFFmpegBasePrivate() {
        if (frame) {
            av_frame_free(&frame);
            frame = 0;
        }
    }
    void updateColorDetails(VideoFrame* f);
    qreal getDAR(AVFrame *f);

    AVFrame *frame; //set once and not change
    int width, height; //The current decoded frame size
};

} //namespace QtAV

#endif // QTAV_VIDEODECODERFFMPEGBASE_H
