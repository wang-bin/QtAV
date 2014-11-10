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

#include "QtAV/VideoShader.h"
#include "QtAV/private/VideoShader_p.h"
#include "QtAV/ColorTransform.h"
#include "utils/OpenGLHelper.h"
#include <cmath>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include "utils/Logger.h"

#define YUVA_DONE 0
/*
 * TODO: glActiveTexture for Qt4
 * texture target (rectangle for VDA)
 */

namespace QtAV {


TexturedGeometry::TexturedGeometry(int count, Triangle t)
    : tri(t)
{
    v.resize(count);
}

int TexturedGeometry::mode() const
{
    if (tri == Strip)
        return GL_TRIANGLE_STRIP;
    return GL_TRIANGLE_FAN;
}

void TexturedGeometry::setPoint(int index, const QPointF &p, const QPointF &tp)
{
    setGeometryPoint(index, p);
    setTexturePoint(index, tp);
}

void TexturedGeometry::setGeometryPoint(int index, const QPointF &p)
{
    v[index].x = p.x();
    v[index].y = p.y();
}

void TexturedGeometry::setTexturePoint(int index, const QPointF &tp)
{
    v[index].tx = tp.x();
    v[index].ty = tp.y();
}

void TexturedGeometry::setRect(const QRectF &r, const QRectF &tr)
{
    setPoint(0, r.topLeft(), tr.topLeft());
    setPoint(1, r.bottomLeft(), tr.bottomLeft());
    if (tri == Strip) {
        setPoint(2, r.topRight(), tr.topRight());
        setPoint(3, r.bottomRight(), tr.bottomRight());
    } else {
        setPoint(3, r.topRight(), tr.topRight());
        setPoint(2, r.bottomRight(), tr.bottomRight());
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
        "a_TexCoords",
        0
    };
    return names;
}

const char* VideoShader::vertexShader() const
{
    static const char kVertexShader[] =
        "attribute vec4 a_Position;\n"
        "attribute vec2 a_TexCoords;\n"
        "uniform mat4 u_MVP_matrix;\n"
        "varying vec2 v_TexCoords;\n"
        "void main() {\n"
        "  gl_Position = u_MVP_matrix * a_Position;\n"
        "  v_TexCoords = a_TexCoords; \n"
        "}\n";
    return kVertexShader;
}

const char* VideoShader::fragmentShader() const
{
    DPTR_D(const VideoShader);
    // because we have to modify the shader, and shader source must be kept, so read the origin
    if (d.video_format.isPlanar()) {
        d.planar_frag = shaderSourceFromFile("shaders/planar.f.glsl");
    } else {
        d.packed_frag = shaderSourceFromFile("shaders/rgb.f.glsl");
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
    if (d.video_format.isPlanar() && d.video_format.bytesPerPixel(0) == 2) {
        if (d.video_format.isBigEndian())
            frag.prepend("#define LA_16BITS_BE\n");
        else
            frag.prepend("#define LA_16BITS_LE\n");
    }
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
    d.u_Texture.resize(textureLocationCount());
    for (int i = 0; i < d.u_Texture.size(); ++i) {
        QString tex_var = QString("u_Texture%1").arg(i);
        d.u_Texture[i] = shaderProgram->uniformLocation(tex_var);
        qDebug("glGetUniformLocation(\"%s\") = %d", tex_var.toUtf8().constData(), d.u_Texture[i]);
    }
    qDebug("glGetUniformLocation(\"u_MVP_matrix\") = %d", d.u_MVP_matrix);
    qDebug("glGetUniformLocation(\"u_colorMatrix\") = %d", d.u_colorMatrix);
    qDebug("glGetUniformLocation(\"u_bpp\") = %d", d.u_bpp);
    qDebug("glGetUniformLocation(\"u_opacity\") = %d", d.u_opacity);
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
    //program()->setUniformValue(matrixLocation(), material->matrix()); //what about sgnode? state.combindMatrix()?
    // uniform end. attribute begins
    return true;
}

QByteArray VideoShader::shaderSourceFromFile(const QString &fileName) const
{
    QFile f(qApp->applicationDirPath() + "/" + fileName);
    if (!f.exists()) {
        f.setFileName(":/" + fileName);
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
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
#ifndef QT_OPENGL_DYNAMIC
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
#else
    QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
#endif
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
        if (*attr[i])
            shaderProgram->bindAttributeLocation(attr[i], i);
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
    const VideoFormat fmt(frame.format());
    d.bpp = fmt.bitsPerPixel(0);
    // http://forum.doom9.org/archive/index.php/t-160211.html
    ColorTransform::ColorSpace cs = ColorTransform::RGB;
    if (fmt.isRGB()) {
        if (fmt.isPlanar())
            cs = ColorTransform::GBR;
    } else {
        if (frame.width() >= 1280 || frame.height() > 576) //values from mpv
            cs = ColorTransform::BT709;
        else
            cs = ColorTransform::BT601;
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
    //resize texture locations to avoid access format later
    return shader;
}

MaterialType* VideoMaterial::type() const
{
    static MaterialType rgbType;
    static MaterialType packedType; // TODO: uyuy, yuy2
    static MaterialType planar16leType;
    static MaterialType planar16beType;
    static MaterialType yuv8Type;
    static MaterialType planar16le_4plane_Type;
    static MaterialType planar16be_4plane_Type;
    static MaterialType yuv8_4plane_Type;

    static MaterialType invalidType;
    const VideoFormat &fmt = d_func().video_format;
    if (fmt.isRGB() && !fmt.isPlanar())
        return &rgbType;
    if (!fmt.isPlanar())
        return &packedType;
    if (fmt.bytesPerPixel(0) == 1) {
        if (fmt.planeCount() == 4)
            return &yuv8_4plane_Type;
        return &yuv8Type;
    }
    if (fmt.isBigEndian()) {
        if (fmt.planeCount() == 4)
            return &planar16be_4plane_Type;
        return &planar16beType;
    } else {
        if (fmt.planeCount() == 4)
            return &planar16le_4plane_Type;
        return &planar16leType;
    }
    return &invalidType;
}

bool VideoMaterial::bind()
{
    DPTR_D(VideoMaterial);
    if (!d.updateTexturesIfNeeded())
        return false;
    const int nb_planes = d.textures.size(); //number of texture id
    if (nb_planes <= 0)
        return false;
    if (nb_planes > 4) //why?
        return false;
    if (d.update_texure) {
        for (int i = 0; i < nb_planes; ++i) {
            bindPlane((i + 1) % nb_planes); // why? i: quick items display wrong textures
        }
        d.update_texure = false;
        return true;
    }
    for (int i = 0; i < nb_planes; ++i) {
        const int p = (i + 1) % nb_planes; // why? i: quick items display wrong textures
        OpenGLHelper::glActiveTexture(GL_TEXTURE0 + p);
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
#ifndef QT_OPENGL_DYNAMIC
        glBindTexture(GL_TEXTURE_2D, d.textures[p]);
#else
        QOpenGLContext::currentContext()->functions()->glBindTexture(GL_TEXTURE_2D, d.textures[p]);
#endif
    }
    return true;
}

void VideoMaterial::bindPlane(int p)
{
    DPTR_D(VideoMaterial);
    //setupQuality?
    if (d.frame.map(GLTextureSurface, &d.textures[p])) {
        OpenGLHelper::glActiveTexture(GL_TEXTURE0 + p);
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
#ifndef QT_OPENGL_DYNAMIC
        glBindTexture(GL_TEXTURE_2D, d.textures[p]);
#else
        QOpenGLContext::currentContext()->functions()->glBindTexture(GL_TEXTURE_2D, d.textures[p]);
#endif
        return;
    }
    // FIXME: why happens on win?
    if (d.frame.bytesPerLine(p) <= 0)
        return;
    OpenGLHelper::glActiveTexture(GL_TEXTURE0 + p);
    //qDebug("bpl[%d]=%d width=%d", p, frame.bytesPerLine(p), frame.planeWidth(p));
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
#ifndef QT_OPENGL_DYNAMIC
    glBindTexture(GL_TEXTURE_2D, d.textures[p]);
    //d.setupQuality();
    // This is necessary for non-power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, d.texture_upload_size[p].width(), d.texture_upload_size[p].height(), d.data_format[p], d.data_type[p], d.frame.bits(p));
#else
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glBindTexture(GL_TEXTURE_2D, d.textures[p]);
    //d.setupQuality();
    // This is necessary for non-power-of-two textures
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, d.texture_upload_size[p].width(), d.texture_upload_size[p].height(), d.data_format[p], d.data_type[p], d.frame.bits(p));
#endif
}

int VideoMaterial::compare(const VideoMaterial *other) const
{
    DPTR_D(const VideoMaterial);
    for (int i = 0; i < d.textures.size(); ++i) {
        const int diff = d.textures[i] - other->d_func().textures[i];
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
    for (int i = 0; i < d.textures.size(); ++i) {
        d.frame.unmap(&d.textures[i]);
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

QRectF VideoMaterial::normalizedROI(const QRectF &roi) const
{
    DPTR_D(const VideoMaterial);
    if (!roi.isValid())
        return QRectF(0, 0, 1, 1);
    float x = roi.x();
    float w = roi.width();
    x *= d.effective_tex_width_ratio;
    w *= d.effective_tex_width_ratio;
    if (qAbs(x) > 1)
        x /= (float)d.width;
    float y = roi.y();
    if (qAbs(y) > 1)
        y /= (float)d.height;
    if (qAbs(w) > 1)
        w /= (float)d.width;
    float h = roi.height();
    if (qAbs(h) > 1)
        h /= (float)d.height;
    return QRectF(x, y, w, h);
}

bool VideoMaterialPrivate::initTexture(GLuint tex, GLint internal_format, GLenum format, GLenum dataType, int width, int height)
{
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
#ifndef QT_OPENGL_DYNAMIC
    glBindTexture(GL_TEXTURE_2D, tex);
    setupQuality();
    // This is necessary for non-power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0/*border, ES not support*/, format, dataType, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
#else
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glBindTexture(GL_TEXTURE_2D, tex);
    setupQuality();
    // This is necessary for non-power-of-two textures
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0/*border, ES not support*/, format, dataType, NULL);
    f->glBindTexture(GL_TEXTURE_2D, 0);
#endif

    return true;
}

bool VideoMaterialPrivate::initTextures(const VideoFormat& fmt)
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
    if (!fmt.isPlanar()) {
        GLint internal_fmt;
        GLenum data_fmt;
        GLenum data_t;
        if (!OpenGLHelper::videoFormatToGL(fmt, &internal_fmt, &data_fmt, &data_t)) {
            qWarning("no opengl format found: internal_fmt=%d, data_fmt=%d, data_t=%d", internal_fmt, data_fmt, data_t);
            qDebug() << fmt;
            return false;
        }
        internal_format = QVector<GLint>(fmt.planeCount(), internal_fmt);
        data_format = QVector<GLenum>(fmt.planeCount(), data_fmt);
        data_type = QVector<GLenum>(fmt.planeCount(), data_t);
        //glPixelStorei(GL_UNPACK_ALIGNMENT, fmt.bytesPerPixel());
        // TODO: if no alpha, data_fmt is not GL_BGRA. align at every upload?
    } else {
        internal_format.resize(fmt.planeCount());
        data_format.resize(fmt.planeCount());
        data_type = QVector<GLenum>(fmt.planeCount(), GL_UNSIGNED_BYTE);
        /*!
         * GLES internal_format == data_format, GL_LUMINANCE_ALPHA is 2 bytes
         * so if NV12 use GL_LUMINANCE_ALPHA, YV12 use GL_ALPHA
         */
        qDebug("///////////bpp %d", fmt.bytesPerPixel());
        internal_format[0] = data_format[0] = GL_LUMINANCE; //or GL_RED for GL
        if (fmt.planeCount() == 2) {
            // NV12/21 semi-planar
            internal_format[1] = data_format[1] = GL_LUMINANCE_ALPHA;
        } else {
            if (fmt.bytesPerPixel(1) == 2) {
                // read 16 bits and compute the real luminance in shader
                internal_format.fill(GL_LUMINANCE_ALPHA); //vec4(L,L,L,A)
                data_format.fill(GL_LUMINANCE_ALPHA);
            } else {
                internal_format[1] = data_format[1] = GL_LUMINANCE; //vec4(L,L,L,1)
                internal_format[2] = data_format[2] = GL_ALPHA;//GL_ALPHA;
                if (fmt.planeCount() == 4)
                    internal_format[3] = data_format[3] = GL_ALPHA; //GL_ALPHA
            }
        }
    }
    for (int i = 0; i < fmt.planeCount(); ++i) {
        //qDebug("format: %#x GL_LUMINANCE_ALPHA=%#x", data_format[i], GL_LUMINANCE_ALPHA);
        if (fmt.bytesPerPixel(i) == 2 && fmt.planeCount() == 3) {
            //data_type[i] = GL_UNSIGNED_SHORT;
        }
        int bpp_gl = OpenGLHelper::bytesOfGLFormat(data_format[i], data_type[i]);
        int pad = std::ceil((qreal)(texture_size[i].width() - effective_tex_width[i])/(qreal)bpp_gl);
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
    if (textures.size() != fmt.planeCount()) {
        qDebug("delete %d textures", textures.size());
        if (!textures.isEmpty()) {
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
#ifndef QT_OPENGL_DYNAMIC
            glDeleteTextures(textures.size(), textures.data());
#else
            QOpenGLContext::currentContext()->functions()->glDeleteTextures(textures.size(), textures.data());
#endif
            textures.clear();
        }
        textures.resize(fmt.planeCount());
#ifndef QT_OPENGL_DYNAMIC
        glGenTextures(textures.size(), textures.data());
#else
        QOpenGLContext::currentContext()->functions()->glGenTextures(textures.size(), textures.data());
#endif
    }
    qDebug("init textures...");
    for (int i = 0; i < textures.size(); ++i) {
        initTexture(textures[i], internal_format[i], data_format[i], data_type[i], texture_size[i].width(), texture_size[i].height());
    }
    return true;
}

bool VideoMaterialPrivate::updateTexturesIfNeeded()
{
    const VideoFormat &fmt = video_format;
    if (!fmt.isValid())
        return false;
    bool update_textures = false;
    // effective size may change even if plane size not changed
    if (update_textures
            || frame.bytesPerLine(0) != plane0Size.width() || frame.height() != plane0Size.height()
            || (plane1_linesize > 0 && frame.bytesPerLine(1) != plane1_linesize)) { // no need to check height if plane 0 sizes are equal?
        update_textures = true;
        //qDebug("---------------------update texture: %dx%d, %s", width, frame.height(), video_format.name().toUtf8().constData());
        const int nb_planes = fmt.planeCount();
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
        effective_tex_width_ratio = (qreal)frame.effectiveBytesPerLine(nb_planes-1)/(qreal)frame.bytesPerLine(nb_planes-1);
        qDebug("effective_tex_width_ratio=%f", effective_tex_width_ratio);
        plane0Size.setWidth(frame.bytesPerLine(0));
        plane0Size.setHeight(frame.height());
    }
    if (update_textures) {
        initTextures(fmt);
    }
    return true;
}

void VideoMaterialPrivate::setupQuality()
{
#ifndef QT_OPENGL_DYNAMIC
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#else
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
}

} //namespace QtAV
