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

#include "QtAV/VideoShader.h"
#include "QtAV/private/VideoShader_p.h"
#include "ColorTransform.h"
#include "opengl/OpenGLHelper.h"
#include <cmath>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include "utils/Logger.h"

#define YUVA_DONE 0
//#define QTAV_DEBUG_GLSL

namespace QtAV {
extern QVector<Uniform> ParseUniforms(const QByteArray& text, GLuint programId = 0);

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
        if (d.video_format.hasAlpha())
            vert.prepend("#define HAS_ALPHA\n");
#endif
    }
    vert.prepend(OpenGLHelper::compatibleShaderHeader(QOpenGLShader::Vertex));

    if (userShaderHeader(QOpenGLShader::Vertex)) {
        QByteArray header("*/");
        header.append(userShaderHeader(QOpenGLShader::Vertex));
        header += "/*";
        vert.replace("%userHeader%", header);
    }

#ifdef QTAV_DEBUG_GLSL
    QString s(vert);
    s = OpenGLHelper::removeComments(s);
    qDebug() << s.toUtf8().constData();
#endif //QTAV_DEBUG_GLSL
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
    const int nb_planes = d.video_format.planeCount();
    if (nb_planes == 2) //TODO: nv21 must be swapped
        frag.prepend("#define IS_BIPLANE\n");
    if (OpenGLHelper::hasRG() && !OpenGLHelper::useDeprecatedFormats())
        frag.prepend("#define USE_RG\n");
    const bool has_alpha = d.video_format.hasAlpha();
    if (d.video_format.isPlanar()) {
        const int bpc = d.video_format.bitsPerComponent();
        if (bpc > 8) {
            //// has and use 16 bit texture (r16 for example): If channel depth is 16 bit, no range convertion required. Otherwise, must convert to color.r*(2^16-1)/(2^bpc-1)
            if (OpenGLHelper::depth16BitTexture() < 16 || !OpenGLHelper::has16BitTexture() || d.video_format.isBigEndian())
                frag.prepend("#define CHANNEL16_TO8\n");
        }
#if YUVA_DONE
        if (has_alpha)
            frag.prepend("#define HAS_ALPHA\n");
#endif
    } else {
        if (has_alpha)
            frag.prepend("#define HAS_ALPHA\n");
        if (d.video_format.isXYZ())
            frag.prepend("#define XYZ_GAMMA\n");
    }

    if (d.texture_target == GL_TEXTURE_RECTANGLE) {
        frag.prepend("#extension GL_ARB_texture_rectangle : enable\n"
                     "#define sampler2D sampler2DRect\n");
        if (OpenGLHelper::GLSLVersion() < 140)
            frag.prepend("#undef texture\n"
                        "#define texture texture2DRect\n"
                        );
        frag.prepend("#define MULTI_COORD\n");
    }
    frag.prepend(OpenGLHelper::compatibleShaderHeader(QOpenGLShader::Fragment));

    QByteArray header("*/");
    if (userShaderHeader(QOpenGLShader::Fragment))
        header += QByteArray(userShaderHeader(QOpenGLShader::Fragment));
    header += "\n";
    header += "uniform vec2 u_texelSize[" + QByteArray::number(nb_planes) + "];\n";
    header += "uniform vec2 u_textureSize[" + QByteArray::number(nb_planes) + "];\n";
    header += "/*";
    frag.replace("%userHeader%", header);

    if (userSample()) {
        QByteArray sample_code("*/\n#define USER_SAMPLER\n");
        sample_code += QByteArray(userSample());
        sample_code += "/*";
        frag.replace("%userSample%", sample_code);
    }

    if (userPostProcess()) {
        QByteArray pp_code("*/");
        pp_code += QByteArray(userPostProcess()); //why the content is wrong sometimes if no ctor?
        pp_code += "/*";
        frag.replace("%userPostProcess%", pp_code);
    }
    frag.replace("%planes%", QByteArray::number(nb_planes));
#ifdef QTAV_DEBUG_GLSL
    QString s(frag);
    s = OpenGLHelper::removeComments(s);
    qDebug() << s.toUtf8().constData();
