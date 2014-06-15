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

#include <QtAV/AudioOutput.h>
#include <QtAV/private/AudioOutput_p.h>

namespace QtAV {
AudioOutput::AudioOutput()
    :AVOutput(*new AudioOutputPrivate())
{
    DPTR_D(AudioOutput);
    d_func().format.setSampleFormat(preferredSampleFormat());
    d_func().format.setChannelLayout(preferredChannelLayout());
}

AudioOutput::AudioOutput(AudioOutputPrivate &d)
    :AVOutput(d)
{
    d_func().format.setSampleFormat(preferredSampleFormat());
    d_func().format.setChannelLayout(preferredChannelLayout());
}

AudioOutput::~AudioOutput()
{
}

bool AudioOutput::receiveData(const QByteArray &data, qreal pts)
{
    DPTR_D(AudioOutput);
    //DPTR_D(AVOutput);
    //locker(&mutex)
    //TODO: make sure d.data thread safe. lock around here? for audio and video(main thread problem)?
    /* you can use d.data directly in AVThread. In other thread, it's not safe, you must do something
     * to make sure the data is not be modified in AVThread when using it*/
    if (d.paused)
        return false;
    d.data = data;
    d.nextEnqueueInfo().data_size = data.size();
    d.nextEnqueueInfo().timestamp = pts;
    d.bufferAdded();
    return write();
}

int AudioOutput::maxChannels() const
{
    return d_func().max_channels;
}

void AudioOutput::setAudioFormat(const AudioFormat& format)
{
    DPTR_D(AudioOutput);
    if (!isSupported(format)) {
        return;
    }
    d.format = format;
}

AudioFormat& AudioOutput::audioFormat()
{
    return d_func().format;
}

const AudioFormat& AudioOutput::audioFormat() const
{
    return d_func().format;
}

void AudioOutput::setSampleRate(int rate)
{
    d_func().format.setSampleRate(rate);
}

int AudioOutput::sampleRate() const
{
    return d_func().format.sampleRate();
}

// TODO: check isSupported
void AudioOutput::setChannels(int channels)
{
    DPTR_D(AudioOutput);
    if (channels > d.max_channels) {
        qWarning("not support. max channels is %d", d.max_channels);
        return;
    }
    d.format.setChannels(channels);
}

int AudioOutput::channels() const
{
    return d_func().format.channels();
}

void AudioOutput::setVolume(qreal volume)
{
    DPTR_D(AudioOutput);
    d.vol = qMax<qreal>(volume, 0);
    d.mute = d.vol == 0;
}

qreal AudioOutput::volume() const
{
    return qMax<qreal>(d_func().vol, 0);
}

void AudioOutput::setMute(bool yes)
{
    d_func().mute = yes;
}

bool AudioOutput::isMute() const
{
    return !isAvailable() || d_func().mute;
}

void AudioOutput::setSpeed(qreal speed)
{
    d_func().speed = speed;
}

qreal AudioOutput::speed() const
{
    return d_func().speed;
}

bool AudioOutput::isSupported(const AudioFormat &format) const
{
    Q_UNUSED(format);
    return isSupported(format.sampleFormat()) && isSupported(format.channelLayout());
}

bool AudioOutput::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    Q_UNUSED(sampleFormat);
    return true;
}

bool AudioOutput::isSupported(AudioFormat::ChannelLayout channelLayout) const
{
    Q_UNUSED(channelLayout);
    return true;
}

AudioFormat::SampleFormat AudioOutput::preferredSampleFormat() const
{
    return AudioFormat::SampleFormat_Float;
}

AudioFormat::ChannelLayout AudioOutput::preferredChannelLayout() const
{
    return AudioFormat::ChannelLayout_Stero;
}

void AudioOutput::waitForNextBuffer()
{
}

qreal AudioOutput::timestamp() const
{
    DPTR_D(const AudioOutput);
    return d.nextDequeueInfo().timestamp;
}

} //namespace QtAV
