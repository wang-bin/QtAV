/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_AUDIOENCODER_H
#define QTAV_AUDIOENCODER_H

#include <QtAV/AVEncoder.h>
#include <QtAV/FactoryDefine.h>
#include <QtAV/AudioFrame.h>

class QSize;
namespace QtAV {

typedef int AudioEncoderId;
class AudioEncoder;
FACTORY_DECLARE(AudioEncoder)

class AudioEncoderPrivate;
class Q_AV_EXPORT AudioEncoder : public AVEncoder
{
    Q_OBJECT
    Q_PROPERTY(QtAV::AudioFormat audioFormat READ audioFormat WRITE setAudioFormat NOTIFY audioFormatChanged)
    Q_DISABLE_COPY(AudioEncoder)
    DPTR_DECLARE_PRIVATE(AudioEncoder)
public:
    static AudioEncoder* create(AudioEncoderId id);
    /*!
     * \brief create
     * create an encoder from registered names
     * \param name can be "FFmpeg". FFmpeg encoder will be created for empty name
     * \return 0 if not registered
     */
    static AudioEncoder* create(const QString& name = QStringLiteral("FFmpeg"));
    virtual AudioEncoderId id() const = 0;
    QString name() const Q_DECL_OVERRIDE; //name from factory
    /*!
     * \brief encode
     * encode a audio frame to a Packet
     * \param frame pass an invalid frame to get delayed frames
     * \return
     */
    virtual bool encode(const AudioFrame& frame = AudioFrame()) = 0;

    /// output parameters
    /*!
     * \brief audioFormat
     * If not set or set to an invalid format, a supported format will be used and audioFormat() will be that format after open()
     * \return
     * TODO: check supported formats
     */
    const AudioFormat& audioFormat() const;
    void setAudioFormat(const AudioFormat& format);
Q_SIGNALS:
    void audioFormatChanged();
protected:
    AudioEncoder(AudioEncoderPrivate& d);
private:
    AudioEncoder();
};

} //namespace QtAV
#endif // QTAV_AUDIOENCODER_H
