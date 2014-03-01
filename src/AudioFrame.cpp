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

#include "QtAV/AudioFrame.h"
#include "private/Frame_p.h"
#include "QtAV/AudioResampler.h"

extern "C" {
#include <libavutil/samplefmt.h>
}

namespace QtAV {

class AudioFramePrivate : public FramePrivate
{
public:
    AudioFramePrivate()
        : FramePrivate()
        , samples_per_ch(0)
        , conv(0)
    {}
    ~AudioFramePrivate() {}
    bool convertTo(const AudioFormat& other) {
        if (format == other)
            return true;
        if (!conv) {
            qWarning("must call AudioFrame::setAudioResampler");
            return false;
        }
        conv->setInAudioFormat(format);
        conv->setOutAudioFormat(other);
        conv->setInSampesPerChannel(samples_per_ch); //TODO
        if (!conv->convert((const quint8**)planes.data())) {
            format = AudioFormat(); //invalid format
            return false;
        }
        format = other;
        data = conv->outData();
        samples_per_ch = conv->outSamplesPerChannel();
        // TODO: setBytesPerLine(), setBits() compute here or from resampler?
        line_sizes.resize(format.planeCount());
        planes.resize(format.planeCount());
    }

    AudioFormat format;
    int samples_per_ch;
    AudioResampler *conv;
};

AudioFrame::AudioFrame():
    Frame(*new AudioFramePrivate())
{
}

/*!
    Constructs a shallow copy of \a other.  Since AudioFrame is
    explicitly shared, these two instances will reflect the same frame.

*/
AudioFrame::AudioFrame(const AudioFrame &other)
    : Frame(other)
{
}

AudioFrame::AudioFrame(const QByteArray &data, const AudioFormat &format)
    : Frame(*new AudioFramePrivate())
{
    Q_D(AudioFrame);
    d->format = format;
    d->data = data;
    d->samples_per_ch = data.size() / d->format.channels();
    init();
}

/*!
    Assigns the contents of \a other to this video frame.  Since AudioFrame is
    explicitly shared, these two instances will reflect the same frame.

*/
AudioFrame &AudioFrame::operator =(const AudioFrame &other)
{
    d_ptr = other.d_ptr;
    return *this;
}

AudioFrame::~AudioFrame()
{
}

AudioFrame AudioFrame::clone() const
{
    Q_D(const AudioFrame);
    if (!d->format.isValid())
        return AudioFrame();
    AudioFrame f(QByteArray(), d->format);
    f.setSamplesPerChannel(samplesPerChannel());
    f.allocate();
    // TODO: Frame.planes(), bytesPerLines()
    int nb_planes = f.planeCount();
    QVector<uchar*> dst(nb_planes);
    for (int i = 0; i < nb_planes; ++i) {
        dst[i] = f.bits(i);
    }
    av_samples_copy(dst.data(), d->planes.data(), 0, 0, samplesPerChannel(), d->format.channels(), (AVSampleFormat)d->format.sampleFormatFFmpeg());
    return f;
}

int AudioFrame::allocate()
{
    Q_D(AudioFrame);
    int line_size;
    int size = av_samples_get_buffer_size(&line_size, d->format.channels(), d->samples_per_ch, (AVSampleFormat)d->format.sampleFormatFFmpeg(), 0);
    d->data.resize(size);
    init();
    return size;
}

AudioFormat AudioFrame::format() const
{
    return d_func()->format;
}

void AudioFrame::setSamplesPerChannel(int samples)
{
    d_func()->samples_per_ch = samples;
}

int AudioFrame::samplesPerChannel() const
{
    return d_func()->samples_per_ch;
}

void AudioFrame::setAudioResampler(AudioResampler *conv)
{
    d_func()->conv = conv;
}

// TODO: alignment. use av_samples_fill_arrays
void AudioFrame::init()
{
    Q_D(AudioFrame);
    const int nb_planes = d->format.planeCount();
    d->line_sizes.resize(nb_planes);
    d->planes.resize(nb_planes);
    for (int i = 0; i < nb_planes; ++i) {
        setBytesPerLine(d->data.size()/nb_planes, i);
    }
    if (d->data.isEmpty())
        return;
    for (int i = 0; i < nb_planes; ++i) {
        setBits((uchar*)d->data.constData() + i*bytesPerLine(i), i);
    }
}

} //namespace QtAV
