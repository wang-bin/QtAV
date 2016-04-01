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

#include "QtAV/OpenGLVideo.h"
#include <QtGui/QColor>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QSurface>
#define QT_VAO (QT_VERSION >= QT_VERSION_CHECK(5, 1, 0))
#if QT_VAO
#include <QtGui/QOpenGLVertexArrayObject>
#endif //QT_VAO
#endif //5.0
#include "QtAV/SurfaceInterop.h"
#include "QtAV/VideoShader.h"
#include "ShaderManager.h"
#include "opengl/OpenGLHelper.h"
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
        , material_type(0)
        , update_geo(true)
        , try_vbo(true)
        , try_vao(true)
        , tex_target(0)
        , valiad_tex_width(1.0)
        , user_shader(NULL)
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
            material = 0;
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
        char const *const *attr = shader->attributeNames();
        for (int i = 0; attr[i]; ++i) {
            shader->program()->disableAttributeArray(i); //TODO: in setActiveShader
        }
        // release vbo. qpainter is affected if vbo is bound
        if (try_vbo && vbo.isCreated()) {
            vbo.release();
        }
    }
public:
    QOpenGLContext *ctx;
    ShaderManager *manager;
    VideoMaterial *material;
    qint64 material_type;
    bool update_geo;
    bool try_vbo; // check environment var and opengl support
    QOpenGLBuffer vbo; //VertexBuffer
    bool try_vao;
#if QT_VAO
    QOpenGLVertexArrayObject vao;
#endif //QT_VAO
    int tex_target;
    qreal valiad_tex_width;
    QSize video_size;
    QRectF target;
    QRectF roi; //including invalid padding width
    TexturedGeometry geometry;
    QRectF rect;
    QMatrix4x4 matrix;
    VideoShader *user_shader;
};

void OpenGLVideoPrivate::bindAttributes(VideoShader* shader, const QRectF &t, const QRectF &r)
{
    const bool tex_rect = shader->textureTarget() == GL_TEXTURE_RECTANGLE;
    // also check size change for normalizedROI computation if roi is not normalized
    const bool roi_changed = valiad_tex_width != material->validTextureWidth() || roi != r || video_size != material->frameSize();
    const int tc = shader->textureLocationCount();
    if (roi_changed) {
        roi = r;
        valiad_tex_width = material->validTextureWidth();
        video_size = material->frameSize();
    }
    if (tex_target != shader->textureTarget()) {
        tex_target = shader->textureTarget();
        update_geo = true;
    }
    QRectF& target_rect = rect;
    if (target.isValid()) {
        if (roi_changed || target != t) {
            target = t;
            update_geo = true;
        }
    } else {
        if (roi_changed) {
            update_geo = true;
        }
    }
    if (!update_geo)
        goto end;
    //qDebug("updating geometry...");
    geometry.setRect(target_rect, material->mapToTexture(0, roi));
    if (tex_rect) {
        geometry.setTextureCount(tc);
        for (int i = 1; i < tc; ++i) {
            // tc can > planes, but that will compute chroma plane
            geometry.setTextureRect(material->mapToTexture(i, roi), i);
        }
    }
    update_geo = false;
    if (!try_vbo)
        goto end;
    { //VAO scope BEGIN
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
    if (!vbo.isCreated()) {
        if (!vbo.create()) {
            try_vbo = false; // not supported by OpenGL
            try_vao = false; // also disable VAO. destroy?
            qWarning("VBO is not supported");
            goto end;
        }
    }
    //qDebug("updating vbo...");
    vbo.bind(); //check here
    vbo.allocate(geometry.data(), geometry.size());
#if QT_VAO
    if (try_vao) {
        shader->program()->setAttributeBuffer(0, GL_FLOAT, 0, geometry.tupleSize(), geometry.stride());
        shader->program()->setAttributeBuffer(1, GL_FLOAT, geometry.tupleSize()*sizeof(float), geometry.tupleSize(), geometry.stride());
        if (tex_rect) {
            for (int i = 1; i < tc; ++i) {
                shader->program()->setAttributeBuffer(i + 1, GL_FLOAT, i*geometry.textureSize() + geometry.tupleSize()*sizeof(float), geometry.tupleSize(), geometry.stride());
            }
        }
        char const *const *attr = shader->attributeNames();
        for (int i = 0; attr[i]; ++i) {
            shader->program()->enableAttributeArray(i); //TODO: in setActiveShader
        }
    }
#endif
    vbo.release();
    } //VAO scope END
end:
#if QT_VAO
    if (try_vao && vao.isCreated()) {
        vao.bind();
        return;
    }
#endif
    if (try_vbo && vbo.isCreated()) {
        vbo.bind();
        shader->program()->setAttributeBuffer(0, GL_FLOAT, 0, geometry.tupleSize(), geometry.stride());
        shader->program()->setAttributeBuffer(1, GL_FLOAT, geometry.tupleSize()*sizeof(float), geometry.tupleSize(), geometry.stride());
        if (tex_rect) {
            for (int i = 1; i < tc; ++i) {
                shader->program()->setAttributeBuffer(i + 1, GL_FLOAT, i*geometry.textureSize() + geometry.tupleSize()*sizeof(float), geometry.tupleSize(), geometry.stride());
            }
        }
    } else {
        shader->program()->setAttributeArray(0, GL_FLOAT, geometry.data(0), geometry.tupleSize(), geometry.stride());
        shader->program()->setAttributeArray(1, GL_FLOAT, geometry.data(1), geometry.tupleSize(), geometry.stride());
        if (tex_rect) {
            for (int i = 1; i < tc; ++i) {
                shader->program()->setAttributeArray(i + 1, GL_FLOAT, geometry.data(1), i*geometry.textureSize() + geometry.tupleSize(), geometry.stride());
            }
        }
    }
    char const *const *attr = shader->attributeNames();
    for (int i = 0; attr[i]; ++i) {
        shader->program()->enableAttributeArray(i); //TODO: in setActiveShader
    }
}

