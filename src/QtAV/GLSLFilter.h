/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

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

#ifndef QTAV_GLSLFILTER_H
#define QTAV_GLSLFILTER_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/Filter.h>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#undef QOpenGLFramebufferObject
#define QOpenGLFramebufferObject QGLFramebufferObject
#endif
QT_BEGIN_NAMESPACE
class QOpenGLFramebufferObject;
QT_END_NAMESPACE

namespace QtAV {
class OpenGLVideo;
class GLSLFilterPrivate;
class Q_AV_EXPORT GLSLFilter : public VideoFilter
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GLSLFilter)
    Q_PROPERTY(QSize outputSize READ outputSize WRITE setOutputSize NOTIFY outputSizeChanged)
public:
    GLSLFilter(QObject* parent = 0);
    bool isSupported(VideoFilterContext::Type ct) const  Q_DECL_OVERRIDE {
        return ct == VideoFilterContext::OpenGL;
    }

    /*!
     * \brief opengl
     * Currently you can only use it to set custom shader OpenGLVideo.setUserShader()
     */
    OpenGLVideo* opengl() const;
    QOpenGLFramebufferObject* fbo() const;
    /*!
     * \brief outputSize
     * Output frame size. FBO uses the same size to render. An empty size means using the input frame size
     * \return
     */
    QSize outputSize() const;
    void setOutputSize(const QSize& value);
    void setOutputSize(int width, int height);
Q_SIGNALS:
    void outputSizeChanged(const QSize& size);
protected:
    GLSLFilter(GLSLFilterPrivate& d, QObject *parent = 0);
    /*!
     * \brief process
     * Draw video frame into fbo and apply the user shader from opengl()->userShader();
     * \param frame input frame can be a frame holding host memory data, or any other GPU frame can interop with OpenGL texture (including frames from HW decoders in QtAV).
     * Output frame holds an RGB texture, which can be processed in the next GPU filter, or rendered by OpenGL renderers.
     * When process() is done, FBO before before process() is bounded.
     */
    void process(Statistics* statistics, VideoFrame* frame = 0) Q_DECL_OVERRIDE;
};
} //namespace QtAV
#endif // QTAV_GLSLFILTER_H
