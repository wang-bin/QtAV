/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QAV_QPAINTERRENDERER_H
#define QAV_QPAINTERRENDERER_H

#include <QtAV/VideoRenderer.h>
#include <QtGui/QImage>
//TODO: not abstract.
namespace QtAV {

class QPainterRendererPrivate;
class Q_AV_EXPORT QPainterRenderer : public VideoRenderer
{
    DPTR_DECLARE_PRIVATE(QPainterRenderer)
public:
    QPainterRenderer();
    bool isSupported(VideoFormat::PixelFormat pixfmt) const Q_DECL_OVERRIDE;
protected:
    bool preparePixmap(const VideoFrame& frame);
    void drawBackground() Q_DECL_OVERRIDE;
    //draw the current frame using the current paint engine. called by paintEvent()
    void drawFrame() Q_DECL_OVERRIDE;

    QPainterRenderer(QPainterRendererPrivate& d);
};

} //namespace QtAV
#endif // QAV_QPAINTERRENDERER_H
