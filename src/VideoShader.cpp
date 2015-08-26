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

#include "QtAV/VideoShader.h"
#include "QtAV/private/VideoShader_p.h"
#include "QtAV/ColorTransform.h"
#include "utils/OpenGLHelper.h"
#include <cmath>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include "utils/Logger.h"

#define YUVA_DONE 0
#define glsl(x) #x "\n"

namespace QtAV {

TexturedGeometry::TexturedGeometry(int texCount, int count, Triangle t)
    : tri(t)
    , points_per_tex(count)
    , nb_tex(texCount)
{
    if (texCount < 1)
        texCount = 1;
    v.resize(nb_tex*points_per_tex);
}

void TexturedGeometry::setTextureCount(int value)
{
    if (value < 1)
        value = 1;
    if (value == nb_tex)
        return;
    nb_tex = value;
    v.resize(nb_tex*points_per_tex);
}

int TexturedGeometry::textureCount() const
{
    return nb_tex;
}

int TexturedGeometry::size() const
{
    return nb_tex * textureSize();
}

int TexturedGeometry::textureSize() const
{
    return textureVertexCount() * stride();
}

int TexturedGeometry::mode() const
{
    if (tri == Strip)
        return GL_TRIANGLE_STRIP;
    return GL_TRIANGLE_FAN;
}

void TexturedGeometry::setPoint(int index, const QPointF &p, const QPointF &tp, int texIndex)
{
    setGeometryPoint(index, p, texIndex);
    setTexturePoint(index, tp, texIndex);
}

void TexturedGeometry::setGeometryPoint(int index, const QPointF &p, int texIndex)
{
    v[texIndex*points_per_tex + index].x = p.x();
    v[texIndex*points_per_tex + index].y = p.y();
}

void TexturedGeometry::setTexturePoint(int index, const QPointF &tp, int texIndex)
{
    v[texIndex*points_per_tex + index].tx = tp.x();
    v[texIndex*points_per_tex + index].ty = tp.y();
}

void TexturedGeometry::setRect(const QRectF &r, const QRectF &tr, int texIndex)
{
    setPoint(0, r.topLeft(), tr.topLeft(), texIndex);
    setPoint(1, r.bottomLeft(), tr.bottomLeft(), texIndex);
    if (tri == Strip) {
        setPoint(2, r.topRight(), tr.topRight(), texIndex);
        setPoint(3, r.bottomRight(), tr.bottomRight(), texIndex);
    } else {
        setPoint(3, r.topRight(), tr.topRight(), texIndex);
        setPoint(2, r.bottomRight(), tr.bottomRight(), texIndex);
    }
}

void TexturedGeometry::setGeometryRect(const QRectF &r, int texIndex)
{
    setGeometryPoint(0, r.topLeft(), texIndex);
    setGeometryPoint(1, r.bottomLeft(), texIndex);
    if (tri == Strip) {
        setGeometryPoint(2, r.topRight(), texIndex);
        setGeometryPoint(3, r.bottomRight(), texIndex);
    } else {
        setGeometryPoint(3, r.topRight(), texIndex);
        setGeometryPoint(2, r.bottomRight(), texIndex);
    }
}

void TexturedGeometry::setTextureRect(const QRectF &tr, int texIndex)
{
    setTexturePoint(0, tr.topLeft(), texIndex);
    setTexturePoint(1, tr.bottomLeft(), texIndex);
    if (tri == Strip) {
        setTexturePoint(2, tr.topRight(), texIndex);
        setTexturePoint(3, tr.bottomRight(), texIndex);
    } else {
        setTexturePoint(3, tr.topRight(), texIndex);
        setTexturePoint(2, tr.bottomRight(), texIndex);
    }
}

VideoShader::VideoShader(VideoShaderPrivate &d):
    DPTR_INIT(&d)
{
}

VideoShader::VideoShader()
{
    //d.planar_frag = shaderSourceFromFile("shaders/planar.f.glsl");
    //d.packed_frag = shaderSourceFromFile("shaders/rgb.f.glsl");
}

VideoShader::~VideoShader()
{
}

/*
 * use gl api to get active attributes/uniforms
 * use glsl parser to get attributes?
 */
char const *const* VideoShader::attributeNames() const
{
    static const char *names[] = {
        "a_Position",
        "a_TexCoords0",
        0
    };
    if (textureTarget() == GL_TEXTURE_2D)
        return names;
    DPTR_D(const VideoShader);
    static const char *names_multicoord[] = {
        "a_Position",
        "a_TexCoords0",
        "a_TexCoords1",
        "a_TexCoords2",
        0
    };
#if YUVA_DONE
    static const char *names_multicoord_4[] = {
        "a_Position",
        "a_TexCoords0",
        "a_TexCoords1",
        "a_TexCoords2",
        "a_TexCoords3",
        0
    };
    if (d_func().video_format.planeCount() == 4)
        return names_multicoord_4;
#endif
    // TODO: names_multicoord_4planes
    return d.video_format.isPlanar() ? names_multicoord : names;
}

const char* VideoShader::vertexShader() const
{
    DPTR_D(const VideoShader);
    // because we have to modify the shader, and shader source must be kept, so read the origin
    d.vert = shaderSourceFromFile(QStringLiteral("shaders/video.vert"));
    QByteArray& vert = d.vert;
    if (vert.isEmpty()) {
        qWarning("Empty vertex shader!");
        return 0;
    }
    if (textureTarget() == GL_TEXTURE_RECTANGLE && d.video_format.isPlanar()) {
        vert.prepend("#define MULTI_COORD\n");
#if YUVA_DONE
        if (d.video_format.planeCount() == 4)
            vert.prepend("#define PLANE_4\n");
#endif
    }
    return vert.constData();
}

const char* VideoShader::fragmentShader() const
{
    DPTR_D(const VideoShader);
    // because we have to modify the shader, and shader source must be kept, so read the origin
    if (d.video_format.isPlanar()) {
        d.planar_frag = shaderSourceFromFile(QStringLiteral("shaders/planar.f.glsl"));
    } else {
        d.packed_frag = shaderSourceFromFile(QStringLiteral("shaders/packed.f.glsl"));
    }
    QByteArray& frag = d.video_format.isPlanar() ? d.planar_frag : d.packed_frag;
    if (frag.isEmpty()) {
        qWarning("Empty fragment shader!");
        return 0;
    }
#if YUVA_DONE
    if (d.video_format.planeCount() == 4) {
        frag.prepend("#define PLANE_4\n");
    }
#endif
    if (d.video_format.isPlanar()) {
        if (d.video_format.bytesPerPixel(0) == 2) {
            if (d.video_format.isBigEndian())
                frag.prepend("#define LA_16BITS_BE\n");
            else
                frag.prepend("#define LA_16BITS_LE\n");
        }
    } else {
        if (!d.video_format.isRGB())
            frag.prepend("#define PACKED_YUV");
    }
    if (d.texture_target == GL_TEXTURE_RECTANGLE) {
        frag.prepend("#extension GL_ARB_texture_rectangle : enable\n"
                     "#define texture2D texture2DRect\n"
                     "#define sampler2D sampler2DRect\n");
    }
    if (textureTarget() == GL_TEXTURE_RECTANGLE)
        frag.prepend("#define MULTI_COORD\n");
    return frag.constData();
}

void VideoShader::initialize(QOpenGLShaderProgram *shaderProgram)
{
    DPTR_D(VideoShader);
    if (!textureLocationCount())
        return;
    d.owns_program = !shaderProgram;
    if (shaderProgram) {
        d.program = shaderProgram;
    }
    shaderProgram = program();
    if (!shaderProgram->isLinked()) {
        compile(shaderProgram);
    }
    d.u_MVP_matrix = shaderProgram->uniformLocation("u_MVP_matrix");
    // fragment shader
    d.u_colorMatrix = shaderProgram->uniformLocation("u_colorMatrix");
    d.u_bpp = shaderProgram->uniformLocation("u_bpp");
    d.u_opacity = shaderProgram->uniformLocation("u_opacity");
    d.u_c = shaderProgram->uniformLocation("u_c");
    d.u_Texture.resize(textureLocationCount());
    for (int i = 0; i < d.u_Texture.size(); ++i) {
        const QString tex_var = QStringLiteral("u_Texture%1").arg(i);
        d.u_Texture[i] = shaderProgram->uniformLocation(tex_var);
        qDebug("glGetUniformLocation(\"%s\") = %d", tex_var.toUtf8().constData(), d.u_Texture[i]);
    }
    qDebug("glGetUniformLocation(\"u_MVP_matrix\") = %d", d.u_MVP_matrix);
    qDebug("glGetUniformLocation(\"u_colorMatrix\") = %d", d.u_colorMatrix);
    qDebug("glGetUniformLocation(\"u_opacity\") = %d", d.u_opacity);
    if (d.u_c >= 0)
        qDebug("glGetUniformLocation(\"u_c\") = %d", d.u_c);
    if (d.u_bpp >= 0)
        qDebug("glGetUniformLocation(\"u_bpp\") = %d", d.u_bpp);
}

int VideoShader::textureLocationCount() const
{
    DPTR_D(const VideoShader);
    // TODO: avoid accessing video_format.
    if (!d.video_format.isPlanar())
        return 1;
    return d.video_format.channels();
}

int VideoShader::textureLocation(int channel) const
{
    DPTR_D(const VideoShader);
    Q_ASSERT(channel < d.u_Texture.size());
    return d.u_Texture[channel];
}

int VideoShader::matrixLocation() const
{
    return d_func().u_MVP_matrix;
}

int VideoShader::colorMatrixLocation() const
{
    return d_func().u_colorMatrix;
}

int VideoShader::bppLocation() const
{
    return d_func().u_bpp;
}

int VideoShader::opacityLocation() const
{
    return d_func().u_opacity;
}

int VideoShader::channelMapLocation() const
{
    return d_func().u_c;
}

int VideoShader::textureTarget() const
{
    return d_func().texture_target;
}

void VideoShader::setTextureTarget(int type)
{
    d_func().texture_target = type;
}

VideoFormat VideoShader::videoFormat() const
{
    return d_func().video_format;
}

void VideoShader::setVideoFormat(const VideoFormat &format)
{
    d_func().video_format = format;
}

QOpenGLShaderProgram* VideoShader::program()
{
    DPTR_D(VideoShader);
    if (!d.program) {
        d.owns_program = true;
        d.program = new QOpenGLShaderProgram();
    }
    return d.program;
}

bool VideoShader::update(VideoMaterial *material)
{
    if (!material)
        return false;
    if (!material->bind())
        return false;
    //material->unbind();
    const VideoFormat fmt(material->currentFormat());
    //format is out of date because we may use the same shader for different formats
    setVideoFormat(fmt);
    // uniforms begin
    program()->bind(); //glUseProgram(id). for glUniform
    // all texture ids should be binded when renderering even for packed plane!
    const int nb_planes = fmt.planeCount(); //number of texture id
    for (int i = 0; i < nb_planes; ++i) {
        // use glUniform1i to swap planes. swap uv: i => (3-i)%3
        // TODO: in shader, use uniform sample2D u_Texture[], and use glUniform1iv(u_Texture, 3, {...})
        program()->setUniformValue(textureLocation(i), (GLint)i);
    }
    if (nb_planes < textureLocationCount()) {
        for (int i = nb_planes; i < textureLocationCount(); ++i) {
            program()->setUniformValue(textureLocation(i), (GLint)(nb_planes - 1));
        }
    }
    //qDebug() << "color mat " << material->colorMatrix();
    program()->setUniformValue(colorMatrixLocation(), material->colorMatrix());
    if (bppLocation() >= 0)
        program()->setUniformValue(bppLocation(), (GLfloat)material->bpp());
    if (channelMapLocation() >= 0)
        program()->setUniformValue(channelMapLocation(), material->channelMap());
    //program()->setUniformValue(matrixLocation(), material->matrix()); //what about sgnode? state.combindMatrix()?
    // uniform end. attribute begins
    return true;
}

QByteArray VideoShader::shaderSourceFromFile(const QString &fileName) const
{
    QFile f(qApp->applicationDirPath() + QStringLiteral("/") + fileName);
    if (!f.exists()) {
        f.setFileName(QStringLiteral(":/") + fileName);
    }
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Can not load shader %s: %s", f.fileName().toUtf8().constData(), f.errorString().toUtf8().constData());
        return QByteArray();
    }
    QByteArray src = f.readAll();
    f.close();
    return src;
}

