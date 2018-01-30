/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_OPENGLVIDEO_H
#define QTAV_OPENGLVIDEO_H
#ifndef QT_NO_OPENGL
#include <QtAV/QtAV_Global.h>
#include <QtAV/VideoFormat.h>
#include <QtCore/QHash>
#include <QtGui/QMatrix4x4>
#include <QtCore/QObject>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLContext>
#else
#include <QtOpenGL/QGLContext>
#define QOpenGLContext QGLContext
#endif
QT_BEGIN_NAMESPACE
class QColor;
QT_END_NAMESPACE

namespace QtAV {

class VideoFrame;
class VideoShader;
class OpenGLVideoPrivate;
/*!
 * \brief The OpenGLVideo class
 * high level api for renderering a video frame. use VideoShader, VideoMaterial and ShaderManager internally.
 * By default, VBO is used. Set environment var QTAV_NO_VBO=1 or 0 to disable/enable VBO.
 * VAO will be enabled if supported. Disabling VAO is the same as VBO.
 */
class Q_AV_EXPORT OpenGLVideo : public QObject
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(OpenGLVideo)
public:
    enum MeshType {
        RectMesh,
        SphereMesh
    };
    static bool isSupported(VideoFormat::PixelFormat pixfmt);
    OpenGLVideo();
    /*!
     * \brief setOpenGLContext
     * a context must be set before renderering.
     * \param ctx
     * 0: current context in OpenGL is done. shaders will be released.
     * QOpenGLContext is QObject in Qt5, and gl resources here will be released automatically if context is destroyed.
     * But you have to call setOpenGLContext(0) for Qt4 explicitly in the old context.
     * Viewport is also set here using context surface/paintDevice size and devicePixelRatio.
     * devicePixelRatio may be wrong for multi-screen with 5.0<qt<5.5, so you should call setProjectionMatrixToRect later in this case
     */
    void setOpenGLContext(QOpenGLContext *ctx);
    QOpenGLContext* openGLContext();
    void setCurrentFrame(const VideoFrame& frame);
    void fill(const QColor& color);
    /*!
     * \brief render
     * all are in Qt's coordinate
     * \param target: the rect renderering to. in Qt's coordinate. not normalized here but in shader. // TODO: normalized check?
     * invalid value (default) means renderering to the whole viewport
     * \param roi: normalized rect of texture to renderer.
     * \param transform: additinal transformation.
     */
    void render(const QRectF& target = QRectF(), const QRectF& roi = QRectF(), const QMatrix4x4& transform = QMatrix4x4());
    /*!
     * \brief setProjectionMatrixToRect
     * the rect will be viewport
     */
    void setProjectionMatrixToRect(const QRectF& v);
    void setViewport(const QRectF& r);

    void setBrightness(qreal value);
    void setContrast(qreal value);
    void setHue(qreal value);
    void setSaturation(qreal value);

    void setUserShader(VideoShader* shader);
    VideoShader* userShader() const;

    void setMeshType(MeshType value);
    MeshType meshType() const;
Q_SIGNALS:
    void beforeRendering();
    /*!
     * \brief afterRendering
     * Emitted when video frame is rendered.
     * With DirectConnection, it can be used to draw GL on top of video, or to do screen scraping of the current frame buffer.
     */
    void afterRendering();
protected:
    DPTR_DECLARE(OpenGLVideo)

private Q_SLOTS:
    /* used by Qt5 whose QOpenGLContext is QObject and we can call this when context is about to destroy.
     * shader manager and material will be reset
     */
    void resetGL();
    void updateViewport();
};


} //namespace QtAV
#endif //QT_NO_OPENGL
#endif // QTAV_OPENGLVIDEO_H