#endif //QTAV_DEBUG_GLSL
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
        build(shaderProgram);
    }
    d.u_Matrix = shaderProgram->uniformLocation("u_Matrix");
    // fragment shader
    d.u_colorMatrix = shaderProgram->uniformLocation("u_colorMatrix");
    d.u_to8 = shaderProgram->uniformLocation("u_to8");
    d.u_opacity = shaderProgram->uniformLocation("u_opacity");
    d.u_c = shaderProgram->uniformLocation("u_c");
    d.u_texelSize = shaderProgram->uniformLocation("u_texelSize");
    d.u_textureSize = shaderProgram->uniformLocation("u_textureSize");
    d.u_Texture.resize(textureLocationCount());
    qDebug("uniform locations:");
    for (int i = 0; i < d.u_Texture.size(); ++i) {
        const QString tex_var = QStringLiteral("u_Texture%1").arg(i);
        d.u_Texture[i] = shaderProgram->uniformLocation(tex_var);
        qDebug("%s: %d", tex_var.toUtf8().constData(), d.u_Texture[i]);
    }
    qDebug("u_Matrix: %d", d.u_Matrix);
    qDebug("u_colorMatrix: %d", d.u_colorMatrix);
    qDebug("u_opacity: %d", d.u_opacity);
    if (d.u_c >= 0)
        qDebug("u_c: %d", d.u_c);
    if (d.u_to8 >= 0)
        qDebug("u_to8: %d", d.u_to8);
    if (d.u_texelSize >= 0)
        qDebug("u_texelSize: %d", d.u_texelSize);
    if (d.u_textureSize >= 0)
        qDebug("u_textureSize: %d", d.u_textureSize);

    d.user_uniforms[VertexShader].clear();
    d.user_uniforms[FragmentShader].clear();
    if (userShaderHeader(QOpenGLShader::Vertex)) {
        qDebug("user uniform locations in vertex shader:");
        d.user_uniforms[VertexShader] = ParseUniforms(QByteArray(userShaderHeader(QOpenGLShader::Vertex)), shaderProgram->programId());
    }
    if (userShaderHeader(QOpenGLShader::Fragment)) {
        qDebug("user uniform locations in fragment shader:");
        d.user_uniforms[FragmentShader] = ParseUniforms(QByteArray(userShaderHeader(QOpenGLShader::Fragment)), shaderProgram->programId());
    }
    d.rebuild_program = false;
    d.update_builtin_uniforms = true;
    programReady(); // program and uniforms are ready
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
    return d_func().u_Matrix;
}

int VideoShader::colorMatrixLocation() const
{
    return d_func().u_colorMatrix;
}

int VideoShader::opacityLocation() const
{
    return d_func().u_opacity;
}

int VideoShader::channelMapLocation() const
{
    return d_func().u_c;
}

int VideoShader::texelSizeLocation() const
{
    return d_func().u_texelSize;
}

int VideoShader::textureSizeLocation() const
{
    return d_func().u_textureSize;
}

int VideoShader::uniformLocation(const char *name) const
{
    DPTR_D(const VideoShader);
    if (!d.program)
        return -1;
    return d.program->uniformLocation(name); //TODO: store in a hash
}

int VideoShader::textureTarget() const
{
    return d_func().texture_target;
}

void VideoShader::setTextureTarget(int type)
{
    d_func().texture_target = type;
}

