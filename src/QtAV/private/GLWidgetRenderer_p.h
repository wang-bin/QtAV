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
#include <QtCore/QVector>
#include "private/VideoRenderer_p.h"
#include <QtAV/VideoFormat.h>
#include <QtAV/ColorTransform.h>

namespace QtAV {

class Q_AV_EXPORT GLWidgetRendererPrivate : public VideoRendererPrivate, public QGLFunctions
{
public:
    GLWidgetRendererPrivate():
        VideoRendererPrivate()
      , hasGLSL(true)
      , update_texcoords(true)
      , effective_tex_width_ratio(1)
      , program(0)
      , vert(0)
      , frag(0)
      , a_Position(0)
      , a_TexCoords(0)
      , u_matrix(0)
      , painter(0)
      , pixel_fmt(VideoFormat::Format_Invalid)
    {
        if (QGLFormat::openGLVersionFlags() == QGLFormat::OpenGL_Version_None) {
            available = false;
            return;
        }
    }
    ~GLWidgetRendererPrivate() {
        releaseResource();
    }
    GLuint loadShader(GLenum shaderType, const char* pSource);
    GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);
    bool releaseResource();
    bool initTexture(GLuint tex, GLint internal_format, GLenum format, GLenum dataType, int width, int height);
    QString getShaderFromFile(const QString& fileName);
    bool prepareShaderProgram(const VideoFormat& fmt);
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
            const GLfloat matrix[] = {
                (float)out_rect.width()/(float)renderer_width, 0, 0, 0,
                0, (float)out_rect.height()/(float)renderer_height, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
            glUniformMatrix4fv(u_matrix, 1, GL_FALSE/*transpose or not*/, matrix);
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
    GLuint program;
    GLuint vert, frag;
    GLuint a_Position;
    GLuint a_TexCoords;
    QVector<GLuint> u_Texture; //u_TextureN uniform. size is channel count
    GLuint u_matrix;
    GLuint u_colorMatrix;

    QPainter *painter;

    VideoFormat::PixelFormat pixel_fmt;
    QSize plane0Size;
    ColorTransform colorTransform;
};

} //namespace QtAV
#endif // QTAV_GLWIDGETRENDERER_P_H
