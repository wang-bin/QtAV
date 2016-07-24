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

#include "Geometry.h"
#include <QtDebug>

namespace QtAV {

Attribute::Attribute(DataType type, int tupleSize, int offset, bool normalize)
    : m_normalize(normalize)
    , m_type(type)
    , m_tupleSize(tupleSize)
    , m_offset(offset)
{}

Attribute::Attribute(const QByteArray& name, DataType type, int tupleSize, int offset, bool normalize)
    : m_normalize(normalize)
    , m_type(type)
    , m_tupleSize(tupleSize)
    , m_offset(offset)
    , m_name(name)
{}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const Attribute &a) {
    dbg.nospace() << "attribute: " << a.name();
    dbg.nospace() << ", offset " << a.offset();
    dbg.nospace() << ", tupleSize " << a.tupleSize();
    dbg.nospace() << ", dataType " << a.type();
    dbg.nospace() << ", normalize " << a.normalize();
    return dbg.space();
}
#endif

Geometry::Geometry(int vertexCount, int indexCount, DataType indexType)
    : m_primitive(TriangleStrip)
    , m_itype(indexType)
    , m_vcount(vertexCount)
    , m_icount(indexCount)
{}

int Geometry::indexDataSize() const
{
    switch (indexType()) {
    case TypeU16: return indexCount()*2;
    case TypeU32: return indexCount()*4;
    default: return indexCount();
    }
}

void Geometry::setIndexValue(int index, int value)
{
    switch (indexType()) {
    case TypeU8: {
        quint8* d = (quint8*)m_idata.constData();
        *(d+index) = value;
    }
        break;
    case TypeU16: {
        quint16* d = (quint16*)m_idata.constData();
        *(d+index) = value;
    }
        break;
    case TypeU32: {
        quint32* d = (quint32*)m_idata.constData();
        *(d+index) = value;
    }
        break;
    default:
        break;
    }
}

void Geometry::setIndexValue(int index, int v1, int v2, int v3)
{
    switch (indexType()) {
    case TypeU8: {
        quint8* d = (quint8*)m_idata.constData();
        *(d+index++) = v1;
        *(d+index++) = v2;
        *(d+index++) = v2;
    }
        break;
    case TypeU16: {
        quint16* d = (quint16*)m_idata.constData();
        *(d+index++) = v1;
        *(d+index++) = v2;
        *(d+index++) = v3;
    }
        break;
    case TypeU32: {
        quint32* d = (quint32*)m_idata.constData();
        *(d+index++) = v1;
        *(d+index++) = v2;
        *(d+index++) = v3;
    }
        break;
    default:
        break;
    }
}

void Geometry::allocate(int nbVertex, int nbIndex)
{
    m_icount = nbIndex;
    m_vcount = nbVertex;
    m_vdata.resize(nbVertex*stride());
    if (nbIndex <= 0) {
        m_idata.clear(); // required?
        return;
    }
    switch (indexType()) {
    case TypeU8:
        m_idata.resize(nbIndex*sizeof(quint8));
        break;
    case TypeU16:
        m_idata.resize(nbIndex*sizeof(quint16));
        break;
    case TypeU32:
        m_idata.resize(nbIndex*sizeof(quint32));
        break;
    default:
        break;
    }
}

bool Geometry::compare(const Geometry *other) const
{
    if (this == other)
        return true;
    if (!other)
        return false;
    if (stride() != other->stride())
        return false;
    return attributes() == other->attributes();
}

TexturedGeometry::TexturedGeometry()
    : Geometry()
    , nb_tex(0)
{
    setVertexCount(4);
    a = QVector<Attribute>()
            << Attribute(TypeF32, 2, 0)
            << Attribute(TypeF32, 2, 2*sizeof(float))
               ;
    setTextureCount(1);
}

void TexturedGeometry::setTextureCount(int value)
{
    if (value < 1)
        value = 1;
    if (value == nb_tex)
        return;
    nb_tex = value;
    allocate(vertexCount());
    if (a.size()-1 < value) { // the first is position
        for (int i = a.size()-1; i < value; ++i)
            a << Attribute(TypeF32, 2, (i+1)* 2*sizeof(float));
    } else {
        a.resize(value + 1);
    }
}

int TexturedGeometry::textureCount() const
{
    return nb_tex;
}

void TexturedGeometry::setPoint(int index, const QPointF &p, const QPointF &tp, int texIndex)
{
    setGeometryPoint(index, p);
    setTexturePoint(index, tp, texIndex);
}

void TexturedGeometry::setGeometryPoint(int index, const QPointF &p)
{
    float *v = (float*)(m_vdata.constData() + index*stride());
    *v = p.x();
    *(v+1) = p.y();
}

void TexturedGeometry::setTexturePoint(int index, const QPointF &tp, int texIndex)
{
    float *v = (float*)(m_vdata.constData() + index*stride() + (texIndex+1)*2*sizeof(float));
    *v = tp.x();
    *(v+1) = tp.y();
}

void TexturedGeometry::setRect(const QRectF &r, const QRectF &tr, int texIndex)
{
    setPoint(0, r.topLeft(), tr.topLeft(), texIndex);
    setPoint(1, r.bottomLeft(), tr.bottomLeft(), texIndex);
    switch (primitive()) {
    case TriangleStrip:
        setPoint(2, r.topRight(), tr.topRight(), texIndex);
        setPoint(3, r.bottomRight(), tr.bottomRight(), texIndex);
        break;
    case TriangleFan:
        setPoint(3, r.topRight(), tr.topRight(), texIndex);
        setPoint(2, r.bottomRight(), tr.bottomRight(), texIndex);
        break;
    case Triangles:
        break;
    default:
        break;
    }
}

void TexturedGeometry::setGeometryRect(const QRectF &r)
{
    setGeometryPoint(0, r.topLeft());
    setGeometryPoint(1, r.bottomLeft());
    switch (primitive()) {
    case TriangleStrip:
        setGeometryPoint(2, r.topRight());
        setGeometryPoint(3, r.bottomRight());
        break;
    case TriangleFan:
        setGeometryPoint(3, r.topRight());
        setGeometryPoint(2, r.bottomRight());
        break;
    case Triangles:
        break;
    default:
        break;
    }
}

void TexturedGeometry::setTextureRect(const QRectF &tr, int texIndex)
{
    setTexturePoint(0, tr.topLeft(), texIndex);
    setTexturePoint(1, tr.bottomLeft(), texIndex);
    switch (primitive()) {
    case TriangleStrip:
        setTexturePoint(2, tr.topRight(), texIndex);
        setTexturePoint(3, tr.bottomRight(), texIndex);
        break;
    case TriangleFan:
        setTexturePoint(3, tr.topRight(), texIndex);
        setTexturePoint(2, tr.bottomRight(), texIndex);
        break;
    case Triangles:
        break;
    default:
        break;
    }
}

const QVector<Attribute>& TexturedGeometry::attributes() const
{
    return a;
}
} //namespace QtAV