void VideoShader::setMaterialType(qint32 value)
{
    d_func().material_type = value;
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
    Q_ASSERT(material && "null material");
    DPTR_D(VideoShader);
    const qint32 mt = material->type();
    if (mt != d.material_type || d.rebuild_program) {
        // TODO: use shader program cache (per shader), check shader type
        qDebug("Rebuild shader program requested: %d. Material type %d=>%d", d.rebuild_program, d.material_type, mt);
        program()->removeAllShaders(); //not linked
        // initialize shader, the same as VideoMaterial::createShader
        setVideoFormat(material->currentFormat());
        setTextureTarget(material->textureTarget());
        setMaterialType(material->type());

        initialize();
    }
    //material->unbind();
    const VideoFormat fmt(material->currentFormat()); //FIXME: maybe changed in setCurrentFrame(
    //format is out of date because we may use the same shader for different formats
    setVideoFormat(fmt);
    // uniforms begin
    program()->bind(); //glUseProgram(id). for glUniform
    if (!setUserUniformValues()) {
        if (!d.user_uniforms[VertexShader].isEmpty()) {
            for (int i = 0; i < d.user_uniforms[VertexShader].size(); ++i) {
                Uniform& u = d.user_uniforms[VertexShader][i];
                setUserUniformValue(u);
                if (u.dirty)
                    u.setGL();
            }
        }
        if (!d.user_uniforms[FragmentShader].isEmpty()) {
            for (int i = 0; i < d.user_uniforms[FragmentShader].size(); ++i) {
                Uniform& u = d.user_uniforms[FragmentShader][i];
                setUserUniformValue(u);
                if (u.dirty)
                    u.setGL();
            }
        }
    }
    // shader type changed, eq mat changed, or other material properties changed (e.g. texture, 8bit=>10bit)
    if (!d.update_builtin_uniforms && !material->isDirty())
        return true;
    d.update_builtin_uniforms = false;
    // all texture ids should be binded when renderering even for packed plane!
    const int nb_planes = fmt.planeCount(); //number of texture id
    // TODO: sample2D array
    for (int i = 0; i < nb_planes; ++i) {
        // use glUniform1i to swap planes. swap uv: i => (3-i)%3
        program()->setUniformValue(textureLocation(i), (GLint)i);
    }
    if (nb_planes < textureLocationCount()) {
        for (int i = nb_planes; i < textureLocationCount(); ++i) {
            program()->setUniformValue(textureLocation(i), (GLint)(nb_planes - 1));
        }
    }
    program()->setUniformValue(colorMatrixLocation(), material->colorMatrix());
    program()->setUniformValue(opacityLocation(), (GLfloat)1.0);
    if (d_func().u_to8 >= 0)
        program()->setUniformValue(d_func().u_to8, material->vectorTo8bit());
    if (channelMapLocation() >= 0)
        program()->setUniformValue(channelMapLocation(), material->channelMap());
    //program()->setUniformValue(matrixLocation(), ); //what about sgnode? state.combindMatrix()?
    if (texelSizeLocation() >= 0)
        program()->setUniformValueArray(texelSizeLocation(), material->texelSize().constData(), nb_planes);
    if (textureSizeLocation() >= 0)
        program()->setUniformValueArray(textureSizeLocation(), material->textureSize().constData(), nb_planes);
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

bool VideoShader::build(QOpenGLShaderProgram *shaderProgram)
{
    if (shaderProgram->isLinked()) {
        qWarning("Shader program is already linked");
    }
    shaderProgram->removeAllShaders();
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
        return false;
    }
    return true;
}

