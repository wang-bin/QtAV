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

#include "QtAV/OpenGLVideo.h"
#include <QtGui/QColor>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QSurface>
#define QT_VAO (QT_VERSION >= QT_VERSION_CHECK(5, 1, 0))
#if QT_VAO
#include <QtGui/QOpenGLVertexArrayObject>
#endif //QT_VAO
#else
#include <QtOpenGL/QGLBuffer>
#include <QtOpenGL/QGLShaderProgram>
#include <QtOpenGL/QGLFunctions>
typedef QGLBuffer QOpenGLBuffer;
#define QOpenGLShaderProgram QGLShaderProgram
#define QOpenGLShader QGLShader
#define QOpenGLFunctions QGLFunctions
#endif
#include "QtAV/SurfaceInterop.h"
#include "QtAV/VideoShader.h"
#include "ShaderManager.h"
#include "utils/OpenGLHelper.h"
#include "utils/Logger.h"

namespace QtAV {

// FIXME: why crash if inherits both QObject and DPtrPrivate?
class OpenGLVideoPrivate : public DPtrPrivate<OpenGLVideo>
{
public:
    OpenGLVideoPrivate()
        : ctx(0)
        , manager(0)
        , material(new VideoMaterial())
        , update_geo(true)
        , try_vbo(true)
        , try_vao(true)
        , valiad_tex_width(1.0)
    {
        static bool disable_vbo = qgetenv("QTAV_NO_VBO").toInt() > 0;
        try_vbo = !disable_vbo;
        static bool disable_vao = qgetenv("QTAV_NO_VAO").toInt() > 0;
        try_vao = !disable_vao;
    }
    ~OpenGLVideoPrivate() {
        if (material) {
            delete material;
            material = 0;
        }
    }

