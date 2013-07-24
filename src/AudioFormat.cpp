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

#include "QtAV/AudioFormat.h"

namespace QtAV {

const qint64 kHz = 1000000LL;

class AudioFormatPrivate : public QSharedData
{
public:
    AudioFormatPrivate():
        planar(false)
      , sample_format(AudioFormat::SampleFormat_Input)
      , channels(0)
      , sample_rate(0)
      , channel_layout(0)
    {}
    bool planar;
    AudioFormat::SampleFormat sample_format;
    int channels;
    int sample_rate;
    int bytes_per_sample;
    AudioFormat::ChannelLayout channel_layout;
};

bool AudioFormat::isPlanar(SampleFormat format)
{
    return format == SampleFormat_Unsigned8Planar
            || format == SampleFormat_Signed16Planar
            || format == SampleFormat_Signed32Planar
            || format == SampleFormat_FloatPlanar
            || format == SampleFormat_DoublePlanar;
}


AudioFormat::AudioFormat():
    d(new AudioFormatPrivate())
{
}

AudioFormat::AudioFormat(const AudioFormat &other):
    d(other.d)
{
}

AudioFormat::~AudioFormat()
{
}

/*!
    Assigns \a other to this QAudioFormat implementation.
*/
AudioFormat& AudioFormat::operator=(const AudioFormat &other)
{
    d = other.d;
    return *this;
}

/*!
  Returns true if this AudioFormat is equal to the \a other
  AudioFormat; otherwise returns false.

  All elements of AudioFormat are used for the comparison.
*/
bool AudioFormat::operator==(const AudioFormat &other) const
{
    return d->sample_rate == other.d->sample_rate &&
            d->channels == other.d->channels &&
            //d->sampleSize == other.d->sampleSize &&
            d->sample_format == other.d->sample_format;
}

/*!
  Returns true if this AudioFormat is not equal to the \a other
  AudioFormat; otherwise returns false.

  All elements of AudioFormat are used for the comparison.
*/
bool AudioFormat::operator!=(const AudioFormat& other) const
{
    return !(*this == other);
}

/*!
    Returns true if all of the parameters are valid->
*/
bool AudioFormat::isValid() const
{
    return d->sample_rate > 0 && (d->channels > 0 || d->channel_layout > 0) &&
            d->sample_format != AudioFormat::SampleFormat_Unknown;
}

bool AudioFormat::isPlanar() const
{
    return d->planar;
}

/*!
   Sets the sample rate to \a samplerate Hertz.

*/
void AudioFormat::setSampleRate(int sampleRate)
{
    d->sample_rate = sampleRate;
}

/*!
    Returns the current sample rate in Hertz.

*/
int AudioFormat::sampleRate() const
{
    return d->sample_rate;
}

/*!
   Sets the channel layout to \a layout. Currently use FFmpeg's. see avutil/channel_layout.h
*/
void AudioFormat::setChannelLayout(ChannelLayout layout)
{
    d->channel_layout = layout;
}

AudioFormat::ChannelLayout AudioFormat::channelLayout() const
{
    return d->channel_layout;
}

/*!
   Sets the channel count to \a channels.

*/
void AudioFormat::setChannels(int channels)
{
    d->channels = channels;
}

/*!
    Returns the current channel count value.

*/
int AudioFormat::channels() const
{
    return d->channels;
}

/*!
   Sets the sampleFormat to \a sampleFormat.
*/
void AudioFormat::setSampleFormat(AudioFormat::SampleFormat sampleFormat)
{
    d->sample_format = sampleFormat;
    d->planar = AudioFormat::isPlanar(sampleFormat);
    switch (d->sample_format) {
    case AudioFormat::SampleFormat_Unsigned8:
    case AudioFormat::SampleFormat_Unsigned8Planar:
        d->bytes_per_sample = 8 >> 3;
        break;
    case AudioFormat::SampleFormat_Signed16:
    case AudioFormat::SampleFormat_Signed16Planar:
        d->bytes_per_sample = 16 >> 3;
        break;
    case AudioFormat::SampleFormat_Signed32:
    case AudioFormat::SampleFormat_Signed32Planar:
    case AudioFormat::SampleFormat_Float:
    case AudioFormat::SampleFormat_FloatPlanar:
        d->bytes_per_sample = 32 >> 3;
        break;
    case AudioFormat::SampleFormat_Double:
    case AudioFormat::SampleFormat_DoublePlanar:
        d->bytes_per_sample = 64 >> 3;
        break;
    default:
        d->bytes_per_sample = 0;
        break;
    }
}

/*!
    Returns the current SampleFormat value.
*/
AudioFormat::SampleFormat AudioFormat::sampleFormat() const
{
    return d->sample_format;
}

void AudioFormat::setSampleFormatFFmpeg(int ffSampleFormat)
{
    //currently keep the values the same as latest FFmpeg's
    setSampleFormat((AudioFormat::SampleFormat)ffSampleFormat);
}

int AudioFormat::sampleFormatFFmpeg() const
{
    return d->sample_format;
}


/*!
    Returns the number of bytes required for this audio format for \a duration microseconds.

    Returns 0 if this format is not valid->

    Note that some rounding may occur if \a duration is not an exact fraction of the
    sampleRate().

    \sa durationForBytes()
 */
qint32 AudioFormat::bytesForDuration(qint64 duration) const
{
    return bytesPerFrame() * framesForDuration(duration);
}

/*!
    Returns the number of microseconds represented by \a bytes in this format.

    Returns 0 if this format is not valid->

    Note that some rounding may occur if \a bytes is not an exact multiple
    of the number of bytes per frame.

    \sa bytesForDuration()
*/
qint64 AudioFormat::durationForBytes(qint32 bytes) const
{
    if (!isValid() || bytes <= 0)
        return 0;

    // We round the byte count to ensure whole frames
    return qint64(kHz * (bytes / bytesPerFrame())) / sampleRate();
}

/*!
    Returns the number of bytes required for \a frameCount frames of this format.

    Returns 0 if this format is not valid->

    \sa bytesForDuration()
*/
qint32 AudioFormat::bytesForFrames(qint32 frameCount) const
{
    return frameCount * bytesPerFrame();
}

/*!
    Returns the number of frames represented by \a byteCount in this format.

    Note that some rounding may occur if \a byteCount is not an exact multiple
    of the number of bytes per frame.

    Each frame has one sample per channel.

    \sa framesForDuration()
*/
qint32 AudioFormat::framesForBytes(qint32 byteCount) const
{
    int size = bytesPerFrame();
    if (size > 0)
        return byteCount / size;
    return 0;
}

/*!
    Returns the number of frames required to represent \a duration microseconds in this format.

    Note that some rounding may occur if \a duration is not an exact fraction of the
    \l sampleRate().
*/
qint32 AudioFormat::framesForDuration(qint64 duration) const
{
    if (!isValid())
        return 0;

    return qint32((duration * sampleRate()) / kHz);
}

/*!
    Return the number of microseconds represented by \a frameCount frames in this format.
*/
qint64 AudioFormat::durationForFrames(qint32 frameCount) const
{
    if (!isValid() || frameCount <= 0)
        return 0;

    return (frameCount * kHz) / sampleRate();
}

int AudioFormat::bytesPerFrame() const
{
    if (!isValid())
        return 0;

    return bytesPerSample() * channels();
}

int AudioFormat::bytesPerSample() const
{
    return d->bytes_per_sample;
}

int AudioFormat::bitRate() const
{
    return bytesPerSecond() << 3;
}

int AudioFormat::bytesPerSecond() const
{
    return channels() * bytesPerSample() * sampleRate();
}

} //namespace QtAV
