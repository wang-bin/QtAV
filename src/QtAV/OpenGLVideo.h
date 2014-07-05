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

#ifndef QTAV_OPENGLVIDEO_H
#define QTAV_OPENGLVIDEO_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/ColorTransform.h>
#include <QtAV/VideoFormat.h>

class QOpenGLShaderProgram;
namespace QtAV {

class Q_AV_EXPORT VideoShader
{
public:
    VideoShader();
    virtual ~VideoShader();
    virtual char const *const *attributeNames() const; // Array must end with null.
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;

    /*!
     * \brief initialize
     * \param shaderProgram: 0 means create a shader program internally. if not linked, vertex/fragment shader will be added and linked
     */
    virtual void initialize(QOpenGLShaderProgram* shaderProgram = 0);

    /*!
     * \brief textureCount
     * number of texture locations is
     * 1: packed RGB
     * number of channels: yuv or plannar RGB
     * TODO: always use plannar shader and 1 tex per channel?
     * \param index
     * \return texture location in shader
     */
    int textureCount() const;
    int textureLocation(int index) const;
    int matrixLocation() const;
    int colorMatrixLocation() const;
    VideoFormat videoFormat() const;
    void setVideoFormat(const VideoFormat& format);
    // TODO: setColorTransform() ?
    ColorTransform::ColorSpace colorSpace() const;
    void setColorSpace(ColorTransform::ColorSpace cs);

protected:
    QOpenGLShaderProgram* program();
    QByteArray shaderSourceFromFile(const QString& fileName) const;
    virtual void compile(QOpenGLShaderProgram* shaderProgram);

    ColorTransform::ColorSpace m_color_space;
    QOpenGLShaderProgram *m_program;
    // TODO: compare with texture width uniform used in qtmm
    int u_MVP_matrix;
    int u_colorMatrix;
    QVector<int> u_Texture;
    VideoFormat m_video_format;
    mutable QByteArray m_planar_frag, m_packed_frag;
};

class VideoFrame;
class OpenGLVideoPrivate;
class Q_AV_EXPORT OpenGLVideo
{
    DPTR_DECLARE_PRIVATE(OpenGLVideo)
public:
    void setCurrentFrame(const VideoFrame& frame);
    void bind();
    void bindPlane(int plane);
    int compare(const OpenGLVideo* other) const;
protected:
    DPTR_DECLARE(OpenGLVideo)
};

} //namespace QtAV

#endif // QTAV_OPENGLVIDEO_H
