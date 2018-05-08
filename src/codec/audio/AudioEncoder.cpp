/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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

#include "QtAV/AudioEncoder.h"
#include "QtAV/private/AVEncoder_p.h"
#include "QtAV/private/factory.h"
#include "utils/Logger.h"

namespace QtAV {
FACTORY_DEFINE(AudioEncoder)

void AudioEncoder_RegisterAll()
{
    static bool called = false;
    if (called)
        return;
    called = true;
    // factory.h does not check whether an id is registered
    if (AudioEncoder::id("FFmpeg")) //registered on load
        return;
    extern bool RegisterAudioEncoderFFmpeg_Man();
    RegisterAudioEncoderFFmpeg_Man();
}

QStringList AudioEncoder::supportedCodecs()
{
    static QStringList codecs;
    if (!codecs.isEmpty())
        return codecs;
    const AVCodec* c = NULL;
#if AVCODEC_STATIC_REGISTER
    void* it = NULL;
    while ((c = av_codec_iterate(&it))) {
#else
    avcodec_register_all();
    while ((c = av_codec_next(c))) {
#endif
        if (!av_codec_is_encoder(c) || c->type != AVMEDIA_TYPE_AUDIO)
            continue;
        codecs.append(QString::fromLatin1(c->name));
    }
    return codecs;
}

AudioEncoder::AudioEncoder(AudioEncoderPrivate &d):
    AVEncoder(d)
{
}

QString AudioEncoder::name() const
{
    return QLatin1String(AudioEncoder::name(id()));
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

int AudioEncoder::frameSize() const
{
    return d_func().frame_size;
}
} //namespace QtAV
