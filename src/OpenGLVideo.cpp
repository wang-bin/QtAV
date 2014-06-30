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
#else
#include <QtOpenGL/QGLShaderProgram>
typedef QGLShaderProgram QOpenGLShaderProgram;
#endif

namespace QtAV {

VideoShader::VideoShader()
    : m_color_space(ColorTransform::BT601)
    , u_MVP_matrix(-1)
    , u_colorMatrix(-1)
{
    //m_planar_frag = shaderSourceFromFile("shaders/yuv_rgb.f.glsl");
    //m_packed_frag = shaderSourceFromFile("shaders/rgb.f.glsl");
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
    // because we have to modify the shader, and shader source must be kept, so read the origin
    if (m_video_format.isPlanar()) {
        m_planar_frag = shaderSourceFromFile("shaders/yuv_rgb.f.glsl");
    } else {
        m_packed_frag = shaderSourceFromFile("shaders/rgb.f.glsl");
    }
    QByteArray& frag = m_video_format.isPlanar() ? m_planar_frag : m_packed_frag;
    if (frag.isEmpty()) {
        qWarning("Empty fragment shader!");
        return 0;
    }
    if (!m_video_format.isRGB()) {
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
    u_Texture.resize(textureCount());
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

int VideoShader::textureCount() const
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

} //namespace QtAV
