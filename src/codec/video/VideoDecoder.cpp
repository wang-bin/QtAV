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

#include <QtAV/VideoDecoder.h>
#include <QtAV/private/AVDecoder_p.h>
#include <QtCore/QSize>
#include "QtAV/private/factory.h"
#include "utils/Logger.h"

namespace QtAV {

FACTORY_DEFINE(VideoDecoder)

extern void RegisterVideoDecoderFFmpeg_Man();
extern void RegisterVideoDecoderDXVA_Man();
extern void RegisterVideoDecoderCUDA_Man();
extern void RegisterVideoDecoderVAAPI_Man();
extern void RegisterVideoDecoderVDA_Man();
extern void RegisterVideoDecoderCedarv_Man();

void VideoDecoder_RegisterAll()
{
    RegisterVideoDecoderFFmpeg_Man();
#if QTAV_HAVE(DXVA)
    RegisterVideoDecoderDXVA_Man();
#endif //QTAV_HAVE(DXVA)
#if QTAV_HAVE(CUDA)
    RegisterVideoDecoderCUDA_Man();
#endif //QTAV_HAVE(CUDA)
#if QTAV_HAVE(VAAPI)
    RegisterVideoDecoderVAAPI_Man();
#endif //QTAV_HAVE(VAAPI)
#if QTAV_HAVE(VDA)
    RegisterVideoDecoderVDA_Man();
#endif //QTAV_HAVE(VDA)
#if QTAV_HAVE(CEDARV)
    RegisterVideoDecoderCedarv_Man();
#endif //QTAV_HAVE(CEDARV)
}

VideoDecoder* VideoDecoder::create(VideoDecoderId id)
{
    return VideoDecoderFactory::create(id);
}

VideoDecoder* VideoDecoder::create(const QString& name)
{
    return VideoDecoderFactory::create(VideoDecoderFactory::id(name.toUtf8().constData(), false));
}

VideoDecoder::VideoDecoder(VideoDecoderPrivate &d):
    AVDecoder(d)
{
}

QString VideoDecoder::name() const
{
    return QString(VideoDecoderFactory::name(id()).c_str());
}

void VideoDecoder::resizeVideoFrame(const QSize &size)
{
    resizeVideoFrame(size.width(), size.height());
}

/*
 * width, height: the decoded frame size
 * 0, 0 to reset to original video frame size
 */
void VideoDecoder::resizeVideoFrame(int width, int height)
{
    DPTR_D(VideoDecoder);
    d.width = width;
    d.height = height;
}

int VideoDecoder::width() const
{
    return d_func().width;
}

int VideoDecoder::height() const
{
    return d_func().height;
}

} //namespace QtAV
