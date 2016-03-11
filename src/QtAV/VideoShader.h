/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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
    int texelSizeLocation() const;
    VideoFormat videoFormat() const;
    // defalut is GL_TEXTURE_2D
    int textureTarget() const;
    QOpenGLShaderProgram* program();
    /*!
     * \brief update
     * Upload textures, setup uniforms before rendering.
     * If material type changed, build a new shader program.
     */
    bool update(VideoMaterial* material);

    int uniformLocation(const char* name) const;

    /// User configurable shader APIs BEGIN
    /*!
     * Keywords will be replaced in user shader code:
     * %planes%, %planes-1% => plane count, plane count -1
     * Uniforms can be used: (N: 0 ~ planes-1)
     * u_TextureN, v_TexCoordsN, u_texelSize(array of vec2), u_opacity, u_c(channel map), u_colorMatrix, u_to8(vec2, computing 16bit value with 8bit components)
     */
    /*!
     * \brief userUniforms
     * The additional uniforms will be used in shader.
     * TODO: add to shader code automatically(need uniformType(const char*name))?
     * \return
     */
    virtual QStringList userUniforms() const { return QStringList();}
    /*!
     * \brief userFragmentShaderHeader
     * Must add additional uniform declarations here
     */
    virtual const char* userFragmentShaderHeader() const {return 0;}
    /*!
     * \brief setUserUniformValues
     * If return false (not implemented for example), fallback to call setUserUniformValue(name) for each userUniforms()
     * Call program()->setUniformValue(...) here
     */
    virtual void setUserUniformValues() {}
    /*!
     * \brief userSample
     * The custom sampling function to replace texture2D()/texture() (replace %1 in shader).
     * \code
     *     vec4 sample2d(sampler2D tex, vec2 pos) { .... }
     * \endcode
     */
    virtual const char* userSample() const { return 0;}
    /*!
     * \brief userColorTransform
     * Override the default color transform matrix.
     * TODO: no override, just partial? call it everytime?
     * \return
     */
    virtual QMatrix4x4 userColorTransform() const {return QMatrix4x4();}
    /*!
     * \brief userPostProcess
     * Process rgb color
     */
    virtual const char* userPostProcess() const {return 0;}
    /*!
     * \brief userUpload
     * It's the code in C++ side. You can upload a texture for blending in userPostProcessor(), or LUT texture used by userSampler() or userPostProcessor() etc.
     */
    virtual bool userUpload() {return false;}
    /// User configurable shader APIs BEGIN

protected:
    QByteArray shaderSourceFromFile(const QString& fileName) const;
    void build(QOpenGLShaderProgram* shaderProgram);

    VideoShader(VideoShaderPrivate &d);
    DPTR_DECLARE(VideoShader)
private:
    void setVideoFormat(const VideoFormat& format);
    void setTextureTarget(int type);
    void setMaterialType(qint32 value);
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
    void initializeShader(VideoShader* shader) const;
    virtual qint32 type() const;
    static QString typeName(qint32 value);

    bool bind(); // TODO: roi
    void unbind();
    int compare(const VideoMaterial* other) const;

    bool hasAlpha() const;
    const QMatrix4x4 &colorMatrix() const;
    const QMatrix4x4& matrix() const;
    const QMatrix4x4& channelMap() const;
    int bitsPerComponent() const; //0 if the value of components are different
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
     * \brief texelSize
     * The size of texture unit
     * \return (1.0/textureWidth, 1.0/textureHeight)
     */
    QSizeF texelSize(int plane) const; //vec2?
    QVector<QVector2D> texelSize() const;
    /*!
     * \brief textureSize
     * It can be used with a uniform to emulate GLSL textureSize() which exists in new versions.
     */
    QSize textureSize(int plane) const;
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
