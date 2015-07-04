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

class InteropResource
{
public:
    //virtual ~InteropResource() {} // FIXME: will delete vadisplay too late and crash at vaTerminate(), why?
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
    virtual bool unmap(GLuint tex) { Q_UNUSED(tex); return true;}
};
typedef QSharedPointer<InteropResource> InteropResourcePtr;

// inherits VAAPI_GLX, VAAPI_X11: unload va lib in dtor, va-xxx is not loaded and may affect surface_t dtor which call vaTerminate()
// dtor: ~SurfaceInteropVAAPI()=>~surface_t()=>~VAAPI_X11()=>~VAAPI_GLX()=>~VideoSurfaceInterop()
class SurfaceInteropVAAPI Q_DECL_FINAL: public VideoSurfaceInterop
{
public:
    SurfaceInteropVAAPI(const InteropResourcePtr& res) : m_resource(res) {}
    void setSurface(const surface_ptr& surface,  int w, int h) { m_surface = surface;frame_width=w;frame_height=h; }
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
protected:
    void* mapToHost(const VideoFormat &format, void *handle, int plane);
private:
    surface_ptr m_surface;
    InteropResourcePtr m_resource;
    int frame_width, frame_height;
};
// load/resolve symbols only once in decoder and pass a VAAPI_XXX ptr
// or use pool

class GLXInteropResource Q_DECL_FINAL: public InteropResource, public VAAPI_GLX
{
public:
    bool map(const surface_ptr &surface, GLuint tex, int w, int h, int) Q_DECL_OVERRIDE;
private:
    surface_glx_ptr surfaceGLX(const display_ptr& dpy, GLuint tex);
    QMap<GLuint,surface_glx_ptr> glx_surfaces; // render to different texture. surface_glx_ptr is created with texture
};

#ifndef QT_OPENGL_ES_2
class X11InteropResource Q_DECL_FINAL: public InteropResource, public VAAPI_X11
{
public:
    X11InteropResource();
    ~X11InteropResource();
    bool map(const surface_ptr &surface, GLuint tex, int w, int h, int) Q_DECL_OVERRIDE;
    bool unmap(GLuint tex) Q_DECL_OVERRIDE;
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

} //namespace vaapi
} //namespace QtAV

#endif // QTAV_SURFACEINTEROPVAAPI_H
