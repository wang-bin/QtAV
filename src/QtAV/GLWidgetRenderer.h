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

#include <QtAV/ImageRenderer.h>
#include <QtOpenGL/QGLWidget>

/*TODO:
 *  gl command
 */

namespace QtAV {

class GLWidgetRendererPrivate;
class Q_EXPORT GLWidgetRenderer : public QGLWidget, public ImageRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GLWidgetRenderer)
public:
    GLWidgetRenderer(QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0);
    virtual ~GLWidgetRenderer();

protected:
    virtual bool write();
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
};

} //namespace QtAV

#endif // QTAV_GLWidgetRenderer_H
