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

#ifndef QAV_ENCODER_H
#define QAV_ENCODER_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/AVError.h>
#include <QtAV/Packet.h>
#include <QtCore/QVariant>
#include <QtCore/QObject>

namespace QtAV {
class AVEncoderPrivate;
class Q_AV_EXPORT AVEncoder : public QObject
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(AVEncoder)
    Q_PROPERTY(QString codecName READ codecName WRITE setCodecName NOTIFY codecNameChanged)
public:
    virtual ~AVEncoder();
    virtual QString name() const = 0;
    virtual QString description() const;
    /*!
     * \brief setCodecName
     * An encoder can support more than 1 codec.
     */
    void setCodecName(const QString& name);
    QString codecName() const;
    bool open();
    bool close();
    bool isOpen() const;
    virtual void flush();
    Packet encoded() const;

    /*!
     * used by ff muxer. Be sure all parameters are set. (open?)
     */
    virtual void copyAVCodecContext(void* ctx);
    void* codecContext() const;
    // avcodec_open2
    /*!
     * \brief setOptions
     * 1. Set options for AVCodecContext if it's a ffmpeg codec. if contains key "avcodec", use it's value as a hash to set. a value of hash type is ignored.
     * libav's AVDictionary. we can ignore the flags used in av_dict_xxx because we can use hash api.
     * In addition, av_dict is slow.
     * empty value does nothing to current context if it is open, but will change AVDictionary options to null in next open.
     * AVDictionary is used in avcodec_open2() and will not change unless user call setOptions().
     * 2. Set properties for AVEncoder. Use AVEncoder::name() or lower case as a key to set properties. If key not found, assume key is "avcodec"
     */
    void setOptions(const QVariantHash &dict);
    QVariantHash options() const;

Q_SIGNALS:
    void error(const QtAV::AVError& e); //explictly use QtAV::AVError in connection for Qt4 syntax
    void codecNameChanged();
protected:
    AVEncoder(AVEncoderPrivate& d);
    DPTR_DECLARE(AVEncoder)
private:
    Q_DISABLE_COPY(AVEncoder)
    AVEncoder(); // base class, not direct create. only final class has is enough
};
} //namespace QtAV
#endif // QAV_ENCODER_H
