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
#include "SubImagesGeometry.h"
#include "utils/Logger.h"

namespace QtAV {
static const int kMaxTexWidth = 4096; //FIXME: glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
// if texture1d, we can directly copy ASS_Image.bitmap without line by line copy, i.e. tiled, and upload only once


typedef struct {
    float x, y; // depends on target rect
    float tx, ty; // depends on texture size and rects layout
    union {
        quint8 r, g, b, a; //to be normalized
        quint32 rgba;
    };
} VertexData;

static VertexData* SetUnnormalizedVertexData(VertexData* v, int tx, int ty, int tw, int th, qint32 color, bool useIndecies)
{
    union {
        quint8 r, g, b, a;
        quint32 rgba;
    };
    r = color >> 24;
    g = (color >> 16) & 0xff;
    b = (color >> 8) & 0xff;
    a = 255 - (color & 0xff);
    // normalize later
    v[0].rgba = rgba;
    v[0].tx = tx;
    v[0].ty = ty;
    v[1].rgba = rgba;
    v[1].tx = tx;
    v[1].ty = ty + th;
    v[2].rgba = rgba;
    v[2].tx = tx + tw;
    v[2].ty = ty;
    v[3].rgba = rgba;
    v[3].tx = tx + tw;
    v[3].ty = ty + th;
    if (!useIndecies) {
        v[4] = v[1];
        v[5] = v[2];
        return v + 6;
    }
    return v + 4;
}

static VertexData* SetVertexPositionAndNormalize(VertexData* v, float x, float y, float w, float h, float texW, float texH, bool useIndecies)
{
    v[0].x = x;
    v[0].y = y;
    v[1].x = x;
    v[1].y = y + h;
    v[2].x = x + w;
    v[2].y = y;
    v[3].x = x + w;
    v[3].y = y + h;
    v[0].tx /= texW;
    v[0].ty /= texH;
    v[1].tx /= texW;
    v[1].ty /= texH;
    v[2].tx /= texW;
    v[2].ty /= texH;
    v[3].tx /= texW;
    v[3].ty /= texH;
    qDebug("%f,%f<=%f,%f; %d,%d,%d,%d", v[3].x, v[3].y, v[3].tx, v[3].ty, v[3].r, v[3].g, v[3].b, v[3].a);
    if (!useIndecies) {
        v[4] = v[1];
        v[5] = v[2];
        return v + 6;
    }
    return v + 4;
}

SubImagesGeometry::SubImagesGeometry()
    : Geometry()
    , m_normalized(false)
    , m_w(0)
    , m_h(0)
{
    setPrimitiveType(Geometry::Triangles);
    m_attributes << Attribute(TypeFloat, 2)
                 << Attribute(TypeFloat, 2, 2*sizeof(float))
                 << Attribute(TypeUnsignedByte, 4, 4*sizeof(float), true);
}

bool SubImagesGeometry::setSubImages(const SubImageSet &images)
{
    // TODO: operator ==
    if (m_images.id == images.id && m_images.w == images.w && m_images.h == images.h && m_images.format == images.format)
        return false;
    m_images = images;
    return true;
}

bool SubImagesGeometry::generateVertexData(const QRect &rect, bool useIndecies, int maxWidth)
{
    if (useIndecies)
        allocate(4*m_images.subimages.size(), 6*m_images.subimages.size());
    else
        allocate(6*m_images.subimages.size());
    qDebug("images: %d/%d, %dx%d", m_images.isValid(), m_images.subimages.size(), m_images.w, m_images.h);
    m_rects_upload.clear();
    m_w = m_h = 0;
    m_normalized = false;
    if (!m_images.isValid())
        return false;

    int W = 0, H = 0;
    int x = 0, h = 0;
    VertexData* vd = (VertexData*)vertexData();
    foreach (const SubImage& i, m_images.subimages) {
        if (x + i.w >= maxWidth && maxWidth > 0) {
            W = qMax(W, x);
            H += h + 1;
            x = 0;
            h = 0;
        }
        // we use w instead of stride even if we must upload stride. when maping texture coordinates and view port coordinates, we can use the visual rects instead of stride, i.e. the geometry vertices are (x, y, w, h), not (x, y, stride, h)
        m_rects_upload.append(QRect(x, H, i.stride, i.h));
        vd = SetUnnormalizedVertexData(vd, x, H, i.w, i.h, i.color, useIndecies);
        // if an edge is shared by 2 rects (normalized), wen can't know the attribute values on the edge
        x += i.w + 1;
        h = qMax(h, i.h);
    }
    W = qMax(W, x);
    H += h + 1;
    m_w = W;
    m_h = H;
    qDebug("sub texture %dx%d", m_w, m_h);

    const float dx0 = rect.x();
    const float dy0 = rect.y();
    const float sx = float(rect.width())/float(m_images.w);
    const float sy = float(rect.height())/float(m_images.h);
    vd = (VertexData*)vertexData();
    foreach (const SubImage& i, m_images.subimages) {
        qDebug() << rect;
        qDebug("i: %d,%d", i.x, i.y);
        vd = SetVertexPositionAndNormalize(vd, dx0 + float(i.x)*sx, dy0 + float(i.y)*sy, i.w*sx, i.h*sy, m_w, m_h, useIndecies);
        m_normalized = true;
    }
    return true;
}

int SubImagesGeometry::stride() const
{
    return sizeof(VertexData);
}

} //namespace QtAV
