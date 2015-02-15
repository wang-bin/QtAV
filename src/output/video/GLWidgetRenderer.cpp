/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAVWidgets/GLWidgetRenderer.h"
#include "QtAV/private/VideoRenderer_p.h"
#include "utils/OpenGLHelper.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/qmath.h>
#include <QtCore/QVector>
#include <QResizeEvent>
#include <QtOpenGL/QGLShaderProgram>
#include <QtOpenGL/QGLShader>
#define NO_QGL_SHADER (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
//TODO: vsync http://stackoverflow.com/questions/589064/how-to-enable-vertical-sync-in-opengl
//TODO: check gl errors
//glEGLImageTargetTexture2DOES:http://software.intel.com/en-us/articles/using-opengl-es-to-accelerate-apps-with-legacy-2d-guis


//#ifdef GL_EXT_unpack_subimage
#ifndef GL_UNPACK_ROW_LENGTH
#ifdef GL_UNPACK_ROW_LENGTH_EXT
#define GL_UNPACK_ROW_LENGTH GL_UNPACK_ROW_LENGTH_EXT
#endif //GL_UNPACK_ROW_LENGTH_EXT
#endif //GL_UNPACK_ROW_LENGTH

#include "QtAV/ColorTransform.h"
#include "QtAV/FilterContext.h"

#define UPLOAD_ROI 0
#define ROI_TEXCOORDS 1

#define AVALIGN(x, a) (((x)+(a)-1)&~((a)-1))


//TODO: QGLfunctions?
namespace QtAV {

const GLfloat kVertices[] = {
    -1, 1,
    1, 1,
    1, -1,
    -1, -1,
};

static void checkGlError(const char* op = 0) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR)
        return;
    qWarning("GL error %s (%#x): %s", op, error, glGetString(error));
}

#define CHECK_GL_ERROR(FUNC) \
    FUNC; \
    checkGlError(#FUNC);

static const char kVertexShader[] =
    "attribute vec4 a_Position;\n"
    "attribute vec2 a_TexCoords;\n"
    "uniform mat4 u_MVP_matrix;\n"
    "varying vec2 v_TexCoords;\n"
    "void main() {\n"
    "  gl_Position = u_MVP_matrix * a_Position;\n"
    "  v_TexCoords = a_TexCoords; \n"
    "}\n";


class GLWidgetRendererPrivate : public VideoRendererPrivate
#if QTAV_HAVE(QGLFUNCTIONS)
        , public QGLFunctions
#endif //QTAV_HAVE(QGLFUNCTIONS)
{
public:
    GLWidgetRendererPrivate():
        VideoRendererPrivate()
      , hasGLSL(true)
      , update_texcoords(true)
      , effective_tex_width_ratio(1)
      , shader_program(0)
      , program(0)
      , vert(0)
      , frag(0)
      , a_Position(-1)
      , a_TexCoords(-1)
      , u_matrix(-1)
      , u_bpp(-1)
      , u_opacity(-1)
      , painter(0)
      , video_format(VideoFormat::Format_Invalid)
      , material_type(0)
    {
        if (QGLFormat::openGLVersionFlags() == QGLFormat::OpenGL_Version_None) {
            available = false;
            return;
        }
        colorTransform.setOutputColorSpace(ColorTransform::RGB);
    }
    ~GLWidgetRendererPrivate() {
        releaseShaderProgram();
        if (!textures.isEmpty()) {
            glDeleteTextures(textures.size(), textures.data());
            textures.clear();
        }
        if (shader_program) {
            delete shader_program;
            shader_program = 0;
        }
        if (painter) {
            delete painter;
            painter= 0;
        }
    }
    bool initWithContext(const QGLContext *ctx) {
        Q_UNUSED(ctx);
#if !NO_QGL_SHADER
        shader_program = new QGLShaderProgram(ctx, 0);
#endif
        return true;
    }

    GLuint loadShader(GLenum shaderType, const char* pSource);
    GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);
    bool releaseShaderProgram();
    QString getShaderFromFile(const QString& fileName);
    bool prepareShaderProgram(const VideoFormat& fmt, ColorTransform::ColorSpace cs);
    bool initTexture(GLuint tex, GLint internal_format, GLenum format, GLenum dataType, int width, int height);
    bool initTextures(const VideoFormat& fmt);
    void updateTexturesIfNeeded();
    void upload(const QRect& roi);
    void uploadPlane(int p, GLint internal_format, GLenum format, const QRect& roi);
    //GL 4.x: GL_FRAGMENT_SHADER_DERIVATIVE_HINT,GL_TEXTURE_COMPRESSION_HINT
    //GL_DONT_CARE(default), GL_FASTEST, GL_NICEST
    /*
     * it seems that only glTexParameteri matters when drawing an image
     * MAG_FILTER/MIN_FILTER combinations: http://gregs-blog.com/2008/01/17/opengl-texture-filter-parameters-explained/
     * TODO: understand what each parameter means. GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T
     * what is MIPMAP?
     */
    void setupQuality() {
        switch (quality) {
        case VideoRenderer::QualityBest:
            //FIXME: GL_LINEAR_MIPMAP_LINEAR+GL_LINEAR=white screen?
            //texture zoom out
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            //texture zoom in
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifndef QT_OPENGL_ES_2
            if (!hasGLSL) {
                glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
                glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
                //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
                glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
            }
#endif //QT_OPENGL_ES_2
            break;
        case VideoRenderer::QualityFastest:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifndef QT_OPENGL_ES_2
            if (!hasGLSL) {
                glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
                glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
                //glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
                glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
            }
#endif //QT_OPENGL_ES_2
            break;
        default:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //GL_NEAREST
#ifndef QT_OPENGL_ES_2
            if (!hasGLSL) {
                glHint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);
                glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
                //glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
                glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_DONT_CARE);
            }
#endif //QT_OPENGL_ES_2
            break;
        }
    }

    void setupAspectRatio() {
        mpv_matrix.setToIdentity();
        mpv_matrix.scale((GLfloat)out_rect.width()/(GLfloat)renderer_width, (GLfloat)out_rect.height()/(GLfloat)renderer_height, 1);
        if (orientation)
            mpv_matrix.rotate(orientation, 0, 0, 1); // Z axis
    }

    class VideoMaterialType {};
    VideoMaterialType* materialType(const VideoFormat& fmt) const {
        static VideoMaterialType rgbType;
        static VideoMaterialType packedType; // TODO: uyuy, yuy2
        static VideoMaterialType planar16leType;
        static VideoMaterialType planar16beType;
        static VideoMaterialType yuv8Type;
        static VideoMaterialType invalidType;
        if (fmt.isRGB() && !fmt.isPlanar())
            return &rgbType;
        if (!fmt.isPlanar())
            return &packedType;
        if (fmt.bytesPerPixel(0) == 1)
            return &yuv8Type;
        if (fmt.isBigEndian())
            return &planar16beType;
        else
            return &planar16leType;
        return &invalidType;
    }
    void updateShaderIfNeeded();

    bool hasGLSL;
    bool update_texcoords;
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
    QGLShaderProgram *shader_program;
    GLuint program;
    GLuint vert, frag;
    GLint a_Position;
    GLint a_TexCoords;
    QVector<GLint> u_Texture; //u_TextureN uniform. size is channel count
    GLint u_matrix;
    GLint u_colorMatrix;
    GLint u_bpp;
    GLint u_opacity;

    QPainter *painter;

    VideoFormat video_format;
    QSize plane0Size;
    // width is in bytes. different alignments may result in different plane 1 linesize even if plane 0 are the same
    int plane1_linesize;
    ColorTransform colorTransform;
    QMatrix4x4 mpv_matrix;
    VideoMaterialType *material_type;
};


