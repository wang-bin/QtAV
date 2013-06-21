/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

namespace QtAV {

class GLWidgetRendererPrivate;
class Q_EXPORT GLWidgetRenderer : public QGLWidget, public VideoRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GLWidgetRenderer)
public:
    GLWidgetRenderer(QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0);
    virtual ~GLWidgetRenderer();
    virtual VideoRendererId id() const;

protected:
    virtual void convertData(const QByteArray &data);
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
    virtual bool write();
};
typedef GLWidgetRenderer VideoRendererGLWidget;

} //namespace QtAV

#endif // QTAV_GLWidgetRenderer_H
