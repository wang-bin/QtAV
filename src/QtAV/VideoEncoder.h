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

#ifndef QTAV_VIDEOENCODER_H
#define QTAV_VIDEOENCODER_H

#include <QtAV/AVEncoder.h>
#include <QtAV/VideoFrame.h>
#include <QtCore/QStringList>

namespace QtAV {
typedef int VideoEncoderId;
class VideoEncoderPrivate;
class Q_AV_EXPORT VideoEncoder : public AVEncoder
{
    Q_OBJECT
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(qreal frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged)
    Q_PROPERTY(QtAV::VideoFormat::PixelFormat pixelFormat READ pixelFormat WRITE setPixelFormat NOTIFY pixelFormatChanged)
    Q_DISABLE_COPY(VideoEncoder)
    DPTR_DECLARE_PRIVATE(VideoEncoder)
public:
    static QStringList supportedCodecs();
    static VideoEncoder* create(VideoEncoderId id);
    /*!
     * \brief create
     * create an encoder from registered names
     * \param name can be "FFmpeg". FFmpeg encoder will be created for empty name
     * \return 0 if not registered
     */
    static VideoEncoder* create(const char* name = "FFmpeg");
    virtual VideoEncoderId id() const = 0;
    QString name() const Q_DECL_OVERRIDE; //name from factory
    /*!
     * \brief encode
     * encode a video frame to a Packet
     * \param frame pass an invalid frame to get delayed frames
     * \return
     */
    virtual bool encode(const VideoFrame& frame = VideoFrame()) = 0;
    /// output parameters
    /*!
     * \brief setWidth
     * set the encoded video width. The same as input frame size if value <= 0
     */
    void setWidth(int value);
    int width() const;
    void setHeight(int value);
    int height() const;
    /// TODO: check avctx->supported_framerates. use frame_rate_used
    /*!
     * \brief setFrameRate
     * If frame rate is not set, frameRate() returns -1, but internally the default frame rate 25 will be used
     * \param value
     */
    void setFrameRate(qreal value);
    qreal frameRate() const;
    static qreal defaultFrameRate() { return 25;}
    /*!
     * \brief setPixelFormat
     * If not set or set to an unsupported format, a supported format will be used and pixelFormat() will be that format after open()
     */
    void setPixelFormat(const VideoFormat::PixelFormat format);
    /*!
     * \brief pixelFormat
     * \return user requested format. may be different with actually used format
     */
    VideoFormat::PixelFormat pixelFormat() const;
    // TODO: supportedPixelFormats() const;
Q_SIGNALS:
    void widthChanged();
    void heightChanged();
    void frameRateChanged();
    void pixelFormatChanged();

public:
    template<class C> static bool Register(VideoEncoderId id, const char* name) { return Register(id, create<C>, name);}
    /*!
     * \brief next
     * \param id NULL to get the first id address
     * \return address of id or NULL if not found/end
     */
    static VideoEncoderId* next(VideoEncoderId* id = 0);
    static const char* name(VideoEncoderId id);
    static VideoEncoderId id(const char* name);
private:
    template<class C> static VideoEncoder* create() { return new C();}
    typedef VideoEncoder* (*VideoEncoderCreator)();
    static bool Register(VideoEncoderId id, VideoEncoderCreator, const char *name);
protected:
    VideoEncoder(VideoEncoderPrivate& d);
private:
    VideoEncoder();
};

} //namespace QtAV
#endif // QTAV_VIDEOENCODER_H
