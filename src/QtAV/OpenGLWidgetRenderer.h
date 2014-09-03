/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_OPENGLWIDGETRENDERER_H
#define QTAV_OPENGLWIDGETRENDERER_H

#include <QtWidgets/QOpenGLWidget>
#include <QtAV/VideoRenderer.h>

namespace QtAV {

class OpenGLWidgetRendererPrivate;
class Q_AV_EXPORT OpenGLWidgetRenderer : public QOpenGLWidget, public VideoRenderer
{
    DPTR_DECLARE_PRIVATE(OpenGLWidgetRenderer)
public:
    explicit OpenGLWidgetRenderer(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~OpenGLWidgetRenderer();
    virtual VideoRendererId id() const Q_DECL_OVERRIDE;
    virtual bool isSupported(VideoFormat::PixelFormat pixfmt) const Q_DECL_OVERRIDE;
    virtual QWidget* widget() Q_DECL_OVERRIDE { return this; }

protected:
    virtual bool receiveFrame(const VideoFrame& frame) Q_DECL_OVERRIDE;
    virtual bool needUpdateBackground() const Q_DECL_OVERRIDE;
    //called in paintEvent before drawFrame() when required
    virtual void drawBackground() Q_DECL_OVERRIDE;
    //draw the current frame using the current paint engine. called by paintEvent()
    virtual void drawFrame() Q_DECL_OVERRIDE;
    virtual void initializeGL() Q_DECL_OVERRIDE;
    virtual void paintGL() Q_DECL_OVERRIDE;
    virtual void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *);
private:
    virtual void onSetOutAspectRatioMode(OutAspectRatioMode mode) Q_DECL_OVERRIDE;
    virtual void onSetOutAspectRatio(qreal ratio) Q_DECL_OVERRIDE;
    virtual bool onSetBrightness(qreal b) Q_DECL_OVERRIDE;
    virtual bool onSetContrast(qreal c) Q_DECL_OVERRIDE;
    virtual bool onSetHue(qreal h) Q_DECL_OVERRIDE;
    virtual bool onSetSaturation(qreal s) Q_DECL_OVERRIDE;
};
typedef OpenGLWidgetRenderer VideoRendererOpenGLWidget;

} //namespace QtAV

#endif // QTAV_OPENGLWIDGETRENDERER_H