//http://www.opengl.org/wiki/GLSL#Error_Checking
// TODO: use QGLShaderProgram for better compatiblity
GLuint GLWidgetRendererPrivate::loadShader(GLenum shaderType, const char* pSource) {
    if (!hasGLSL)
        return 0;
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_FALSE) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*)malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    qWarning("Could not compile shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

GLuint GLWidgetRendererPrivate::createProgram(const char* pVertexSource, const char* pFragmentSource) {
    if (!hasGLSL)
        return 0;
    program = glCreateProgram(); //TODO: var name conflict. temp var is better
    if (!program)
        return 0;
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }
    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        glDeleteShader(vertexShader);
        return 0;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);
    glLinkProgram(program);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLint bufLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
        if (bufLength) {
            char* buf = (char*)malloc(bufLength);
            if (buf) {
                glGetProgramInfoLog(program, bufLength, NULL, buf);
                qWarning("Could not link program:\n%s\n", buf);
                free(buf);
            }
        }
        glDetachShader(program, vertexShader);
        glDeleteShader(vertexShader);
        glDetachShader(program, pixelShader);
        glDeleteShader(pixelShader);
        glDeleteProgram(program);
        program = 0;
        return 0;
    }
    //Always detach shaders after a successful link.
    glDetachShader(program, vertexShader);
    glDetachShader(program, pixelShader);
    vert = vertexShader;
    frag = pixelShader;
    return program;
}

bool GLWidgetRendererPrivate::releaseShaderProgram()
{
    video_format.setPixelFormat(VideoFormat::Format_Invalid);
#if NO_QGL_SHADER
    if (vert) {
        if (program)
            glDetachShader(program, vert);
        glDeleteShader(vert);
    }
    if (frag) {
        if (program)
            glDetachShader(program, frag);
        glDeleteShader(frag);
    }
    if (program) {
        glDeleteProgram(program);
        program = 0;
    }
#else
    shader_program->removeAllShaders();
#endif
    return true;
}

