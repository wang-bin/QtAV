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

#include "QtAV/AudioResampler.h"
#include "QtAV/AudioFormat.h"
#include "QtAV/private/AudioResampler_p.h"
#include "QtAV/private/factory.h"

namespace QtAV {

FACTORY_DEFINE(AudioResampler)

extern void RegisterAudioResamplerFF_Man();
extern void RegisterAudioResamplerLibav_Man();

void AudioResampler_RegisterAll()
{
#if QTAV_HAVE(SWRESAMPLE)
    RegisterAudioResamplerFF_Man();
#endif //QTAV_HAVE(SWRESAMPLE)
#if QTAV_HAVE(AVRESAMPLE)
    RegisterAudioResamplerLibav_Man();
#endif //QTAV_HAVE(AVRESAMPLE)
}

AudioResampler::AudioResampler()
{
}

AudioResampler::AudioResampler(AudioResamplerPrivate& d):DPTR_INIT(&d)
{
}

AudioResampler::~AudioResampler()
{
}

QByteArray AudioResampler::outData() const
{
    return d_func().data_out;
}

bool AudioResampler::prepare()
{
    if (!inAudioFormat().isValid()) {
        qWarning("src audio parameters 'channel layout(or channels), sample rate and sample format must be set before initialize resampler");
        return false;
    }
    return true;
}

bool AudioResampler::convert(const quint8 **data)
{
    Q_UNUSED(data);
    return false;
}

void AudioResampler::setSpeed(qreal speed)
{
    DPTR_D(AudioResampler);
    if (d.speed == speed)
        return;
    d.speed = speed;
    prepare();
}

qreal AudioResampler::speed() const
{
    return d_func().speed;
}


void AudioResampler::setInAudioFormat(const AudioFormat& format)
{
    DPTR_D(AudioResampler);
    if (d.in_format == format)
        return;
    d.in_format = format;
    prepare();
}

AudioFormat& AudioResampler::inAudioFormat()
{
    return d_func().in_format;
}

const AudioFormat& AudioResampler::inAudioFormat() const
{
    return d_func().in_format;
}

void AudioResampler::setOutAudioFormat(const AudioFormat& format)
{
    d_func().out_format = format;
}

AudioFormat& AudioResampler::outAudioFormat()
{
    return d_func().out_format;
}

const AudioFormat& AudioResampler::outAudioFormat() const
{
    return d_func().out_format;
}

void AudioResampler::setInSampesPerChannel(int samples)
{
    d_func().in_samples_per_channel = samples;
}

int AudioResampler::outSamplesPerChannel() const
{
    return d_func().out_samples_per_channel;
}

//channel count can be computed by av_get_channel_layout_nb_channels(chl)
void AudioResampler::setInSampleRate(int isr)
{
    DPTR_D(AudioResampler);
    d.in_format.setSampleRate(isr);
    setInAudioFormat(d.in_format);
}

void AudioResampler::setOutSampleRate(int osr)
{
    DPTR_D(AudioResampler);
    d.out_format.setSampleRate(osr);
    setOutAudioFormat(d.out_format);
}

void AudioResampler::setInSampleFormat(int isf)
{
    DPTR_D(AudioResampler);
    d.in_format.setSampleFormatFFmpeg(isf);
    setInAudioFormat(d.in_format);
}

void AudioResampler::setOutSampleFormat(int osf)
{
    DPTR_D(AudioResampler);
    d.out_format.setSampleFormatFFmpeg(osf);
    setOutAudioFormat(d.out_format);
}

void AudioResampler::setInChannelLayout(qint64 icl)
{
    DPTR_D(AudioResampler);
    d.in_format.setChannelLayoutFFmpeg(icl);
    setInAudioFormat(d.in_format);
}

void AudioResampler::setOutChannelLayout(qint64 ocl)
{
    DPTR_D(AudioResampler);
    d.out_format.setChannelLayoutFFmpeg(ocl);
    setOutAudioFormat(d.out_format);
}

void AudioResampler::setInChannels(int channels)
{
    DPTR_D(AudioResampler);
    d.in_format.setChannels(channels);
    setInAudioFormat(d.in_format);
}

void AudioResampler::setOutChannels(int channels)
{
    DPTR_D(AudioResampler);
    d.out_format.setChannels(channels);
    setOutAudioFormat(d.out_format);
}

} //namespace QtAV
