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
#include <QtCore/QVector>
#include <QtAV/QtAV_Global.h>

namespace QtAV {

class Attribute {
    bool m_normalize;
    int m_type;
    int m_tupleSize, m_offset;
    QByteArray m_name;
public:
    Attribute(int type = 0, int tupleSize = 0, int offset = 0, bool normalize = true);
    Attribute(const QByteArray& name, int type, int tupleSize, int offset = 0, bool normalize = true);
    QByteArray name() const {return m_name;}
    int type() const {return m_type;}
    int tupleSize() const {return m_tupleSize;}
    int offset() const {return m_offset;}
    bool normalize() const {return m_normalize;}
};

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
        TriangleFan = 0x0006,
    };
    Geometry();
    virtual ~Geometry() {}
    PrimitiveType primitiveType() const {return m_primitive;}
    void setPrimitiveType(PrimitiveType value) {  m_primitive = value;}
    int vertexCount() const {return m_vcount;}
    void setVertexCount(int value) {m_vcount = value;}
    virtual int stride() const = 0;
    virtual const QVector<Attribute>& attributes() const = 0;
    virtual void* attributesData() const = 0;
    virtual int attributeDataSize() const = 0;
    virtual void* indexData() const {return 0;} //use index buffer if not null
private:
    PrimitiveType m_primitive;
    int m_vcount;
};

// TODO: move to OpenGLVideo.cpp and no export
class TexturedGeometry : public Geometry {
public:
    TexturedGeometry(int texCount = 1);
    /*!
     * \brief setTextureCount
     * sometimes we needs more than 1 texture coordinates, for example we have to set rectangle texture
     * coordinates for each plane.
     */
    void setTextureCount(int value);
    int textureCount() const;
    void setPoint(int index, const QPointF& p, const QPointF& tp, int texIndex = 0);
    void setGeometryPoint(int index, const QPointF& p, int texIndex = 0);
    void setTexturePoint(int index, const QPointF& tp, int texIndex = 0);
    void setRect(const QRectF& r, const QRectF& tr, int texIndex = 0);
    void setGeometryRect(const QRectF& r, int texIndex = 0);
    void setTextureRect(const QRectF& tr, int texIndex = 0);
    int stride() const Q_DECL_OVERRIDE { return sizeof(Point); }
    const QVector<Attribute>& attributes() const Q_DECL_OVERRIDE;
    void* attributesData() const Q_DECL_OVERRIDE {
        return (void*)v.data();
    }
    int attributeDataSize() const Q_DECL_OVERRIDE {
        return nb_tex * vertexCount() * stride();
    }
private:
    typedef struct {
        float x, y;
        float tx, ty;
    } Point;
    int nb_tex;
    QVector<Point> v;
    QVector<Attribute> a;
};
} //namespace QtAV