QString GLWidgetRendererPrivate::getShaderFromFile(const QString &fileName)
{
    QFile f(qApp->applicationDirPath() + "/" + fileName);
    if (!f.exists()) {
        f.setFileName(":/" + fileName);
    }
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Can not load shader %s: %s", f.fileName().toUtf8().constData(), f.errorString().toUtf8().constData());
        return QString();
    }
    QString src = f.readAll();
    f.close();
    return src;
}

bool GLWidgetRendererPrivate::prepareShaderProgram(const VideoFormat &fmt, ColorTransform::ColorSpace cs)
{
    // isSupported(pixfmt)
    if (!fmt.isValid())
        return false;
    releaseShaderProgram();
    video_format.setPixelFormatFFmpeg(fmt.pixelFormatFFmpeg());
    colorTransform.setInputColorSpace(cs);
    // TODO: only to kinds, packed.glsl, planar.glsl
    QString frag;
    if (fmt.isPlanar()) {
        frag = getShaderFromFile("shaders/planar.f.glsl");
    } else {
        frag = getShaderFromFile("shaders/rgb.f.glsl");
    }
    if (frag.isEmpty())
        return false;
    if (fmt.isRGB()) {
        frag.prepend("#define INPUT_RGB\n");
    } else {
        frag.prepend(QString("#define YUV%1P\n").arg(fmt.bitsPerPixel(0)));
    }
    if (fmt.isPlanar() && fmt.bytesPerPixel(0) == 2) {
        if (fmt.isBigEndian())
            frag.prepend("#define LA_16BITS_BE\n");
        else
            frag.prepend("#define LA_16BITS_LE\n");
    }
    if (cs == ColorTransform::BT601) {
        frag.prepend("#define CS_BT601\n");
    } else if (cs == ColorTransform::BT709) {
        frag.prepend("#define CS_BT709\n");
    }
#if NO_QGL_SHADER
    program = createProgram(kVertexShader, frag.toUtf8().constData());
    if (!program) {
        qWarning("Could not create shader program.");
        return false;
    }
    // vertex shader. we can set attribute locations calling glBindAttribLocation
    a_Position = glGetAttribLocation(program, "a_Position");
    a_TexCoords = glGetAttribLocation(program, "a_TexCoords");
    u_matrix = glGetUniformLocation(program, "u_MVP_matrix");
    u_bpp = glGetUniformLocation(program, "u_bpp");
    u_opacity = glGetUniformLocation(program, "u_opacity");
    // fragment shader
    u_colorMatrix = glGetUniformLocation(program, "u_colorMatrix");
#else
    if (!shader_program->addShaderFromSourceCode(QGLShader::Vertex, kVertexShader)) {
        qWarning("Failed to add vertex shader: %s", shader_program->log().toUtf8().constData());
        return false;
    }
    if (!shader_program->addShaderFromSourceCode(QGLShader::Fragment, frag)) {
        qWarning("Failed to add fragment shader: %s", shader_program->log().toUtf8().constData());
        return false;
    }
    if (!shader_program->link()) {
        qWarning("Failed to link shader program...%s", shader_program->log().toUtf8().constData());
        return false;
    }
    // vertex shader
    a_Position = shader_program->attributeLocation("a_Position");
    a_TexCoords = shader_program->attributeLocation("a_TexCoords");
    u_matrix = shader_program->uniformLocation("u_MVP_matrix");
    u_bpp = shader_program->uniformLocation("u_bpp");
    u_opacity = shader_program->uniformLocation("u_opacity");
    // fragment shader
    u_colorMatrix = shader_program->uniformLocation("u_colorMatrix");
#endif //NO_QGL_SHADER
    qDebug("glGetAttribLocation(\"a_Position\") = %d\n", a_Position);
    qDebug("glGetAttribLocation(\"a_TexCoords\") = %d\n", a_TexCoords);
    qDebug("glGetUniformLocation(\"u_MVP_matrix\") = %d\n", u_matrix);
    qDebug("glGetUniformLocation(\"u_bpp\") = %d\n", u_bpp);
    qDebug("glGetUniformLocation(\"u_opacity\") = %d\n", u_opacity);
    qDebug("glGetUniformLocation(\"u_colorMatrix\") = %d\n", u_colorMatrix);

    if (fmt.isRGB())
        u_Texture.resize(1);
    else
        u_Texture.resize(fmt.channels());
    for (int i = 0; i < u_Texture.size(); ++i) {
        QString tex_var = QString("u_Texture%1").arg(i);
#if NO_QGL_SHADER
        u_Texture[i] = glGetUniformLocation(program, tex_var.toUtf8().constData());
#else
        u_Texture[i] = shader_program->uniformLocation(tex_var);
#endif
        qDebug("glGetUniformLocation(\"%s\") = %d\n", tex_var.toUtf8().constData(), u_Texture[i]);
    }
    return true;
}

