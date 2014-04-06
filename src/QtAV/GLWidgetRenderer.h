/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_GLWIDGETRENDERER_H
#define QTAV_GLWIDGETRENDERER_H

#include <QtAV/VideoRenderer.h>
#include <QtOpenGL/QGLWidget>
// TODO: QGLFunctions is in Qt4.8+. meego is 4.7
#if QTAV_HAVE(QGLFUNCTIONS)
#include <QtOpenGL/QGLFunctions>
#endif //QT_VERSION

namespace QtAV {

class GLWidgetRendererPrivate;
class Q_AV_EXPORT GLWidgetRenderer : public QGLWidget, public VideoRenderer
#if QTAV_HAVE(QGLFUNCTIONS) //TODO: why use QT_VERSION will result in moc error?
        , public QGLFunctions
#endif //QTAV_HAVE(QGLFUNCTIONS)
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GLWidgetRenderer)
public:
    GLWidgetRenderer(QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0);
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
    /*!
     * \brief onSetBrightness
     *  only works for GLSL. otherwise return false, means that do nothing, brightness() does not change.
     * \return
     */
    virtual bool onSetBrightness(qreal b);
    virtual bool onSetContrast(qreal c);
    virtual bool onSetHue(qreal h);
    virtual bool onSetSaturation(qreal s);

};
typedef GLWidgetRenderer VideoRendererGLWidget;

} //namespace QtAV

#endif // QTAV_GLWidgetRenderer_H