void VideoShader::rebuildLater()
{
    d_func().rebuild_program = true;
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
        qDebug("texture target: %#x=>%#x", d.target, new_target);
        // FIXME: not thread safe (in qml)
        d.target = new_target;
        d.init_textures_required = true;
    }
    // TODO: check hw interop change. if change from an interop owns texture to not owns texture, VideoShader must recreate textures because old textures are deleted by previous interop
    const VideoFormat fmt(frame.format());
    const int bpc_old = d.bpc;
    d.bpc = fmt.bitsPerComponent();
    if (d.bpc > 8 && (d.bpc != bpc_old || d.video_format.isBigEndian() != fmt.isBigEndian())) {
        //FIXME: Assume first plane has 1 channel. So not work with NV21
        const int range = (1 << d.bpc) - 1;
        // FFmpeg supports 9, 10, 12, 14, 16 bits
        // 10p in little endian: yyyyyyyy yy000000 => (L, L, L, A)  //(yyyyyyyy, 000000yy)?
        if (OpenGLHelper::depth16BitTexture() < 16 || !OpenGLHelper::has16BitTexture() || fmt.isBigEndian()) {
            if (fmt.isBigEndian())
                d.vec_to8 = QVector2D(256.0, 1.0)*255.0/(float)range;
            else
                d.vec_to8 = QVector2D(1.0, 256.0)*255.0/(float)range;
            d.colorTransform.setChannelDepthScale(1.0);
        } else {
            /// 16bit (R16 e.g.) texture does not support >8bit be channels
            /// 10p be: R2 R1(Host) = R1*2^8+R2 = 000000rr rrrrrrrr ->(GL) R=R2*2^8+R1
            /// 10p le: R1 R2(Host) = rrrrrrrr rr000000
            //d.vec_to8 = QVector2D(1.0, 0.0)*65535.0/(float)range;
            d.colorTransform.setChannelDepthScale(65535.0/(qreal)range, YUVA_DONE && fmt.hasAlpha());
        }
    } else {
        if (d.bpc <= 8)
            d.colorTransform.setChannelDepthScale(1.0);
    }
    // http://forum.doom9.org/archive/index.php/t-160211.html
    ColorSpace cs = frame.colorSpace();// ColorSpace_RGB;
    if (cs == ColorSpace_Unknown) {
        if (fmt.isRGB()) {
            if (fmt.isPlanar())
                cs = ColorSpace_GBR;
            else
                cs = ColorSpace_RGB;
        } else if (fmt.isXYZ()) {
            cs = ColorSpace_XYZ;
        } else {
            if (frame.width() >= 1280 || frame.height() > 576) //values from mpv
                cs = ColorSpace_BT709;
            else
                cs = ColorSpace_BT601;
        }
    }
    d.colorTransform.setInputColorSpace(cs);
    d.colorTransform.setInputColorRange(frame.colorRange());
    // TODO: use graphics driver's color range option if possible
    static const ColorRange kRgbDispRange = qgetenv("QTAV_DISPLAY_RGB_RANGE") == "limited" ? ColorRange_Limited : ColorRange_Full;
    d.colorTransform.setOutputColorRange(kRgbDispRange);
    d.frame = frame;
    if (fmt != d.video_format) {
        qDebug() << fmt;
        qDebug("pixel format changed: %s => %s %d", qPrintable(d.video_format.name()), qPrintable(fmt.name()), fmt.pixelFormat());
        d.video_format = fmt;
        d.init_textures_required = true;
    }
}

VideoFormat VideoMaterial::currentFormat() const
{
    DPTR_D(const VideoMaterial);
    return d.video_format;
}

VideoShader* VideoMaterial::createShader() const
{
    VideoShader *shader = new VideoShader();
    // initialize shader
    shader->setVideoFormat(currentFormat());
    shader->setTextureTarget(textureTarget());
    shader->setMaterialType(type());
    //resize texture locations to avoid access format later
    return shader;
}

QString VideoMaterial::typeName(qint32 value)
{
    return QString("gl material 16to8bit: %1, planar: %2, has alpha: %3, 2d texture: %4, 2nd plane rg: %5, xyz: %6")
            .arg(!!(value&1))
            .arg(!!(value&(1<<1)))
            .arg(!!(value&(1<<2)))
            .arg(!!(value&(1<<3)))
            .arg(!!(value&(1<<4)))
            .arg(!!(value&(1<<5)))
            ;
}

