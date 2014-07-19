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
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>

namespace QtAV {

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
    if (d.video_format.isRGB()) {
        frag.prepend("#define INPUT_RGB\n");
    } else {
        switch (d.color_space) {
        case ColorTransform::BT601:
            frag.prepend("#define CS_BT601\n");
            break;
        case ColorTransform::BT709:
            frag.prepend("#define CS_BT709\n");
        default:
            break;
        }
        if (d.video_format.isPlanar() && d.video_format.bytesPerPixel(0) == 2) {
            if (d.video_format.isBigEndian())
                frag.prepend("#define YUV16BITS_BE_LUMINANCE_ALPHA\n");
            else
                frag.prepend("#define YUV16BITS_LE_LUMINANCE_ALPHA\n");
        }
        frag.prepend(QString("#define YUV%1P\n").arg(d.video_format.bitsPerPixel(0)).toUtf8());
    }
    return frag.constData();
}

void VideoShader::initialize(QOpenGLShaderProgram *shaderProgram)
{
    DPTR_D(VideoShader);
    d.u_Texture.resize(textureLocationCount());
    if (!shaderProgram) {
        shaderProgram = program();
    }
    if (!shaderProgram->isLinked()) {
        compile(shaderProgram);
    }
    d.u_MVP_matrix = shaderProgram->uniformLocation("u_MVP_matrix");
    // fragment shader
    d.u_colorMatrix = shaderProgram->uniformLocation("u_colorMatrix");
    for (int i = 0; i < d.u_Texture.size(); ++i) {
        QString tex_var = QString("u_Texture%1").arg(i);
        d.u_Texture[i] = shaderProgram->uniformLocation(tex_var);
        qDebug("glGetUniformLocation(\"%s\") = %d\n", tex_var.toUtf8().constData(), d.u_Texture[i]);
    }
    qDebug("glGetUniformLocation(\"u_MVP_matrix\") = %d\n", d.u_MVP_matrix);
    qDebug("glGetUniformLocation(\"u_colorMatrix\") = %d\n", d.u_colorMatrix);
}

int VideoShader::textureLocationCount() const
{
    DPTR_D(const VideoShader);
    if (d.video_format.isRGB() && !d.video_format.isPlanar())
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

VideoFormat VideoShader::videoFormat() const
{
    return d_func().video_format;
}

void VideoShader::setVideoFormat(const VideoFormat &format)
{
    d_func().video_format = format;
}
#if 0
ColorTransform::ColorSpace VideoShader::colorSpace() const
{
    return d.color_space;
}

void VideoShader::setColorSpace(ColorTransform::ColorSpace cs)
{
    d.color_space = cs;
}
#endif
QOpenGLShaderProgram* VideoShader::program()
{
    DPTR_D(VideoShader);
    if (!d.program)
        d.program = new QOpenGLShaderProgram();
    return d.program;
}

void VideoShader::update(VideoMaterial *material)
{
    material->bind();

    // uniforms begin
    program()->bind(); //glUseProgram(id). for glUniform
    // all texture ids should be binded when renderering even for packed plane!
    const int nb_planes = videoFormat().planeCount(); //number of texture id
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
    program()->setUniformValue(colorMatrixLocation(), material->colorMatrix());
    program()->setUniformValue(bppLocation(), (GLfloat)material->bpp());
    //program()->setUniformValue(matrixLocation(), material->matrix()); //what about sgnode? state.combindMatrix()?
    // uniform end. attribute begins
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
    Q_ASSERT_X(!shaderProgram.isLinked(), "VideoShader::compile()", "Compile called multiple times!");
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader());
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader());
    int maxVertexAttribs = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
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
    // TODO: lock?
    d.frame = frame;
    d.bpp = frame.format().bitsPerPixel(0);
    d.update_texure = true;
}

VideoShader* VideoMaterial::createShader() const
{
    VideoShader *shader = new VideoShader();
    shader->setVideoFormat(d_func().frame.format());
    //
    return shader;
}

MaterialType* VideoMaterial::type() const
{
    static MaterialType rgbType;
    static MaterialType yuv16leType;
    static MaterialType yuv16beType;
    static MaterialType yuv8Type;
    static MaterialType invalidType;
    const VideoFormat &fmt = d_func().frame.format();
    if (fmt.isRGB() && !fmt.isPlanar())
        return &rgbType;
    if (fmt.bytesPerPixel(0) == 1)
        return &yuv8Type;
    if (fmt.isBigEndian())
        return &yuv16beType;
    else
        return &yuv16leType;
    return &invalidType;
}

void VideoMaterial::bind()
{
    DPTR_D(VideoMaterial);
    const int nb_planes = d.textures.size(); //number of texture id
    if (d.update_texure) {
        for (int i = 0; i < nb_planes; ++i) {
            bindPlane(i);
        }
        d.update_texure = false;
        return;
    }
    if (d.textures.isEmpty())
        return;
    QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();
    for (int i = 0; i < nb_planes; ++i) {
        functions->glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, d.textures[i]);
    }
}

void VideoMaterial::bindPlane(int p)
{
    DPTR_D(VideoMaterial);
    QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();
    //setupQuality?
    if (d.frame.map(GLTextureSurface, &d.textures[p])) {
        functions->glActiveTexture(GL_TEXTURE0 + p);
        glBindTexture(GL_TEXTURE_2D, d.textures[p]);
        return;
    }
    // FIXME: why happens on win?
    if (d.frame.bytesPerLine(p) <= 0)
        return;
    functions->glActiveTexture(GL_TEXTURE0 + p);
    glBindTexture(GL_TEXTURE_2D, d.textures[p]);
    //setupQuality();
    //qDebug("bpl[%d]=%d width=%d", p, frame.bytesPerLine(p), frame.planeWidth(p));
    // This is necessary for non-power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexSubImage2D(GL_TEXTURE_2D
                 , 0                //level
                 , 0                // xoffset
                 , 0                // yoffset
                 , d.texture_upload_size[p].width()
                 , d.texture_upload_size[p].height()
                 , d.data_format[p]          //format, must the same as internal format?
                 , d.data_type[p]
                 , d.frame.bits(p));
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

void VideoMaterial::getTextureCoordinates(const QRect& roi, float* t)
{
    DPTR_D(VideoMaterial);
    /*!
      tex coords: ROI/frameRect()*effective_tex_width_ratio
    */
    t[0] = (GLfloat)roi.x()*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.frame.width();
    t[1] = (GLfloat)roi.y()/(GLfloat)d.frame.height();
    t[2] = (GLfloat)(roi.x() + roi.width())*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.frame.width();
    t[3] = (GLfloat)roi.y()/(GLfloat)d.frame.height();
    t[4] = (GLfloat)(roi.x() + roi.width())*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.frame.width();
    t[5] = (GLfloat)(roi.y()+roi.height())/(GLfloat)d.frame.height();
    t[6] = (GLfloat)roi.x()*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.frame.width();
    t[7] = (GLfloat)(roi.y()+roi.height())/(GLfloat)d.frame.height();
}

void VideoMaterial::setupQuality()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

} //namespace QtAV
