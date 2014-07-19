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

#ifndef QTAV_VIDEOSHADER_P_H
#define QTAV_VIDEOSHADER_P_H

#include "QtAV/VideoFrame.h"
#include "QtAV/ColorTransform.h"
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

namespace QtAV {

class VideoShader;
class Q_AV_EXPORT VideoShaderPrivate : public DPtrPrivate<VideoShader>
{
public:
    VideoShaderPrivate()
        : program(0)
        , u_MVP_matrix(-1)
        , u_colorMatrix(-1)
        , u_bpp(-1)
    {}
    virtual ~VideoShaderPrivate() {
        if (program) {
            program->removeAllShaders();
            delete program;
            program = 0;
        }
    }

    QOpenGLShaderProgram *program;
    // TODO: compare with texture width uniform used in qtmm
    int u_MVP_matrix;
    int u_colorMatrix;
    int u_bpp;
    QVector<int> u_Texture;
    VideoFormat video_format;
    mutable QByteArray planar_frag, packed_frag;
};

class VideoMaterial;
class VideoMaterialPrivate : public DPtrPrivate<VideoMaterial>
{
public:
    VideoMaterialPrivate()
        : update_texure(true)
        , bpp(1)
        , video_format(VideoFormat::Format_Invalid)
        , plane1_linesize(0)
        , effective_tex_width_ratio(1.0)
    {
        colorTransform.setOutputColorSpace(ColorTransform::RGB);
    }
    ~VideoMaterialPrivate() {
        if (!textures.isEmpty())
            glDeleteTextures(textures.size(), textures.data());
    }
    bool initTexture(GLuint tex, GLint internal_format, GLenum format, GLenum dataType, int width, int height);
    bool initTextures(const VideoFormat& fmt);
    void updateTexturesIfNeeded();
    void setupQuality();

    bool update_texure; // reduce upload/map times. true: new frame not bound. false: current frame is bound
    int bpp;
    QRect viewport;
    QRect out_rect;
    VideoFrame frame;
    /*
     *  old format. used to check whether we have to update textures. set to current frame's format after textures are updated.
     * TODO: only VideoMaterial.type() is enough to check and update shader. so remove it
     */
    VideoFormat video_format;
    QSize plane0Size;
    // width is in bytes. different alignments may result in different plane 1 linesize even if plane 0 are the same
    int plane1_linesize;

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
};

} //namespace QtAV

#endif // QTAV_VideoShader_P_H
