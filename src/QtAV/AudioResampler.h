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

#ifndef QTAV_AUDIORESAMPLER_H
#define QTAV_AUDIORESAMPLER_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/FactoryDefine.h>

namespace QtAV {

typedef int AudioResamplerId;
class AudioResampler;
FACTORY_DECLARE(AudioResampler)

class AudioFormat;
class AudioResamplerPrivate;
class Q_EXPORT AudioResampler //export is required for users who want add their own subclass outside QtAV
{
    DPTR_DECLARE_PRIVATE(AudioResampler)
public:
    AudioResampler();
    virtual ~AudioResampler();

    QByteArray outData() const;
    /* check whether the parameters are supported. If not, you should use ff*/
    virtual bool prepare(); //call after all parameters are setted
    virtual bool convert(const quint8** data);
    //speed: >0, default is 1
    void setSpeed(qreal speed); //out_sample_rate = out_sample_rate/speed
    qreal speed() const;

    void setInAudioFormat(const AudioFormat& format);
    AudioFormat& inAudioFormat();
    const AudioFormat& inAudioFormat() const;

    void setOutAudioFormat(const AudioFormat& format);
    AudioFormat& outAudioFormat();
    const AudioFormat& outAudioFormat() const;

    //decoded frame's samples/channel
    void setInSampesPerChannel(int samples);
    //channel count can be computed by av_get_channel_layout_nb_channels(chl)
    void setInSampleRate(int isr);
    void setOutSampleRate(int osr); //default is in
    //TODO: enum
    void setInSampleFormat(int isf); //FFmpeg sample format
    void setOutSampleFormat(int osf); //FFmpeg sample format. set by user. default is in
    //TODO: enum. layout will be set to the default layout of the channels if not defined
    void setInChannelLayout(qint64 icl);
    void setOutChannelLayout(qint64 ocl); //default is in
    void setInChannels(int channels);
    void setOutChannels(int channels);
    //Are getter functions required?
protected:
    AudioResampler(AudioResamplerPrivate& d);
    DPTR_DECLARE(AudioResampler)
};

} //namespace QtAV

#endif // QTAV_AUDIORESAMPLER_H
