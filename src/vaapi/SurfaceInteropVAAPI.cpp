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
#include "SurfaceInteropVAAPI.h"
#include "utils/OpenGLHelper.h"
#include "QtAV/VideoFrame.h"
#include "utils/Logger.h"
#if VA_X11_INTEROP
#ifdef QT_X11EXTRAS_LIB
#include <QtX11Extras/QX11Info>
#endif //QT_X11EXTRAS_LIB
#include <va/va_x11.h>
#if QTAV_HAVE(EGL_CAPI)
#define EGL_CAPI_NS
#include "capi/egl_api.h"
#include <EGL/eglext.h>
#endif //QTAV_HAVE(EGL_CAPI)
#if !defined(QT_OPENGL_ES_2)
#include <GL/glx.h>
#endif //!defined(QT_OPENGL_ES_2)
#endif //VA_X11_INTEROP

namespace QtAV {
namespace vaapi {
void SurfaceInteropVAAPI::setSurface(const surface_ptr& surface,  int w, int h) {
    m_surface = surface;
    frame_width = (w ? w : surface->width());
    frame_height = (h ? h : surface->height());
}

void* SurfaceInteropVAAPI::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (!handle)
        return NULL;
    if (!fmt.isRGB())
        return 0;

    if (!m_surface)
        return 0;
    if (type == GLTextureSurface) {
        if (m_resource->map(m_surface, *((GLuint*)handle), frame_width, frame_height, plane))
            return handle;
        return NULL;
    } else if (type == HostMemorySurface) {
        return mapToHost(fmt, handle, plane);
    }
    return NULL;
}

void SurfaceInteropVAAPI::unmap(void *handle)
{
    m_resource->unmap(*((GLuint*)handle));
}

void* SurfaceInteropVAAPI::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    Q_UNUSED(plane);
    int nb_fmts = vaMaxNumImageFormats(m_surface->vadisplay());
    //av_mallocz_array
    VAImageFormat *p_fmt = (VAImageFormat*)calloc(nb_fmts, sizeof(*p_fmt));
    if (!p_fmt) {
        return NULL;
    }
    if (vaQueryImageFormats(m_surface->vadisplay(), p_fmt, &nb_fmts)) {
        free(p_fmt);
        return NULL;
    }
    VAImage image;
    for (int i = 0; i < nb_fmts; i++) {
        if (p_fmt[i].fourcc == VA_FOURCC_YV12 ||
            p_fmt[i].fourcc == VA_FOURCC_IYUV ||
            p_fmt[i].fourcc == VA_FOURCC_NV12) {
            qDebug("vaCreateImage: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
            if (vaCreateImage(m_surface->vadisplay(), &p_fmt[i], m_surface->width(), m_surface->height(), &image) != VA_STATUS_SUCCESS) {
                image.image_id = VA_INVALID_ID;
                qDebug("vaCreateImage error: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
                continue;
            }
            /* Validate that vaGetImage works with this format */
            if (vaGetImage(m_surface->vadisplay(), m_surface->get(), 0, 0, m_surface->width(), m_surface->height(), image.image_id) != VA_STATUS_SUCCESS) {
                vaDestroyImage(m_surface->vadisplay(), image.image_id);
                qDebug("vaGetImage error: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
                image.image_id = VA_INVALID_ID;
                continue;
            }
            break;
        }
    }
    free(p_fmt);
    if (image.image_id == VA_INVALID_ID)
        return NULL;
    void *p_base;
    VA_ENSURE_TRUE(vaMapBuffer(m_surface->vadisplay(), image.buf, &p_base), NULL);

    VideoFormat::PixelFormat pixfmt = VideoFormat::Format_Invalid;
    bool swap_uv = image.format.fourcc != VA_FOURCC_NV12;
    switch (image.format.fourcc) {
    case VA_FOURCC_YV12:
    case VA_FOURCC_IYUV:
        pixfmt = VideoFormat::Format_YUV420P;
        break;
    case VA_FOURCC_NV12:
        pixfmt = VideoFormat::Format_NV12;
        break;
    default:
        break;
    }
    if (pixfmt == VideoFormat::Format_Invalid) {
        qWarning("unsupported vaapi pixel format: %#x", image.format.fourcc);
        vaDestroyImage(m_surface->vadisplay(), image.image_id);
        return NULL;
    }
    const VideoFormat fmt(pixfmt);
    uint8_t *src[3];
    int pitch[3];
    for (int i = 0; i < fmt.planeCount(); ++i) {
        src[i] = (uint8_t*)p_base + image.offsets[i];
        pitch[i] = image.pitches[i];
    }
    VideoFrame frame = VideoFrame::fromGPU(fmt, frame_width, frame_height, m_surface->height(), src, pitch, true, swap_uv);
    if (format != fmt)
        frame = frame.to(format);
    VAWARN(vaUnmapBuffer(m_surface->vadisplay(), image.buf));
    vaDestroyImage(m_surface->vadisplay(), image.image_id);
    image.image_id = VA_INVALID_ID;
    VideoFrame *f = reinterpret_cast<VideoFrame*>(handle);
    frame.setTimestamp(f->timestamp());
    *f = frame;
    return f;
}
#ifndef QT_NO_OPENGL
surface_glx_ptr GLXInteropResource::surfaceGLX(const display_ptr &dpy, GLuint tex)
{
    surface_glx_ptr glx = glx_surfaces[tex];
    if (glx)
        return glx;
    glx = surface_glx_ptr(new surface_glx_t(dpy));
    if (!glx->create(tex))
        return surface_glx_ptr();
    glx_surfaces[tex] = glx;
    return glx;
}

bool GLXInteropResource::map(const surface_ptr& surface, GLuint tex, int w, int h, int)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
    surface_glx_ptr glx = surfaceGLX(surface->display(), tex);
    if (!glx) {
        qWarning("Fail to create vaapi glx surface");
        return false;
    }
    if (!glx->copy(surface))
        return false;
    VAWARN(vaSyncSurface(surface->vadisplay(), surface->get()));
    return true;
}
#endif //QT_NO_OPENGL
#if VA_X11_INTEROP
class X11 {
protected:
    Display *display;
public:
    X11() : display(NULL), pixmap(0) {}
    virtual ~X11() {
        if (pixmap)
            XFreePixmap((::Display*)display, pixmap);
        pixmap = 0;
    }
    virtual Display* ensureGL() = 0;
    virtual bool bindPixmap(int w, int h) = 0;
    virtual void bindTexture() = 0;
    Pixmap pixmap;
protected:
    int createPixmap(int w, int h) {
        if (pixmap) {
            qDebug("XFreePixmap");
            XFreePixmap((::Display*)display, pixmap);
            pixmap = 0;
        }
        XWindowAttributes xwa;
        XGetWindowAttributes((::Display*)display, DefaultRootWindow((::Display*)display), &xwa);
        pixmap = XCreatePixmap((::Display*)display, DefaultRootWindow((::Display*)display), w, h, xwa.depth);
        // mpv always use 24 bpp
        qDebug("XCreatePixmap %lu: %dx%d, depth: %d", pixmap, w, h, xwa.depth);
        if (!pixmap) {
            qWarning("X11InteropResource could not create pixmap");
            return 0;
        }
        return xwa.depth;
    }
};

#define RESOLVE_FUNC(func, resolver, st) do {\
    if (!func) { \
        typedef void (*ft)(void); \
        ft f = resolver((st)#func); \
        if (!f) { \
            qWarning(#func " is not available"); \
            return 0; \
        } \
        ft *fp = (ft*)(&func); \
        *fp = f; \
    }} while(0)
#define RESOLVE_EGL(func) RESOLVE_FUNC(func, eglGetProcAddress, const char*)
#define RESOLVE_GLX(func) RESOLVE_FUNC(func, glXGetProcAddressARB, const GLubyte*)

#if QTAV_HAVE(EGL_CAPI)
//static PFNEGLQUERYNATIVEDISPLAYNVPROC eglQueryNativeDisplayNV = NULL;
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = NULL;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = NULL;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = NULL;
class EGL Q_DECL_FINAL: public X11 {
public:
    EGL() : dpy(EGL_NO_DISPLAY) {
        for (unsigned i = 0; i < sizeof(image)/sizeof(image[0]); ++i)
            image[i] = EGL_NO_IMAGE_KHR;
    }
    ~EGL() {
        for (unsigned i = 0; i < sizeof(image)/sizeof(image[0]); ++i) {
            if (image[i] != EGL_NO_IMAGE_KHR) {
                EGL_WARN(eglDestroyImageKHR(dpy, image[i]));
            }
        }
    }
    Display* ensureGL() Q_DECL_OVERRIDE {
        if (display && eglCreateImageKHR && glEGLImageTargetTexture2DOES)
            return display;
        // we must use the native display egl used, otherwise eglCreateImageKHR will fail.
#ifdef QT_X11EXTRAS_LIB
        display = (Display*)QX11Info::display();
#endif //QT_X11EXTRAS_LIB
        //RESOLVE_EGL(eglQueryNativeDisplayNV);
        RESOLVE_EGL(glEGLImageTargetTexture2DOES);
        RESOLVE_EGL(eglCreateImageKHR);
        RESOLVE_EGL(eglDestroyImageKHR);
        return display;
    }
    bool bindPixmap(int w, int h) Q_DECL_OVERRIDE {
        if (!createPixmap(w, h))
            return false;
        if (dpy == EGL_NO_DISPLAY) {
            qDebug("eglGetCurrentDisplay");
            dpy = eglGetCurrentDisplay();
        }
        EGL_ENSURE(image[0] =  eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmap, NULL), false);
        if (!image[0]) {
            qWarning("eglCreateImageKHR error %#X, image: %p", eglGetError(), image[0]);
            return false;
        }
        return true;
    }
    void bindTexture() Q_DECL_OVERRIDE {
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image[0]);
    }
    EGLDisplay dpy;
    EGLImageKHR image[4];
};
#endif //QTAV_HAVE(EGL_CAPI)
#if !defined(QT_OPENGL_ES_2)
typedef void (*glXBindTexImageEXT_t)(Display *dpy, GLXDrawable draw, int buffer, int *a);
typedef void (*glXReleaseTexImageEXT_t)(Display *dpy, GLXDrawable draw, int buffer);
static glXReleaseTexImageEXT_t glXReleaseTexImageEXT = 0;
static glXBindTexImageEXT_t glXBindTexImageEXT = 0;

class GLX Q_DECL_FINAL: public X11 {
public:
    GLX() : fbc(0), glxpixmap(0) {}
    ~GLX() {
        if (glxpixmap) { //TODO: does the thread matters?
            glXReleaseTexImageEXT(display, glxpixmap, GLX_FRONT_EXT);
            XSync((::Display*)display, False);
            glXDestroyPixmap((::Display*)display, glxpixmap);
        }
        glxpixmap = 0;
    }
    Display* ensureGL() Q_DECL_OVERRIDE {
        if (fbc && display)
            return display;
        if (!display) {
            qDebug("glXGetCurrentDisplay");
            display = (Display*)glXGetCurrentDisplay();
            if (!display)
                return 0;
        }
        int xscr = DefaultScreen(display);
        const char *glxext = glXQueryExtensionsString((::Display*)display, xscr);
        if (!glxext || !strstr(glxext, "GLX_EXT_texture_from_pixmap"))
            return 0;
        RESOLVE_GLX(glXBindTexImageEXT);
        RESOLVE_GLX(glXReleaseTexImageEXT);
        int attribs[] = {
            GLX_RENDER_TYPE, GLX_RGBA_BIT, //xbmc
            GLX_X_RENDERABLE, True, //xbmc
            GLX_BIND_TO_TEXTURE_RGBA_EXT, True,
            GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
            GLX_BIND_TO_TEXTURE_TARGETS_EXT, GLX_TEXTURE_2D_BIT_EXT,
            GLX_Y_INVERTED_EXT, True,
            GLX_DOUBLEBUFFER, False,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_ALPHA_SIZE, 8, //0 for 24 bpp(vdpau)? mpv is 0
            None
        };
        int fbcount;
        GLXFBConfig *fbcs = glXChooseFBConfig((::Display*)display, xscr, attribs, &fbcount);
        if (!fbcount) {
            qWarning("No texture-from-pixmap support");
            return 0;
        }
        if (fbcount)
            fbc = fbcs[0];
        XFree(fbcs);
        return display;
    }
    bool bindPixmap(int w, int h) Q_DECL_OVERRIDE {
        const int depth = createPixmap(w, h);
        if (depth <= 0)
            return false;
        const int attribs[] = {
            GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
            GLX_TEXTURE_FORMAT_EXT, depth == 32 ? GLX_TEXTURE_FORMAT_RGBA_EXT : GLX_TEXTURE_FORMAT_RGB_EXT,
            GLX_MIPMAP_TEXTURE_EXT, False,
            None,
        };
        glxpixmap = glXCreatePixmap((::Display*)display, fbc, pixmap, attribs);
        return true;
    }
    void bindTexture() Q_DECL_OVERRIDE {
        glXBindTexImageEXT(display, glxpixmap, GLX_FRONT_EXT, NULL);
    }
    GLXFBConfig fbc;
    GLXPixmap glxpixmap;
};
#endif // !defined(QT_OPENGL_ES_2)

X11InteropResource::X11InteropResource()
    : InteropResource()
    , VAAPI_X11()
    , xdisplay(NULL)
    , width(0)
    , height(0)
    , x11(NULL)
{}

X11InteropResource::~X11InteropResource()
{
    delete x11;
}

bool X11InteropResource::ensurePixmaps(int w, int h)
{
    if (width == w && height == h)
        return true;
    if (!x11) {
#if QTAV_HAVE(EGL_CAPI)
#if defined(QT_OPENGL_ES_2)
        static const bool use_egl = true;
#else
        // FIXME: may fallback to xcb_glx(default)
        static const bool use_egl = qgetenv("QT_XCB_GL_INTEGRATION") == "xcb_egl";
#endif //defined(QT_OPENGL_ES_2)
        if (use_egl) {
            x11 = new EGL();
        } else
#endif //QTAV_HAVE(EGL_CAPI)
        {
#if !defined(QT_OPENGL_ES_2)
            x11 = new GLX();
#endif // defined(QT_OPENGL_ES_2)
        }
    }
    if (!x11) {
        qWarning("no EGL and GLX interop (TFP) support");
        return false;
    }
    xdisplay  = x11->ensureGL();
    if (!xdisplay)
        return false;
    if (!x11->bindPixmap(w, h))
        return false;
    width = w;
    height = h;
    return true;
}

bool X11InteropResource::map(const surface_ptr& surface, GLuint tex, int w, int h, int)
{
    if (surface->width() <= 0 || surface->height() <= 0) {
        qWarning("invalid surface size");
        return false;
    }
    if (!ensurePixmaps(w, h)) //pixmap with frame size
        return false;
    VAWARN(vaSyncSurface(surface->vadisplay(), surface->get()));
    // FIXME: invalid surface at the first time vaPutSurface is called. If return false, vaPutSurface will always fail, why?
    VAWARN(vaPutSurface(surface->vadisplay(), surface->get(), x11->pixmap
                                , 0, 0, w, h
                                , 0, 0, w, h
                                , NULL, 0, VA_FRAME_PICTURE | surface->colorSpace())
                   );
    XSync((::Display*)xdisplay, False);
    DYGL(glBindTexture(GL_TEXTURE_2D, tex));
    x11->bindTexture();
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
    return true;
}

bool X11InteropResource::unmap(GLuint tex)
{
    Q_UNUSED(tex);
    // can not call glXReleaseTexImageEXT otherwise the texture will containts no image data
    return true;
}
#endif //VA_X11_INTEROP
} //namespace QtAV
} //namespace vaapi