OpenGLVideo::OpenGLVideo() {}

bool OpenGLVideo::isSupported(VideoFormat::PixelFormat pixfmt)
{
    return pixfmt != VideoFormat::Format_RGB48BE;
}

void OpenGLVideo::setOpenGLContext(QOpenGLContext *ctx)
{
    DPTR_D(OpenGLVideo);
    if (d.ctx == ctx)
        return;
    d.resetGL(); //TODO: is it ok to destroygl resources in another context?
    d.ctx = ctx; // Qt4: set to null in resetGL()
    if (!ctx) {
        return;
    }
    if (d.material)
        delete d.material;
    d.material = new VideoMaterial();
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    d.manager = ctx->findChild<ShaderManager*>(QStringLiteral("__qtav_shader_manager"));
    QSizeF surfaceSize = ctx->surface()->size();
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    surfaceSize *= ctx->screen()->devicePixelRatio();
#else
    surfaceSize *= qApp->devicePixelRatio(); //TODO: window()->devicePixelRatio() is the window screen's
#endif
#else
    QSizeF surfaceSize = QSizeF(ctx->device()->width(), ctx->device()->height());
#endif
    setProjectionMatrixToRect(QRectF(QPointF(), surfaceSize));
    if (d.manager)
        return;
    // TODO: what if ctx is delete?
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    d.manager = new ShaderManager(ctx);
    QObject::connect(ctx, SIGNAL(aboutToBeDestroyed()), this, SLOT(resetGL()), Qt::DirectConnection); //direct?
#else
    d.manager = new ShaderManager(this);
#endif
    d.manager->setObjectName(QStringLiteral("__qtav_shader_manager"));
    /// get gl info here because context is current(qt ensure it)
    //const QByteArray extensions(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
    bool hasGLSL = QOpenGLShaderProgram::hasOpenGLShaderPrograms();
    qDebug("OpenGL version: %d.%d  hasGLSL: %d", ctx->format().majorVersion(), ctx->format().minorVersion(), hasGLSL);
    static bool sInfo = true;
    if (sInfo) {
        sInfo = false;
        qDebug("GL_VERSION: %s", DYGL(glGetString(GL_VERSION)));
        qDebug("GL_VENDOR: %s", DYGL(glGetString(GL_VENDOR)));
        qDebug("GL_RENDERER: %s", DYGL(glGetString(GL_RENDERER)));
        qDebug("GL_SHADING_LANGUAGE_VERSION: %s", DYGL(glGetString(GL_SHADING_LANGUAGE_VERSION)));
        /// check here with current context can ensure the right result. If the first check is in VideoShader/VideoMaterial/decoder or somewhere else, the context can be null
        bool v = OpenGLHelper::isOpenGLES();
        qDebug("Is OpenGLES: %d", v);
        v = OpenGLHelper::isEGL();
        qDebug("Is EGL: %d", v);
        const int glsl_ver = OpenGLHelper::GLSLVersion();
        qDebug("GLSL version: %d", glsl_ver);
        v = OpenGLHelper::isPBOSupported();
        qDebug("Has PBO: %d", v);
        v = OpenGLHelper::has16BitTexture();
        qDebug("Has 16bit texture: %d", v);
        v = OpenGLHelper::hasRG();
        qDebug("Has RG texture: %d", v);
        qDebug() << ctx->format();
    }
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
    if (d.ctx && d.ctx == QOpenGLContext::currentContext()) {
        DYGL(glViewport(d.rect.x(), d.rect.y(), d.rect.width(), d.rect.height()));
    }
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

void OpenGLVideo::setUserShader(VideoShader *shader)
{
    d_func().user_shader = shader;
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
    DYGL(glViewport(d.rect.x(), d.rect.y(), d.rect.width(), d.rect.height())); // viewport was used in gpu filters is wrong, qt quick fbo item's is right(so must ensure setProjectionMatrixToRect was called correctly)
    const qint64 mt = d.material->type();
    if (d.material_type != mt) {
        qDebug() << "material changed: " << VideoMaterial::typeName(d.material_type) << " => " << VideoMaterial::typeName(mt);
        d.material_type = mt;
    }
    VideoShader *shader = d.user_shader;
    if (!shader)
        shader = d.manager->prepareMaterial(d.material, mt); //TODO: print shader type name if changed. prepareMaterial(,sample_code, pp_code)
    shader->update(d.material);
    d.material->setColorMatrixDirty(false); //
    shader->program()->setUniformValue(shader->matrixLocation(), transform*d.matrix);
    // uniform end. attribute begin
    d.bindAttributes(shader, target, roi);
    // normalize?
    const bool blending = d.material->hasAlpha();
    if (blending) {
        DYGL(glEnable(GL_BLEND));
        DYGL(glBlendFunc(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA));
    }
    DYGL(glDrawArrays(d.geometry.mode(), 0, d.geometry.textureVertexCount()));
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
