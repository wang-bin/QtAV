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

#ifndef QTAV_VAAPI_HELPER_H
#define QTAV_VAAPI_HELPER_H

#include <assert.h>
#include <va/va.h>
#include <QtCore/QLibrary>
#include <QtCore/QSharedPointer>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qopengl.h>
#elif defined(QT_OPENGL_LIB)
#include <qgl.h>
#endif
#include "utils/SharedPtr.h"

namespace QtAV {
#ifndef VA_FOURCC_RGBX
#define VA_FOURCC_RGBX		0x58424752
#endif
#ifndef VA_FOURCC_BGRX
    #define VA_FOURCC_BGRX		0x58524742
#endif
#ifndef VA_SURFACE_ATTRIB_SETTABLE
// travis-ci use old vaapi
struct VASurfaceAttrib;
inline VAStatus vaCreateSurfaces(VADisplay dpy, unsigned int format, unsigned int width, unsigned int height, VASurfaceID *surfaces, unsigned int num_surfaces, VASurfaceAttrib *attrib_list, unsigned int num_attribs
) {
    return ::vaCreateSurfaces(dpy, width, height, format, num_surfaces, surfaces);
}
#endif

#define VA_ENSURE_TRUE(x, ...) \
    do { \
        VAStatus ret = x; \
        if (ret != VA_STATUS_SUCCESS) { \
            qWarning("VA-API error@%d. " #x ": %#x %s", __LINE__, ret, vaErrorStr(ret)); \
            return __VA_ARGS__; \
        } \
    } while(0)
#define VA_ENSURE(...) VA_ENSURE_TRUE(__VA_ARGS__)
#define VAWARN(a) \
do { \
  VAStatus res = a; \
  if(res != VA_STATUS_SUCCESS) \
    qWarning("VA-API error %s@%d. " #a ": %#x %s", __FILE__, __LINE__, res, vaErrorStr(res)); \
} while(0);

namespace vaapi {
const char *profileName(VAProfile profile);
/*!
 * \brief va_new_image
 * create image (if img is not null)/find format for the first supported fourcc from given fourcc list.
 * if s is not null, also test vaGetImage for the fourcc
 */
VAImageFormat va_new_image(VADisplay display, const unsigned int* fourccs, VAImage* img = 0, int w = 0, int h = 0, VASurfaceID s = VA_INVALID_SURFACE);
class dll_helper {
public:
    dll_helper(const QString& soname, int version = -1);
    virtual ~dll_helper() { m_lib.unload();}
    bool isLoaded() const { return m_lib.isLoaded(); }
    void* resolve(const char *symbol) { return (void*)m_lib.resolve(symbol);}
private:
    QLibrary m_lib;
};

class va_0_38 : protected dll_helper {
public:
    typedef struct {
        uintptr_t           handle;
        uint32_t            type;
        uint32_t            mem_type;
        size_t              mem_size;
    } VABufferInfo;
    static va_0_38& instance() {
        static va_0_38 self;
        return self;
    }
    static bool isValid() { return instance().f_vaAcquireBufferHandle && instance().f_vaReleaseBufferHandle;}
    static VAStatus vaAcquireBufferHandle(VADisplay dpy, VABufferID buf_id, VABufferInfo *buf_info) {
        if (!instance().f_vaAcquireBufferHandle)
            return VA_STATUS_ERROR_UNIMPLEMENTED;
        return instance().f_vaAcquireBufferHandle(dpy, buf_id, buf_info);
    }
    static VAStatus vaReleaseBufferHandle(VADisplay dpy, VABufferID buf_id) {
        if (!instance().f_vaReleaseBufferHandle)
            return VA_STATUS_ERROR_UNIMPLEMENTED;
        return instance().f_vaReleaseBufferHandle(dpy, buf_id);
    }
protected:
    va_0_38() : dll_helper(QString::fromLatin1("va"), 1) {
        f_vaAcquireBufferHandle = (vaAcquireBufferHandle_t)resolve("vaAcquireBufferHandle");
        f_vaReleaseBufferHandle = (vaReleaseBufferHandle_t)resolve("vaReleaseBufferHandle");
    }
private:
    typedef VAStatus (*vaAcquireBufferHandle_t)(VADisplay dpy, VABufferID buf_id, VABufferInfo *buf_info);
    typedef VAStatus (*vaReleaseBufferHandle_t)(VADisplay dpy, VABufferID buf_id);
    static vaAcquireBufferHandle_t f_vaAcquireBufferHandle;
    static vaReleaseBufferHandle_t f_vaReleaseBufferHandle;
};
class VAAPI_DRM : protected dll_helper {
public:
    typedef VADisplay vaGetDisplayDRM_t(int fd);
    VAAPI_DRM(): dll_helper(QString::fromLatin1("va-drm"),1) {
        fp_vaGetDisplayDRM = (vaGetDisplayDRM_t*)resolve("vaGetDisplayDRM");
    }
    VADisplay vaGetDisplayDRM(int fd) {
        assert(fp_vaGetDisplayDRM);
        return fp_vaGetDisplayDRM(fd);
    }
private:
    vaGetDisplayDRM_t* fp_vaGetDisplayDRM;
};

typedef struct _XDisplay Display;
class VAAPI_X11 : protected dll_helper {
public:
    typedef unsigned long Drawable;
    typedef VADisplay vaGetDisplay_t(Display *);
    typedef VAStatus vaPutSurface_t(VADisplay, VASurfaceID,	Drawable,
                                   short, short, unsigned short,  unsigned short,
                                   short, short, unsigned short, unsigned short,
                                   VARectangle *, unsigned int,  unsigned int);
    VAAPI_X11(): dll_helper(QString::fromLatin1("va-x11"),1) {
        fp_vaGetDisplay = (vaGetDisplay_t*)resolve("vaGetDisplay");
        fp_vaPutSurface = (vaPutSurface_t*)resolve("vaPutSurface");
    }
    VADisplay vaGetDisplay(Display *dpy) {
        assert(fp_vaGetDisplay);
        return fp_vaGetDisplay(dpy);
    }
    VAStatus vaPutSurface (VADisplay dpy, VASurfaceID surface,	Drawable draw, /* X Drawable */
        short srcx, short srcy, unsigned short srcw,  unsigned short srch,
        short destx, short desty, unsigned short destw, unsigned short desth,
        VARectangle *cliprects, /* client supplied destination clip list */
        unsigned int number_cliprects, /* number of clip rects in the clip list */
        unsigned int flags /* PutSurface flags */
    ) {
        assert(fp_vaPutSurface);
        return fp_vaPutSurface(dpy, surface, draw, srcx, srcy, srcw, srch, destx, desty, destw, desth, cliprects, number_cliprects, flags);
    }
private:
    vaGetDisplay_t* fp_vaGetDisplay;
    vaPutSurface_t* fp_vaPutSurface;
};

typedef void*   EGLClientBuffer;
class VAAPI_EGL : protected dll_helper { //not implemented
    typedef VAStatus vaGetEGLClientBufferFromSurface_t(VADisplay dpy, VASurfaceID surface, EGLClientBuffer *buffer/* out*/);
    vaGetEGLClientBufferFromSurface_t* fp_vaGetEGLClientBufferFromSurface;
public:
    VAAPI_EGL(): dll_helper(QString::fromLatin1("va-egl"),1) {
        fp_vaGetEGLClientBufferFromSurface = (vaGetEGLClientBufferFromSurface_t*)resolve("vaGetEGLClientBufferFromSurface");
    }
    VAStatus vaGetEGLClientBufferFromSurface(VADisplay dpy, VASurfaceID surface, EGLClientBuffer *buffer/* out*/) {
        assert(fp_vaGetEGLClientBufferFromSurface);
        return fp_vaGetEGLClientBufferFromSurface(dpy, surface, buffer);
    }
};
#ifndef QT_NO_OPENGL
class VAAPI_GLX : protected dll_helper {
public:
    typedef VADisplay vaGetDisplayGLX_t(Display *);
    typedef VAStatus vaCreateSurfaceGLX_t(VADisplay, GLenum, GLuint, void **);
    typedef VAStatus vaDestroySurfaceGLX_t(VADisplay, void *);
    typedef VAStatus vaCopySurfaceGLX_t(VADisplay, void *, VASurfaceID, unsigned int);
    VAAPI_GLX(): dll_helper(QString::fromLatin1("va-glx"),1) {
        fp_vaGetDisplayGLX = (vaGetDisplayGLX_t*)resolve("vaGetDisplayGLX");
        fp_vaCreateSurfaceGLX = (vaCreateSurfaceGLX_t*)resolve("vaCreateSurfaceGLX");
        fp_vaDestroySurfaceGLX = (vaDestroySurfaceGLX_t*)resolve("vaDestroySurfaceGLX");
        fp_vaCopySurfaceGLX = (vaCopySurfaceGLX_t*)resolve("vaCopySurfaceGLX");
    }
    VADisplay vaGetDisplayGLX(Display *dpy) {
        assert(fp_vaGetDisplayGLX);
        return fp_vaGetDisplayGLX(dpy);
    }
    VAStatus vaCreateSurfaceGLX(VADisplay dpy, GLenum target, GLuint texture, void **gl_surface) {
        assert(fp_vaCreateSurfaceGLX);
        return fp_vaCreateSurfaceGLX(dpy, target, texture, gl_surface);
    }
    VAStatus vaDestroySurfaceGLX(VADisplay dpy, void *gl_surface) {
        assert(fp_vaDestroySurfaceGLX);
        return fp_vaDestroySurfaceGLX(dpy, gl_surface);
    }
    /**
     * Copy a VA surface to a VA/GLX surface
     *
     * This function will not return until the copy is completed. At this
     * point, the underlying GL texture will contain the surface pixels
     * in an RGB format defined by the user.
     *
     * The application shall maintain the live GLX context itself.
     * Implementations are free to use glXGetCurrentContext() and
     * glXGetCurrentDrawable() functions for internal purposes.
     *
     * @param[in]  dpy        the VA display
     * @param[in]  gl_surface the VA/GLX destination surface
     * @param[in]  surface    the VA source surface
     * @param[in]  flags      the PutSurface flags
     * @return VA_STATUS_SUCCESS if successful
     */
    VAStatus vaCopySurfaceGLX(VADisplay dpy, void *gl_surface, VASurfaceID surface, unsigned int flags) {
        assert(fp_vaCopySurfaceGLX);
        return fp_vaCopySurfaceGLX(dpy, gl_surface, surface, flags);
    }
private:
    vaGetDisplayGLX_t* fp_vaGetDisplayGLX;
    vaCreateSurfaceGLX_t* fp_vaCreateSurfaceGLX;
    vaDestroySurfaceGLX_t* fp_vaDestroySurfaceGLX;
    vaCopySurfaceGLX_t* fp_vaCopySurfaceGLX;
};
#endif //QT_NO_OPENGL

class NativeDisplayBase;
typedef QSharedPointer<NativeDisplayBase> NativeDisplayPtr;
struct NativeDisplay {
    enum Type {
        Auto,
        X11,
        GLX, //the same as X11 but use vaGetDisplayGLX()?
        DRM,
        Wayland,
        VA
    };
    intptr_t handle;
    Type type;
    NativeDisplay() : handle(-1), type(Auto) {}
};
class display_t;
typedef QSharedPointer<display_t> display_ptr;
class display_t {
public:
    // display can have a valid handle (!=-1, 0), then it's an external display. you have to manager the external display handle yourself
    static display_ptr create(const NativeDisplay& display);
    ~display_t();
    operator VADisplay() const { return m_display;}
    VADisplay get() const {return m_display;}
    void getVersion(int* majorV, int* minorV) { *majorV = m_major; *minorV = m_minor;}
    NativeDisplay::Type nativeDisplayType() const;
    intptr_t nativeHandle() const;
private:
    VADisplay m_display;
    NativeDisplayPtr m_native;
    int m_major, m_minor;
};

class surface_t {
public:
    surface_t(int w, int h, VASurfaceID id, const display_ptr& display)
        : m_id(id)
        , m_display(display)
        , m_width(w)
        , m_height(h)
        , color_space(VA_SRC_BT709)
    {}
    ~surface_t() {
        //qDebug("VAAPI - destroying surface 0x%x", (int)m_id);
        if (m_id != VA_INVALID_SURFACE)
            VAWARN(vaDestroySurfaces(m_display->get(), &m_id, 1))
    }
    operator VASurfaceID() const { return m_id;}
    VASurfaceID get() const { return m_id;}
    int width() const { return m_width;}
    int height() const { return m_height;}
    void setColorSpace(int cs = VA_SRC_BT709) { color_space = cs;}
    int colorSpace() const { return color_space;}
    display_ptr display() const { return m_display;}
    VADisplay vadisplay() const { return m_display->get();}
private:
    VASurfaceID m_id;
    display_ptr m_display;
    int m_width, m_height;
    int color_space;
};
typedef SharedPtr<surface_t> surface_ptr;
#ifndef QT_NO_OPENGL
class surface_glx_t : public VAAPI_GLX {
public:
    surface_glx_t(const display_ptr& dpy) : m_dpy(dpy), m_glx(0) {}
    ~surface_glx_t() {destroy();}
    bool create(GLuint tex) {
        destroy();
        VA_ENSURE_TRUE(vaCreateSurfaceGLX(m_dpy->get(), GL_TEXTURE_2D, tex, &m_glx), false);
        return true;
    }
    bool destroy() {
        if (!m_glx)
            return true;
        VA_ENSURE_TRUE(vaDestroySurfaceGLX(m_dpy->get(), m_glx), false);
        m_glx = 0;
        return true;
    }
    bool copy(const surface_ptr& surface) {
        if (!m_glx)
            return false;
        VA_ENSURE_TRUE(vaCopySurfaceGLX(m_dpy->get(), m_glx, surface->get(), VA_FRAME_PICTURE | surface->colorSpace()), false);
        return true;
    }
private:
    display_ptr m_dpy;
    void* m_glx;
};
typedef QSharedPointer<surface_glx_t> surface_glx_ptr; //store in a vector
#endif //QT_NO_OPENGL
} //namespace vaapi
} //namespace QtAV
#endif // QTAV_VAAPI_HELPER_H
