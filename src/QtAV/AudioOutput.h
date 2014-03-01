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

#ifndef QAV_AUDIOOUTPUT_H
#define QAV_AUDIOOUTPUT_H

#include <QtAV/AVOutput.h>
#include <QtAV/FactoryDefine.h>
#include <QtAV/AudioFormat.h>
//TODO: audio device class

namespace QtAV {

typedef int AudioOutputId;
class AudioOutput;
FACTORY_DECLARE(AudioOutput)

class AudioFormat;
class AudioOutputPrivate;
class Q_AV_EXPORT AudioOutput : public AVOutput
{
    DPTR_DECLARE_PRIVATE(AudioOutput)
public:
    AudioOutput();
    virtual ~AudioOutput() = 0;
    /* store the data ref, then call convertData() and write(). tryPause() will be called*/
    bool receiveData(const QByteArray& data);

    int maxChannels() const;
    //virtual bool isSupported(const AudioFormat& format);
    void setAudioFormat(const AudioFormat& format);
    AudioFormat& audioFormat();
    const AudioFormat& audioFormat() const;

    void setSampleRate(int rate); //deprecated
    int sampleRate() const; //deprecated

    void setChannels(int channels); //deprecated
    int channels() const; //deprecated

    void setVolume(qreal volume);
    qreal volume() const;
    void setMute(bool yes);
    bool isMute() const;
    /*!
     * \brief setSpeed  set audio playing speed
     *
     * The speed affects the playing only if audio is available and clock type is
     * audio clock. For example, play a video contains audio without special configurations.
     * To change the playing speed in other cases, use AVPlayer::setSpeed(qreal)
     * \param speed
     */
    void setSpeed(qreal speed);
    qreal speed() const;

    /*!
     * \brief isSuppported
     * \param format
     * \return true if format is supported. default is true
     */
    virtual bool isSuppported(const AudioFormat& format) const;
    virtual bool isSuppported(AudioFormat::SampleFormat) const;
    /*!
     * \brief preferredSampleFormat
     * \return the preferred sample format
     *  If the specified format is not supported, resample to preffered format
     */
    virtual AudioFormat::SampleFormat preferredSampleFormat() const;
    //virtual AudioFormat::ChannelLayout preferredChannelLayout() const;

protected:
    AudioOutput(AudioOutputPrivate& d);
    virtual bool write() = 0;
};

} //namespace QtAV
#endif // QAV_AUDIOOUTPUT_H
