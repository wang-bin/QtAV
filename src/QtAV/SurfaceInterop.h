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
    /*!
     * \brief map
     * currently is used to map a frame from hardware decoder to opengl texture, host memory.
     * \param type currently only support GLTextureSurface and HostMemorySurface for some decoders
     * \param fmt
     *   HostMemorySurface: must be a packed rgb format
     * \param handle address of real handle
     *   GLTextureSurface: usually opengl texture. maybe other objects for some decoders in the feature
     *   HostMemorySurface: a VideoFrame ptr
     * \param plane
     * \return Null if not supported or failed. handle if success.
     */
    virtual void* map(SurfaceType type, const VideoFormat& fmt, void* handle = 0, int plane = 0) {
        Q_UNUSED(type);
        Q_UNUSED(fmt);
        Q_UNUSED(handle);
        Q_UNUSED(plane);
        return 0;
    }
    // TODO: SurfaceType. unmap is currenty used by opengl rendering
    virtual void unmap(void* handle) { Q_UNUSED(handle);}
    /*!
     * \brief createHandle
     * It is used by opengl renderer to create a texture when rendering frame from VDA decoder
     * \return NULL if not used when for opengl rendering. handle if create here
     */
    virtual void* createHandle(void* handle, SurfaceType type, const VideoFormat &fmt, int plane, int planeWidth, int planeHeight) {
        Q_UNUSED(handle);
        Q_UNUSED(type);
        Q_UNUSED(fmt);
        Q_UNUSED(plane);
        Q_UNUSED(planeWidth);
        Q_UNUSED(planeHeight);
        return 0;
    }
};

typedef QSharedPointer<VideoSurfaceInterop> VideoSurfaceInteropPtr;
} //namespace QtAV

Q_DECLARE_METATYPE(QtAV::VideoSurfaceInteropPtr)

#endif // QTAV_SURFACEINTEROP_H
