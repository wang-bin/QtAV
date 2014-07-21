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
#include <QtAV/ColorTransform.h>
#include <QtAV/VideoFormat.h>
#include <QtCore/QHash>
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
class Q_AV_EXPORT OpenGLVideo
{
    DPTR_DECLARE_PRIVATE(OpenGLVideo)
public:
    OpenGLVideo();
    /*!
     * \brief setOpenGLContext
     * a context must be set before renderering.
     * \param ctx
     * 0: current context in OpenGL is done. shaders all will be released
     */
    void setOpenGLContext(QOpenGLContext *ctx);
    void setCurrentFrame(const VideoFrame& frame);
    /*!
     * \brief render
     * \param roi
     * region of interest of video frame
     * TODO: QRectF
     */
    void render(const QRect& roi);
    void setViewport(const QRect& rect);
    /*!
     * \brief setVideoRect
     * call this if video display area change or aspect ratio change
     * \param rect
     */
    void setVideoRect(const QRect& rect);

    void setBrightness(qreal value);
    void setContrast(qreal value);
    void setHue(qreal value);
    void setSaturation(qreal value);
protected:
    DPTR_DECLARE(OpenGLVideo)
};


} //namespace QtAV

#endif // QTAV_OPENGLVIDEO_H
