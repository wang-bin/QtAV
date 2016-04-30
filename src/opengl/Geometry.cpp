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
#include "opengl/OpenGLHelper.h"

namespace QtAV {

Attribute::Attribute(int type, int tupleSize, int offset, bool normalize)
    : m_normalize(normalize)
    , m_type(type)
    , m_tupleSize(tupleSize)
    , m_offset(offset)
{}

Attribute::Attribute(const QByteArray& name, int type, int tupleSize, int offset, bool normalize)
    : m_normalize(normalize)
    , m_type(type)
    , m_tupleSize(tupleSize)
    , m_offset(offset)
    , m_name(name)
{}

Geometry::Geometry()
    : m_primitive(TriangleStrip)
{}

TexturedGeometry::TexturedGeometry(int texCount)
    : nb_tex(texCount)
{
    setVertexCount(4);
    if (texCount < 1)
        texCount = 1;
    v.resize(nb_tex*vertexCount());
    a = QVector<Attribute>()
            << Attribute(GL_FLOAT, 2, 0)
            << Attribute(GL_FLOAT, 2, 2*sizeof(float))
               ;
}

void TexturedGeometry::setTextureCount(int value)
{
    if (value < 1)
        value = 1;
    if (value == nb_tex)
        return;
    nb_tex = value;
    v.resize(nb_tex*vertexCount());
    if (a.size()-1 < value) { // the first is position
        for (int i = a.size()-1; i < value; ++i)
            a << Attribute(GL_FLOAT, 2, i*vertexCount() * stride() + 2*sizeof(float));
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
    setGeometryPoint(index, p, texIndex);
    setTexturePoint(index, tp, texIndex);
}

void TexturedGeometry::setGeometryPoint(int index, const QPointF &p, int texIndex)
{
    v[texIndex*vertexCount() + index].x = p.x();
    v[texIndex*vertexCount() + index].y = p.y();
}

void TexturedGeometry::setTexturePoint(int index, const QPointF &tp, int texIndex)
{
    v[texIndex*vertexCount() + index].tx = tp.x();
    v[texIndex*vertexCount() + index].ty = tp.y();
}

void TexturedGeometry::setRect(const QRectF &r, const QRectF &tr, int texIndex)
{
    setPoint(0, r.topLeft(), tr.topLeft(), texIndex);
    setPoint(1, r.bottomLeft(), tr.bottomLeft(), texIndex);
    switch (primitiveType()) {
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

void TexturedGeometry::setGeometryRect(const QRectF &r, int texIndex)
{
    setGeometryPoint(0, r.topLeft(), texIndex);
    setGeometryPoint(1, r.bottomLeft(), texIndex);
    switch (primitiveType()) {
    case TriangleStrip:
        setGeometryPoint(2, r.topRight(), texIndex);
        setGeometryPoint(3, r.bottomRight(), texIndex);
        break;
    case TriangleFan:
        setGeometryPoint(3, r.topRight(), texIndex);
        setGeometryPoint(2, r.bottomRight(), texIndex);
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
    switch (primitiveType()) {
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
