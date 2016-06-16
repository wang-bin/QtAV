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

#ifndef QTAV_SURFACEINTEROPVAAPI_H
#define QTAV_SURFACEINTEROPVAAPI_H

#include <QtCore/qglobal.h>
#include "vaapi_helper.h"
#define VA_X11_INTEROP 1
#ifndef QT_NO_OPENGL
#include <QtCore/QMap>
#include <QtCore/QSharedPointer>
#include "QtAV/SurfaceInterop.h"

namespace QtAV {
namespace vaapi {

bool checkEGL_DMA();
bool checkEGL_Pixmap();

class InteropResource
{
public:
    virtual ~InteropResource() {}
    // egl supports yuv extension
    /*!
     * \brief map
     * \param surface va decoded surface
     * \param tex opengl texture
     * \param w frame width(visual width) without alignment, <= dxva surface width
     * \param h frame height(visual height)
     * \param plane useless now
     * \return true if success
     */
    virtual bool map(const surface_ptr &surface, GLuint tex, int w, int h, int plane) = 0;
    virtual bool unmap(const surface_ptr &surface, GLuint tex) {
        Q_UNUSED(surface);
        Q_UNUSED(tex);
        return true;
    }
};
typedef QSharedPointer<InteropResource> InteropResourcePtr;

class SurfaceInteropVAAPI Q_DECL_FINAL: public VideoSurfaceInterop
{
public:
    SurfaceInteropVAAPI(const InteropResourcePtr& res) : frame_width(0), frame_height(0), m_resource(res) {}
    void setSurface(const surface_ptr& surface, int w, int h); // use surface->width/height if w/h is 0
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
protected:
    void* mapToHost(const VideoFormat &format, void *handle, int plane);
private:
    int frame_width, frame_height;
    // NOTE: must ensure va-x11/va-glx is unloaded after all va calls(don't know why, but it's true), for example vaTerminate(), to avoid crash
    // so declare InteropResourcePtr first then surface_ptr. InteropResource (va-xxx.so) will be destroyed later than surface_t (vaTerminate())
    // also call vaInitialize() before vaTerminate() can avoid such crashes. Don't know why.
    InteropResourcePtr m_resource;
    surface_ptr m_surface;
};
// load/resolve symbols only once in decoder and pass a VAAPI_XXX ptr
// or use pool
class GLXInteropResource Q_DECL_FINAL: public InteropResource, protected VAAPI_GLX
{
public:
    bool map(const surface_ptr &surface, GLuint tex, int w, int h, int) Q_DECL_OVERRIDE;
private:
    surface_glx_ptr surfaceGLX(const display_ptr& dpy, GLuint tex);
    QMap<GLuint,surface_glx_ptr> glx_surfaces; // render to different texture. surface_glx_ptr is created with texture
};

class X11;
class X11InteropResource Q_DECL_FINAL: public InteropResource, protected VAAPI_X11
{
public:
    X11InteropResource();
    ~X11InteropResource();
    bool map(const surface_ptr &surface, GLuint tex, int w, int h, int) Q_DECL_OVERRIDE;
    bool unmap(const surface_ptr &surface, GLuint tex) Q_DECL_OVERRIDE;
private:
    bool ensurePixmaps(int w, int h);
    Display *xdisplay;
    int width, height;
    X11 *x11;
};
#if QTAV_HAVE(EGL_CAPI)
// libva-egl is dead and not complete. here we use dma
class EGL;
class EGLInteropResource Q_DECL_FINAL : public InteropResource
{
public:
    EGLInteropResource();
    ~EGLInteropResource();
    bool map(const surface_ptr &surface, GLuint tex, int w, int h, int plane) Q_DECL_OVERRIDE;
    bool unmap(const surface_ptr &surface, GLuint tex) Q_DECL_OVERRIDE;
private:
    bool ensure();
    void destroy(VADisplay va_dpy); //destroy dma buffer and egl images
    uintptr_t vabuf_handle;
    VAImage va_image;
    QMap<GLuint, int> mapped;
    EGL *egl;
};
#endif //QTAV_HAVE(EGL_CAPI)
} //namespace vaapi
} //namespace QtAV
#endif //QT_NO_OPENGL
#endif // QTAV_SURFACEINTEROPVAAPI_H
