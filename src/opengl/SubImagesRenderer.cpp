/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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

#include "SubImagesRenderer.h"
#include "opengl/SubImagesGeometry.h"
#include "QtAV/GeometryRenderer.h"

namespace QtAV {

#define GLSL(x) #x "\n"

static const char kVert[] = GLSL(
        attribute vec4 a_Position;
        attribute vec2 a_TexCoords;
        attribute vec4 a_Color;
        uniform mat4 u_Matrix;
        varying vec2 v_TexCoords;
        varying vec4 v_Color;
        void main() {
            gl_Position = u_Matrix * a_Position;
            v_TexCoords = a_TexCoords;
            v_Color = a_Color;
        });

static const char kFrag[] = GLSL(
            uniform sampler2D u_Texture;
            varying vec2 v_TexCoords;
            varying vec4 v_Color;
            void main() {
                gl_FragColor.rgb = v_Color.rgb;
                gl_FragColor.a = v_Color.a*texture2D(u_Texture, v_TexCoords).r;
            }
            );

SubImagesRenderer::SubImagesRenderer()
    : m_geometry(new SubImagesGeometry())
    , m_renderer(new GeometryRenderer())
    , m_tex(0)
{}

SubImagesRenderer::~SubImagesRenderer()
{
    delete m_geometry;
    delete m_renderer;
}

void SubImagesRenderer::render(const SubImageSet &ass, const QRect &target, const QMatrix4x4 &transform)
{
    if (m_geometry->setSubImages(ass) || m_rect != target) {
        m_rect = target;
        if (!m_geometry->generateVertexData(m_rect, true))
            return;
        uploadTexture(m_geometry);
        m_renderer->updateGeometry(m_geometry);
    }
    if (!m_program.isLinked()) {
        m_program.removeAllShaders();
        QByteArray vs(kVert);
        vs.prepend(OpenGLHelper::compatibleShaderHeader(QOpenGLShader::Vertex));
        m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vs);
        QByteArray fs(kFrag);
        fs.prepend(OpenGLHelper::compatibleShaderHeader(QOpenGLShader::Fragment));
        m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fs);
        // TODO: foreach geometry.attributes
        m_program.bindAttributeLocation("a_Position", 0);
        m_program.bindAttributeLocation("a_TexCoords", 1);
        m_program.bindAttributeLocation("a_Color", 2);
        if (!m_program.link())
            qWarning() << m_program.log();
    }
    m_program.bind();
    gl().ActiveTexture(GL_TEXTURE0);
    DYGL(glBindTexture(GL_TEXTURE_2D, m_tex));
    m_program.setUniformValue("u_Texture", 0);
    m_program.setUniformValue("u_Matrix", transform*m_mat);
    DYGL(glEnable(GL_BLEND));
    if (m_geometry->images().format() == SubImageSet::ASS)
        gl().BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    else
        gl().BlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    m_renderer->render();

    DYGL(glDisable(GL_BLEND));
}

void SubImagesRenderer::setProjectionMatrixToRect(const QRectF &v)
{
    m_mat.setToIdentity();
    m_mat.ortho(v);
}

void SubImagesRenderer::uploadTexture(SubImagesGeometry *g)
{
    if (!m_tex) {
        DYGL(glGenTextures(1, &m_tex)); //TODO: delete
    }
    GLint internal_fmt;
    GLenum data_type;
    GLenum fmt;
    if (g->images().format() == SubImageSet::ASS)
        OpenGLHelper::videoFormatToGL(VideoFormat(VideoFormat::Format_Y8), &internal_fmt, &fmt, &data_type);
    else //rgb32
        OpenGLHelper::videoFormatToGL(VideoFormat(VideoFormat::Format_ARGB32), &internal_fmt, &fmt, &data_type);
    DYGL(glBindTexture(GL_TEXTURE_2D, m_tex));
    DYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    DYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    DYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    DYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    DYGL(glTexImage2D(GL_TEXTURE_2D, 0, internal_fmt, g->width(), g->height(), 0, fmt, data_type, NULL));
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (int i = 0; i < g->uploadRects().size(); ++i) {
        const QRect& r = g->uploadRects().at(i);
        const SubImage& sub = g->images().images.at(i);
        DYGL(glTexSubImage2D(GL_TEXTURE_2D, 0, r.x(), r.y(), r.width(), r.height(), fmt, data_type, sub.data.constData()));
    }
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
}

} //namespace QtAV
