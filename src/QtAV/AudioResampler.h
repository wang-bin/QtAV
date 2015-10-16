/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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

namespace QtAV {

typedef int AudioResamplerId;
class AudioFormat;
class AudioResamplerPrivate;
class Q_AV_EXPORT AudioResampler //export is required for users who want add their own subclass outside QtAV
{
    DPTR_DECLARE_PRIVATE(AudioResampler)
public:
    AudioResampler();
    virtual ~AudioResampler();
    // if QtAV is static linked (ios for example), components may be not automatically registered. Add registerAll() to workaround
    static void registerAll();
    template<class C>
    static bool Register(AudioResamplerId id, const char* name) { return Register(id, create<C>, name);}
    static AudioResampler* create(AudioResamplerId id);
    static AudioResampler* create(const char* name);
    /*!
     * \brief next
     * \param id NULL to get the first id address
     * \return address of id or NULL if not found/end
     */
    static AudioResamplerId* next(AudioResamplerId* id = 0);
    static const char* name(AudioResamplerId id);
    static AudioResamplerId id(const char* name);

    QByteArray outData() const;
    /* check whether the parameters are supported. If not, you should use ff*/
    /*!
     * \brief prepare
     * Check whether the parameters are supported and setup the resampler
     * setIn/OutXXX will call prepare() if format is changed
     */
    virtual bool prepare();
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
    // > 0 valid after resample done
    int outSamplesPerChannel() const;
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
private:
    template<class C>
    static AudioResampler* create() {
        return new C();
    }
    typedef AudioResampler* (*AudioResamplerCreator)();
    static bool Register(AudioResamplerId id, AudioResamplerCreator, const char *name);

protected:
    AudioResampler(AudioResamplerPrivate& d);
    DPTR_DECLARE(AudioResampler)
};

} //namespace QtAV

#endif // QTAV_AUDIORESAMPLER_H