bool GLWidgetRendererPrivate::initTexture(GLuint tex, GLint internalFormat, GLenum format, GLenum dataType, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, tex);
    setupQuality();
    // This is necessary for non-power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D
                 , 0                //level
                 , internalFormat               //internal format. 4? why GL_RGBA? GL_RGB?
                 , width
                 , height
                 , 0                //border, ES not support
                 , format          //format, must the same as internal format?
                 , dataType
                 , NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool GLWidgetRendererPrivate::initTextures(const VideoFormat &fmt)
{
    // isSupported(pixfmt)
    if (!fmt.isValid())
        return false;
    video_format.setPixelFormatFFmpeg(fmt.pixelFormatFFmpeg());

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
            qWarning("no opengl format found");
            return false;
        }
        internal_format = QVector<GLint>(fmt.planeCount(), internal_fmt);
        data_format = QVector<GLenum>(fmt.planeCount(), data_fmt);
        data_type = QVector<GLenum>(fmt.planeCount(), data_t);
    } else {
        internal_format.resize(fmt.planeCount());
        data_format.resize(fmt.planeCount());
        data_type = QVector<GLenum>(fmt.planeCount(), GL_UNSIGNED_BYTE);
        if (fmt.isPlanar()) {
        /*!
         * GLES internal_format == data_format, GL_LUMINANCE_ALPHA is 2 bytes
         * so if NV12 use GL_LUMINANCE_ALPHA, YV12 use GL_ALPHA
         */
            qDebug("///////////bpp %d", fmt.bytesPerPixel());
            internal_format[0] = data_format[0] = GL_LUMINANCE; //or GL_RED for GL
            if (fmt.planeCount() == 2) {
                internal_format[1] = data_format[1] = GL_LUMINANCE_ALPHA;
            } else {
                if (fmt.bytesPerPixel(1) == 2) {
                    // read 16 bits and compute the real luminance in shader
                    internal_format.fill(GL_LUMINANCE_ALPHA); //vec4(L,L,L,A)
                    data_format.fill(GL_LUMINANCE_ALPHA);
                } else {
                    internal_format[1] = data_format[1] = GL_LUMINANCE; //vec4(L,L,L,1)
                    internal_format[2] = data_format[2] = GL_ALPHA;//GL_ALPHA;
                }
            }
            for (int i = 0; i < internal_format.size(); ++i) {
                // xbmc use bpp not bpp(plane)
                //internal_format[i] = GetGLInternalFormat(data_format[i], fmt.bytesPerPixel(i));
                //data_format[i] = internal_format[i];
            }
        } else {
            //glPixelStorei(GL_UNPACK_ALIGNMENT, fmt.bytesPerPixel());
            // TODO: if no alpha, data_fmt is not GL_BGRA. align at every upload?
        }
    }
    for (int i = 0; i < fmt.planeCount(); ++i) {
        //qDebug("format: %#x GL_LUMINANCE_ALPHA=%#x", data_format[i], GL_LUMINANCE_ALPHA);
        if (fmt.bytesPerPixel(i) == 2 && fmt.planeCount() == 3) {
            //data_type[i] = GL_UNSIGNED_SHORT;
        }
        int bpp_gl = OpenGLHelper::bytesOfGLFormat(data_format[i], data_type[i]);
        int pad = qCeil((qreal)(texture_size[i].width() - effective_tex_width[i])/(qreal)bpp_gl);
        texture_size[i].setWidth(qCeil((qreal)texture_size[i].width()/(qreal)bpp_gl));
        texture_upload_size[i].setWidth(qCeil((qreal)texture_upload_size[i].width()/(qreal)bpp_gl));
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
        glDeleteTextures(textures.size(), textures.data());
        qDebug("delete %d textures", textures.size());
        textures.clear();
        textures.resize(fmt.planeCount());
        glGenTextures(textures.size(), textures.data());
    }

    if (!hasGLSL) {
        initTexture(textures[0], internal_format[0], data_format[0], data_type[0], texture_size[0].width(), texture_size[0].height());
        // more than 1?
        qWarning("Does not support GLSL!");
        return false;
    }
    qDebug("init textures...");
    for (int i = 0; i < textures.size(); ++i) {
        initTexture(textures[i], internal_format[i], data_format[i], data_type[i], texture_size[i].width(), texture_size[i].height());
    }
    return true;
}

