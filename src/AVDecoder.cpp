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

bool AVDecoder::open()
{
    DPTR_D(AVDecoder);
    if (!d.codec_ctx) {
        qWarning("FFmpeg codec context not ready");
        return false;
    }
    AVCodec *codec = 0;
    if (!d.name.isEmpty()) {
        codec = avcodec_find_decoder_by_name(d.name.toUtf8().constData());
    } else {
        codec = avcodec_find_decoder(d.codec_ctx->codec_id);
    }
    if (!codec) {
        if (d.name.isEmpty()) {
            qWarning("No codec could be found with id %d", d.codec_ctx->codec_id);
        } else {
            qWarning("No codec could be found with name %s", d.name.toUtf8().constData());
        }
        return false;
    }

    //setup codec context
    if (d.low_resolution > codec->max_lowres) {
        qWarning("Use the max value for lowres supported by the decoder (%d)", codec->max_lowres);
        d.low_resolution = codec->max_lowres;
    }
    d.codec_ctx->lowres = d.low_resolution;
    if (d.codec_ctx->lowres) {
        d.codec_ctx->flags |= CODEC_FLAG_EMU_EDGE;
    }
    if (d.fast) {
        d.codec_ctx->flags2 |= CODEC_FLAG2_FAST;
    } else {
        //d.codec_ctx->flags2 &= ~CODEC_FLAG2_FAST; //ffplay has no this
    }
    if (codec->capabilities & CODEC_CAP_DR1) {
        d.codec_ctx->flags |= CODEC_FLAG_EMU_EDGE;
    }
    //AVDISCARD_NONREF, AVDISCARD_ALL
    //set thread
    if (d.threads == -1)
        d.threads = qMax(0, QThread::idealThreadCount());
    if (d.threads > 0)
        d.codec_ctx->thread_count = d.threads;
    if (d.threads > 1)
        d.codec_ctx->thread_type = d.thread_slice ? FF_THREAD_SLICE : FF_THREAD_FRAME;

    //set dict used by avcodec_open2(). see ffplay
    // AVDictionary *opts;
    int ret = avcodec_open2(d.codec_ctx, codec, NULL);
    if (ret < 0) {
        qWarning("open video codec failed: %s", av_err2str(ret));
        return false;
    }
    d.is_open = true;
    return true;
}

bool AVDecoder::close()
{
    if (!isOpen()) {
        return true;
    }
    DPTR_D(AVDecoder);
    d.is_open = false;
    // TODO: reset config?
    if (!d.codec_ctx) {
        qWarning("FFmpeg codec context not ready");
        return false;
    }
    int ret = avcodec_close(d.codec_ctx);
    if (ret < 0) {
        qWarning("failed to close decoder: %s", av_err2str(ret));
        return false;
    }
    return true;
}

bool AVDecoder::isOpen() const
{
    return d_func().is_open;
}

void AVDecoder::flush()
{
    if (!isAvailable())
        return;
    if (!isOpen())
        return;
    avcodec_flush_buffers(d_func().codec_ctx);
}

/*
 * do nothing if equal
 * close the old one. the codec context can not be shared in more than 1 decoder.
 */
void AVDecoder::setCodecContext(AVCodecContext *codecCtx)
{
    DPTR_D(AVDecoder);
    if (d.codec_ctx == codecCtx)
        return;
    close(); //
    d.is_open = false;
    d.codec_ctx = codecCtx;
}

//TODO: reset other parameters?
AVCodecContext* AVDecoder::codecContext() const
{
    return d_func().codec_ctx;
}

void AVDecoder::setLowResolution(int lowres)
{
    d_func().low_resolution = lowres;
}

void AVDecoder::setCodecName(const QString &name)
{
    d_func().name = name;
}

QString AVDecoder::codecName() const
{
    DPTR_D(const AVDecoder);
    return d.name.isEmpty() ? d.codec_ctx->codec->name : d.name;
}

int AVDecoder::lowResolution() const
{
    return d_func().low_resolution;
}

void AVDecoder::setDecodeThreads(int threads)
{
    DPTR_D(AVDecoder);
    d.threads = threads;// threads >= 0 ? threads : qMax(0, QThread::idealThreadCount());
    d.threads = qMax(d.threads, 0); //check max?
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
#if DECODER_DONE //currently codec context is opened in demuxer, we must setup context before open
    if (d.threads > 0)
        d.codec_ctx->thread_count = d.threads;
    if (d.threads > 1)
        d.codec_ctx->thread_type = d.thread_slice ? FF_THREAD_SLICE : FF_THREAD_FRAME;
    //else
      //  d.codec_ctx->thread_type = 0; //FF_THREAD_FRAME:1, FF_THREAD_SLICE:2

    //? !CODEC_ID_H264 && !CODEC_ID_VP8
    d.codec_ctx->lowres = d.low_resolution;
#endif //DECODER_DONE
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
