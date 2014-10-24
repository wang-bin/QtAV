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
    int bppLocation() const;
    int opacityLocation() const;
    VideoFormat videoFormat() const;
    void setVideoFormat(const VideoFormat& format);
    QOpenGLShaderProgram* program();
    bool update(VideoMaterial* material);
protected:
    QByteArray shaderSourceFromFile(const QString& fileName) const;
    virtual void compile(QOpenGLShaderProgram* shaderProgram);

    VideoShader(VideoShaderPrivate &d);
    DPTR_DECLARE(VideoShader)
};

class MaterialType {};
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
    virtual MaterialType* type() const;

    bool bind(); // TODO: roi
    void unbind();
    void bindPlane(int p); // TODO: roi
    int compare(const VideoMaterial* other) const;

    bool hasAlpha() const;
    const QMatrix4x4 &colorMatrix() const;
    const QMatrix4x4& matrix() const;
    int bpp() const; //1st plane
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
    /*!
     * \brief normalizedROI
     * \param roi logical roi of a video frame
     * \return
     * valid and normalized roi. \sa validTextureWidth()
     */
    QRectF normalizedROI(const QRectF& roi) const;
    void setBrightness(qreal value);
    void setContrast(qreal value);
    void setHue(qreal value);
    void setSaturation(qreal value);
protected:
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
    TexturedGeometry(int count = 4, Triangle t = Strip);
    Triangle triangle() const { return tri;}
    int mode() const;
    int stride() const { return sizeof(Point); }
    int vertexCount() const { return v.size(); }
    void setPoint(int index, const QPointF& p, const QPointF& tp);
    void setGeometryPoint(int index, const QPointF& p);
    void setTexturePoint(int index, const QPointF& tp);
    void setRect(const QRectF& r, const QRectF& tr);
    void* data(int idx = 0) { return (char*)v.data() + idx*2*sizeof(float); } //convert to char* float*?
    const void* data(int idx = 0) const { return (char*)v.constData() + idx*2*sizeof(float); }
private:
    Triangle tri;
    QVector<Point> v;
};

} //namespace QtAV

#endif // QTAV_VIDEOSHADER_H
