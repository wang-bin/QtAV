/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#ifndef QTAV_OPENGLRENDERERBASE_H
#define QTAV_OPENGLRENDERERBASE_H

#include <QtAV/VideoRenderer.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLFunctions>
#elif QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
#include <QtOpenGL/QGLFunctions>
#define QOpenGLFunctions QGLFunctions
#endif

namespace QtAV {

/*!
 * \brief The OpenGLRendererBase class
 * Renderering video frames using GLSL. A more generic high level class OpenGLVideo is used internally.
 * TODO: for Qt5, no QtOpenGL, use QWindow instead.
 */
class OpenGLRendererBasePrivate;
class Q_AV_EXPORT OpenGLRendererBase : public VideoRenderer
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
        , public QOpenGLFunctions
#endif
{
    DPTR_DECLARE_PRIVATE(OpenGLRendererBase)
public:
    virtual ~OpenGLRendererBase();
    bool isSupported(VideoFormat::PixelFormat pixfmt) const Q_DECL_OVERRIDE;
    OpenGLVideo* opengl() const Q_DECL_OVERRIDE;
protected:
    virtual bool receiveFrame(const VideoFrame& frame) Q_DECL_OVERRIDE;
    //called in paintEvent before drawFrame() when required
    virtual void drawBackground() Q_DECL_OVERRIDE;
    //draw the current frame using the current paint engine. called by paintEvent()
    virtual void drawFrame() Q_DECL_OVERRIDE;
    void onInitializeGL();
    void onPaintGL();
    void onResizeGL(int w, int h);
    void onResizeEvent(int w, int h);
    void onShowEvent();
private:
    void onSetOutAspectRatioMode(OutAspectRatioMode mode) Q_DECL_OVERRIDE;
    void onSetOutAspectRatio(qreal ratio) Q_DECL_OVERRIDE;
    bool onSetOrientation(int value) Q_DECL_OVERRIDE;
    bool onSetBrightness(qreal b) Q_DECL_OVERRIDE;
    bool onSetContrast(qreal c) Q_DECL_OVERRIDE;
    bool onSetHue(qreal h) Q_DECL_OVERRIDE;
    bool onSetSaturation(qreal s) Q_DECL_OVERRIDE;
protected:
    OpenGLRendererBase(OpenGLRendererBasePrivate &d);
};

} //namespace QtAV

#endif // QTAV_OPENGLRENDERERBASE_H
