/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

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

static AVCodec* get_codec(const QString &name, const QString& hwa, AVCodecID cid)
{
    QString fullname(name);
    if (name.isEmpty()) {
        if (hwa.isEmpty())
            return avcodec_find_decoder(cid);
        fullname = QString("%1_%2").arg(avcodec_get_name(cid)).arg(hwa);
    }
    AVCodec *codec = avcodec_find_decoder_by_name(fullname.toUtf8().constData());
    if (codec)
        return codec;
    const AVCodecDescriptor* cd = avcodec_descriptor_get_by_name(fullname.toUtf8().constData());
    if (cd)
        return avcodec_find_decoder(cd->id);
    return NULL;
}

AVDecoder::AVDecoder(AVDecoderPrivate &d)
    :DPTR_INIT(&d)
{
#if !AVCODEC_STATIC_REGISTER
    avcodec_register_all(); // avcodec_find_decoder will always be used
#endif
}

AVDecoder::~AVDecoder()
{
    setCodecContext(0); // FIXME:  will call virtual
}

QString AVDecoder::name() const
{
    return QString();
}

QString AVDecoder::description() const
{
    return QString();
}

bool AVDecoder::open()
{
    DPTR_D(AVDecoder);
    // codec_ctx can't be null for none-ffmpeg based decoders because we may use it's properties in those decoders
    if (!d.codec_ctx) {
        qWarning("FFmpeg codec context not ready");
        return false;
    }
    const QString hwa = property("hwaccel").toString();
    AVCodec* codec = get_codec(codecName(), hwa, d.codec_ctx->codec_id);
    if (!codec) { // TODO: can be null for none-ffmpeg based decoders
        QString es(tr("No codec could be found for '%1'"));
        if (d.codec_name.isEmpty()) {
            es = es.arg(QLatin1String(avcodec_get_name(d.codec_ctx->codec_id)));
            if (!hwa.isEmpty())
                es.append('_').append(hwa);
        } else {
            es = es.arg(d.codec_name);
        }
        qWarning() << es;
        AVError::ErrorCode ec(AVError::CodecError);
        switch (d.codec_ctx->codec_type) {
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
        Q_EMIT error(AVError(ec, es));
        return false;
    }
    // hwa extra init can be here
    if (!d.open()) {
        d.close();
        return false;
    }
    // CODEC_FLAG_OUTPUT_CORRUPT, CODEC_FLAG2_SHOW_ALL?
    // TODO: skip for none-ffmpeg based decoders
    d.applyOptionsForDict();
    av_opt_set_int(d.codec_ctx, "refcounted_frames", d.enableFrameRef(), 0); // why dict may have no effect?
    // TODO: only open for ff decoders
    //av_dict_set(&d.dict, "lowres", "1", 0);
    // dict is used for a specified AVCodec options (priv_class), av_opt_set_xxx(avctx) is only for avctx
    AV_ENSURE_OK(avcodec_open2(d.codec_ctx, codec, d.options.isEmpty() ? NULL : &d.dict), false);
    d.is_open = true;
    static const char* thread_name[] = { "Single", "Frame", "Slice"};
    qDebug("%s thread type: %s, count: %d", metaObject()->className(), thread_name[d.codec_ctx->active_thread_type], d.codec_ctx->thread_count);
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
    flush();
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
    // avcodec_alloc_context3(codec) equals to avcodec_alloc_context3(NULL) + avcodec_get_context_defaults3(codec), codec specified private data is initialized
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
    if (name() == QLatin1String("avcodec"))
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
    // enable ref if possible
    av_dict_set(&dict, "refcounted_frames", enableFrameRef() ? "1" : "0", 0);
    if (options.isEmpty())
        return;
    // TODO: use QVariantMap only
    if (!options.contains(QStringLiteral("avcodec")))
        return;
     qDebug("set AVCodecContext dict:");
    // workaround for VideoDecoderFFmpeg. now it does not call av_opt_set_xxx, so set here in dict
    // TODO: wrong if opt is empty
    Internal::setOptionsToDict(options.value(QStringLiteral("avcodec")), &dict);
}

void AVDecoderPrivate::applyOptionsForContext()
{
    if (!codec_ctx)
        return;
    if (options.isEmpty()) {
        // av_opt_set_defaults(codec_ctx); //can't set default values! result maybe unexpected
        return;
    }

    if (!options.contains(QStringLiteral("avcodec")))
        return;
    // workaround for VideoDecoderFFmpeg. now it does not call av_opt_set_xxx, so set here in dict
    // TODO: wrong if opt is empty
    Internal::setOptionsToFFmpegObj(options.value(QStringLiteral("avcodec")), codec_ctx);
}
} //namespace QtAV
