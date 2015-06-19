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

#include <QtAV/AudioEncoder.h>
#include <QtAV/private/AVEncoder_p.h>
#include "QtAV/private/factory.h"
#include "utils/Logger.h"

namespace QtAV {

FACTORY_DEFINE(AudioEncoder)

void AudioEncoder_RegisterAll()
{
    extern void RegisterAudioEncoderFFmpeg_Man();
    RegisterAudioEncoderFFmpeg_Man();
}

AudioEncoder* AudioEncoder::create(AudioEncoderId id)
{
    return AudioEncoderFactory::create(id);
}

AudioEncoder* AudioEncoder::create(const QString& name)
{
    return AudioEncoderFactory::create(AudioEncoderFactory::id(name.toUtf8().constData(), false));
}

AudioEncoder::AudioEncoder(AudioEncoderPrivate &d):
    AVEncoder(d)
{
}

QString AudioEncoder::name() const
{
    return QString(AudioEncoderFactory::name(id()).c_str());
}

void AudioEncoder::setAudioFormat(const AudioFormat& format)
{
    DPTR_D(AudioEncoder);
    if (d.format == format)
        return;
    d.format = format;
    d.format_used = format;
    Q_EMIT audioFormatChanged();
}

const AudioFormat& AudioEncoder::audioFormat() const
{
    return d_func().format_used;
}

} //namespace QtAV
