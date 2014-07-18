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

#include "QtAV/OpenGLVideo.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>
#else
#include <QtOpenGL/QGLShaderProgram>
#include <QtOpenGL/QGLFunctions>
typedef QGLShaderProgram QOpenGLShaderProgram;
typedef QGLShader QOpenGLShader;
typedef QGLFunctions QOpenGLFunctions;
#endif
#include "QtAV/SurfaceInterop.h"
#include "QtAV/VideoFrame.h"
#include "QtAV/private/ShaderManager.h"

namespace QtAV {


class OpenGLVideoPrivate : public DPtrPrivate<OpenGLVideo>
{
public:
    OpenGLVideoPrivate()
        : material(new VideoMaterial())
    {}
    ~OpenGLVideoPrivate() {
        if (material) {
            delete material;
            material = 0;
        }
    }

    ShaderManager manager;
    VideoMaterial *material;
    QRect viewport;
    QRect out_rect;
    QMatrix4x4 matrix;
};

void OpenGLVideo::setCurrentFrame(const VideoFrame &frame)
{
    d_func().material->setCurrentFrame(frame);
}

void OpenGLVideo::setViewport(const QRect& rect)
{
    DPTR_D(OpenGLVideo);
    d.viewport = rect;
    d.matrix(0, 0) = (GLfloat)d.out_rect.width()/(GLfloat)d.viewport.width();
    d.matrix(1, 1) = (GLfloat)d.out_rect.height()/(GLfloat)d.viewport.height();
}

void OpenGLVideo::setVideoRect(const QRect &rect)
{
    DPTR_D(OpenGLVideo);
    d.out_rect = rect;
    if (!d.viewport.isValid())
        return;
    d.matrix(0, 0) = (GLfloat)d.out_rect.width()/(GLfloat)d.viewport.width();
    d.matrix(1, 1) = (GLfloat)d.out_rect.height()/(GLfloat)d.viewport.height();
}

void OpenGLVideo::render(const QRect &roi)
{
    DPTR_D(OpenGLVideo);
    glViewport(d.viewport.x(), d.viewport.y(), d.viewport.width(), d.viewport.height());
    VideoShader *shader = d.manager.prepareMaterial(d.material);
    shader->update(d.material);
    shader->program()->setUniformValue(shader->matrixLocation(), d.matrix);
    // uniform end. attribute begin
    const int kTupleSize = 2;
    GLfloat texCoords[kTupleSize*4];
    d.material->getTextureCoordinates(roi, texCoords);
    const GLfloat kVertices[] = {
        -1, 1,
        1, 1,
        1, -1,
        -1, -1,
    };
    shader->program()->setAttributeArray(0, GL_FLOAT, kVertices, kTupleSize);
    shader->program()->setAttributeArray(1, GL_FLOAT, texCoords, kTupleSize);

    char const *const *attr = shader->attributeNames();
    for (int i = 0; attr[i]; ++i) {
        shader->program()->enableAttributeArray(i); //TODO: in setActiveShader
    }

   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

   // d.shader->program()->release(); //glUseProgram(0)
   for (int i = 0; attr[i]; ++i) {
       shader->program()->disableAttributeArray(i); //TODO: in setActiveShader
   }

   d.material->unbind();
}


VideoShader::VideoShader()
    : m_color_space(ColorTransform::RGB)
    , u_MVP_matrix(-1)
    , u_colorMatrix(-1)
    , u_bpp(-1)
{
    //m_planar_frag = shaderSourceFromFile("shaders/planar.f.glsl");
    //m_packed_frag = shaderSourceFromFile("shaders/rgb.f.glsl");
}

VideoShader::~VideoShader()
{
    if (m_program) {
        m_program->removeAllShaders();
        delete m_program;
        m_program = 0;
    }
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
    // because we have to modify the shader, and shader source must be kept, so read the origin
    if (m_video_format.isPlanar()) {
        m_planar_frag = shaderSourceFromFile("shaders/planar.f.glsl");
    } else {
        m_packed_frag = shaderSourceFromFile("shaders/rgb.f.glsl");
    }
    QByteArray& frag = m_video_format.isPlanar() ? m_planar_frag : m_packed_frag;
    if (frag.isEmpty()) {
        qWarning("Empty fragment shader!");
        return 0;
    }
    if (m_video_format.isRGB()) {
        frag.prepend("#define INPUT_RGB\n");
    } else {
        switch (m_color_space) {
        case ColorTransform::BT601:
            frag.prepend("#define CS_BT601\n");
            break;
        case ColorTransform::BT709:
            frag.prepend("#define CS_BT709\n");
        default:
            break;
        }
        if (m_video_format.isPlanar() && m_video_format.bytesPerPixel(0) == 2) {
            if (m_video_format.isBigEndian())
                frag.prepend("#define YUV16BITS_BE_LUMINANCE_ALPHA\n");
            else
                frag.prepend("#define YUV16BITS_LE_LUMINANCE_ALPHA\n");
        }
        frag.prepend(QString("#define YUV%1P\n").arg(m_video_format.bitsPerPixel(0)).toUtf8());
    }
    return frag.constData();
}

void VideoShader::initialize(QOpenGLShaderProgram *shaderProgram)
{
    u_Texture.resize(textureLocationCount());
    if (!shaderProgram) {
        shaderProgram = program();
    }
    if (!shaderProgram->isLinked()) {
        compile(shaderProgram);
    }
    u_MVP_matrix = shaderProgram->uniformLocation("u_MVP_matrix");
    // fragment shader
    u_colorMatrix = shaderProgram->uniformLocation("u_colorMatrix");
    for (int i = 0; i < u_Texture.size(); ++i) {
        QString tex_var = QString("u_Texture%1").arg(i);
        u_Texture[i] = shaderProgram->uniformLocation(tex_var);
        qDebug("glGetUniformLocation(\"%s\") = %d\n", tex_var.toUtf8().constData(), u_Texture[i]);
    }
    qDebug("glGetUniformLocation(\"u_MVP_matrix\") = %d\n", u_MVP_matrix);
    qDebug("glGetUniformLocation(\"u_colorMatrix\") = %d\n", u_colorMatrix);
}

int VideoShader::textureLocationCount() const
{
    if (m_video_format.isRGB() && !m_video_format.isPlanar())
        return 1;
    return m_video_format.channels();
}

int VideoShader::textureLocation(int channel) const
{
    Q_ASSERT(channel < u_Texture.size());
    return u_Texture[channel];
}

int VideoShader::matrixLocation() const
{
    return u_MVP_matrix;
}

int VideoShader::colorMatrixLocation() const
{
    return u_colorMatrix;
}

int VideoShader::bppLocation() const
{
    return u_bpp;
}

VideoFormat VideoShader::videoFormat() const
{
    return m_video_format;
}

void VideoShader::setVideoFormat(const VideoFormat &format)
{
    m_video_format = format;
}

ColorTransform::ColorSpace VideoShader::colorSpace() const
{
    return m_color_space;
}

void VideoShader::setColorSpace(ColorTransform::ColorSpace cs)
{
    m_color_space = cs;
}

QOpenGLShaderProgram* VideoShader::program()
{
    if (!m_program)
        m_program = new QOpenGLShaderProgram();
    return m_program;
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
    /*
     * in Qt4 QMatrix4x4 stores qreal (double), while GLfloat may be float
     * QShaderProgram deal with this case. But compares sizeof(QMatrix4x4) and (GLfloat)*16
     * which seems not correct because QMatrix4x4 has a flag var
     */
    GLfloat *mat = (GLfloat*)material->colorTransform().matrixRef().data();
    GLfloat glm[16];
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    if (sizeof(qreal) != sizeof(GLfloat)) {
#else
    if (sizeof(float) != sizeof(GLfloat)) {
#endif
        material->colorTransform().matrixData(glm);
        mat = glm;
    }
    //QMatrix4x4 stores value in Column-major order to match OpenGL. so transpose is not required in glUniformMatrix4fv

   program()->setUniformValue(colorMatrixLocation(), material->colorTransform().matrixRef());
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


class VideoMaterialPrivate : public DPtrPrivate<VideoMaterial>
{
public:
    VideoMaterialPrivate()
        : supports_glsl(true)
        , update_texure(true)
        , bpp(1)
    {}
    ~VideoMaterialPrivate() {
        if (!textures.isEmpty())
            glDeleteTextures(textures.size(), textures.data());
    }

    bool supports_glsl;
    bool update_texure; // reduce upload/map times. true: new frame not bound. false: current frame is bound
    QRect viewport;
    QRect out_rect;
    VideoFrame frame;

    QVector<GLuint> textures; //texture ids. size is plane count
    QVector<QSize> texture_size;
    /*
     * actually if render a full frame, only plane 0 is enough. other planes are the same as texture size.
     * because linesize[0]>=linesize[1]
     * uploade size is required when
     * 1. y/u is not an integer because of alignment. then padding size of y < padding size of u, and effective size y/u != texture size y/u
     * 2. odd size. enlarge y
     */
    QVector<QSize> texture_upload_size;

    QVector<int> effective_tex_width; //without additional width for alignment
    qreal effective_tex_width_ratio;
    QVector<GLint> internal_format;
    QVector<GLenum> data_format;
    QVector<GLenum> data_type;

    QVector<GLfloat> texture_coords;
    ColorTransform colorTransform;
    QMatrix4x4 matrix;
    int bpp;
};

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

const ColorTransform &VideoMaterial::colorTransform() const
{
    return d_func().colorTransform;
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