void GLWidgetRendererPrivate::updateTexturesIfNeeded()
{
    const VideoFormat &fmt = video_frame.format();
    bool update_textures = false;
    if (fmt != video_format) {
        update_textures = true; //FIXME
        qDebug("updateTexturesIfNeeded pixel format changed: %s => %s", qPrintable(video_format.name()), qPrintable(fmt.name()));
    }
    // effective size may change even if plane size not changed
    if (update_textures
            || video_frame.bytesPerLine(0) != plane0Size.width() || video_frame.height() != plane0Size.height()
            || (plane1_linesize > 0 && video_frame.bytesPerLine(1) != plane1_linesize)) { // no need to check height if plane 0 sizes are equal?
        update_textures = true;
        qDebug("---------------------update texture: %dx%d, %s", video_frame.width(), video_frame.height(), video_frame.format().name().toUtf8().constData());
        const int nb_planes = fmt.planeCount();
        texture_size.resize(nb_planes);
        texture_upload_size.resize(nb_planes);
        effective_tex_width.resize(nb_planes);
        for (int i = 0; i < nb_planes; ++i) {
            qDebug("plane linesize %d: padded = %d, effective = %d", i, video_frame.bytesPerLine(i), video_frame.effectiveBytesPerLine(i));
            qDebug("plane width %d: effective = %d", video_frame.planeWidth(i), video_frame.effectivePlaneWidth(i));
            qDebug("planeHeight %d = %d", i, video_frame.planeHeight(i));
            // we have to consider size of opengl format. set bytesPerLine here and change to width later
            texture_size[i] = QSize(video_frame.bytesPerLine(i), video_frame.planeHeight(i));
            texture_upload_size[i] = texture_size[i];
            effective_tex_width[i] = video_frame.effectiveBytesPerLine(i); //store bytes here, modify as width later
            // TODO: ratio count the GL_UNPACK_ALIGN?
            //effective_tex_width_ratio = qMin((qreal)1.0, (qreal)video_frame.effectiveBytesPerLine(i)/(qreal)video_frame.bytesPerLine(i));
        }
        plane1_linesize = 0;
        if (nb_planes > 1) {
            texture_size[0].setWidth(texture_size[1].width() * effective_tex_width[0]/effective_tex_width[1]);
            // height? how about odd?
            plane1_linesize = video_frame.bytesPerLine(1);
        }
        effective_tex_width_ratio = (qreal)video_frame.effectiveBytesPerLine(nb_planes-1)/(qreal)video_frame.bytesPerLine(nb_planes-1);
        qDebug("effective_tex_width_ratio=%f", effective_tex_width_ratio);
        plane0Size.setWidth(video_frame.bytesPerLine(0));
        plane0Size.setHeight(video_frame.height());
    }
    if (update_textures) {
        initTextures(fmt);
    }
}

void GLWidgetRendererPrivate::updateShaderIfNeeded()
{
    const VideoFormat& fmt(video_frame.format());
    if (fmt != video_format) {
        qDebug("pixel format changed: %s => %s", qPrintable(video_format.name()), qPrintable(fmt.name()));
    }
    VideoMaterialType *newType = materialType(fmt);
    if (material_type == newType)
        return;
    material_type = newType;
    // http://forum.doom9.org/archive/index.php/t-160211.html
    ColorTransform::ColorSpace cs = ColorTransform::RGB;
    if (fmt.isRGB()) {
        if (fmt.isPlanar())
            cs = ColorTransform::GBR;
    } else {
        if (video_frame.width() >= 1280 || video_frame.height() > 576) //values from mpv
            cs = ColorTransform::BT709;
        else
            cs = ColorTransform::BT601;
    }
    if (!prepareShaderProgram(fmt, cs)) {
        qWarning("shader program create error...");
        return;
    } else {
        qDebug("shader program created!!!");
    }
}

void GLWidgetRendererPrivate::upload(const QRect &roi)
{
    for (int i = 0; i < video_frame.planeCount(); ++i) {
        uploadPlane(i, internal_format[i], data_format[i], roi);
    }
}

