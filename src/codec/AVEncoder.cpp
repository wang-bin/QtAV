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

#include <QtAV/AVEncoder.h>
#include <QtAV/private/AVEncoder_p.h>
#include <QtAV/version.h>
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
    emit codecNameChanged();
}

QString AVEncoder::codecName() const
{
    DPTR_D(const AVEncoder);
    if (!d.codec_name.isEmpty())
        return d.codec_name;
    if (d.avctx)
        return avcodec_get_name(d.avctx->codec_id);
    return "";
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
    if (name() == "avcodec")
        return;
    QVariant opt;
    if (dict.contains(name()))
        opt = dict.value(name());
    else if (dict.contains(name().toLower()))
        opt = dict.value(name().toLower());
    else
        return; // TODO: set property if no name() key found?
    if (opt.type() == QVariant::Hash) {
        QVariantHash property_dict(opt.toHash());
        if (property_dict.isEmpty())
            return;
        QHashIterator<QString, QVariant> i(property_dict);
        while (i.hasNext()) {
            i.next();
            if (i.value().type() == QVariant::Hash) // for example "vaapi": {...}
                continue;
            setProperty(i.key().toUtf8().constData(), i.value());
            qDebug("decoder meta property: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        }
    } else if (opt.type() == QVariant::Map) {
        QVariantMap property_dict(opt.toMap());
        if (property_dict.isEmpty())
            return;
        QMapIterator<QString, QVariant> i(property_dict);
        while (i.hasNext()) {
            i.next();
            if (i.value().type() == QVariant::Map) // for example "vaapi": {...}
                continue;
            setProperty(i.key().toUtf8().constData(), i.value());
            qDebug("decoder meta property: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        }
    }
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
    // TODO: use QVariantMap only
    QVariant opt;
    if (options.contains("avcodec"))
        opt = options.value("avcodec");
    if (opt.type() == QVariant::Hash) {
        QVariantHash avcodec_dict = opt.toHash();
        // workaround for AVEncoder. now it does not call av_opt_set_xxx, so set here in dict
        // TODO: wrong if opt is empty
        //if (dict.contains("FFmpeg"))
        //    avcodec_dict.unite(dict.value("FFmpeg").toHash());
        QHashIterator<QString, QVariant> i(avcodec_dict);
        while (i.hasNext()) {
            i.next();
            const QByteArray key(i.key().toLower().toUtf8());
            switch (i.value().type()) {
            case QVariant::Hash: // for example "vaapi": {...}
                continue;
            case QVariant::Bool:
            case QVariant::Int: {
                // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
                av_dict_set(&dict, key.constData(), QByteArray::number(i.value().toInt()).constData(), 0);
            }
                break;
            case QVariant::ULongLong:
            case QVariant::LongLong: {
                av_dict_set(&dict, key.constData(), QByteArray::number(i.value().toLongLong()).constData(), 0);
            }
                break;
            default:
                // avcodec key and value are in lower case
                av_dict_set(&dict, i.key().toLower().toUtf8().constData(), i.value().toByteArray().toLower().constData(), 0);
                break;
            }
            qDebug("avcodec dict: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        }
    } else if (opt.type() == QVariant::Map) {
        QVariantMap avcodec_dict = opt.toMap();
        //if (dict.contains("FFmpeg"))
        //    avcodec_dict.unite(dict.value("FFmpeg").toMap());
        QMapIterator<QString, QVariant> i(avcodec_dict);
        while (i.hasNext()) {
            i.next();
            const QByteArray key(i.key().toLower().toUtf8());
            switch (i.value().type()) {
            case QVariant::Map: // for example "vaapi": {...}
                continue;
            case QVariant::Bool:
            case QVariant::UInt:
            case QVariant::Int: {
                // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
                av_dict_set(&dict, key.constData(), QByteArray::number(i.value().toInt()), 0);
            }
                break;
            case QVariant::ULongLong:
            case QVariant::LongLong: {
                av_dict_set(&dict, key.constData(), QByteArray::number(i.value().toLongLong()).constData(), 0);
            }
                break;
            default:
                // avcodec key and value are in lower case
                av_dict_set(&dict, i.key().toLower().toUtf8().constData(), i.value().toByteArray().toLower().constData(), 0);
                break;
            }
            qDebug("avcodec dict: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        }
    }
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
    QVariant opt;
    if (options.contains("avcodec"))
        opt = options.value("avcodec");
    if (opt.type() == QVariant::Hash) {
        QVariantHash avcodec_dict = opt.toHash();
        // workaround for AVEncoder. now it does not call av_opt_set_xxx, so set here in dict
        // TODO: wrong if opt is empty
        //if (dict.contains("FFmpeg"))
        //    avcodec_dict.unite(dict.value("FFmpeg").toHash());
        QHashIterator<QString, QVariant> i(avcodec_dict);
        while (i.hasNext()) {
            i.next();
            const QByteArray key(i.key().toLower().toUtf8());
            switch (i.value().type()) {
            case QVariant::Hash: // for example "vaapi": {...}
                continue;
            case QVariant::Bool:
            case QVariant::Int:
                // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
                av_opt_set_int(avctx, key.constData(), i.value().toInt(), 0);
                break;
            case QVariant::ULongLong:
            case QVariant::LongLong:
                av_opt_set_int(avctx, key.constData(), i.value().toLongLong(), 0);
                break;
            default:
                break;
            }
            qDebug("avcodec option: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        }
    } else if (opt.type() == QVariant::Map) {
        QVariantMap avcodec_dict = opt.toMap();
        // workaround for AVEncoder. now it does not call av_opt_set_xxx, so set here in dict
        //if (dict.contains("FFmpeg"))
        //    avcodec_dict.unite(dict.value("FFmpeg").toMap());
        QMapIterator<QString, QVariant> i(avcodec_dict);
        while (i.hasNext()) {
            i.next();
            const QByteArray key(i.key().toLower().toUtf8());
            switch (i.value().type()) {
            case QVariant::Map: // for example "vaapi": {...}
                continue;
            case QVariant::Bool:
            case QVariant::UInt:
            case QVariant::Int:
                // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
                av_opt_set_int(avctx, key.constData(), i.value().toInt(), 0);
                break;
            case QVariant::ULongLong:
            case QVariant::LongLong:
                av_opt_set_int(avctx, key.constData(), i.value().toLongLong(), 0);
                break;
            default:
                break;
            }
            qDebug("avcodec option: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        }
    }
}

} //namespace QtAV
