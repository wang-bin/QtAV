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

#ifndef QTAV_GLWIDGETRENDERER2_H
#define QTAV_GLWIDGETRENDERER2_H

#include <QtAV/VideoRenderer.h>
#include <QtOpenGL/QGLWidget>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLFunctions>
#else
#include <QtOpenGL/QGLFunctions>
#define QOpenGLFunctions QGLFunctions
#endif

namespace QtAV {

class GLWidgetRenderer2Private;
/*!
 * \brief The GLWidgetRenderer2 class
 * Renderering video frames using GLSL. A more generic high level class OpenGLVideo is used internally.
 * TODO: for Qt5, no QtOpenGL, use QWindow instead.
 */
class Q_AV_EXPORT GLWidgetRenderer2 : public QGLWidget, public VideoRenderer, public QOpenGLFunctions
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GLWidgetRenderer2)
public:
    GLWidgetRenderer2(QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0);
    virtual ~GLWidgetRenderer2();
    virtual VideoRendererId id() const;
    virtual bool isSupported(VideoFormat::PixelFormat pixfmt) const;
    virtual QWidget* widget() { return this; }

protected:
    virtual bool receiveFrame(const VideoFrame& frame);
    virtual bool needUpdateBackground() const;
    //called in paintEvent before drawFrame() when required
    virtual void drawBackground();
    //draw the current frame using the current paint engine. called by paintEvent()
    virtual void drawFrame();
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);
    virtual void resizeEvent(QResizeEvent *);
    virtual void showEvent(QShowEvent *);
private:
    virtual void onSetOutAspectRatioMode(OutAspectRatioMode mode);
    virtual void onSetOutAspectRatio(qreal ratio);
    virtual bool onSetBrightness(qreal b);
    virtual bool onSetContrast(qreal c);
    virtual bool onSetHue(qreal h);
    virtual bool onSetSaturation(qreal s);
};
typedef GLWidgetRenderer2 VideoRendererGLWidget2;

} //namespace QtAV

#endif // QTAV_GLWIDGETRENDERER2_H