void GLWidgetRendererPrivate::uploadPlane(int p, GLint internalFormat, GLenum format, const QRect& roi)
{
    // FIXME: why happens on win?
    if (video_frame.bytesPerLine(p) <= 0)
        return;
    OpenGLHelper::glActiveTexture(GL_TEXTURE0 + p); //xbmc: only for es, not for desktop?
    glBindTexture(GL_TEXTURE_2D, textures[p]);
    ////nv12: 2
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);//GetAlign(video_frame.bytesPerLine(p)));
#if defined(GL_UNPACK_ROW_LENGTH)
//    glPixelStorei(GL_UNPACK_ROW_LENGTH, video_frame.bytesPerLine(p));
#endif
    //qDebug("bpl[%d]=%d width=%d", p, video_frame.bytesPerLine(p), video_frame.planeWidth(p));
    // This is necessary for non-power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //uploading part of image eats less gpu memory, but may be more cpu(gles)
    //FIXME: more cpu usage then qpainter. FBO, VBO?
    //roi for planes?
    if (ROI_TEXCOORDS || roi.size() == video_frame.size()) {
        glTexSubImage2D(GL_TEXTURE_2D
                     , 0                //level
                     , 0                // xoffset
                     , 0                // yoffset
                     , texture_upload_size[p].width()
                     , texture_upload_size[p].height()
                     , format          //format, must the same as internal format?
                     , data_type[p]
                     , video_frame.bits(p));
    } else {
        int roi_x = roi.x();
        int roi_y = roi.y();
        int roi_w = roi.width();
        int roi_h = roi.height();
        int plane_w = video_frame.planeWidth(p);
        VideoFormat fmt = video_frame.format();
        if (p == 0) {
            plane0Size = QSize(roi_w, roi_h); //
        } else {
            roi_x = fmt.chromaWidth(roi_x);
            roi_y = fmt.chromaHeight(roi_y);
            roi_w = fmt.chromaWidth(roi_w);
            roi_h = fmt.chromaHeight(roi_h);
        }
        qDebug("roi: %d, %d %dx%d", roi_x, roi_y, roi_w, roi_h);
#if 0// defined(GL_UNPACK_ROW_LENGTH) && defined(GL_UNPACK_SKIP_PIXELS)
// http://stackoverflow.com/questions/205522/opengl-subtexturing
        glPixelStorei(GL_UNPACK_ROW_LENGTH, plane_w);
        //glPixelStorei or compute pointer
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, roi_x);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, roi_y);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, roi_w, roi_h, 0, format, GL_UNSIGNED_BYTE, video_frame.bits(p));
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, roi_w, roi_h, format, GL_UNSIGNED_BYTE, video_frame.bits(p));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
#else // GL ES
//define it? or any efficient way?
        //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, roi_w, roi_h, 0, format, GL_UNSIGNED_BYTE, NULL);
        const char *src = (char*)video_frame.bits(p) + roi_y*plane_w + roi_x*fmt.bytesPerPixel(p);
#define UPLOAD_LINE 1
#if UPLOAD_LINE
        for (int y = 0; y < roi_h; y++) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, roi_w, 1, format, GL_UNSIGNED_BYTE, src);
        }
#else
        int line_size = roi_w*fmt.bytesPerPixel(p);
        char *sub = (char*)malloc(roi_h*line_size);
        char *dst = sub;
        for (int y = 0; y < roi_h; y++) {
            memcpy(dst, src, line_size);
            src += video_frame.bytesPerLine(p);
            dst += line_size;
        }
        // FIXME: crash
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, roi_w, roi_h, format, GL_UNSIGNED_BYTE, sub);
        free(sub);
#endif //UPLOAD_LINE
#endif //GL_UNPACK_ROW_LENGTH
    }
#if defined(GL_UNPACK_ROW_LENGTH)
//    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

GLWidgetRenderer::GLWidgetRenderer(QWidget *parent, const QGLWidget* shareWidget, Qt::WindowFlags f):
    QGLWidget(parent, shareWidget, f),VideoRenderer(*new GLWidgetRendererPrivate())
{
    DPTR_INIT_PRIVATE(GLWidgetRenderer);
    DPTR_D(GLWidgetRenderer);
    setPreferredPixelFormat(VideoFormat::Format_YUV420P);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    /* To rapidly update custom widgets that constantly paint over their entire areas with
     * opaque content, e.g., video streaming widgets, it is better to set the widget's
     * Qt::WA_OpaquePaintEvent, avoiding any unnecessary overhead associated with repainting the
     * widget's background
     */
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    //default: swap in qpainter dtor. we should swap before QPainter.endNativePainting()
    setAutoBufferSwap(false);
    setAutoFillBackground(false);
    d.painter = new QPainter();
    d.filter_context = VideoFilterContext::create(VideoFilterContext::QtPainter);
    d.filter_context->paint_device = this;
    d.filter_context->painter = d.painter;
}

VideoRendererId GLWidgetRenderer::id() const
{
    return VideoRendererId_GLWidget;
}

bool GLWidgetRenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    Q_UNUSED(pixfmt);
    return pixfmt != VideoFormat::Format_YUYV && pixfmt != VideoFormat::Format_UYVY;
}

bool GLWidgetRenderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(GLWidgetRenderer);
    d.video_frame = frame;

    update(); //can not call updateGL() directly because no event and paintGL() will in video thread
    return true;
}

bool GLWidgetRenderer::needUpdateBackground() const
{
    return true;
}

