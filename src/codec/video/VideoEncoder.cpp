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

#include "QtAV/VideoEncoder.h"
#include "QtAV/private/AVEncoder_p.h"
#include "QtAV/private/factory.h"
#include "utils/Logger.h"

namespace QtAV {
FACTORY_DEFINE(VideoEncoder)

void VideoEncoder_RegisterAll()
{
    static bool called = false;
    if (called)
        return;
    called = true;
    // factory.h does not check whether an id is registered
    if (!VideoEncoderFactory::Instance().registeredIds().empty()) //registered on load
        return;
    extern bool RegisterVideoEncoderFFmpeg_Man();
    RegisterVideoEncoderFFmpeg_Man();
}

QStringList VideoEncoder::supportedCodecs()
{
    // should check every registered encoders
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
        if (!av_codec_is_encoder(c) || c->type != AVMEDIA_TYPE_VIDEO)
            continue;
        codecs.append(QString::fromLatin1(c->name));
    }
    return codecs;
}

VideoEncoder::VideoEncoder(VideoEncoderPrivate &d):
    AVEncoder(d)
{
}

QString VideoEncoder::name() const
{
    return QLatin1String(VideoEncoder::name(id()));
}

void VideoEncoder::setWidth(int value)
{
    DPTR_D(VideoEncoder);
    if (d.width == value)
        return;
    d.width = value;
    emit widthChanged();
}

int VideoEncoder::width() const
{
    return d_func().width;
}

void VideoEncoder::setHeight(int value)
{
    DPTR_D(VideoEncoder);
    if (d.height == value)
        return;
    d.height = value;
    emit heightChanged();
}

int VideoEncoder::height() const
{
    return d_func().height;
}

void VideoEncoder::setFrameRate(qreal value)
{
    DPTR_D(VideoEncoder);
    if (d.frame_rate == value)
        return;
    d.frame_rate = value;
    emit frameRateChanged();
}

qreal VideoEncoder::frameRate() const
{
    return d_func().frame_rate;
}

void VideoEncoder::setPixelFormat(const VideoFormat::PixelFormat format)
{
    DPTR_D(VideoEncoder);
    if (d.format.pixelFormat() == format)
        return;
    d.format.setPixelFormat(format);
    d.format_used = format;
    Q_EMIT pixelFormatChanged();
}

VideoFormat::PixelFormat VideoEncoder::pixelFormat() const
{
    return d_func().format_used;
}

} //namespace QtAV
