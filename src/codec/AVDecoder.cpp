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

#include <QtAV/AVDecoder.h>
#include <QtAV/private/AVDecoder_p.h>
#include <QtAV/version.h>
#include "utils/internal.h"
#include "utils/Logger.h"

namespace QtAV {

AVDecoder::AVDecoder(AVDecoderPrivate &d)
    :DPTR_INIT(&d)
{
    avcodec_register_all(); // avcodec_find_decoder will always be used
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
    return QString();
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
        QString es(tr("No codec could be found for '%1'"));
        if (d.codec_name.isEmpty()) {
            es = es.arg(avcodec_get_name(d.codec_ctx->codec_id));
        } else {
            es = es.arg(d.codec_name);
        }
        qWarning() << es;
        AVError::ErrorCode ec(AVError::CodecError);
        switch (d.codec_ctx->coder_type) {
        case AVMEDIA_TYPE_VIDEO:
            ec = AVError::VideoCodecNotFound;
            break;
        case AVMEDIA_TYPE_AUDIO:
            ec = AVError::AudioCodecNotFound;
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            ec = AVError::SubtitleCodecNotFound;
        default:
            break;
        }
        emit error(AVError(ec, es));
        return false;
    }    
    // hwa extra init can be here
    if (!d.open()) {
        d.close();
        return false;
    }
    d.applyOptionsForDict();
    AV_ENSURE_OK(avcodec_open2(d.codec_ctx, codec, d.options.isEmpty() ? NULL : &d.dict), false);
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
    if (d.codec_ctx) {
        AV_ENSURE_OK(avcodec_close(d.codec_ctx), false);
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
void AVDecoder::setCodecContext(void *codecCtx)
{
    DPTR_D(AVDecoder);
    AVCodecContext *ctx = (AVCodecContext*)codecCtx;
    if (d.codec_ctx == ctx)
        return;
    if (isOpen()) {
        qWarning("Can not copy codec properties when it's open");
        close(); //
    }
    d.is_open = false;
    if (!ctx) {
        avcodec_free_context(&d.codec_ctx);
        d.codec_ctx = 0;
        return;
    }
    if (!d.codec_ctx)
        d.codec_ctx = avcodec_alloc_context3(NULL);
    if (!d.codec_ctx) {
        qWarning("avcodec_alloc_context3 failed");
        return;
    }
    AV_ENSURE_OK(avcodec_copy_context(d.codec_ctx, ctx));
}

//TODO: reset other parameters?
void* AVDecoder::codecContext() const
{
    return d_func().codec_ctx;
}

void AVDecoder::setCodecName(const QString &name)
{
    DPTR_D(AVDecoder);
    if (d.codec_name == name)
        return;
    d.codec_name = name;
    Q_EMIT codecNameChanged();
}

QString AVDecoder::codecName() const
{
    DPTR_D(const AVDecoder);
    return d.codec_name;
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
    return true;
}

bool AVDecoder::decode(const QByteArray &encoded)
{
    Q_UNUSED(encoded);
    return true;
}

int AVDecoder::undecodedSize() const
{
    return d_func().undecoded_size;
}

void AVDecoder::setOptions(const QVariantHash &dict)
{
    DPTR_D(AVDecoder);
    d.options = dict;
    // if dict is empty, can not return here, default options will be set for AVCodecContext
    // apply to AVCodecContext
    d.applyOptionsForContext();
    /* set AVDecoder meta properties.
     * we do not check whether the property exists thus we can set dynamic properties.
     */
    if (dict.isEmpty())
        return;
    if (name() == "avcodec")
        return;
    QVariant opt(dict);
    if (dict.contains(name()))
        opt = dict.value(name());
    else if (dict.contains(name().toLower()))
        opt = dict.value(name().toLower());
    Internal::setOptionsForQObject(opt, this);
}

QVariantHash AVDecoder::options() const
{
    return d_func().options;
}

void AVDecoderPrivate::applyOptionsForDict()
{
    if (dict) {
        av_dict_free(&dict);
        dict = 0; //aready 0 in av_free
    }
    if (options.isEmpty())
        return;
    // TODO: use QVariantMap only
    if (!options.contains("avcodec"))
        return;
     qDebug("set AVCodecContext dict:");
    // workaround for VideoDecoderFFmpeg. now it does not call av_opt_set_xxx, so set here in dict
    // TODO: wrong if opt is empty
    Internal::setOptionsToDict(options.value("avcodec"), &dict);
}

void AVDecoderPrivate::applyOptionsForContext()
{
    if (!codec_ctx)
        return;
    if (options.isEmpty()) {
        // av_opt_set_defaults(codec_ctx); //can't set default values! result maybe unexpected
        return;
    }
    // TODO: use QVariantMap only
    if (!options.contains("avcodec"))
        return;
    // workaround for VideoDecoderFFmpeg. now it does not call av_opt_set_xxx, so set here in dict
    // TODO: wrong if opt is empty
    Internal::setOptionsToFFmpegObj(options.value("avcodec"), codec_ctx);
}

} //namespace QtAV
