/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QAV_AUDIODECODER_H
#define QAV_AUDIODECODER_H

#include <QtAV/AVDecoder.h>
#include <QtAV/AudioFrame.h>
#include <QtAV/FactoryDefine.h>

#define USE_AUDIO_FRAME 1
//TODO: decoder.in/outAudioFormat()?
namespace QtAV {
typedef int AudioDecoderId;
// built-in decoders
extern Q_AV_EXPORT AudioDecoderId AudioDecoderId_FFmpeg;

class AudioResampler;
class AudioDecoderPrivate;
class Q_AV_EXPORT AudioDecoder : public AVDecoder
{
    Q_DISABLE_COPY(AudioDecoder)
    DPTR_DECLARE_PRIVATE(AudioDecoder)
public:
    static AudioDecoder* create(AudioDecoderId id);
    /*!
     * \brief create
     * create a decoder from registered name
     * \param name can be "FFmpeg"
     * \return 0 if not registered
     */
    static AudioDecoder* create(const QString& name);
    virtual AudioDecoderId id() const = 0;
    QString name() const; //name from factory
    virtual QByteArray data() const; //decoded data
    virtual AudioFrame frame() = 0;
    AudioResampler *resampler(); //TODO: remove. can not share the same resampler for multiple frames
protected:
    AudioDecoder(AudioDecoderPrivate& d);
private:
    AudioDecoder();
};

FACTORY_DECLARE(AudioDecoder)
} //namespace QtAV
#endif // QAV_AUDIODECODER_H
