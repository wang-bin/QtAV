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

#ifndef QTAV_OPENGLVIDEO_H
#define QTAV_OPENGLVIDEO_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/VideoFormat.h>
#include <QtCore/QHash>
#include <QtGui/QMatrix4x4>
#include <QtCore/QObject>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLContext>
#else
#if !defined(QT_NO_OPENGL)
#include <QtOpenGL/QGLContext>
#define QOpenGLContext QGLContext
#endif //!defined(QT_NO_OPENGL)
#endif

namespace QtAV {

class VideoFrame;
class OpenGLVideoPrivate;
/*!
 * \brief The OpenGLVideo class
 * high level api for renderering a video frame. use VideoShader, VideoMaterial and ShaderManager internally
 */
class Q_AV_EXPORT OpenGLVideo : public QObject
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(OpenGLVideo)
public:
    OpenGLVideo();
    /*!
     * \brief setOpenGLContext
     * a context must be set before renderering.
     * \param ctx
     * 0: current context in OpenGL is done. shaders all will be released.
     * QOpenGLContext is QObject in Qt5, and gl resources here will be released automatically if context is destroyed.
     * But you have to call setOpenGLContext(0) for Qt4 explicitly.
     */
    void setOpenGLContext(QOpenGLContext *ctx);
    QOpenGLContext* openGLContext();
    void setCurrentFrame(const VideoFrame& frame);
    /*!
     * \brief render
     * all are in Qt's coordinate
     * \param target: the rect renderering to. in Qt's coordinate. not normalized here but in shader. // TODO: normalized check?
     * invalid value (default) means renderering to the whole viewport
     * \param roi: normalized rect of texture to renderer.
     * \param transform: additinal transformation.
     */
    void render(const QRectF& target = QRectF(), const QRectF& roi = QRectF(), const QMatrix4x4& transform = QMatrix4x4());
    void setProjectionMatrixToRect(const QRectF& v);
    void setProjectionMatrix(const QMatrix4x4 &matrix);

    void setBrightness(qreal value);
    void setContrast(qreal value);
    void setHue(qreal value);
    void setSaturation(qreal value);
protected:
    DPTR_DECLARE(OpenGLVideo)

private slots:
    /* used by Qt5 whose QOpenGLContext is QObject and we can call this when context is about to destroy.
     * shader manager and material will be reset
     */
    void resetGL();
};


} //namespace QtAV

#endif // QTAV_OPENGLVIDEO_H