void GLWidgetRenderer::drawBackground()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLWidgetRenderer::drawFrame()
{
    DPTR_D(GLWidgetRenderer);
    d.updateTexturesIfNeeded();
    d.updateShaderIfNeeded();
    QRect roi = realROI();
    const int nb_planes = d.video_frame.planeCount(); //number of texture id
    int mapped = 0;
    for (int i = 0; i < nb_planes; ++i) {
        if (d.video_frame.map(GLTextureSurface, &d.textures[i])) {
            OpenGLHelper::glActiveTexture(GL_TEXTURE0 + i);
            // if mapped by SurfaceInterop, the texture may be not bound
            glBindTexture(GL_TEXTURE_2D, d.textures[i]); //we've bind 0 after upload()
            mapped++;
        }
    }
    if (mapped < nb_planes) {
        d.upload(roi);
    }
    //TODO: compute kTexCoords only if roi changed
#if ROI_TEXCOORDS
/*!
  tex coords: ROI/frameRect()*effective_tex_width_ratio
*/
    const GLfloat kTexCoords[] = {
        (GLfloat)roi.x()*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.video_frame.width(), (GLfloat)roi.y()/(GLfloat)d.video_frame.height(),
        (GLfloat)(roi.x() + roi.width())*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.video_frame.width(), (GLfloat)roi.y()/(GLfloat)d.video_frame.height(),
        (GLfloat)(roi.x() + roi.width())*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.video_frame.width(), (GLfloat)(roi.y()+roi.height())/(GLfloat)d.video_frame.height(),
        (GLfloat)roi.x()*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.video_frame.width(), (GLfloat)(roi.y()+roi.height())/(GLfloat)d.video_frame.height(),
    };
#else
    const GLfloat kTexCoords[] = {
            0, 0,
            1, 0,
            1, 1,
            0, 1,
    };
#endif //ROI_TEXCOORDS
    if (d.video_format.hasAlpha()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA);
    }
#ifndef QT_OPENGL_ES_2
    //GL_XXX may not defined in ES2. so macro is required
    if (!d.hasGLSL) {
        //qpainter will reset gl state, so need glMatrixMode and clear color(in drawBackground())
        //TODO: study what state will be reset
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glPushMatrix();
        glScalef((float)d.out_rect.width()/(float)d.renderer_width, (float)d.out_rect.height()/(float)d.renderer_height, 0);
        glVertexPointer(2, GL_FLOAT, 0, kVertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, kTexCoords);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
        glDisable(GL_BLEND);
        for (int i = 0; i < d.textures.size(); ++i) {
            d.video_frame.unmap(&d.textures[i]);
        }
        return;
    }
#endif //QT_OPENGL_ES_2
    // uniforms begin
    // shader program may not ready before upload
#if NO_QGL_SHADER
    glUseProgram(d.program); //for glUniform
#else
    d.shader_program->bind();
#endif //NO_QGL_SHADER
    // all texture ids should be binded when renderering even for packed plane!
    for (int i = 0; i < nb_planes; ++i) {
        // use glUniform1i to swap planes. swap uv: i => (3-i)%3
        // TODO: in shader, use uniform sample2D u_Texture[], and use glUniform1iv(u_Texture, 3, {...})
#if NO_QGL_SHADER
        glUniform1i(d.u_Texture[i], i);
#else
        d.shader_program->setUniformValue(d.u_Texture[i], (GLint)i);
#endif
    }
    if (nb_planes < d.u_Texture.size()) {
        for (int i = nb_planes; i < d.u_Texture.size(); ++i) {
#if NO_QGL_SHADER
            glUniform1i(d.u_Texture[i], nb_planes - 1);
#else
            d.shader_program->setUniformValue(d.u_Texture[i], (GLint)(nb_planes - 1));
#endif
        }
    }
    /*
     * in Qt4 QMatrix4x4 stores qreal (double), while GLfloat may be float
     * QShaderProgram deal with this case. But compares sizeof(QMatrix4x4) and (GLfloat)*16
     * which seems not correct because QMatrix4x4 has a flag var
     */
    GLfloat *mat = (GLfloat*)d.colorTransform.matrixRef().data();
    GLfloat glm[16];
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    if (sizeof(qreal) != sizeof(GLfloat)) {
#else
    if (sizeof(float) != sizeof(GLfloat)) {
#endif
        d.colorTransform.matrixData(glm);
        mat = glm;
    }
    //QMatrix4x4 stores value in Column-major order to match OpenGL. so transpose is not required in glUniformMatrix4fv
#if NO_QGL_SHADER
    glUniformMatrix4fv(d.u_colorMatrix, 1, GL_FALSE, mat);
    glUniformMatrix4fv(d.u_matrix, 1, GL_FALSE/*transpose or not*/, d.mpv_matrix.constData());
    glUniform1f(d.u_bpp, (GLfloat)d.video_format.bitsPerPixel(0));
    glUniform1f(d.u_opacity, (GLfloat)1.0);
#else
   d.shader_program->setUniformValue(d.u_colorMatrix, d.colorTransform.matrixRef());
   d.shader_program->setUniformValue(d.u_matrix, d.mpv_matrix);
   d.shader_program->setUniformValue(d.u_bpp, (GLfloat)d.video_format.bitsPerPixel(0));
   d.shader_program->setUniformValue(d.u_opacity, (GLfloat)1.0);
#endif
   // uniforms done. attributes begin
   //qpainter need. TODO: VBO?
#if NO_QGL_SHADER
   glVertexAttribPointer(d.a_Position, 2, GL_FLOAT, GL_FALSE, 0, kVertices);
   glVertexAttribPointer(d.a_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, kTexCoords);
   glEnableVertexAttribArray(d.a_Position);
   glEnableVertexAttribArray(d.a_TexCoords);
#else
   d.shader_program->setAttributeArray(d.a_Position, GL_FLOAT, kVertices, 2);
   d.shader_program->setAttributeArray(d.a_TexCoords, GL_FLOAT, kTexCoords, 2);
   d.shader_program->enableAttributeArray(d.a_Position);
   d.shader_program->enableAttributeArray(d.a_TexCoords);
#endif
   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#if NO_QGL_SHADER
   //glUseProgram(0);
   glDisableVertexAttribArray(d.a_TexCoords);
   glDisableVertexAttribArray(d.a_Position);
#else
   d.shader_program->release();
   d.shader_program->disableAttributeArray(d.a_TexCoords);
   d.shader_program->disableAttributeArray(d.a_Position);
#endif
   glDisable(GL_BLEND);

    for (int i = 0; i < d.textures.size(); ++i) {
        d.video_frame.unmap(&d.textures[i]);
    }
}

