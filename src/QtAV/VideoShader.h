/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_VIDEOSHADER_H
#define QTAV_VIDEOSHADER_H

#include <QtAV/VideoFrame.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLShaderProgram>
#else
#include <QtOpenGL/QGLShaderProgram>
#define QOpenGLShaderProgram QGLShaderProgram
#endif

class QOpenGLShaderProgram;
namespace QtAV {

class VideoMaterial;
class VideoShaderPrivate;
/*!
 * \brief The VideoShader class
 * Represents a shader for rendering a video frame.
 * Low-level api. Used by OpenGLVideo and Scene Graph.
 */
class Q_AV_EXPORT VideoShader
{
    DPTR_DECLARE_PRIVATE(VideoShader)
public:
    VideoShader();
    virtual ~VideoShader();
    /*!
     * \brief attributeNames
     * Array must end with null. { position, texcoord, ..., 0}, location is bound to 0, 1, ...
     * \return
     */
    virtual char const *const *attributeNames() const;
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;
    /*!
     * \brief initialize
     * \param shaderProgram: 0 means create a shader program internally. if not linked, vertex/fragment shader will be added and linked
     */
    // initialize(VideoMaterial*, QOpenGLShaderProgram*)
    virtual void initialize(QOpenGLShaderProgram* shaderProgram = 0);
    /*!
     * \brief textureLocationCount
     * number of texture locations is
     * 1: packed RGB
     * number of channels: yuv or plannar RGB
     * TODO: always use plannar shader and 1 tex per channel?
     * \param index
     * \return texture location in shader
     */
    int textureLocationCount() const;
    int textureLocation(int index) const;
    int matrixLocation() const;
    int colorMatrixLocation() const;
    int opacityLocation() const;
    int channelMapLocation() const;
    VideoFormat videoFormat() const;
    // defalut is GL_TEXTURE_2D
    int textureTarget() const;
    QOpenGLShaderProgram* program();
    bool update(VideoMaterial* material);
protected:
    QByteArray shaderSourceFromFile(const QString& fileName) const;
    virtual void compile(QOpenGLShaderProgram* shaderProgram);

    VideoShader(VideoShaderPrivate &d);
    DPTR_DECLARE(VideoShader)
private:
    void setVideoFormat(const VideoFormat& format);
    void setTextureTarget(int type);
    friend class VideoMaterial;
};

class VideoMaterialPrivate;
/*!
 * \brief The VideoMaterial class
 * Encapsulates rendering state for a video shader program.
 * Low-level api. Used by OpenGLVideo and Scene Graph
 */
class Q_AV_EXPORT VideoMaterial
{
    DPTR_DECLARE_PRIVATE(VideoMaterial)
public:
    VideoMaterial();
    virtual ~VideoMaterial() {}
    void setCurrentFrame(const VideoFrame& frame);
    VideoFormat currentFormat() const;
    VideoShader* createShader() const;
    virtual qint64 type() const;
    static QString typeName(qint64 value);

    bool bind(); // TODO: roi
    void unbind();
    int compare(const VideoMaterial* other) const;

    bool hasAlpha() const;
    const QMatrix4x4 &colorMatrix() const;
    const QMatrix4x4& matrix() const;
    const QMatrix4x4& channelMap() const;
    int bpp() const; //1st plane
    QVector2D vectorTo8bit() const;
    int planeCount() const;
    /*!
     * \brief validTextureWidth
     * Value is (0, 1]. Normalized valid width of a plane.
     * A plane may has padding invalid data at the end for aligment.
     * Use this value to reduce texture coordinate computation.
     * ---------------------
     * |                |   |
     * | valid width    |   |
     * |                |   |
     * ----------------------
     * | <- aligned width ->|
     * \return valid width ratio
     */
    qreal validTextureWidth() const;
    QSize frameSize() const;
    /*!
     * \brief normalizedROI
     * \param roi logical roi of a video frame.
     * the same as mapToTexture(roi, 1)
     */
    QRectF normalizedROI(const QRectF& roi) const;
    /*!
     * \brief mapToFrame
     * map a point p or a rect r to video texture in a given plane and scaled to valid width.
     * p or r is in video frame's rect coordinates, no matter which plane is
     * \param normalize -1: auto(do not normalize for rectangle texture). 0: no. 1: yes
     * \return
     * point or rect in current texture valid coordinates. \sa validTextureWidth()
     */
    QPointF mapToTexture(int plane, const QPointF& p, int normalize = -1) const;
    QRectF mapToTexture(int plane, const QRectF& r, int normalize = -1) const;
    void setBrightness(qreal value);
    void setContrast(qreal value);
    void setHue(qreal value);
    void setSaturation(qreal value);
protected:
    // TODO: roi
    // whether to update texture is set internal
    void bindPlane(int p, bool updateTexture = true);
    VideoMaterial(VideoMaterialPrivate &d);
    DPTR_DECLARE(VideoMaterial)
};

class Q_AV_EXPORT TexturedGeometry {
public:
    typedef struct {
        float x, y;
        float tx, ty;
    } Point;
    enum Triangle { Strip, Fan };
    TexturedGeometry(int texCount = 1, int count = 4, Triangle t = Strip);
    /*!
     * \brief setTextureCount
     * sometimes we needs more than 1 texture coordinates, for example we have to set rectangle texture
     * coordinates for each plane.
     */
    void setTextureCount(int value);
    int textureCount() const;
    /*!
     * \brief size
     * totoal data size in bytes
     */
    int size() const;
    /*!
     * \brief textureSize
     * data size of 1 texture. equals textureVertexCount()*stride()
     */
    int textureSize() const;
    Triangle triangle() const { return tri;}
    int mode() const;
    int tupleSize() const { return 2;}
    int stride() const { return sizeof(Point); }
    /// vertex count per texture
    int textureVertexCount() const { return points_per_tex;}
    /// totoal vertex count
    int vertexCount() const { return v.size(); }
    void setPoint(int index, const QPointF& p, const QPointF& tp, int texIndex = 0);
    void setGeometryPoint(int index, const QPointF& p, int texIndex = 0);
    void setTexturePoint(int index, const QPointF& tp, int texIndex = 0);
    void setRect(const QRectF& r, const QRectF& tr, int texIndex = 0);
    void setGeometryRect(const QRectF& r, int texIndex = 0);
    void setTextureRect(const QRectF& tr, int texIndex = 0);
    void* data(int idx = 0, int texIndex = 0) { return (char*)v.data() + texIndex*textureSize() + idx*2*sizeof(float); } //convert to char* float*?
    const void* data(int idx = 0, int texIndex = 0) const { return (char*)v.constData() + texIndex*textureSize() + idx*2*sizeof(float); }
    const void* constData(int idx = 0, int texIndex = 0) const { return (char*)v.constData() + texIndex*textureSize() + idx*2*sizeof(float); }
private:
    Triangle tri;
    int points_per_tex;
    int nb_tex;
    QVector<Point> v;
};

} //namespace QtAV

#endif // QTAV_VIDEOSHADER_H
