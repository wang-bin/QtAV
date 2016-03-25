/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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

#ifndef QTAV_QOPENGLWIDGET_H
#define QTAV_QOPENGLWIDGET_H

#include <QtAVWidgets/global.h>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#error "Qt5 is required!"
#endif
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QSurfaceFormat>
#include <QtWidgets/QWidget>

namespace QtAV {

/*!
 * \brief The QOpenGLWidget class
 * A widget for rendering OpenGL graphics without QtOpenGL module
 */
class Q_AVWIDGETS_EXPORT QOpenGLWidget : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(QOpenGLWidget)
public:
    explicit QOpenGLWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~QOpenGLWidget();
    void setFormat(const QSurfaceFormat &format);
    QSurfaceFormat format() const;
    bool isValid() const;
    void makeCurrent();
    void doneCurrent();
    QOpenGLContext *context() const;
protected:
    QPaintDevice* redirected(QPoint *offset) const Q_DECL_OVERRIDE;
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
private:
    void initialize();
    void render();
    void invokeUserPaint();

    bool m_initialized;
    bool m_fakeHidden;
    QOpenGLContext *m_context;
    QOpenGLPaintDevice *m_paintDevice;
    QSurfaceFormat m_requestedFormat;
};
} //namespace QtAV

#endif //QTAV_QOPENGLWIDGET_H
