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

#ifndef QTAV_OPENGLWINDOWRENDERER_H
#define QTAV_OPENGLWINDOWRENDERER_H
#ifndef QT_NO_OPENGL
#include <QtGui/QOpenGLWindow>
#include <QtAV/OpenGLRendererBase.h>

namespace QtAV {

class OpenGLWindowRendererPrivate;
class Q_AV_EXPORT OpenGLWindowRenderer : public QOpenGLWindow, public OpenGLRendererBase
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(OpenGLWindowRenderer)
    Q_PROPERTY(qreal sourceAspectRatio READ sourceAspectRatio NOTIFY sourceAspectRatioChanged)
public:
    explicit OpenGLWindowRenderer(UpdateBehavior updateBehavior = NoPartialUpdate, QWindow *parent = 0);
    virtual VideoRendererId id() const Q_DECL_OVERRIDE;
    QWindow* qwindow() Q_DECL_OVERRIDE Q_DECL_FINAL { return this; }
Q_SIGNALS:
    void sourceAspectRatioChanged(qreal value) Q_DECL_OVERRIDE Q_DECL_FINAL;
protected:
    virtual void initializeGL() Q_DECL_OVERRIDE;
    virtual void paintGL() Q_DECL_OVERRIDE;
    virtual void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    //virtual void showEvent(QShowEvent *);
};
typedef OpenGLWindowRenderer VideoRendererOpenGLWindow;

} //namespace QtAV
#endif //QT_NO_OPENGL
#endif // QTAV_OPENGLWINDOWRENDERER_H
