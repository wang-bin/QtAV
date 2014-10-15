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

#include "QtAV/AudioFormat.h"
#include "QtAV/private/AVCompat.h"
#ifndef QT_NO_DEBUG_STREAM
#include <QtDebug>
#endif //QT_NO_DEBUG_STREAM
//FF_API_OLD_SAMPLE_FMT. e.g. FFmpeg 0.9
#ifdef SampleFormat
#undef SampleFormat
#endif


namespace QtAV {

const qint64 kHz = 1000000LL;

typedef struct {
    qint64 ff;
    AudioFormat::ChannelLayout cl;
} ChannelLayoutMap;

static const ChannelLayoutMap kChannelLayoutMap[] = {
    { AV_CH_FRONT_LEFT, AudioFormat::ChannelLayout_Left },
    { AV_CH_FRONT_RIGHT, AudioFormat::ChannelLayout_Right },
    { AV_CH_FRONT_CENTER, AudioFormat::ChannelLayout_Center },
    { AV_CH_LAYOUT_MONO, AudioFormat::ChannelLayout_Mono },
    { AV_CH_LAYOUT_STEREO, AudioFormat::ChannelLayout_Stero },
    { 0, AudioFormat::ChannelLayout_Unsupported}
};

AudioFormat::ChannelLayout AudioFormat::channelLayoutFromFFmpeg(qint64 clff)
{
    for (unsigned int i = 0; i < sizeof(kChannelLayoutMap)/sizeof(ChannelLayoutMap); ++i) {
        if (kChannelLayoutMap[i].ff == clff)
            return kChannelLayoutMap[i].cl;
    }
    return AudioFormat::ChannelLayout_Unsupported;
}

qint64 AudioFormat::channelLayoutToFFmpeg(AudioFormat::ChannelLayout cl)
{
    for (unsigned int i = 0; i < sizeof(kChannelLayoutMap)/sizeof(ChannelLayoutMap); ++i) {
        if (kChannelLayoutMap[i].cl == cl)
            return kChannelLayoutMap[i].ff;
    }
    return 0;
}

class AudioFormatPrivate : public QSharedData
{
public:
    AudioFormatPrivate():
        planar(false)
      , sample_format(AudioFormat::SampleFormat_Input)
      , channels(0)
      , sample_rate(0)
      , channel_layout(AudioFormat::ChannelLayout_Unsupported)
      , channel_layout_ff(0)
    {}
    void setChannels(int cs) {
        channels = cs;
        if (av_get_channel_layout_nb_channels(channel_layout_ff) != channels) {
            channel_layout_ff = av_get_default_channel_layout(channels);
            channel_layout = AudioFormat::channelLayoutFromFFmpeg(channel_layout_ff);
        }
    }
    void setChannelLayoutFF(qint64 clff) {
        channel_layout_ff = clff;
        if (av_get_channel_layout_nb_channels(channel_layout_ff) != channels) {
            channels = av_get_channel_layout_nb_channels(channel_layout_ff);
        }
    }

    bool planar;
    AudioFormat::SampleFormat sample_format;
    int channels;
    int sample_rate;
    int bytes_per_sample;
    AudioFormat::ChannelLayout channel_layout;
    qint64 channel_layout_ff;
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
    Assigns \a other to this AudioFormat implementation.
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
            //compare channel layout first because it determines channel count
            d->channel_layout_ff == other.d->channel_layout_ff &&
            d->channel_layout == other.d->channel_layout &&
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

int AudioFormat::planeCount() const
{
    return isPlanar() ? 1 : channels();
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
void AudioFormat::setChannelLayoutFFmpeg(qint64 layout)
{
    //FFmpeg channel layout is more complete, so we just it
    d->channel_layout = channelLayoutFromFFmpeg(layout);
    d->setChannelLayoutFF(layout);
}

qint64 AudioFormat::channelLayoutFFmpeg() const
{
    return d->channel_layout_ff;
}

void AudioFormat::setChannelLayout(ChannelLayout layout)
{
    qint64 clff = channelLayoutToFFmpeg(layout);
    d->channel_layout = layout;
    //TODO: shall we set ffmpeg channel layout to 0(not valid value)?
    if (!clff)
        return;
    d->setChannelLayoutFF(clff);
}

AudioFormat::ChannelLayout AudioFormat::channelLayout() const
{
    return d->channel_layout;
}

QString AudioFormat::channelLayoutName() const
{
    char cl[128];
    av_get_channel_layout_string(cl, sizeof(cl), -1, channelLayoutFFmpeg()); //TODO: ff version
    return cl;
}

/*!
   Sets the channel count to \a channels.

*/
void AudioFormat::setChannels(int channels)
{
    d->setChannels(channels);
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

QString AudioFormat::sampleFormatName() const
{
    return av_get_sample_fmt_name((AVSampleFormat)sampleFormatFFmpeg());
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

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QtAV::AudioFormat &fmt)
{
    dbg.nospace() << "QtAV::AudioFormat(" << fmt.sampleRate();
    dbg.nospace() << "Hz, " << fmt.bytesPerSample();
    dbg.nospace() << "Bytes, channelCount:" << fmt.channels();
    dbg.nospace() << ", channelLayout: " << fmt.channelLayoutName();
    dbg.nospace() << ", sampleFormat: " << fmt.sampleFormatName();
    dbg.nospace() << ")";

    return dbg.space();
}

QDebug operator<<(QDebug dbg, QtAV::AudioFormat::SampleFormat sampleFormat)
{
    dbg.nospace() << av_get_sample_fmt_name((AVSampleFormat)sampleFormat);
    return dbg.space();
}

QDebug operator<<(QDebug dbg, QtAV::AudioFormat::ChannelLayout channelLayout)
{
    char cl[128];
    av_get_channel_layout_string(cl, sizeof(cl), -1, AudioFormat::channelLayoutToFFmpeg(channelLayout)); //TODO: ff version
    dbg.nospace() << cl;
    return dbg.space();
}
#endif


namespace {
    class AudioFormatPrivateRegisterMetaTypes
    {
    public:
        AudioFormatPrivateRegisterMetaTypes()
        {
            qRegisterMetaType<QtAV::AudioFormat>();
            qRegisterMetaType<QtAV::AudioFormat::SampleFormat>();
            qRegisterMetaType<QtAV::AudioFormat::ChannelLayout>();
        }
    } _registerMetaTypes;
}

} //namespace QtAV
