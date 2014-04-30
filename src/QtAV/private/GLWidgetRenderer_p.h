/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_GLWIDGETRENDERER_P_H
#define QTAV_GLWIDGETRENDERER_P_H
#include <QtOpenGL/qgl.h>
#include <QtOpenGL/QGLShaderProgram>

#include <QtCore/QVector>
#include "QtAV/private/VideoRenderer_p.h"
#include <QtAV/VideoFormat.h>
#include <QtAV/ColorTransform.h>

#define NO_QGL_SHADER (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))

namespace QtAV {

class Q_AV_EXPORT GLWidgetRendererPrivate : public VideoRendererPrivate
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
      , painter(0)
      , video_format(VideoFormat::Format_Invalid)
    {
        if (QGLFormat::openGLVersionFlags() == QGLFormat::OpenGL_Version_None) {
            available = false;
            return;
        }
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
    }
    bool initWithContext(const QGLContext *ctx) {
#if !NO_QGL_SHADER
        shader_program = new QGLShaderProgram(ctx, 0);
#endif
        return true;
    }

    GLuint loadShader(GLenum shaderType, const char* pSource);
    GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);
    bool releaseShaderProgram();
    QString getShaderFromFile(const QString& fileName);
    bool prepareShaderProgram(const VideoFormat& fmt);
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
        if (hasGLSL) {
            if (u_matrix < 0) {
                //qDebug("%s @%d mat=%d", __FUNCTION__, __LINE__, u_matrix);
                return;
            }
#if !NO_QGL_SHADER && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
            const qreal matrix[] = {
#else
            const GLfloat matrix[] = {
#endif
                (float)out_rect.width()/(float)renderer_width, 0, 0, 0,
                0, (float)out_rect.height()/(float)renderer_height, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
#if NO_QGL_SHADER
            glUniformMatrix4fv(u_matrix, 1, GL_FALSE/*transpose or not*/, matrix);
#else
            shader_program->setUniformValue(u_matrix, QMatrix4x4(matrix));
#endif
        }
#ifndef QT_OPENGL_ES_2
        if (!hasGLSL) {
            glScalef((float)out_rect.width()/(float)renderer_width, (float)out_rect.height()/(float)renderer_height, 0);
        }
#endif //QT_OPENGL_ES_2
    }

    bool hasGLSL;
    bool update_texcoords;
    QVector<GLuint> textures; //texture ids. size is plane count
    QVector<QSize> texture_size;
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

    QPainter *painter;

    VideoFormat video_format;
    QSize plane0Size;
    ColorTransform colorTransform;
};

} //namespace QtAV
#endif // QTAV_GLWIDGETRENDERER_P_H