qint32 VideoMaterial::type() const
{
    DPTR_D(const VideoMaterial);
    const VideoFormat &fmt = d.video_format;
    const bool tex_2d = d.target == GL_TEXTURE_2D;
    // 2d,alpha,planar,8bit
    const int rg_biplane = fmt.planeCount()==2 && !OpenGLHelper::useDeprecatedFormats() && OpenGLHelper::hasRG();
    const int channel16_to8 = d.bpc > 8 && (OpenGLHelper::depth16BitTexture() < 16 || !OpenGLHelper::has16BitTexture() || fmt.isBigEndian());
    return (fmt.isXYZ()<<5)|(rg_biplane<<4)|(tex_2d<<3)|(fmt.hasAlpha()<<2)|(fmt.isPlanar()<<1)|(channel16_to8);
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
        d.uploadPlane(p, d.update_texure);
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
void VideoMaterialPrivate::uploadPlane(int p, bool updateTexture)
{
    GLuint &tex = textures[p];
    gl().ActiveTexture(GL_TEXTURE0 + p); //0 must active?
    if (!updateTexture) {
        DYGL(glBindTexture(target, tex));
        return;
    }
    if (!frame.constBits(0)) {
        // try_pbo ? pbo_id : 0. 0= > interop.createHandle
        GLuint tex0 = tex;
        if (frame.map(GLTextureSurface, &tex, p)) {
            if (tex0 != tex) {
                if (owns_texture[tex0])
                    DYGL(glDeleteTextures(1, &tex0));
                owns_texture.remove(tex0);
                owns_texture[tex] = false;
            }
            DYGL(glBindTexture(target, tex)); // glActiveTexture was called, but maybe bind to 0 in map
            return;
        }
        qWarning("map hw surface error");
        return;
    }
    // FIXME: why happens on win?
    if (frame.bytesPerLine(p) <= 0)
        return;
    if (try_pbo) {
        //qDebug("bind PBO %d", p);
        QOpenGLBuffer &pb = pbo[p];
        pb.bind();
        // glMapBuffer() causes sync issue.
        // Call glBufferData() with NULL pointer before glMapBuffer(), the previous data in PBO will be discarded and
        // glMapBuffer() returns a new allocated pointer or an unused block immediately even if GPU is still working with the previous data.
        // https://www.opengl.org/wiki/Buffer_Object_Streaming#Buffer_re-specification
        pb.allocate(pb.size());
        GLubyte* ptr = (GLubyte*)pb.map(QOpenGLBuffer::WriteOnly);
        if (ptr) {
            memcpy(ptr, frame.constBits(p), pb.size());
            pb.unmap();
        }
    }
    //qDebug("bpl[%d]=%d width=%d", p, frame.bytesPerLine(p), frame.planeWidth(p));
    DYGL(glBindTexture(target, tex));
    //setupQuality();
    //DYGL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    //DYGL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    // This is necessary for non-power-of-two textures
    //glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(stride)); 8, 4, 2, 1
    // glPixelStorei(GL_UNPACK_ROW_LENGTH, stride/glbpp); // for stride%glbpp > 0?
    DYGL(glTexSubImage2D(target, 0, 0, 0, texture_size[p].width(), texture_size[p].height(), data_format[p], data_type[p], try_pbo ? 0 : frame.constBits(p)));
    if (false) { //texture_size[].width()*gl_bpp != bytesPerLine[]
        for (int y = 0; y < plane0Size.height(); ++y)
            DYGL(glTexSubImage2D(target, 0, 0, y, texture_size[p].width(), 1, data_format[p], data_type[p], try_pbo ? 0 : frame.constBits(p)+y*plane0Size.width()));
    }
    //DYGL(glBindTexture(target, 0)); // no bind 0 because glActiveTexture was called
    if (try_pbo) {
        pbo[p].release();
    }
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
    setDirty(false);
}

int VideoMaterial::compare(const VideoMaterial *other) const
{
    DPTR_D(const VideoMaterial);
    for (int i = 0; i < d.textures.size(); ++i) {
        const int diff = d.textures[i] - other->d_func().textures[i]; //TODO
        if (diff)
            return diff;
    }
    return d.bpc - other->bitsPerComponent();
}

int VideoMaterial::textureTarget() const
{
    return d_func().target;
}

bool VideoMaterial::isDirty() const
{
    return d_func().dirty;
}

void VideoMaterial::setDirty(bool value)
{
    d_func().dirty = value;
}

const QMatrix4x4& VideoMaterial::colorMatrix() const
{
    return d_func().colorTransform.matrixRef();
}

const QMatrix4x4 &VideoMaterial::channelMap() const
{
    return d_func().channel_map;
}

int VideoMaterial::bitsPerComponent() const
{
    return d_func().bpc;
}

QVector2D VideoMaterial::vectorTo8bit() const
{
    return d_func().vec_to8;
}

int VideoMaterial::planeCount() const
{
    return d_func().frame.planeCount();
}

qreal VideoMaterial::brightness() const
{
    return d_func().colorTransform.brightness();
}

void VideoMaterial::setBrightness(qreal value)
{
    d_func().colorTransform.setBrightness(value);
    d_func().dirty = true;
}

qreal VideoMaterial::contrast() const
{
    return d_func().colorTransform.contrast();
}

void VideoMaterial::setContrast(qreal value)
{
    d_func().colorTransform.setContrast(value);
    d_func().dirty = true;
}

qreal VideoMaterial::hue() const
{
    return d_func().colorTransform.hue();
}

void VideoMaterial::setHue(qreal value)
{
    d_func().colorTransform.setHue(value);
    d_func().dirty = true;
}

qreal VideoMaterial::saturation() const
{
    return d_func().colorTransform.saturation();
}

void VideoMaterial::setSaturation(qreal value)
{
    d_func().colorTransform.setSaturation(value);
    d_func().dirty = true;
}

qreal VideoMaterial::validTextureWidth() const
{
    return d_func().effective_tex_width_ratio;
}

QSize VideoMaterial::frameSize() const
{
    return QSize(d_func().width, d_func().height);
}

QSizeF VideoMaterial::texelSize(int plane) const
{
    DPTR_D(const VideoMaterial);
    return QSizeF(1.0/(qreal)d.texture_size[plane].width(), 1.0/(qreal)d.texture_size[plane].height());
}

QVector<QVector2D> VideoMaterial::texelSize() const
{
    return d_func().v_texel_size;
}

QSize VideoMaterial::textureSize(int plane) const
{
    return d_func().texture_size[plane];
}

QVector<QVector2D> VideoMaterial::textureSize() const
{
    return d_func().v_texture_size;
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
    if (d.texture_size.isEmpty()) { //It should not happen if it's called in QtAV
        qWarning("textures not ready");
        return p;
    }
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
    if (d.texture_size.isEmpty()) { //It should not happen if it's called in QtAV
        qWarning("textures not ready");
        return QRectF();
    }
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
    //pb.setUsagePattern(QOpenGLBuffer::DynamicCopy);
    qDebug("Allocate PBO size %d", size);
    pb.allocate(size);
    pb.release(); //bind to 0
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
    // FIXME: when to delete
    if (!QOpenGLContext::currentContext()) {
        qWarning("No gl context");
        return;
    }
    if (!textures.isEmpty()) {
        for (int i = 0; i < textures.size(); ++i) {
            GLuint &tex = textures[i];
            if (owns_texture[tex])
                DYGL(glDeleteTextures(1, &tex));
        }
        //DYGL(glDeleteTextures(textures.size(), textures.data()));
    }
    owns_texture.clear();
    textures.clear();
    pbo.clear();
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
    const int nb_planes = fmt.planeCount();
    internal_format.resize(nb_planes);
    data_format.resize(nb_planes);
    data_type.resize(nb_planes);
    if (!OpenGLHelper::videoFormatToGL(fmt, (GLint*)internal_format.constData(), (GLenum*)data_format.constData(), (GLenum*)data_type.constData(), &channel_map)) {
        qWarning() << "No OpenGL support for " << fmt;
        return false;
    }
    qDebug() << "texture internal format: " << internal_format;
    qDebug() << "texture data format: " << data_format;
    qDebug() << "texture data type: " << data_type;
    qDebug("///////////bpp %d, bpc: %d", fmt.bytesPerPixel(), fmt.bitsPerComponent());
    for (int i = 0; i < nb_planes; ++i) {
        const int bpp_gl = OpenGLHelper::bytesOfGLFormat(data_format[i], data_type[i]);
        const int pad = std::ceil((qreal)(texture_size[i].width() - effective_tex_width[i])/(qreal)bpp_gl);
        texture_size[i].setWidth(std::ceil((qreal)texture_size[i].width()/(qreal)bpp_gl));
        effective_tex_width[i] /= bpp_gl; //fmt.bytesPerPixel(i);
        v_texture_size[i] = QVector2D(texture_size[i].width(), texture_size[i].height());
        //effective_tex_width_ratio =
        qDebug("texture width: %d - %d = pad: %d. bpp(gl): %d", texture_size[i].width(), effective_tex_width[i], pad, bpp_gl);
        if (target == GL_TEXTURE_RECTANGLE)
            v_texel_size[i] = QVector2D(1.0, 1.0);
        else
            v_texel_size[i] = QVector2D(1.0/(float)texture_size[i].width(), 1.0/(float)texture_size[i].height());
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
    if (textures.size() > nb_planes) { //TODO: why check this?
        const int nb_delete = textures.size() - nb_planes;
        qDebug("try to delete %d textures", nb_delete);
        if (!textures.isEmpty()) {
            for (int i = 0; i < nb_delete; ++i) {
                GLuint &t = textures[nb_planes+i];
                qDebug("try to delete texture[%d]: %u. can delete: %d", nb_planes+i, t, owns_texture[t]);
                if (owns_texture[t])
                    DYGL(glDeleteTextures(1, &t));
            }
            //DYGL(glDeleteTextures(nb_delete, textures.data() + nb_planes));
        }
        owns_texture.clear();
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
    // update textures if format, texture target, valid texture width(normalized), plane 0 size or plane 1 line size changed
    bool update_textures = init_textures_required;
    const int nb_planes = fmt.planeCount();
    // effective size may change even if plane size not changed
    bool effective_tex_width_ratio_changed = true;
    for (int i = 0; i < nb_planes; ++i) {
        if ((qreal)frame.effectiveBytesPerLine(i)/(qreal)frame.bytesPerLine(i) == effective_tex_width_ratio) {
            effective_tex_width_ratio_changed = false;
            break;
        }
    }
    const int linsize0 = frame.bytesPerLine(0);
    if (update_textures
            || effective_tex_width_ratio_changed
            || linsize0 != plane0Size.width() || frame.height() != plane0Size.height()
            || (plane1_linesize > 0 && frame.bytesPerLine(1) != plane1_linesize)) { // no need to check height if plane 0 sizes are equal?
        update_textures = true;
        dirty = true;
        v_texel_size.resize(nb_planes);
        v_texture_size.resize(nb_planes);
        texture_size.resize(nb_planes);
        effective_tex_width.resize(nb_planes);
        effective_tex_width_ratio = 1.0;
        for (int i = 0; i < nb_planes; ++i) {
            qDebug("plane linesize %d: padded = %d, effective = %d. theoretical plane size: %dx%d", i, frame.bytesPerLine(i), frame.effectiveBytesPerLine(i), frame.planeWidth(i), frame.planeHeight(i));
            // we have to consider size of opengl format. set bytesPerLine here and change to width later
            texture_size[i] = QSize(frame.bytesPerLine(i), frame.planeHeight(i));
            effective_tex_width[i] = frame.effectiveBytesPerLine(i); //store bytes here, modify as width later
            // usually they are the same. If not, the difference is small. min value can avoid rendering the invalid data.
            effective_tex_width_ratio = qMin(effective_tex_width_ratio, (qreal)frame.effectiveBytesPerLine(i)/(qreal)frame.bytesPerLine(i));
        }
        plane1_linesize = 0;
        if (nb_planes > 1) {
            // height? how about odd?
            plane1_linesize = frame.bytesPerLine(1);
        }
        /*
          let wr[i] = valid_bpl[i]/bpl[i], frame from avfilter maybe wr[1] < wr[0]
          e.g. original frame plane 0: 720/768; plane 1,2: 360/384,
          filtered frame plane 0: 720/736, ... (16 aligned?)
         */
        qDebug("effective_tex_width_ratio=%f", effective_tex_width_ratio);
        plane0Size.setWidth(linsize0);
        plane0Size.setHeight(frame.height());
    }
    if (update_textures) {
        updateTextureParameters(fmt);
        // check pbo support
        try_pbo = try_pbo && OpenGLHelper::isPBOSupported();
        // check PBO support with bind() is fine, no need to check extensions
        if (try_pbo) {
            pbo.resize(nb_planes);
            for (int i = 0; i < nb_planes; ++i) {
                qDebug("Init PBO for plane %d", i);
                pbo[i] = QOpenGLBuffer(QOpenGLBuffer::PixelUnpackBuffer); //QOpenGLBuffer is shared, must initialize 1 by 1 but not use fill
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
            qDebug("try to delete texture for plane %d (id=%u). can delete: %d", p, tex, owns_texture[tex]);
            if (owns_texture[tex])
                DYGL(glDeleteTextures(1, &tex));
            owns_texture.remove(tex);
            tex = 0;
        }
        if (!tex) {
            qDebug("creating texture for plane %d", p);
            GLuint* handle = (GLuint*)frame.createInteropHandle(&tex, GLTextureSurface, p); // take the ownership
            if (handle) {
                tex = *handle;
                owns_texture[tex] = true;
            } else {
                DYGL(glGenTextures(1, &tex));
                owns_texture[tex] = true;
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
