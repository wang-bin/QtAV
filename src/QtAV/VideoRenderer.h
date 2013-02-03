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

#ifndef QAV_VIDEORENDERER_H
#define QAV_VIDEORENDERER_H

#include <QtCore/QByteArray>
#include <QtCore/QSize>
#include <QtAV/AVOutput.h>

struct AVCodecContext;
struct AVFrame;
class QImage;
class QObject;
namespace QtAV {

class VideoRendererPrivate;
class Q_EXPORT VideoRenderer : public AVOutput
{
    DPTR_DECLARE_PRIVATE(VideoRenderer)
public:
    VideoRenderer();
    virtual ~VideoRenderer() = 0;
    //for testing performance
    void scaleInQt(bool q);
    bool scaleInQt() const;
    //the size of image (QByteArray) that decoded
    void setSourceSize(const QSize& s); //private?
    void setSourceSize(int width, int height); //private?
    QSize lastSize() const;
    int lastWidth() const;
    int lastHeight() const;
    //TODO: unregister
    virtual bool open();
    virtual bool close();
    virtual QImage currentFrameImage() const = 0; //const QImage& const?
    void resizeVideo(const QSize& size);
    void resizeVideo(int width, int height);
    QSize videoSize() const;
    int videoWidth() const;
    int videoHeight() const;

protected:
    VideoRenderer(VideoRendererPrivate &d);
};

} //namespace QtAV
#endif // QAV_VIDEORENDERER_H
