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

#include "AudioResampler.h"
#include "private/AudioResampler_p.h"
#include "factory.h"

namespace QtAV {

FACTORY_DEFINE(AudioResampler)

extern void RegisterAudioResamplerFF_Man();

void AudioResampler_RegisterAll()
{
#if QTAV_HAVE(SWRESAMPLE) || QTAV_HAVE(AVRESAMPLE)
    RegisterAudioResamplerFF_Man();
#endif
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
    DPTR_D(AudioResampler);
    if (!d.in_channel_layout || !d.in_sample_rate || d.in_sample_format == AV_SAMPLE_FMT_NONE) {
        qWarning("src audio parameters in_channel_layout, in_sample_rate, in_sample_format must be set before initialize resampler");
        return false;
    }
    return true;
}

bool AudioResampler::convert(const quint8 **data)
{
    return false;
}

void AudioResampler::setSpeed(qreal speed)
{
    d_func().speed = speed;
}

qreal AudioResampler::speed() const
{
    return d_func().speed;
}

void AudioResampler::setInSampes(int samples)
{
    d_func().in_samples = samples;
}

//channel count can be computed by av_get_channel_layout_nb_channels(chl)
void AudioResampler::setInSampleRate(int isr)
{
    d_func().in_sample_rate = isr;
}

void AudioResampler::setOutSampleRate(int osr)
{
    d_func().out_sample_rate = osr;
}

void AudioResampler::setInSampleFormat(int isf)
{
    d_func().in_sample_format = isf;
}

void AudioResampler::setOutSampleFormat(int osf)
{
    d_func().out_sample_format = osf;
}

void AudioResampler::setInChannelLayout(int icl)
{
    d_func().in_channel_layout = icl;
}

void AudioResampler::setOutChannelLayout(int ocl)
{
    d_func().out_channel_layout = ocl;
}

void AudioResampler::setInChannels(int channels)
{
    d_func().in_nb_channels = channels;
}

void AudioResampler::setOutChannels(int channels)
{
    d_func().out_nb_channels = channels;
}

} //namespace QtAV
