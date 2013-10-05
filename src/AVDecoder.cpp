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

#include <QtAV/AVDecoder.h>
#include <private/AVDecoder_p.h>

namespace QtAV {
AVDecoder::AVDecoder()
{
}

AVDecoder::AVDecoder(AVDecoderPrivate &d)
    :DPTR_INIT(&d)
{

}

AVDecoder::~AVDecoder()
{
}

void AVDecoder::flush()
{
    if (isAvailable())
        avcodec_flush_buffers(d_func().codec_ctx);
}

void AVDecoder::setCodecContext(AVCodecContext *codecCtx)
{
    DPTR_D(AVDecoder);
    d.codec_ctx = codecCtx;
}

AVCodecContext* AVDecoder::codecContext() const
{
    return d_func().codec_ctx;
}

void AVDecoder::setLowResolution(int lowres)
{
    d_func().low_resolution = lowres;
}

int AVDecoder::lowResolution() const
{
    return d_func().low_resolution;
}

void AVDecoder::setDecodeThreads(int threads)
{
    DPTR_D(AVDecoder);
    d.threads = threads > 0 ? threads : av_cpu_count();
}

int AVDecoder::decodeThreads() const
{
    return d_func().threads;
}

void AVDecoder::setThreadSlice(bool s)
{
    d_func().thread_slice = s;
}

bool AVDecoder::isAvailable() const
{
    return d_func().codec_ctx != 0;
}

bool AVDecoder::prepare()
{
    DPTR_D(AVDecoder);
    if (!d.codec_ctx) {
        qWarning("call this after AVCodecContext is set!");
        return false;
    }
    qDebug("Decoding threads count: %d", d.threads);
    d.codec_ctx->thread_count = d.threads;
    if (d.threads > 1)
        d.codec_ctx->thread_type = d.thread_slice ? FF_THREAD_SLICE : FF_THREAD_FRAME;
    else
        d.codec_ctx->thread_type = 0; //FF_THREAD_FRAME:1, FF_THREAD_SLICE:2

    //? !CODEC_ID_H264 && !CODEC_ID_VP8
    d.codec_ctx->lowres = d.low_resolution;

    return true;
}

bool AVDecoder::decode(const QByteArray &encoded)
{
    Q_UNUSED(encoded);
    return true;
}

QByteArray AVDecoder::data() const
{
    return d_func().decoded;
}

} //namespace QtAV