void VideoShader::compile(QOpenGLShaderProgram *shaderProgram)
{
    Q_ASSERT_X(!shaderProgram->isLinked(), "VideoShader::compile()", "Compile called multiple times!");
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader());
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader());
    int maxVertexAttribs = 0;
    DYGL(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs));
    char const *const *attr = attributeNames();
    for (int i = 0; attr[i]; ++i) {
        if (i >= maxVertexAttribs) {
            qFatal("List of attribute names is either too long or not null-terminated.\n"
                   "Maximum number of attributes on this hardware is %i.\n"
                   "Vertex shader:\n%s\n"
                   "Fragment shader:\n%s\n",
                   maxVertexAttribs, vertexShader(), fragmentShader());
        }
        // why must min location == 0?
        if (*attr[i]) {
            shaderProgram->bindAttributeLocation(attr[i], i);
            qDebug("bind attribute: %s => %d", attr[i], i);
        }
    }

    if (!shaderProgram->link()) {
        qWarning("QSGMaterialShader: Shader compilation failed:");
        qWarning() << shaderProgram->log();
    }
}

VideoMaterial::VideoMaterial()
{
}

void VideoMaterial::setCurrentFrame(const VideoFrame &frame)
{
    DPTR_D(VideoMaterial);
    d.update_texure = true;
    // TODO: move to another function before rendering?
    d.width = frame.width();
    d.height = frame.height();
    GLenum new_target = GL_TEXTURE_2D; // not d.target. because metadata "target" is not always set
    QByteArray t = frame.metaData(QStringLiteral("target")).toByteArray().toLower();
    if (t == QByteArrayLiteral("rect"))
        new_target = GL_TEXTURE_RECTANGLE;
    if (new_target != d.target) {
        // FIXME: not thread safe (in qml)
        d.target = new_target;
        d.init_textures_required = true;
    }

    const VideoFormat fmt(frame.format());
    d.bpp = fmt.bitsPerPixel(0);
    // http://forum.doom9.org/archive/index.php/t-160211.html
    ColorSpace cs = frame.colorSpace();// ColorSpace_RGB;
    if (cs == ColorSpace_Unknow) {
        if (fmt.isRGB()) {
            if (fmt.isPlanar())
                cs = ColorSpace_GBR;
            else
                cs = ColorSpace_RGB;
        } else {
            if (frame.width() >= 1280 || frame.height() > 576) //values from mpv
                cs = ColorSpace_BT709;
            else
                cs = ColorSpace_BT601;
        }
    }
    d.colorTransform.setInputColorSpace(cs);
    d.frame = frame;
    if (fmt != d.video_format) {
        qDebug("pixel format changed: %s => %s", qPrintable(d.video_format.name()), qPrintable(fmt.name()));
        d.video_format = fmt;
    }
}

