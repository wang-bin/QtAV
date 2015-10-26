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
#include <QtAV/AudioFrame.h>
#include <QtCore/QStringList>

namespace QtAV {
typedef int AudioEncoderId;
class AudioEncoderPrivate;
class Q_AV_EXPORT AudioEncoder : public AVEncoder
{
    Q_OBJECT
    Q_PROPERTY(QtAV::AudioFormat audioFormat READ audioFormat WRITE setAudioFormat NOTIFY audioFormatChanged)
    Q_DISABLE_COPY(AudioEncoder)
    DPTR_DECLARE_PRIVATE(AudioEncoder)
public:
    static QStringList supportedCodecs();
    static AudioEncoder* create(AudioEncoderId id);
    /*!
     * \brief create
     * create an encoder from registered names
     * \param name can be "FFmpeg". FFmpeg encoder will be created for empty name
     * \return 0 if not registered
     */
    static AudioEncoder* create(const char* name = "FFmpeg");
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
public:
    template<class C> static bool Register(AudioEncoderId id, const char* name) { return Register(id, create<C>, name);}
    /*!
     * \brief next
     * \param id NULL to get the first id address
     * \return address of id or NULL if not found/end
     */
    static AudioEncoderId* next(AudioEncoderId* id = 0);
    static const char* name(AudioEncoderId id);
    static AudioEncoderId id(const char* name);
private:
    template<class C> static AudioEncoder* create() { return new C();}
    typedef AudioEncoder* (*AudioEncoderCreator)();
    static bool Register(AudioEncoderId id, AudioEncoderCreator, const char *name);
protected:
    AudioEncoder(AudioEncoderPrivate& d);
private:
    AudioEncoder();
};

} //namespace QtAV
#endif // QTAV_AUDIOENCODER_H
