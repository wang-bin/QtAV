/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#include <QtAV/AVEncoder.h>
#include <QtAV/private/AVEncoder_p.h>
#include <QtAV/version.h>
#include "utils/internal.h"
#include "utils/Logger.h"

namespace QtAV {

AVEncoder::AVEncoder(AVEncoderPrivate &d)
    :DPTR_INIT(&d)
{
}

AVEncoder::~AVEncoder()
{
    close();
}

QString AVEncoder::description() const
{
    return QString();
}

void AVEncoder::setCodecName(const QString &name)
{
    DPTR_D(AVEncoder);
    if (d.codec_name == name)
        return;
    d.codec_name = name;
    Q_EMIT codecNameChanged();
}

QString AVEncoder::codecName() const
{
    DPTR_D(const AVEncoder);
    if (!d.codec_name.isEmpty())
        return d.codec_name;
    if (d.avctx)
        return QLatin1String(avcodec_get_name(d.avctx->codec_id));
    return QString();
}

void AVEncoder::setBitRate(int value)
{
    DPTR_D(AVEncoder);
    if (d.bit_rate == value)
        return;
    d.bit_rate = value;
    emit bitRateChanged();
}

int AVEncoder::bitRate() const
{
    return d_func().bit_rate;
}

AVEncoder::TimestampMode AVEncoder::timestampMode() const
{
    return TimestampMode(d_func().timestamp_mode);
}

void AVEncoder::setTimestampMode(TimestampMode value)
{
    DPTR_D(AVEncoder);
    if (d.timestamp_mode == (int)value)
        return;
    d.timestamp_mode = (int)value;
    Q_EMIT timestampModeChanged(value);
}

bool AVEncoder::open()
{
    DPTR_D(AVEncoder);
    if (d.avctx) {
        d.applyOptionsForDict();
    }
    if (!d.open()) {
        d.close();
        return false;
    }
    d.is_open = true;
    return true;
}

bool AVEncoder::close()
{
    if (!isOpen()) {
        return true;
    }
    DPTR_D(AVEncoder);
    d.is_open = false;
    // hwa extra finalize can be here
    d.close();
    return true;
}

bool AVEncoder::isOpen() const
{
    return d_func().is_open;
}

void AVEncoder::flush()
{
    if (!isOpen())
        return;
    if (d_func().avctx)
        avcodec_flush_buffers(d_func().avctx);
}

Packet AVEncoder::encoded() const
{
    return d_func().packet;
}

void* AVEncoder::codecContext() const
{
    return d_func().avctx;
}

void AVEncoder::copyAVCodecContext(void* ctx)
{
    if (!ctx)
        return;
    DPTR_D(AVEncoder);
    AVCodecContext* c = static_cast<AVCodecContext*>(ctx);
    if (d.avctx) {
        // dest should be avcodec_alloc_context3(NULL)
        AV_ENSURE_OK(avcodec_copy_context(d.avctx, c));
        d.is_open = false;
        return;
    }
}

void AVEncoder::setOptions(const QVariantHash &dict)
{
    DPTR_D(AVEncoder);
    d.options = dict;
    // if dict is empty, can not return here, default options will be set for AVCodecContext
    // apply to AVCodecContext
    d.applyOptionsForContext();
    /* set AVEncoder meta properties.
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

QVariantHash AVEncoder::options() const
{
    return d_func().options;
}

void AVEncoderPrivate::applyOptionsForDict()
{
    if (dict) {
        av_dict_free(&dict);
        dict = 0; //aready 0 in av_free
    }
    if (options.isEmpty())
        return;
    qDebug("set AVCodecContext dict:");
    // TODO: use QVariantMap only
    if (!options.contains(QStringLiteral("avcodec")))
        return;
    // workaround for VideoDecoderFFmpeg. now it does not call av_opt_set_xxx, so set here in dict
    // TODO: wrong if opt is empty
    Internal::setOptionsToDict(options.value(QStringLiteral("avcodec")), &dict);
}

void AVEncoderPrivate::applyOptionsForContext()
{
    if (!avctx)
        return;
    if (options.isEmpty()) {
        // av_opt_set_defaults(avctx); //can't set default values! result maybe unexpected
        return;
    }
    // TODO: use QVariantMap only
    if (!options.contains(QStringLiteral("avcodec")))
        return;
    // workaround for VideoDecoderFFmpeg. now it does not call av_opt_set_xxx, so set here in dict
    // TODO: wrong if opt is empty
    Internal::setOptionsToFFmpegObj(options.value(QStringLiteral("avcodec")), avctx);
}

} //namespace QtAV
