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

#ifndef QTAV_SURFACEINTEROPD3D11_H
#define QTAV_SURFACEINTEROPD3D11_H

#include "QtAV/SurfaceInterop.h"
#include "directx/dxcompat.h"
#include <d3d11.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;
namespace QtAV {

namespace d3d11 {
enum InteropType {
    InteropAuto,
    InteropEGL,
    InteropGL //NOT IMPLEMENTED
};

class InteropResource
{
public:
    typedef unsigned int GLuint;
    static InteropResource* create(InteropType type = InteropAuto);
    /*!
     * \brief isSupported
     * \return true if support 0-copy interop. Currently only d3d11+egl, i.e. check egl build environment and runtime egl support.
     */
    static bool isSupported(InteropType type = InteropAuto);

    virtual ~InteropResource() {}
    void setDevice(ComPtr<ID3D11Device> dev);
    virtual VideoFormat::PixelFormat format(DXGI_FORMAT dxfmt) const = 0;
    /*!
     * \brief map
     * \param surface dxva decoded surface
     * \param index ID3D11Texture2D array  index
     * \param tex opengl texture
     * \param w frame width(visual width) without alignment, <= dxva surface width
     * \param h frame height(visual height)
     * \param plane useless now
     * \return true if success
     */
    virtual bool map(ComPtr<ID3D11Texture2D> surface, int index, GLuint tex, int w, int h, int plane) = 0;
    virtual bool unmap(GLuint tex) { Q_UNUSED(tex); return true;}
protected:
    ComPtr<ID3D11Device> d3ddev;
    int width, height; // video frame width and dx_surface width without alignment, not dxva decoded surface width
};
typedef QSharedPointer<InteropResource> InteropResourcePtr;


class SurfaceInterop Q_DECL_FINAL: public VideoSurfaceInterop
{
public:
    SurfaceInterop(const InteropResourcePtr& res) : m_resource(res), frame_width(0), frame_height(0) {}
    /*!
     * \brief setSurface
     * \param surface d3d11 decoded surface
     * \param index ID3D11Texture2D array  index
     * \param frame_w frame width(visual width) without alignment, <= d3d11 surface width
     * \param frame_h frame height(visual height)
     */
    void setSurface(ComPtr<ID3D11Texture2D> surface, int index, int frame_w, int frame_h);
    /// GLTextureSurface only supports rgb32
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
protected:
    /// copy from gpu (optimized if possible) and convert to target format if necessary
    void* mapToHost(const VideoFormat &format, void *handle, int plane);
private:
    ComPtr<ID3D11Texture2D> m_surface;
    int m_index;
    InteropResourcePtr m_resource;
    int frame_width, frame_height;
};
} //namespace d3d11
} //namespace QtAV
#endif //QTAV_SURFACEINTEROPD3D11_H
