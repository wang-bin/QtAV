/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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

#include <QtAVWidgets/global.h>
#include <QtOpenGL/QGLWidget>
#include <QtAV/OpenGLRendererBase.h>

namespace QtAV {

class GLWidgetRenderer2Private;
/*!
 * \brief The GLWidgetRenderer2 class
 * Renderering video frames using GLSL. A more generic high level class OpenGLVideo is used internally.
 * TODO: for Qt5, no QtOpenGL, use QWindow instead.
 */
class Q_AVWIDGETS_EXPORT GLWidgetRenderer2 : public QGLWidget, public OpenGLRendererBase
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GLWidgetRenderer2)
public:
    GLWidgetRenderer2(QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0);
    virtual void onUpdate();

    virtual VideoRendererId id() const;
    virtual QWidget* widget() { return this; }
protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);
    virtual void resizeEvent(QResizeEvent *);
    virtual void showEvent(QShowEvent *);
};
typedef GLWidgetRenderer2 VideoRendererGLWidget2;

} //namespace QtAV

#endif // QTAV_GLWIDGETRENDERER2_H