VideoFormat VideoMaterial::currentFormat() const
{
    DPTR_D(const VideoMaterial);
    return d.video_format;
}

VideoShader* VideoMaterial::createShader() const
{
    DPTR_D(const VideoMaterial);
    VideoShader *shader = new VideoShader();
    shader->setVideoFormat(d.video_format);
    shader->setTextureTarget(d.target);
    //resize texture locations to avoid access format later
    return shader;
}

const char *VideoMaterial::type() const
{
    DPTR_D(const VideoMaterial);
    const VideoFormat &fmt = d.video_format;
    const bool tex_2d = d.target == GL_TEXTURE_2D;
    if (!fmt.isPlanar()) {
        if (fmt.isRGB()) {
            if (tex_2d)
                return "packed rgb material";
            return "packed rgb + rectangle texture material";
        }
        if (tex_2d)
            return "packed yuv material";
        return "packed yuv + rectangle texture material";
    }
    if (fmt.bytesPerPixel(0) == 1) {
        if (fmt.planeCount() == 4) {
            if (tex_2d)
                return "8bit 4plane yuv material";
            return "8bit 4plane yuv + rectangle texture material";
        }
        if (tex_2d)
            return "8bit yuv material";
        return "8bit yuv + rectangle texture material";
    }
    if (fmt.isBigEndian()) {
        if (fmt.planeCount() == 4) {
            if (tex_2d)
                return "4plane 16bit-be material";
            return "4plane 16bit-be + rectangle texture material";
        }
        if (tex_2d)
            return "planar 16bit-be material";
        return "planar 16bit-be + rectangle texture material";
    } else {
        if (fmt.planeCount() == 4) {
            if (tex_2d)
                return "4plane 16bit-le material";
            return "4plane 16bit-le + rectangle texture material";
        }
        if (tex_2d)
            return "planar 16bit-le material";
        return "planar 16bit-le + rectangle texture material";
    }
    return "invalid material";
}