    void resetGL() {
        ctx = 0;
        vbo.destroy();
#if QT_VAO
        vao.destroy();
#endif
        if (!manager)
            return;
        manager->setParent(0);
        delete manager;
        manager = 0;
        if (material) {
            delete material;
            material = new VideoMaterial();
        }
    }
    // update geometry(vertex array) set attributes or bind VAO/VBO.
    void bindAttributes(VideoShader* shader, const QRectF& t, const QRectF& r);
    void unbindAttributes(VideoShader* shader) {
#if QT_VAO
        if (try_vao && vao.isCreated()) {
            vao.release();
            return;
        }
#endif //QT_VAO
        // release vbo?
        char const *const *attr = shader->attributeNames();
        for (int i = 0; attr[i]; ++i) {
            shader->program()->disableAttributeArray(i); //TODO: in setActiveShader
        }
    }
public:
    QOpenGLContext *ctx;
    ShaderManager *manager;
    VideoMaterial *material;
    bool update_geo;
    bool try_vbo; // check environment var and opengl support
    QOpenGLBuffer vbo; //VertexBuffer
    bool try_vao;
#if QT_VAO
    QOpenGLVertexArrayObject vao;
#endif //QT_VAO
    qreal valiad_tex_width;
    QSize video_size;
    QRectF target;
    QRectF roi; //including invalid padding width
    TexturedGeometry geometry;
    QRectF rect;
    QMatrix4x4 matrix;
};

void OpenGLVideoPrivate::bindAttributes(VideoShader* shader, const QRectF &t, const QRectF &r)
{
    // also check size change for normalizedROI computation if roi is not normalized
    const bool roi_changed = valiad_tex_width != material->validTextureWidth() || roi != r || video_size != material->frameSize();
    if (roi_changed) {
        roi = r;
        valiad_tex_width = material->validTextureWidth();
        video_size = material->frameSize();
    }
    QRectF& target_rect = rect;
    if (target.isValid()) {
        if (roi_changed || target != t) {
            target = t;
            update_geo = true;
        }
    } else {
        if (roi_changed || update_geo) {
            update_geo = true; // roi_changed
        }
    }
    if (!update_geo)
        goto end;
    //qDebug("updating geometry...");
    geometry.setRect(target_rect, material->normalizedROI(roi));
    update_geo = false;
    if (try_vbo) {
#if QT_VAO
        if (try_vao) {
            //qDebug("updating vao...");
            if (!vao.isCreated()) {
                if (!vao.create()) {
                    try_vao = false;
                    qDebug("VAO is not supported");
                }
            }
        }
        QOpenGLVertexArrayObject::Binder vao_bind(&vao);
        Q_UNUSED(vao_bind);
#endif
        if (!vbo.isCreated())
            vbo.create();
        if (vbo.isCreated()) {
            //qDebug("updating vbo...");
            vbo.bind();
            vbo.allocate(geometry.data(), geometry.vertexCount()*geometry.stride());
#if QT_VAO
            if (try_vao) {
                shader->program()->setAttributeBuffer(0, GL_FLOAT, 0, geometry.tupleSize(), geometry.stride());
                shader->program()->setAttributeBuffer(1, GL_FLOAT, geometry.tupleSize()*sizeof(float), geometry.tupleSize(), geometry.stride());
                char const *const *attr = shader->attributeNames();
                for (int i = 0; attr[i]; ++i) {
                    shader->program()->enableAttributeArray(i); //TODO: in setActiveShader
                }
            }
#endif
            vbo.release();
        } else {
            try_vbo = false; // not supported by OpenGL
            qWarning("VBO is not supported");
        }
    }
end:
#if QT_VAO
    const bool use_vao = try_vao && vao.isCreated();
    if (use_vao) {
        vao.bind();
        return;
    }
#endif
    if (try_vbo && vbo.isCreated()) {
        vbo.bind();
        shader->program()->setAttributeBuffer(0, GL_FLOAT, 0, geometry.tupleSize(), geometry.stride());
        shader->program()->setAttributeBuffer(1, GL_FLOAT, geometry.tupleSize()*sizeof(float), geometry.tupleSize(), geometry.stride());
    } else {
        shader->program()->setAttributeArray(0, GL_FLOAT, geometry.data(0), geometry.tupleSize(), geometry.stride());
        shader->program()->setAttributeArray(1, GL_FLOAT, geometry.data(1), geometry.tupleSize(), geometry.stride());
    }
    char const *const *attr = shader->attributeNames();
    for (int i = 0; attr[i]; ++i) {
        shader->program()->enableAttributeArray(i); //TODO: in setActiveShader
    }
}

OpenGLVideo::OpenGLVideo() {}

bool OpenGLVideo::isSupported(VideoFormat::PixelFormat pixfmt)
{
    return pixfmt != VideoFormat::Format_YUYV && pixfmt != VideoFormat::Format_UYVY
            && pixfmt != VideoFormat::Format_RGB48BE;
}

// TODO: set surface/device size here (viewport?)
void OpenGLVideo::setOpenGLContext(QOpenGLContext *ctx)
{
    DPTR_D(OpenGLVideo);
    if (!ctx) {
        d.resetGL();
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    d.manager = ctx->findChild<ShaderManager*>(QStringLiteral("__qtav_shader_manager"));
    QSizeF surfaceSize = QOpenGLContext::currentContext()->surface()->size();
#else
    d.resetGL();
    QSizeF surfaceSize = QSizeF(ctx->device()->width(), ctx->device()->height());
#endif
    d.ctx = ctx; // Qt4: set to null in resetGL()
    setProjectionMatrixToRect(QRectF(QPointF(), surfaceSize));
    if (d.manager)
        return;
    // TODO: what if ctx is delete?
    d.manager = new ShaderManager(ctx);
    d.manager->setObjectName(QStringLiteral("__qtav_shader_manager"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QObject::connect(ctx, SIGNAL(aboutToBeDestroyed()), this, SLOT(resetGL()), Qt::DirectConnection); //direct?
#endif
}

QOpenGLContext* OpenGLVideo::openGLContext()
{
    return d_func().ctx;
}

void OpenGLVideo::setCurrentFrame(const VideoFrame &frame)
{
    d_func().material->setCurrentFrame(frame);
}

void OpenGLVideo::setProjectionMatrixToRect(const QRectF &v)
{
    DPTR_D(OpenGLVideo);
    d.rect = v;
    d.matrix.setToIdentity();
    d.matrix.ortho(v);
    // Mirrored relative to the usual Qt coordinate system with origin in the top left corner.
    //mirrored = mat(0, 0) * mat(1, 1) - mat(0, 1) * mat(1, 0) > 0;
    d.update_geo = true; // even true for target_rect != d.rect
}

void OpenGLVideo::setProjectionMatrix(const QMatrix4x4 &matrix)
{
    d_func().matrix = matrix;
}

void OpenGLVideo::setBrightness(qreal value)
{
    d_func().material->setBrightness(value);
}

void OpenGLVideo::setContrast(qreal value)
{
    d_func().material->setContrast(value);
}

void OpenGLVideo::setHue(qreal value)
{
    d_func().material->setHue(value);
}

void OpenGLVideo::setSaturation(qreal value)
{
    d_func().material->setSaturation(value);
}

void OpenGLVideo::fill(const QColor &color)
{
    DYGL(glClearColor(color.red(), color.green(), color.blue(), color.alpha()));
    DYGL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void OpenGLVideo::render(const QRectF &target, const QRectF& roi, const QMatrix4x4& transform)
{
    DPTR_D(OpenGLVideo);
    Q_ASSERT(d.manager);
    VideoShader *shader = d.manager->prepareMaterial(d.material);
    shader->update(d.material);
    shader->program()->setUniformValue(shader->opacityLocation(), (GLfloat)1.0);
    shader->program()->setUniformValue(shader->matrixLocation(), transform*d.matrix);
    // uniform end. attribute begin
    d.bindAttributes(shader, target, roi);
    // normalize?
    const bool blending = d.material->hasAlpha();
    if (blending) {
        DYGL(glEnable(GL_BLEND));
        DYGL(glBlendFunc(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA));
    }
    DYGL(glDrawArrays(d.geometry.mode(), 0, d.geometry.vertexCount()));
    if (blending)
        DYGL(glDisable(GL_BLEND));
    // d.shader->program()->release(); //glUseProgram(0)
    d.unbindAttributes(shader);
    d.material->unbind();
}

void OpenGLVideo::resetGL()
{
    qDebug("~~~~~~~~~resetGL %p. from sender %p", d_func().manager, sender());
    d_func().resetGL();
}

} //namespace QtAV
