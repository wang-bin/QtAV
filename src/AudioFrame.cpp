/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/private/Frame_p.h"
#include "QtAV/AudioResampler.h"
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

namespace QtAV {
namespace{
static const struct RegisterMetaTypes
{
    inline RegisterMetaTypes() {
        qRegisterMetaType<QtAV::AudioFrame>("QtAV::AudioFrame");
    }
} _registerMetaTypes;
}

class AudioFramePrivate : public FramePrivate
{
public:
    AudioFramePrivate(const AudioFormat& fmt)
        : FramePrivate()
        , format(fmt)
        , samples_per_ch(0)
        , conv(0)
    {
        if (!format.isValid())
            return;
        const int nb_planes(format.planeCount());
        planes.reserve(nb_planes);
        planes.resize(nb_planes);
        line_sizes.reserve(nb_planes);
        line_sizes.resize(nb_planes);
    }

    AudioFormat format;
    int samples_per_ch;
    AudioResampler *conv;
};

/*!
    Constructs a shallow copy of \a other.  Since AudioFrame is
    explicitly shared, these two instances will reflect the same frame.

*/
AudioFrame::AudioFrame(const AudioFrame &other)
    : Frame(other)
{
}

AudioFrame::AudioFrame(const AudioFormat &format, const QByteArray& data)
    : Frame(new AudioFramePrivate(format))
{
    if (data.isEmpty())
        return;
    Q_D(AudioFrame);
    d->format = format;
    d->data = data;
    if (!d->format.isValid())
        return;
    if (d->data.isEmpty())
        return;
    d->samples_per_ch = data.size() / d->format.channels() / d->format.bytesPerSample();
    const int nb_planes(d->format.planeCount());
    const int bpl(d->data.size()/nb_planes);
    for (int i = 0; i < nb_planes; ++i) {
        setBytesPerLine(bpl, i);
        setBits((uchar*)d->data.constData() + i*bpl, i);
    }
    //init();
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

bool AudioFrame::isValid() const
{
    Q_D(const AudioFrame);
    return d->samples_per_ch > 0 && d->format.isValid();
}

QByteArray AudioFrame::data()
{
    if (!isValid())
        return QByteArray();
    Q_D(AudioFrame);
    if (d->data.isEmpty()) {
        AudioFrame a(clone());
        d->data = a.data();
    }
    return d->data;
}

int AudioFrame::channelCount() const
{
    Q_D(const AudioFrame);
    if (!d->format.isValid())
        return 0;
    return d->format.channels();
}

AudioFrame AudioFrame::clone() const
{
    return mid(0, -1);
}

AudioFrame AudioFrame::mid(int pos, int len) const
{
    Q_D(const AudioFrame);

    if (d->format.sampleFormatFFmpeg() == AV_SAMPLE_FMT_NONE
            || d->format.channels() <= 0) {
        return AudioFrame();
    }

    if (d->samples_per_ch <= 0 || bytesPerLine(0) <= 0 || len == 0) {
        return AudioFrame(format());
    }

    int bufSize = bytesPerLine();
    int posBytes = 0;
    if (pos > 0) {
        posBytes = pos * d->format.bytesPerSample();
        bufSize -= posBytes;
    } else {
        pos = 0;
    }

    int lenBytes = len * d->format.bytesPerSample();
    if (len > 0 && lenBytes < bufSize) {
        bufSize = lenBytes;
    } else {
        lenBytes = bufSize;
    }

    QByteArray buf(bufSize * planeCount(), 0);
    char *dst = buf.data(); //must before buf is shared, otherwise data will be detached.

    for (int i = 0; i < planeCount(); ++i) {
        memcpy(dst, constBits(i) + posBytes, lenBytes);
        dst += lenBytes;
    }

    AudioFrame f(d->format, buf);
    f.setSamplesPerChannel(bufSize / d->format.bytesPerSample());
    f.setTimestamp(d->timestamp + (qreal) d->format.durationForBytes(posBytes) / AudioFormat::kHz);
    // meta data?
    return f;
}

void AudioFrame::prepend(AudioFrame &other)
{
    Q_D(AudioFrame);

    if (d->format != other.format()) {
        qWarning() << "To prepend a frame it must have the same audio format";
        return;
    }

    d->data.prepend(other.data());
    d->samples_per_ch += other.samplesPerChannel();
    d->timestamp = other.timestamp();

    for (int i = 0; i < planeCount(); i++) {
        d->line_sizes[i] += other.bytesPerLine(i);
    }
}

AudioFormat AudioFrame::format() const
{
    return d_func()->format;
}

void AudioFrame::setSamplesPerChannel(int samples)
{
    Q_D(AudioFrame);
    if (!d->format.isValid()) {
        qWarning() << "can not set spc for an invalid format: " << d->format;
        return;
    }
    d->samples_per_ch = samples;
    const int nb_planes = d->format.planeCount();
    const int bpl(d->line_sizes[0] > 0 ? d->line_sizes[0] : d->samples_per_ch*d->format.bytesPerSample() * (d->format.isPlanar() ? 1 : d->format.channels()));
    for (int i = 0; i < nb_planes; ++i) {
        setBytesPerLine(bpl, i);
    }
    if (d->data.isEmpty())
        return;
    if (!constBits(0)) {
        setBits((quint8*)d->data.constData(), 0);
    }
    for (int i = 1; i < nb_planes; ++i) {
        if (!constBits(i)) {
            setBits((uchar*)constBits(i-1) + bpl, i);
        }
    }
}

int AudioFrame::samplesPerChannel() const
{
    return d_func()->samples_per_ch;
}

void AudioFrame::setAudioResampler(AudioResampler *conv)
{
    d_func()->conv = conv;
}

qint64 AudioFrame::duration() const
{
    Q_D(const AudioFrame);
    return d->format.durationForBytes(d->data.size());
}

AudioFrame AudioFrame::to(const AudioFormat &fmt) const
{
    if (!isValid() || !constBits(0))
        return AudioFrame();
    //if (fmt == format())
      //  return clone(); //FIXME: clone a frame from ffmpeg is not enough?
    Q_D(const AudioFrame);
    // TODO: use a pool
    AudioResampler *conv = d->conv;
    QScopedPointer<AudioResampler> c;
    if (!conv) {
        conv = AudioResampler::create(AudioResamplerId_FF);
        if (!conv)
            conv = AudioResampler::create(AudioResamplerId_Libav);
        if (!conv) {
            qWarning("no audio resampler is available");
            return AudioFrame();
        }
        c.reset(conv);
    }
    conv->setInAudioFormat(format());
    conv->setOutAudioFormat(fmt);
    //conv->prepare(); // already called in setIn/OutFormat
    conv->setInSampesPerChannel(samplesPerChannel()); //TODO
    if (!conv->convert((const quint8**)d->planes.constData())) {
        qWarning() << "AudioFrame::to error: " << format() << "=>" << fmt;
        return AudioFrame();
    }
    AudioFrame f(fmt, conv->outData());
    f.setSamplesPerChannel(conv->outSamplesPerChannel());
    f.setTimestamp(timestamp());
    f.d_ptr->metadata = d->metadata; // need metadata?
    return f;
}
} //namespace QtAV
