/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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
#ifndef QTAV_GEOMETRY_H
#define QTAV_GEOMETRY_H
#include <QtCore/QRectF>
#include <QtCore/QVector>
#include <QtAV/QtAV_Global.h>

// TODO: generate vertex/fragment shader code from Geometry.attributes()
namespace QtAV {

enum DataType { //equals to GL_BYTE etc.
    TypeByte = 0x1400,
    TypeUnsignedByte = 0x1401,
    TypeShort = 0x1402,
    TypeUnsignedShort = 0x1403,
    TypeInt = 0x1404,
    TypeUnsignedInt = 0x1405,
    TypeFloat = 0x1406
};

class Attribute {
    bool m_normalize;
    DataType m_type;
    int m_tupleSize, m_offset;
    QByteArray m_name;
public:
    Attribute(DataType type = TypeFloat, int tupleSize = 0, int offset = 0, bool normalize = true);
    Attribute(const QByteArray& name, DataType type = TypeFloat, int tupleSize = 0, int offset = 0, bool normalize = true);
    QByteArray name() const {return m_name;}
    DataType type() const {return m_type;}
    int tupleSize() const {return m_tupleSize;}
    int offset() const {return m_offset;}
    bool normalize() const {return m_normalize;}
};
#ifndef QT_NO_DEBUG_STREAM
Q_AV_EXPORT QDebug operator<<(QDebug debug, const Attribute &a);
#endif

/*!
 * \brief The Geometry class
 * \code
 * foreach (const Attribute& a, g->attributes()) {
 *     program()->setAttributeBuffer(a.name(), a.type(), a.offset(), a.tupleSize(), g->stride());
 * }
 * \endcode
 */
class Geometry {
public:
    /// Strip or Triangles is preferred by ANGLE. The values are equal to opengl
    enum PrimitiveType {
        Triangles = 0x0004,
        TriangleStrip = 0x0005, //default
        TriangleFan = 0x0006, // Not recommended
    };
    Geometry(int vertexCount = 0, int indexCount = 0, DataType indexType = TypeUnsignedShort);
    virtual ~Geometry() {}
    PrimitiveType primitiveType() const {return m_primitive;}
    void setPrimitiveType(PrimitiveType value) {  m_primitive = value;}
    int vertexCount() const {return m_vcount;}
    void setVertexCount(int value) {m_vcount = value;} // TODO: remove, or allocate data here
    // TODO: setStride and no virtual
    virtual int stride() const = 0;
    // TODO: add/set/remove attributes()
    virtual const QVector<Attribute>& attributes() const = 0;
    void* vertexData() { return m_vdata.data();}
    const void* vertexData() const { return m_vdata.constData();}
    void* indexData() { return m_icount > 0 ? m_idata.data() : NULL;}
    const void* indexData() const { return m_icount > 0 ? m_idata.constData() : NULL;}
    int indexCount() const { return m_icount;}
    // GL_UNSIGNED_BYTE/SHORT/INT
    DataType indexType() const {return m_itype;}
    void setIndexType(DataType value) { m_itype = value;}
    /*!
     * \brief allocate
     * Call allocate() when all parameters are set. vertexData() may change
     */
    void allocate(int nbVertex, int nbIndex = 0);
protected:
    PrimitiveType m_primitive;
    DataType m_itype;
    int m_vcount;
    int m_icount;
    QByteArray m_vdata;
    QByteArray m_idata;
};

class TexturedGeometry : public Geometry {
public:
    TexturedGeometry();
    /*!
     * \brief setTextureCount
     * sometimes we needs more than 1 texture coordinates, for example we have to set rectangle texture
     * coordinates for each plane.
     */
    void setTextureCount(int value);
    int textureCount() const;
    void setPoint(int index, const QPointF& p, const QPointF& tp, int texIndex = 0);
    void setGeometryPoint(int index, const QPointF& p);
    void setTexturePoint(int index, const QPointF& tp, int texIndex = 0);
    void setRect(const QRectF& r, const QRectF& tr, int texIndex = 0);
    void setGeometryRect(const QRectF& r);
    void setTextureRect(const QRectF& tr, int texIndex = 0);
    int stride() const Q_DECL_OVERRIDE { return 2*sizeof(float)*(textureCount()+1); }
    const QVector<Attribute>& attributes() const Q_DECL_OVERRIDE;
private:
    int nb_tex;
    QVector<Attribute> a;
};
} //namespace QtAV
#endif //QTAV_GEOMETRY_H
