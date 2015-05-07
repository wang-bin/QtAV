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

#ifndef QTAV_VIDEOENCODER_H
#define QTAV_VIDEOENCODER_H

#include <QtAV/AVEncoder.h>
#include <QtAV/FactoryDefine.h>
#include <QtAV/VideoFrame.h>

class QSize;
namespace QtAV {

typedef int VideoEncoderId;
class VideoEncoder;
FACTORY_DECLARE(VideoEncoder)

class VideoEncoderPrivate;
class Q_AV_EXPORT VideoEncoder : public AVEncoder
{
    Q_OBJECT
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(qreal frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged)
    Q_DISABLE_COPY(VideoEncoder)
    DPTR_DECLARE_PRIVATE(VideoEncoder)
public:
    static VideoEncoder* create(VideoEncoderId id);
    /*!
     * \brief create
     * create a decoder from registered name
     * \param name can be "FFmpeg"
     * \return 0 if not registered
     */
    static VideoEncoder* create(const QString& name);
    virtual VideoEncoderId id() const = 0;
    QString name() const Q_DECL_OVERRIDE; //name from factory
    virtual bool encode(const VideoFrame& frame) = 0;
    void setWidth(int value);
    int width() const;
    void setHeight(int value);
    int height() const;
    void setFrameRate(qreal value);
    qreal frameRate() const;
    // pixel format...
Q_SIGNALS:
    void widthChanged();
    void heightChanged();
    void frameRateChanged();
protected:
    VideoEncoder(VideoEncoderPrivate& d);
private:
    VideoEncoder();
};

} //namespace QtAV
#endif // QTAV_VIDEOENCODER_H