void GLWidgetRenderer::initializeGL()
{
    DPTR_D(GLWidgetRenderer);
    makeCurrent();
    //const QByteArray extensions(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
    d.hasGLSL = QGLShaderProgram::hasOpenGLShaderPrograms();
    qDebug("OpenGL version: %d.%d  hasGLSL: %d", format().majorVersion(), format().minorVersion(), d.hasGLSL);
#if QTAV_HAVE(QGLFUNCTIONS)
    initializeGLFunctions();
    d.initializeGLFunctions();
#endif //QTAV_HAVE(QGLFUNCTIONS)
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    if (!d.hasGLSL) {
#ifndef QT_OPENGL_ES_2
        glShadeModel(GL_SMOOTH); //setupQuality?
        glClearDepth(1.0f);
#endif //QT_OPENGL_ES_2
    }
    else {
        d.initWithContext(context());
    }
    glClearColor(0.0, 0.0, 0.0, 0.0);
}

void GLWidgetRenderer::paintGL()
{
    DPTR_D(GLWidgetRenderer);
    /* we can mix gl and qpainter.
     * QPainter painter(this);
     * painter.beginNativePainting();
     * gl functions...
     * painter.endNativePainting();
     * swapBuffers();
     */
    handlePaintEvent();
    swapBuffers();
    if (d.painter && d.painter->isActive())
        d.painter->end();
}

void GLWidgetRenderer::resizeGL(int w, int h)
{
    DPTR_D(GLWidgetRenderer);
    //qDebug("%s @%d %dx%d", __FUNCTION__, __LINE__, d.out_rect.width(), d.out_rect.height());
    glViewport(0, 0, w, h);
    d.setupAspectRatio();
#ifndef QT_OPENGL_ES_2
    //??
    if (!d.hasGLSL) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
#endif //QT_OPENGL_ES_2
}

void GLWidgetRenderer::resizeEvent(QResizeEvent *e)
{
    DPTR_D(GLWidgetRenderer);
    d.update_background = true;
    resizeRenderer(e->size());
    QGLWidget::resizeEvent(e); //will call resizeGL(). TODO:will call paintEvent()?
}

//TODO: out_rect not correct when top level changed
void GLWidgetRenderer::showEvent(QShowEvent *)
{
    DPTR_D(GLWidgetRenderer);
    d.update_background = true;
    /*
     * Do something that depends on widget below! e.g. recreate render target for direct2d.
     * When Qt::WindowStaysOnTopHint changed, window will hide first then show. If you
     * don't do anything here, the widget content will never be updated.
     */
}

void GLWidgetRenderer::onSetOutAspectRatio(qreal ratio)
{
    Q_UNUSED(ratio);
    d_func().setupAspectRatio();
}

void GLWidgetRenderer::onSetOutAspectRatioMode(OutAspectRatioMode mode)
{
    Q_UNUSED(mode);
    d_func().setupAspectRatio();
}

bool GLWidgetRenderer::onSetOrientation(int value)
{
    Q_UNUSED(value);
    d_func().setupAspectRatio();
    return true;
}

bool GLWidgetRenderer::onSetBrightness(qreal b)
{
    DPTR_D(GLWidgetRenderer);
    if (!d.hasGLSL)
        return false;
    d.colorTransform.setBrightness(b);
    return true;
}

bool GLWidgetRenderer::onSetContrast(qreal c)
{
    DPTR_D(GLWidgetRenderer);
    if (!d.hasGLSL)
        return false;
    d.colorTransform.setContrast(c);
    return true;
}

bool GLWidgetRenderer::onSetHue(qreal h)
{
    DPTR_D(GLWidgetRenderer);
    if (!d.hasGLSL)
        return false;
    d.colorTransform.setHue(h);
    return true;
}

bool GLWidgetRenderer::onSetSaturation(qreal s)
{
    DPTR_D(GLWidgetRenderer);
    if (!d.hasGLSL)
        return false;
    d.colorTransform.setSaturation(s);
    return true;
}

} //namespace QtAV
