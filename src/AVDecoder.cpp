/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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
#include <QtAV/private/AVDecoder_p.h>
#include <QtAV/version.h>

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
    setCodecContext(0);
}

QString AVDecoder::name() const
{
    return "avcodec";
}

QString AVDecoder::description() const
{
    return QString("FFmpeg/Libav avcodec %1.%2.%3").arg(QTAV_VERSION_MAJOR(avcodec_version())).arg(QTAV_VERSION_MINOR(avcodec_version())).arg(QTAV_VERSION_PATCH(avcodec_version()));
}

bool AVDecoder::open()
{
    DPTR_D(AVDecoder);
    if (!d.codec_ctx) {
        qWarning("FFmpeg codec context not ready");
        return false;
    }
    AVCodec *codec = 0;
    if (!d.codec_name.isEmpty()) {
        codec = avcodec_find_decoder_by_name(d.codec_name.toUtf8().constData());
    } else {
        codec = avcodec_find_decoder(d.codec_ctx->codec_id);
    }
    if (!codec) {
        if (d.codec_name.isEmpty()) {
            qWarning("No codec could be found with id %d", d.codec_ctx->codec_id);
        } else {
            qWarning("No codec could be found with name %s", d.codec_name.toUtf8().constData());
        }
        return false;
    }

    //setup video codec context
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
    //set thread

    //d.codec_ctx->strict_std_compliance = FF_COMPLIANCE_STRICT;
    //d.codec_ctx->slice_flags |= SLICE_FLAG_ALLOW_FIELD;
// lavfilter
    //d.codec_ctx->slice_flags |= SLICE_FLAG_ALLOW_FIELD; //lavfilter
    //d.codec_ctx->strict_std_compliance = FF_COMPLIANCE_STRICT;

//from vlc
    //HAVE_AVCODEC_MT macro?
    if (d.threads == -1)
        d.threads = qMax(0, QThread::idealThreadCount());
    if (d.threads > 0)
        d.codec_ctx->thread_count = d.threads;
    d.codec_ctx->thread_safe_callbacks = true;
    switch (d.codec_ctx->codec_id) {
        case QTAV_CODEC_ID(MPEG4):
        case QTAV_CODEC_ID(H263):
            d.codec_ctx->thread_type = 0;
            break;
        case QTAV_CODEC_ID(MPEG1VIDEO):
        case QTAV_CODEC_ID(MPEG2VIDEO):
            d.codec_ctx->thread_type &= ~FF_THREAD_SLICE;
            /* fall through */
# if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 1, 0))
        case QTAV_CODEC_ID(H264):
        case QTAV_CODEC_ID(VC1):
        case QTAV_CODEC_ID(WMV3):
            d.codec_ctx->thread_type &= ~FF_THREAD_FRAME;
# endif
        default:
            break;
    }
/*
    if (d.codec_ctx->thread_type & FF_THREAD_FRAME)
        p_dec->i_extra_picture_buffers = 2 * p_sys->p_context->thread_count;
*/
    // hwa extra init can be here
    if (!d.open()) {
        d.close();
        return false;
    }
    //set dict used by avcodec_open2(). see ffplay
    // AVDictionary *opts;
    int ret = avcodec_open2(d.codec_ctx, codec, d.options.isEmpty() ? NULL : &d.dict);
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
    // hwa extra finalize can be here
    d.close();
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
    d_func().codec_name = name;
}

QString AVDecoder::codecName() const
{
    DPTR_D(const AVDecoder);
    if (!d.codec_name.isEmpty())
        return d.codec_name;
    if (d.codec_ctx)
        return d.codec_ctx->codec->name;
    return "";
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

int AVDecoder::undecodedSize() const
{
    return d_func().undecoded_size;
}

void AVDecoder::setOptions(const QVariantHash &dict)
{
    DPTR_D(AVDecoder);
    d.options = dict;
    if (d.dict) {
        av_dict_free(&d.dict);
        d.dict = 0; //aready 0 in av_free
    }
    if (dict.isEmpty())
        return;
    QVariantHash avcodec_dict(dict);
    if (dict.contains("avcodec"))
        avcodec_dict = dict.value("avcodec").toHash();
    // workaround for VideoDecoderFFmpeg. now it does not call av_opt_set_xxx, so set here in dict
    if (dict.contains("FFmpeg"))
        avcodec_dict.unite(dict.value("FFmpeg").toHash());
    QHashIterator<QString, QVariant> i(avcodec_dict);
    while (i.hasNext()) {
        i.next();
        switch (i.value().type()) {
        case QVariant::Hash: // for example "vaapi": {...}
            continue;
        case QVariant::Bool:
            // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
            av_dict_set(&d.dict, i.key().toLower().toUtf8().constData(), QByteArray::number(i.value().toBool()), 0);
            break;
        default:
            // avcodec key and value are in lower case
            av_dict_set(&d.dict, i.key().toLower().toUtf8().constData(), i.value().toByteArray().toLower().constData(), 0);
            break;
        }
        qDebug("avcodec option: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
    }
    if (name() == "avcodec")
        return;
    QVariantHash property_dict(dict.value(name()).toHash());
    if (property_dict.isEmpty())
        property_dict = dict.value(name().toLower()).toHash();
    if (property_dict.isEmpty())
        return;
    i = QHashIterator<QString, QVariant>(property_dict);
    while (i.hasNext()) {
        i.next();
        if (i.value().type() == QVariant::Hash) // for example "vaapi": {...}
            continue;
        setProperty(i.key().toUtf8().constData(), i.value());
        qDebug("decoder property: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
    }
}

QVariantHash AVDecoder::options() const
{
    return d_func().options;
}

} //namespace QtAV
