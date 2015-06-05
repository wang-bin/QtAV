/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_SURFACEINTEROPVAAPI_H
#define QTAV_SURFACEINTEROPVAAPI_H

#include <QtCore/QMap>
#include <QtCore/QSharedPointer>
#include <QtCore/QMutex>
#include "QtAV/SurfaceInterop.h"
#include "vaapi_helper.h"

#if defined(QT_OPENGL_ES_2)
#include <EGL/egl.h>
#else
#include <GL/glx.h>
#ifdef Bool
#undef Bool
#endif //Bool
#ifdef CursorShape
#undef CursorShape //used in qvariant and x11
#endif //CursorShape
#ifdef Status
#undef Status // qtextstream
#endif //Status
#endif //defined(QT_OPENGL_ES_2)

namespace QtAV {
namespace vaapi {

// inherits VAAPI_GLX, VAAPI_X11: unload va lib in dtor, va-xxx is not loaded and may affect surface_t dtor which call vaTerminate()
// dtor: ~SurfaceInteropVAAPI()=>~surface_t()=>~VAAPI_X11()=>~VAAPI_GLX()=>~VideoSurfaceInterop()
class SurfaceInteropVAAPI : public VideoSurfaceInterop,  public VAAPI_GLX, public VAAPI_X11
{
public:
    void setSurface(const surface_ptr& surface) { //in decoding thread. map in rendering thread
        QMutexLocker lock(&mutex);
        m_surface = surface;
    }
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE Q_DECL_FINAL;
protected:
    void* mapToHost(const VideoFormat &format, void *handle, int plane);
    virtual void* mapToTexture(const VideoFormat &fmt, void *handle, int plane) = 0;

    surface_ptr m_surface; //FIXME: why vaTerminate() crash (in ~display_t()) if put m_surface here?
private:
    QMutex mutex;
};
// TODO: move create glx surface to decoder, interop only map/unmap, 1 interop per frame
// load/resolve symbols only once in decoder and pass a VAAPI_XXX ptr
// or use pool
class VAAPI_GLX_Interop Q_DECL_FINAL: public SurfaceInteropVAAPI
{
public:
    VAAPI_GLX_Interop();
    // return glx surface
    surface_glx_ptr createGLXSurface(void* handle);
    void* mapToTexture(const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
private:
    QMap<GLuint*,surface_glx_ptr> glx_surfaces;
};

#ifndef QT_OPENGL_ES_2
class VAAPI_X_GLX_Interop Q_DECL_FINAL: public SurfaceInteropVAAPI
{
public:
    VAAPI_X_GLX_Interop();
    ~VAAPI_X_GLX_Interop();
    void* mapToTexture(const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
private:
    bool ensureGLX();
    bool ensurePixmaps(int w, int h);
    Display *xdisplay;
    GLXFBConfig fbc;
    Pixmap pixmap;
    GLXPixmap glxpixmap;
    int width, height;

    typedef void (*glXBindTexImage_t)(Display *dpy, GLXDrawable draw, int buffer, int *a);
    typedef void (*glXReleaseTexImage_t)(Display *dpy, GLXDrawable draw, int buffer);
    static glXBindTexImage_t glXBindTexImage;
    static glXReleaseTexImage_t glXReleaseTexImage;
};
#endif //QT_OPENGL_ES_2
} //namespace QtAV
} //namespace vaapi

#endif // QTAV_SURFACEINTEROPVAAPI_H
