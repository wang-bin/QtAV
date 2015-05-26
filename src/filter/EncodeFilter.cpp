/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/EncodeFilter.h"
#include "QtAV/private/Filter_p.h"
#include "QtAV/VideoEncoder.h"
#include "utils/Logger.h"

namespace QtAV {

class VideoEncodeFilterPrivate Q_DECL_FINAL : public VideoFilterPrivate
{
public:
    VideoEncodeFilterPrivate() : enc(0) {}
    ~VideoEncodeFilterPrivate() {
        if (enc) {
            enc->close();
            delete enc;
        }
    }

    VideoEncoder* enc;
};

VideoEncodeFilter::VideoEncodeFilter(QObject *parent)
    : VideoFilter(*new VideoEncodeFilterPrivate(), parent)
{
}

VideoEncoder* VideoEncodeFilter::createEncoder(const QString &name)
{
    DPTR_D(VideoEncodeFilter);
    if (d.enc) {
        d.enc->close();
        delete d.enc;
    }
    d.enc = VideoEncoder::create(name);
    return d.enc;
}

VideoEncoder* VideoEncodeFilter::encoder() const
{
    return d_func().enc;
}

void VideoEncodeFilter::process(Statistics *statistics, VideoFrame *frame)
{
    Q_UNUSED(statistics);
    encode(*frame);
}

void VideoEncodeFilter::encode(const VideoFrame& frame)
{
    DPTR_D(VideoEncodeFilter);
    if (!d.enc)
        return;
    // encode delayed frames can pass an invalid frame
    if (!d.enc->isOpen() && frame.isValid()) {
        d.enc->setWidth(frame.width());
        d.enc->setHeight(frame.height());
        if (!d.enc->open()) { // TODO: error()
            qWarning("Failed to open encoder");
            return;
        }
        Q_EMIT readyToEncode();
    }
    if (d.enc->width() != frame.width() || d.enc->height() != frame.height()) {
        qWarning("Frame size (%dx%d) and video encoder size (%dx%d) mismatch! Close encoder please.", d.enc->width(), d.enc->height(), frame.width(), frame.height());
        return;
    }
    // TODO: async
    VideoFrame f(frame);
    if (f.pixelFormat() != d.enc->pixelFormat())
        f = f.to(d.enc->pixelFormat());
    if (!d.enc->encode(f))
        return;
    Q_EMIT frameEncoded(d.enc->encoded());
}

} //namespace QtAV
