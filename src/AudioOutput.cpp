/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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
#include <private/AudioOutput_p.h>

namespace QtAV {
AudioOutput::AudioOutput()
    :AVOutput(*new AudioOutputPrivate())
{
}

AudioOutput::AudioOutput(AudioOutputPrivate &d)
    :AVOutput(d)
{
}

AudioOutput::~AudioOutput()
{
}

void AudioOutput::setSampleRate(int rate)
{
    d_func().sample_rate = rate;
}

int AudioOutput::sampleRate() const
{
    return d_func().sample_rate;
}

void AudioOutput::setChannels(int channels)
{
    d_func().channels = channels;
    //outputParameters->channelCount = channels;
}

int AudioOutput::channels() const
{
    return d_func().channels;
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

} //namespace QtAV
