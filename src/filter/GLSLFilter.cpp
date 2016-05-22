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

#include "QtAV/GLSLFilter.h"
#include "QtAV/private/Filter_p.h"
#include "QtAV/VideoFrame.h"
#include "opengl/OpenGLHelper.h"
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLFramebufferObject>
#else
#include <QtOpenGL/QGLFramebufferObject>
#endif
#include "QtAV/SurfaceInterop.h"
#include "QtAV/OpenGLVideo.h"

namespace QtAV {
class GLSLFilterPrivate : public VideoFilterPrivate
{
public:
    GLSLFilterPrivate() : VideoFilterPrivate()
      , fbo(0)
    {}

    QOpenGLFramebufferObject *fbo;
    QSize size;
    OpenGLVideo glv;
};

GLSLFilter::GLSLFilter(QObject *parent)
    : VideoFilter(*new GLSLFilterPrivate(), parent)
{}

GLSLFilter::GLSLFilter(GLSLFilterPrivate &d, QObject *parent)
    : VideoFilter(d, parent)
{}

OpenGLVideo* GLSLFilter::opengl() const
{
    return const_cast<OpenGLVideo*>(&d_func().glv);
}

QOpenGLFramebufferObject* GLSLFilter::fbo() const
{
    return d_func().fbo;
}

QSize GLSLFilter::outputSize() const
{
    return d_func().size;
}

void GLSLFilter::setOutputSize(const QSize &value)
{
    DPTR_D(GLSLFilter);
    if (d.size == value)
        return;
    d.size = value;
    Q_EMIT outputSizeChanged(value);
}

void GLSLFilter::setOutputSize(int width, int height)
{
    setOutputSize(QSize(width, height));
}

void GLSLFilter::process(Statistics *statistics, VideoFrame *frame)
{
    Q_UNUSED(statistics);
    if (!QOpenGLContext::currentContext()) {
        qWarning() << "No current gl context for glsl filter: " << this;
        return;
    }
    DPTR_D(GLSLFilter);
    if (!frame || !*frame)
        return;
    GLint currentFbo = 0;
    DYGL(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFbo));
    // now use the frame size
    if (d.fbo) {
        if (d.size.isEmpty()) {
            if (d.fbo->size() != frame->size()) {
                delete d.fbo;
                d.fbo = 0;
            }
        } else {
            if (d.fbo->size() != d.size) {
                delete d.fbo;
                d.fbo = 0;
            }
        }
    }
    if (!d.fbo) {
        d.fbo = new QOpenGLFramebufferObject(outputSize().isEmpty() ? frame->size() : outputSize(), GL_TEXTURE_2D); //TODO: prefer 16bit rgb
        QOpenGLContext *ctx = const_cast<QOpenGLContext*>(QOpenGLContext::currentContext()); //qt4 returns const
        d.glv.setOpenGLContext(ctx);
        d.glv.setProjectionMatrixToRect(QRectF(0, 0, d.fbo->width(), d.fbo->height()));
        qDebug("new fbo texture: %d %dx%d", d.fbo->texture(), d.fbo->width(), d.fbo->height());
    }
    d.fbo->bind();
    DYGL(glViewport(0, 0, d.fbo->width(), d.fbo->height()));
    d.glv.setCurrentFrame(*frame);
    QMatrix4x4 mat; // flip vertical
    mat.scale(1, -1);
    d.glv.render(QRectF(), QRectF(), mat);
    gl().BindFramebuffer(GL_FRAMEBUFFER, (GLuint)currentFbo);
    VideoFormat fmt(VideoFormat::Format_RGB32);
    VideoFrame f(d.fbo->width(), d.fbo->height(), fmt); //
    f.setBytesPerLine(d.fbo->width()*fmt.bytesPerPixel(), 0);
    // set interop;
    class GLTextureInterop : public VideoSurfaceInterop
    {
        GLuint tex;
    public:
        GLTextureInterop(GLuint id) : tex(id) {}
        void* map(SurfaceType, const VideoFormat &, void *handle, int plane) {
            Q_UNUSED(plane);
            GLuint* t = reinterpret_cast<GLuint*>(handle);
            *t = tex;
            return t;
        }
    };
    GLTextureInterop *interop = new GLTextureInterop(d.fbo->texture());
    f.setMetaData(QStringLiteral("surface_interop"), QVariant::fromValue(VideoSurfaceInteropPtr((interop))));
    *frame = f;
}
} //namespace QtAV
