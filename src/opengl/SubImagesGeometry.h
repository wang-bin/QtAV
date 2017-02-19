/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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
#ifndef QTAV_SUBIMAGESGEOMETRY_H
#define QTAV_SUBIMAGESGEOMETRY_H
#include "QtAV/Geometry.h"
#include "QtAV/SubImage.h"

namespace QtAV {
class SubImagesGeometry : public Geometry {
public:
    SubImagesGeometry();
    bool setSubImages(const SubImageSet& images);
    /*!
     * \brief generateVertexData
     * \param rect rect render to. If it's viewport rect, and fit video aspect ratio, ass images created from video frame size needs a scale transform is required when rendering
     * \param useIndecies
     * \param maxWidth
     * \return false if current SubImageSet is invalid
     */
    bool generateVertexData(const QRect& rect, bool useIndecies = false, int maxWidth = -1);
    // available after generateVertexData is called
    int width() { return m_w;}
    int height() { return m_h;}
    int stride() const Q_DECL_OVERRIDE;
    const QVector<Attribute>& attributes() const Q_DECL_OVERRIDE { return m_attributes;}
    const SubImageSet& images() const { return m_images; }
    const QVector<QRect>& uploadRects() const { return m_rects_upload;}
private:
    using Geometry::allocate;
    bool m_normalized;
    int m_w, m_h;
    QVector<Attribute> m_attributes;
    SubImageSet m_images; // for texture upload parameters
    QVector<QRect> m_rects_upload;
};
} //namespace QtAV
#endif //QTAV_SUBIMAGESGEOMETRY_H
