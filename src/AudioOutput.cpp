/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

void AudioOutput::convertData(const QByteArray &data)
{;
	d_func().data = data;
}

} //namespace QtAV