bool VideoMaterial::bind()
{
    DPTR_D(VideoMaterial);
    if (!d.ensureResources())
        return false;
    const int nb_planes = d.textures.size(); //number of texture id
    if (nb_planes <= 0)
        return false;
    if (nb_planes > 4) //why?
        return false;
    d.ensureTextures();
    for (int i = 0; i < nb_planes; ++i) {
        const int p = (i + 1) % nb_planes; //0 must active at last?
        bindPlane(p, d.update_texure); // why? i: quick items display wrong textures
    }
#if 0 //move to unbind should be fine
    if (d.update_texure) {
        d.update_texure = false;
        d.frame = VideoFrame(); //FIXME: why need this? we must unmap correctly before frame is reset.
    }
#endif
    return true;
}

// TODO: move bindPlane to d.uploadPlane
void VideoMaterial::bindPlane(int p, bool updateTexture)
{
    DPTR_D(VideoMaterial);
    GLuint &tex = d.textures[p];
    OpenGLHelper::glActiveTexture(GL_TEXTURE0 + p); //0 must active?
    if (!updateTexture) {
        DYGL(glBindTexture(d.target, tex));
        return;
    }
    // try_pbo ? pbo_id : 0. 0= > interop.createHandle
    if (d.frame.map(GLTextureSurface, &tex, p)) {
        DYGL(glBindTexture(d.target, tex)); // glActiveTexture was called, but maybe bind to 0 in map
        return;
    }
    // FIXME: why happens on win?
    if (d.frame.bytesPerLine(p) <= 0)
        return;
    if (d.try_pbo) {
        //qDebug("bind PBO %d", p);
        QOpenGLBuffer &pb = d.pbo[p];
        pb.bind();
        // glMapBuffer() causes sync issue.
        // Call glBufferData() with NULL pointer before glMapBuffer(), the previous data in PBO will be discarded and
        // glMapBuffer() returns a new allocated pointer or an unused block immediately even if GPU is still working with the previous data.
        // https://www.opengl.org/wiki/Buffer_Object_Streaming#Buffer_re-specification
        pb.allocate(pb.size());
        GLubyte* ptr = (GLubyte*)pb.map(QOpenGLBuffer::WriteOnly);
        if (ptr) {
            memcpy(ptr, d.frame.constBits(p), pb.size());
            pb.unmap();
        }
    }
    //qDebug("bpl[%d]=%d width=%d", p, frame.bytesPerLine(p), frame.planeWidth(p));
    DYGL(glBindTexture(d.target, tex));
    //d.setupQuality();
    // This is necessary for non-power-of-two textures
    DYGL(glTexParameteri(d.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    DYGL(glTexParameteri(d.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    // TODO: data address use surfaceinterop.map()
    DYGL(glTexSubImage2D(d.target, 0, 0, 0, d.texture_upload_size[p].width(), d.texture_upload_size[p].height(), d.data_format[p], d.data_type[p], d.try_pbo ? 0 : d.frame.constBits(p)));
    //DYGL(glBindTexture(d.target, 0)); // no bind 0 because glActiveTexture was called
    if (d.try_pbo) {
        d.pbo[p].release();
    }
}

int VideoMaterial::compare(const VideoMaterial *other) const
{
    DPTR_D(const VideoMaterial);
    for (int i = 0; i < d.textures.size(); ++i) {
        const int diff = d.textures[i] - other->d_func().textures[i]; //TODO
        if (diff)
            return diff;
    }
    return d.bpp - other->bpp();
}

bool VideoMaterial::hasAlpha() const
{
    return d_func().video_format.hasAlpha();
}

void VideoMaterial::unbind()
{
    DPTR_D(VideoMaterial);
    const int nb_planes = d.textures.size(); //number of texture id
    for (int i = 0; i < nb_planes; ++i) {
        // unbind planes in the same order as bind. GPU frame's unmap() can be async works, assume the work finished earlier if it started in map() earlier, thus unbind order matter
        const int p = (i + 1) % nb_planes; //0 must active at last?
        d.frame.unmap(&d.textures[p]);
    }
    if (d.update_texure) {
        d.update_texure = false;
        d.frame = VideoFrame(); //FIXME: why need this? we must unmap correctly before frame is reset.
    }
}

const QMatrix4x4& VideoMaterial::colorMatrix() const
{
    return d_func().colorTransform.matrixRef();
}

const QMatrix4x4& VideoMaterial::matrix() const
{
    return d_func().matrix;
}

const QMatrix4x4 &VideoMaterial::channelMap() const
{
    return d_func().channel_map;
}

int VideoMaterial::bpp() const
{
    return d_func().bpp;
}

int VideoMaterial::planeCount() const
{
    return d_func().frame.planeCount();
}

void VideoMaterial::setBrightness(qreal value)
{
    d_func().colorTransform.setBrightness(value);
}

void VideoMaterial::setContrast(qreal value)
{
    d_func().colorTransform.setContrast(value);
}

void VideoMaterial::setHue(qreal value)
{
    d_func().colorTransform.setHue(value);
}

void VideoMaterial::setSaturation(qreal value)
{
    d_func().colorTransform.setSaturation(value);
}

qreal VideoMaterial::validTextureWidth() const
{
    return d_func().effective_tex_width_ratio;
}

QSize VideoMaterial::frameSize() const
{
    return QSize(d_func().width, d_func().height);
}
QRectF VideoMaterial::normalizedROI(const QRectF &roi) const
{
    return mapToTexture(0, roi, 1);
}

QPointF VideoMaterial::mapToTexture(int plane, const QPointF &p, int normalize) const
{
    if (p.isNull())
        return p;
    DPTR_D(const VideoMaterial);
    float x = p.x();
    float y = p.y();
    const qreal tex0W = d.texture_size[0].width();
    const qreal s = tex0W/qreal(d.width); // only apply to unnormalized input roi
    if (normalize < 0)
        normalize = d.target != GL_TEXTURE_RECTANGLE;
    if (normalize) {
        if (qAbs(x) > 1) {
            x /= (float)tex0W;
            x *= s;
        }
        if (qAbs(y) > 1)
            y /= (float)d.height;
    } else {
        if (qAbs(x) <= 1)
            x *= (float)tex0W;
        else
            x *= s;
        if (qAbs(y) <= 1)
            y *= (float)d.height;
    }
    // multiply later because we compare with 1 before it
    x *= d.effective_tex_width_ratio;
    const qreal pw = d.video_format.normalizedWidth(plane);
    const qreal ph = d.video_format.normalizedHeight(plane);
    return QPointF(x*pw, y*ph);
}

// mapToTexture
QRectF VideoMaterial::mapToTexture(int plane, const QRectF &roi, int normalize) const
{
    DPTR_D(const VideoMaterial);
    const qreal tex0W = d.texture_size[0].width();
    const qreal s = tex0W/qreal(d.width); // only apply to unnormalized input roi
    const qreal pw = d.video_format.normalizedWidth(plane);
    const qreal ph = d.video_format.normalizedHeight(plane);
    if (normalize < 0)
        normalize = d.target != GL_TEXTURE_RECTANGLE;
    if (!roi.isValid()) {
        if (normalize)
            return QRectF(0, 0, d.effective_tex_width_ratio, 1); //NOTE: not (0, 0, 1, 1)
        return QRectF(0, 0, tex0W*pw, d.height*ph);
    }
    float x = roi.x();
    float w = roi.width(); //TODO: texturewidth
    float y = roi.y();
    float h = roi.height();
    if (normalize) {
        if (qAbs(x) > 1) {
            x /= tex0W;
            x *= s;
        }
        if (qAbs(y) > 1)
            y /= (float)d.height;
        if (qAbs(w) > 1) {
            w /= tex0W;
            w *= s;
        }
        if (qAbs(h) > 1)
            h /= (float)d.height;
    } else { //FIXME: what about ==1?
        if (qAbs(x) <= 1)
            x *= tex0W;
        else
            x *= s;
        if (qAbs(y) <= 1)
            y *= (float)d.height;
        if (qAbs(w) <= 1)
            w *= tex0W;
        else
            w *= s;
        if (qAbs(h) <= 1)
            h *= (float)d.height;
    }
    // multiply later because we compare with 1 before it
    x *= d.effective_tex_width_ratio;
    w *= d.effective_tex_width_ratio;
    return QRectF(x*pw, y*ph, w*pw, h*ph);
}

bool VideoMaterialPrivate::initPBO(int plane, int size)
{
    QOpenGLBuffer &pb = pbo[plane];
    if (!pb.isCreated()) {
        qDebug("Creating PBO for plane %d, size: %d...", plane, size);
        pb.create();
    }
    if (!pb.bind()) {
        qWarning("Failed to bind PBO for plane %d!!!!!!", plane);
        try_pbo = false;
        return false;
    }
    qDebug("Allocate PBO size %d", size);
    pb.allocate(size);
    return true;
}

bool VideoMaterialPrivate::initTexture(GLuint tex, GLint internal_format, GLenum format, GLenum dataType, int width, int height)
{
    DYGL(glBindTexture(target, tex));
    setupQuality();
    // This is necessary for non-power-of-two textures
    DYGL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    DYGL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    DYGL(glTexImage2D(target, 0, internal_format, width, height, 0/*border, ES not support*/, format, dataType, NULL));
    DYGL(glBindTexture(target, 0));
    return true;
}

VideoMaterialPrivate::~VideoMaterialPrivate()
{
    if (!textures.isEmpty()) {
        DYGL(glDeleteTextures(textures.size(), textures.data()));
    }
    for (int i = 0; i < pbo.size(); ++i)
        pbo[i].destroy();
}

bool VideoMaterialPrivate::updateTextureParameters(const VideoFormat& fmt)
{
    // isSupported(pixfmt)
    if (!fmt.isValid())
        return false;
    //http://www.berkelium.com/OpenGL/GDC99/internalformat.html
    //NV12: UV is 1 plane. 16 bits as a unit. GL_LUMINANCE4, 8, 16, ... 32?
    //GL_LUMINANCE, GL_LUMINANCE_ALPHA are deprecated in GL3, removed in GL3.1
    //replaced by GL_RED, GL_RG, GL_RGB, GL_RGBA? for 1, 2, 3, 4 channel image
    //http://www.gamedev.net/topic/634850-do-luminance-textures-still-exist-to-opengl/
    //https://github.com/kivy/kivy/issues/1738: GL_LUMINANCE does work on a Galaxy Tab 2. LUMINANCE_ALPHA very slow on Linux
     //ALPHA: vec4(1,1,1,A), LUMINANCE: (L,L,L,1), LUMINANCE_ALPHA: (L,L,L,A)
    /*
     * To support both planar and packed use GL_ALPHA and in shader use r,g,a like xbmc does.
     * or use Swizzle_mask to layout the channels: http://www.opengl.org/wiki/Texture#Swizzle_mask
     * GL ES2 support: GL_RGB, GL_RGBA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_ALPHA
     * http://stackoverflow.com/questions/18688057/which-opengl-es-2-0-texture-formats-are-color-depth-or-stencil-renderable
     */
    const int nb_planes = fmt.planeCount();
    internal_format.resize(nb_planes);
    data_format.resize(nb_planes);
    data_type.resize(nb_planes);
    if (!OpenGLHelper::videoFormatToGL(fmt, (GLint*)internal_format.constData(), (GLenum*)data_format.constData(), (GLenum*)data_type.constData(), &channel_map)) {
        qWarning() << "No OpenGL support for " << fmt;
        return false;
    }
    qDebug("///////////bpp %d", fmt.bytesPerPixel());
    /*!
     * GLES internal_format == data_format, GL_LUMINANCE_ALPHA is 2 bytes
     * so if NV12 use GL_LUMINANCE_ALPHA, YV12 use GL_ALPHA
     */
    if (fmt.bytesPerPixel(1) == 1 && nb_planes > 2) { // QtAV uses the same shader for planar and semi-planar yuv format
        internal_format[2] = data_format[2] = GL_ALPHA;
        if (nb_planes == 4)
            internal_format[3] = data_format[3] = GL_ALPHA; // vec4(,,,A)
    }
    for (int i = 0; i < nb_planes; ++i) {
        //qDebug("format: %#x GL_LUMINANCE_ALPHA=%#x", data_format[i], GL_LUMINANCE_ALPHA);
        if (fmt.bytesPerPixel(i) == 2 && nb_planes == 3) {
            //data_type[i] = GL_UNSIGNED_SHORT;
        }
        const int bpp_gl = OpenGLHelper::bytesOfGLFormat(data_format[i], data_type[i]);
        const int pad = std::ceil((qreal)(texture_size[i].width() - effective_tex_width[i])/(qreal)bpp_gl);
        texture_size[i].setWidth(std::ceil((qreal)texture_size[i].width()/(qreal)bpp_gl));
        texture_upload_size[i].setWidth(std::ceil((qreal)texture_upload_size[i].width()/(qreal)bpp_gl));
        effective_tex_width[i] /= bpp_gl; //fmt.bytesPerPixel(i);
        //effective_tex_width_ratio =
        qDebug("texture width: %d - %d = pad: %d. bpp(gl): %d", texture_size[i].width(), effective_tex_width[i], pad, bpp_gl);
    }

    /*
     * there are 2 fragment shaders: rgb and yuv.
     * only 1 texture for packed rgb. planar rgb likes yuv
     * To support both planar and packed yuv, and mixed yuv(NV12), we give a texture sample
     * for each channel. For packed, each (channel) texture sample is the same. For planar,
     * packed channels has the same texture sample.
     * But the number of actural textures we upload is plane count.
     * Which means the number of texture id equals to plane count
     */
    // always delete old textures otherwise old textures are not initialized with correct parameters
    if (textures.size() > nb_planes) {
        const int nb_delete = textures.size() - nb_planes;
        qDebug("delete %d textures", nb_delete);
        if (!textures.isEmpty()) {
            DYGL(glDeleteTextures(nb_delete, textures.data() + nb_planes));
        }
    }
    textures.resize(nb_planes);
    init_textures_required = true;
    return true;
}

bool VideoMaterialPrivate::ensureResources()
{
    if (!update_texure) //video frame is already uploaded and displayed
        return true;
    const VideoFormat &fmt = video_format;
    if (!fmt.isValid())
        return false;

    bool update_textures = init_textures_required;
    const int nb_planes = fmt.planeCount();
    // will this take too much time?
    const qreal wr = (qreal)frame.effectiveBytesPerLine(nb_planes-1)/(qreal)frame.bytesPerLine(nb_planes-1);
    const int linsize0 = frame.bytesPerLine(0);
    // effective size may change even if plane size not changed
    if (update_textures
            || !qFuzzyCompare(wr, effective_tex_width_ratio)
            || linsize0 != plane0Size.width() || frame.height() != plane0Size.height()
            || (plane1_linesize > 0 && frame.bytesPerLine(1) != plane1_linesize)) { // no need to check height if plane 0 sizes are equal?
        update_textures = true;
        //qDebug("---------------------update texture: %dx%d, %s", width, frame.height(), video_format.name().toUtf8().constData());
        texture_size.resize(nb_planes);
        texture_upload_size.resize(nb_planes);
        effective_tex_width.resize(nb_planes);
        for (int i = 0; i < nb_planes; ++i) {
            qDebug("plane linesize %d: padded = %d, effective = %d", i, frame.bytesPerLine(i), frame.effectiveBytesPerLine(i));
            qDebug("plane width %d: effective = %d", frame.planeWidth(i), frame.effectivePlaneWidth(i));
            qDebug("planeHeight %d = %d", i, frame.planeHeight(i));
            // we have to consider size of opengl format. set bytesPerLine here and change to width later
            texture_size[i] = QSize(frame.bytesPerLine(i), frame.planeHeight(i));
            texture_upload_size[i] = texture_size[i];
            effective_tex_width[i] = frame.effectiveBytesPerLine(i); //store bytes here, modify as width later
            // TODO: ratio count the GL_UNPACK_ALIGN?
            //effective_tex_width_ratio = qMin((qreal)1.0, (qreal)frame.effectiveBytesPerLine(i)/(qreal)frame.bytesPerLine(i));
        }
        plane1_linesize = 0;
        if (nb_planes > 1) {
            texture_size[0].setWidth(texture_size[1].width() * effective_tex_width[0]/effective_tex_width[1]);
            // height? how about odd?
            plane1_linesize = frame.bytesPerLine(1);
        }
        /*
          let wr[i] = valid_bpl[i]/bpl[i], frame from avfilter maybe wr[1] < wr[0]
          e.g. original frame plane 0: 720/768; plane 1,2: 360/384,
          filtered frame plane 0: 720/736, ... (16 aligned?)
         */
        effective_tex_width_ratio = (qreal)frame.effectiveBytesPerLine(nb_planes-1)/(qreal)frame.bytesPerLine(nb_planes-1);
        qDebug("effective_tex_width_ratio=%f", effective_tex_width_ratio);
        plane0Size.setWidth(linsize0);
        plane0Size.setHeight(frame.height());
    }
    if (update_textures) {
        updateTextureParameters(fmt);
        // check pbo support
        // TODO: complete pbo extension set
        try_pbo = try_pbo && OpenGLHelper::isPBOSupported();
        // check PBO support with bind() is fine, no need to check extensions
        if (try_pbo) {
            for (int i = 0; i < nb_planes; ++i) {
                qDebug("Init PBO for plane %d", i);
                if (!initPBO(i, frame.bytesPerLine(i)*frame.planeHeight(i))) {
                    qWarning("Failed to init PBO for plane %d", i);
                    break;
                }
            }
        }
    }
    return true;
}

bool VideoMaterialPrivate::ensureTextures()
{
    if (!init_textures_required)
        return true;
    // create in bindPlane loop will cause wrong texture binding
    const int nb_planes = video_format.planeCount();
    for (int p = 0; p < nb_planes; ++p) {
        GLuint &tex = textures[p];
        if (tex) { // can be 0 if resized to a larger size
            qDebug("deleting texture for plane %d (id=%u)", p, tex);
            DYGL(glDeleteTextures(1, &tex));
            tex = 0;
        }
        if (!tex) {
            qDebug("creating texture for plane %d", p);
            GLuint* handle = (GLuint*)frame.createInteropHandle(&tex, GLTextureSurface, p);
            if (handle) {
                tex = *handle;
            } else {
                DYGL(glGenTextures(1, &tex));
                initTexture(tex, internal_format[p], data_format[p], data_type[p], texture_size[p].width(), texture_size[p].height());
            }
            qDebug("texture for plane %d is created (id=%u)", p, tex);
        }
    }
    init_textures_required = false;
    return true;
}

void VideoMaterialPrivate::setupQuality()
{
    DYGL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    DYGL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
}

} //namespace QtAV
