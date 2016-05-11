/******************************************************************************
    QtAV:  Multimedia framework based on Qt and LibASS
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
#ifndef QTAV_SUBIMAGE_H
#define QTAV_SUBIMAGE_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QVector>

namespace QtAV {
/*!
 * \brief The SubImage struct
 * (x, y, w, h) is relative to parent SubImageSet (0, 0, SubImageSet.w, SubImageSet.h)
 */
struct SubImageSet;
struct Q_AV_EXPORT SubImage {
    SubImage(int x = 0, int y = 0, int w = 0, int h = 0, int stride = 0);
    bool operator ==(const SubImage& o) const {
        return x == o.x && y == o.y && w == o.w && h == o.h && stride == o.stride && color == o.color && data == o.data;
    }
    int x, y;
    int w, h;
    int stride;
    quint32 color; //ass only
    QByteArray data; //size = stride*h
};

struct Q_AV_EXPORT SubImageSet {
    enum Format { ASS, RGBA, Unknown };
    SubImageSet(int width = 0, int height = 0, Format format = Unknown);
    int width() const {return w;}
    int height() const {return h;}
    Format format() const {return fmt;}
    bool isValid() const {return !images.isEmpty() && w > 0 && h > 0 && fmt != Unknown;}
    void reset(int width = 0, int height = 0, Format format = Unknown) {
        fmt = format;
        w = width;
        h = height;
        ++id;
        images.clear();
    }
    bool operator ==(const SubImageSet& other) const {
        return id == other.id && w == other.w && h == other.h && fmt == other.fmt && images == other.images; //TODO: image data
    }
    QVector<SubImage> images;
private:
    Format fmt;
    int w, h;
    int id;
};
} //namespace QtAV
#endif //QTAV_SUBIMAGE_H
