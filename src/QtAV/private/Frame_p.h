/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

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

#ifndef QTAV_FRAME_P_H
#define QTAV_FRAME_P_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QVector>
#include <QtCore/QVariant>
#include <QtCore/QSharedData>

namespace QtAV {

class Frame;
class FramePrivate : public QSharedData
{
    Q_DISABLE_COPY(FramePrivate)
public:
    FramePrivate()
        : timestamp(0)
        , data_align(1)
    {}
    virtual ~FramePrivate() {}

    QVector<uchar*> planes; //slice
    QVector<int> line_sizes; //stride
    QVariantMap metadata;
    QByteArray data;
    qreal timestamp;
    int data_align;
};

} //namespace QtAV

#endif // QTAV_Frame_P_H
