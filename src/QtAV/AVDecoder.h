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

#ifndef QAV_DECODER_H
#define QAV_DECODER_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/AVError.h>
#include <QtAV/Packet.h>
#include <QtCore/QVariant>
#include <QtCore/QObject>

class QByteArray;
struct AVCodecContext;

namespace QtAV {

class AVDecoderPrivate;
class Q_AV_EXPORT AVDecoder : public QObject
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(AVDecoder)
    //Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
public:
    virtual ~AVDecoder();
    virtual QString name() const;
    virtual QString description() const;
    /*!
     * default is open FFmpeg codec context
     * codec config must be done before open
     * NOTE: open() and close() are not thread safe. You'd better call them in the same thread.
     */
    virtual bool open();
    virtual bool close();
    bool isOpen() const;
    virtual void flush();
    void setCodecContext(AVCodecContext* codecCtx); //protected
    AVCodecContext* codecContext() const;
    /*not available if AVCodecContext == 0*/
    bool isAvailable() const;
    // TODO: remove
    virtual bool prepare(); //if resampler or image converter set, call it
    QTAV_DEPRECATED virtual bool decode(const QByteArray& encoded) = 0;
    virtual bool decode(const Packet& packet) = 0;
    int undecodedSize() const; //TODO: remove. always decode whole input data completely

    // avcodec_open2
    /*!
     * \brief setOptions
     * 1. Set options for AVCodecContext. if contains key "avcodec", use it's value as a hash to set. a value of hash type is ignored.
     * libav's AVDictionary. we can ignore the flags used in av_dict_xxx because we can use hash api.
     * In addition, av_dict is slow.
     * empty value does nothing to current context if it is open, but will change AVDictionary options to null in next open.
     * AVDictionary is used in avcodec_open2() and will not change unless user call setOptions().
     * 2. Set properties for AVDecoder. Use AVDecoder::name() or lower case as a key to set properties. If key not found, assume key is "avcodec"
     * \param dict
     * example:
     *  "avcodec": {"vismv":"pf"}, "vaapi":{"display":"DRM"}
     * equals
     *  "vismv":"pf", "vaapi":{"display":"DRM"}
     */
    void setOptions(const QVariantHash &dict);
    QVariantHash options() const;

Q_SIGNALS:
    void error(const QtAV::AVError& e); //explictly use QtAV::AVError in connection for Qt4 syntax
    void descriptionChanged();
protected:
    AVDecoder(AVDecoderPrivate& d);
    DPTR_DECLARE(AVDecoder)
    // force a codec. only used by avcodec sw decoders
    void setCodecName(const QString& name);
    QString codecName() const;
    virtual void codecNameChanged() {}
private:
    Q_DISABLE_COPY(AVDecoder)
    AVDecoder(); // base class, not direct create. only final class has is enough
};

} //namespace QtAV
#endif // QAV_DECODER_H
