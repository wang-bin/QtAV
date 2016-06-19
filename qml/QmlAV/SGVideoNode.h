/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
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

#ifndef QTAV_SGVIDEONODE_H
#define QTAV_SGVIDEONODE_H

#include <QtQuick/QSGGeometryNode>
#include <QtAV/VideoFrame.h>

namespace QtAV {

class SGVideoMaterial;
class SGVideoNode : public QSGGeometryNode
{
public:
    SGVideoNode();
    ~SGVideoNode();
    virtual void setCurrentFrame(const VideoFrame &frame);
    /* Update the vertices and texture coordinates.  Orientation must be in {0,90,180,270} */
    void setTexturedRectGeometry(const QRectF &boundingRect, const QRectF &textureRect, int orientation);
private:
    SGVideoMaterial *m_material;
    QRectF m_rect;
    QRectF m_textureRect;
    int m_orientation;
    qreal m_validWidth;
};

} //namespace QtAV
#endif // QTAV_SGVIDEONODE_H
