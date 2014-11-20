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

#ifndef QAV_DECODER_H
#define QAV_DECODER_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/AVError.h>
#include <QtAV/Packet.h>
#include <QtCore/QVariant>
#include <QtCore/QObject>

class QByteArray;
struct AVCodecContext;
struct AVFrame;

namespace QtAV {

class AVDecoderPrivate;
class Q_AV_EXPORT AVDecoder : public QObject
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(AVDecoder)
public:
    AVDecoder();
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
    // force a codec
    void setCodecName(const QString& name);
    QString codecName() const;

    //? low resolution decoding, 0: normal, 1-> 1/2 size, 2->1/4 size
    void setLowResolution(int lowres);
    int lowResolution() const;
    // -1: auto detect by QThread::idealThreadCount(). 0: set by ffmpeg(default)
    void setDecodeThreads(int threads);
    int decodeThreads() const;
    void setThreadSlice(bool s);
    /*not available if AVCodecContext == 0*/
    bool isAvailable() const;
    virtual bool prepare(); //if resampler or image converter set, call it
    QTAV_DEPRECATED virtual bool decode(const QByteArray& encoded) = 0;
    virtual bool decode(const Packet& packet) = 0;
    QByteArray data() const; //decoded data
    int undecodedSize() const;

    // avcodec_open2
    /*!
     * \brief setOptions
     * 1. set options for AVCodecContext. if contains key "avcodec", use it's value as a hash to set. a value of hash type is ignored.
     * libav's AVDictionary. we can ignore the flags used in av_dict_xxx because we can use hash api.
     * In addition, av_dict is slow.
     * empty means default options in ffmpeg
     * 2. set properties for AVDecoder. use the value of key AVDecoder::name() or it's lower case as a hash to set properties.
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
protected:
    AVDecoder(AVDecoderPrivate& d);

    DPTR_DECLARE(AVDecoder)
};

} //namespace QtAV
#endif // QAV_DECODER_H
