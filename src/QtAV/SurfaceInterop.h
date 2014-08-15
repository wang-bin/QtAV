/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

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

#ifndef QTAV_SURFACEINTEROP_H
#define QTAV_SURFACEINTEROP_H

#include <QtCore/QMetaType>
#include <QtCore/QSharedPointer>
#include <QtAV/CommonTypes.h>
#include <QtAV/VideoFormat.h>

namespace QtAV {

class Q_AV_EXPORT VideoSurfaceInterop
{
public:
    virtual ~VideoSurfaceInterop() {}
    // return 0 if not supported. dxva: to host mem or gl texture
    // handle: address of real handle. can be address of a given texture. generate a new one and return it if handle is null
    virtual void* map(SurfaceType type, const VideoFormat& fmt, void* handle = 0, int plane = 0) {
        Q_UNUSED(type);
        Q_UNUSED(fmt);
        Q_UNUSED(handle);
        Q_UNUSED(plane);
        return 0;
    }
    virtual void unmap(void* handle) { Q_UNUSED(handle);}
};

typedef QSharedPointer<VideoSurfaceInterop> VideoSurfaceInteropPtr;
} //namespace QtAV

Q_DECLARE_METATYPE(QtAV::VideoSurfaceInteropPtr)

#endif // QTAV_SURFACEINTEROP_H
