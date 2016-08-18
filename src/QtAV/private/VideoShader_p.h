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

#ifndef QTAV_VIDEOSHADER_P_H
#define QTAV_VIDEOSHADER_P_H

#include "QtAV/OpenGLTypes.h"
#include "QtAV/VideoFrame.h"
#include "ColorTransform.h"
#include <QVector4D>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>
#else
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
#include <QtOpenGL/QGLFunctions>
#endif
#include <QtOpenGL/QGLBuffer>
#include <QtOpenGL/QGLShaderProgram>
typedef QGLBuffer QOpenGLBuffer;
#define QOpenGLShaderProgram QGLShaderProgram
#define QOpenGLShader QGLShader
#define QOpenGLFunctions QGLFunctions
#define QOpenGLContext QGLContext
#endif

namespace QtAV {
// can not move to OpenGLHelper.h because that's not public/private header
enum ShaderType {
    VertexShader,
    FragmentShader,
    ShaderTypeCount
};

class VideoShader;
class Q_AV_PRIVATE_EXPORT VideoShaderPrivate : public DPtrPrivate<VideoShader>
{
public:
    VideoShaderPrivate()
        : owns_program(false)
        , rebuild_program(false)
        , update_builtin_uniforms(true)
        , program(0)
        , u_Matrix(-1)
        , u_colorMatrix(-1)
        , u_to8(-1)
        , u_opacity(-1)
        , u_c(-1)
        , material_type(0)
        , texture_target(GL_TEXTURE_2D)
    {}
    virtual ~VideoShaderPrivate() {
        if (owns_program && program) {
            if (QOpenGLContext::currentContext()) {
                // FIXME: may be not called from renderering thread. so we still have to detach shaders
                program->removeAllShaders();
            }
            delete program;
        }
        program = 0;
    }

    bool owns_program; // shader program is not created by this. e.g. scene graph create it's own program and we store it here
    bool rebuild_program;
    bool update_builtin_uniforms; //builtin uniforms are static, set the values once is enough if no change
    QOpenGLShaderProgram *program;
    int u_Matrix;
    int u_colorMatrix;
    int u_to8;
    int u_opacity;
    int u_c;
    int u_texelSize;
    int u_textureSize;
    qint32 material_type;
    QVector<int> u_Texture;
    GLenum texture_target;
    VideoFormat video_format;
    mutable QByteArray planar_frag, packed_frag;
    mutable QByteArray vert;
    QVector<Uniform> user_uniforms[ShaderTypeCount];
};

class VideoMaterial;
class VideoMaterialPrivate : public DPtrPrivate<VideoMaterial>
{
public:
    VideoMaterialPrivate()
        : update_texure(true)
        , init_textures_required(true)
        , bpc(0)
        , width(0)
        , height(0)
        , video_format(VideoFormat::Format_Invalid)
        , plane1_linesize(0)
        , effective_tex_width_ratio(1.0)
        , target(GL_TEXTURE_2D)
        , dirty(true)
        , try_pbo(true)
    {
        v_texel_size.reserve(4);
        textures.reserve(4);
        texture_size.reserve(4);
        effective_tex_width.reserve(4);
        internal_format.reserve(4);
        data_format.reserve(4);
        data_type.reserve(4);
        static bool enable_pbo = qgetenv("QTAV_PBO").toInt() > 0;
        if (try_pbo)
            try_pbo = enable_pbo;
        pbo.reserve(4);
        colorTransform.setOutputColorSpace(ColorSpace_RGB);
    }
    ~VideoMaterialPrivate();
    bool initPBO(int plane, int size);
    bool initTexture(GLuint tex, GLint internal_format, GLenum format, GLenum dataType, int width, int height);
    bool updateTextureParameters(const VideoFormat& fmt);
    void uploadPlane(int p, bool updateTexture = true);
    bool ensureResources();
    bool ensureTextures();
    void setupQuality();

    bool update_texure; // reduce upload/map times. true: new frame not bound. false: current frame is bound
    bool init_textures_required; // e.g. target changed
    int bpc;
    int width, height; //avoid accessing frame(need lock)
    VideoFrame frame;
    /*
     *  old format. used to check whether we have to update textures. set to current frame's format after textures are updated.
     * TODO: only VideoMaterial.type() is enough to check and update shader. so remove it
     */
    VideoFormat video_format;
    QSize plane0Size;
    // width is in bytes. different alignments may result in different plane 1 linesize even if plane 0 are the same
    int plane1_linesize;

    // textures.d in updateTextureParameters() changed. happens in qml. why?
    quint8 workaround_vector_crash_on_linux[8]; //TODO: remove
    QVector<GLuint> textures; //texture ids. size is plane count
    QHash<GLuint, bool> owns_texture;
    QVector<QSize> texture_size;

    QVector<int> effective_tex_width; //without additional width for alignment
    qreal effective_tex_width_ratio;
    GLenum target;
    QVector<GLint> internal_format;
    QVector<GLenum> data_format;
    QVector<GLenum> data_type;

    bool dirty;
    ColorTransform colorTransform;
    bool try_pbo;
    QVector<QOpenGLBuffer> pbo;
    QVector2D vec_to8; //TODO: vec3 to support both RG and LA (.rga, vec_to8)
    QMatrix4x4 channel_map;
    QVector<QVector2D> v_texel_size;
    QVector<QVector2D> v_texture_size;
};

} //namespace QtAV

#endif // QTAV_VideoShader_P_H
