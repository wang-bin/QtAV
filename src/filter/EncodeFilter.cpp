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
#include "QtAV/AudioEncoder.h"
#include "QtAV/VideoEncoder.h"
#include "utils/Logger.h"

namespace QtAV {

class AudioEncodeFilterPrivate Q_DECL_FINAL : public AudioFilterPrivate
{
public:
    AudioEncodeFilterPrivate() : enc(0) {}
    ~AudioEncodeFilterPrivate() {
        if (enc) {
            enc->close();
            delete enc;
        }
    }

    AudioEncoder* enc;
};

AudioEncodeFilter::AudioEncodeFilter(QObject *parent)
    : AudioFilter(*new AudioEncodeFilterPrivate(), parent)
{
}

AudioEncoder* AudioEncodeFilter::createEncoder(const QString &name)
{
    DPTR_D(AudioEncodeFilter);
    if (d.enc) {
        d.enc->close();
        delete d.enc;
    }
    d.enc = AudioEncoder::create(name.toLatin1().constData());
    return d.enc;
}

AudioEncoder* AudioEncodeFilter::encoder() const
{
    return d_func().enc;
}

void AudioEncodeFilter::process(Statistics *statistics, AudioFrame *frame)
{
    Q_UNUSED(statistics);
    encode(*frame);
}

void AudioEncodeFilter::encode(const AudioFrame& frame)
{
    DPTR_D(AudioEncodeFilter);
    if (!d.enc)
        return;
    // encode delayed frames can pass an invalid frame
    if (!d.enc->isOpen() && frame.isValid()) {
#if 0 //TODO: if set the input format, check whether it is supported in open()
        if (!d.enc->audioFormat().isValid()) {
            AudioFormat af(frame.format());
            if (af.isPlanar())
                af.setSampleFormat(AudioFormat::packedSampleFormat(af.sampleFormat()));
            d.enc->setAudioFormat(af);
        }
#endif
        if (!d.enc->open()) { // TODO: error()
            qWarning("Failed to open audio encoder");
            return;
        }
        Q_EMIT readyToEncode();
    }
    // TODO: async
    AudioFrame f(frame);
    if (f.format() != d.enc->audioFormat())
        f = f.to(d.enc->audioFormat());
    if (!d.enc->encode(f))
        return;
    if (!d.enc->encoded().isValid())
        return;
    Q_EMIT frameEncoded(d.enc->encoded());
}


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
    d.enc = VideoEncoder::create(name.toLatin1().constData());
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
            qWarning("Failed to open video encoder");
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
    if (!d.enc->encoded().isValid())
        return;
    Q_EMIT frameEncoded(d.enc->encoded());
}

} //namespace QtAV
