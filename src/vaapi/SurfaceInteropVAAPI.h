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
#endif //defined(QT_OPENGL_ES_2)
#ifdef Bool
#undef Bool
#endif //Bool
#ifdef CursorShape
#undef CursorShape //used in qvariant and x11
#endif //CursorShape
#ifdef Status
#undef Status // qtextstream
#endif //Status
namespace QtAV {
namespace vaapi {

// TODO: move create glx surface to decoder, interop only map/unmap, 1 interop per frame
// load/resolve symbols only once in decoder and pass a VAAPI_XXX ptr
// or use pool
class SurfaceInteropVAAPI : public VideoSurfaceInterop, public VAAPI_GLX
{
public:
    SurfaceInteropVAAPI();

    void setSurface(const surface_ptr& surface) {
        QMutexLocker lock(&mutex);
        m_surface = surface;
    }
    // return glx surface
    surface_glx_ptr createGLXSurface(void* handle);
    virtual void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane);
    virtual void unmap(void *handle);
    virtual void* createHandle(SurfaceType type, const VideoFormat& fmt, int plane = 0);
private:
    QMap<GLuint*,surface_glx_ptr> glx_surfaces, tmp_surfaces;
    surface_ptr m_surface;
    QMutex mutex;
};
typedef QSharedPointer<SurfaceInteropVAAPI> SurfaceInteropVAAPIPtr;

#ifndef QT_OPENGL_ES_2
class VAAPI_X_GLX_Interop Q_DECL_FINAL: public VideoSurfaceInterop, public VAAPI_X11
{
public:
    VAAPI_X_GLX_Interop();
    ~VAAPI_X_GLX_Interop();
    void setSurface(const surface_ptr& surface) {
        QMutexLocker lock(&mutex);
        m_surface = surface;
    }
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
private:
    bool ensureGLX();
    bool ensurePixmaps(int w, int h);
    Display *xdisplay;
    GLXFBConfig fbc;
    Pixmap pixmap;
    GLXPixmap glxpixmap;
    int width, height;
    surface_ptr m_surface;

    QMutex mutex;

    typedef void (*glXBindTexImage_t)(Display *dpy, GLXDrawable draw, int buffer, int *a);
    typedef void (*glXReleaseTexImage_t)(Display *dpy, GLXDrawable draw, int buffer);
    static glXBindTexImage_t glXBindTexImage;
    static glXReleaseTexImage_t glXReleaseTexImage;
};
#endif //QT_OPENGL_ES_2
} //namespace QtAV
} //namespace vaapi
//Q_DECLARE_METATYPE(QtAV::vaapi::SurfaceInteropVAAPIPtr)

#endif // QTAV_SURFACEINTEROPVAAPI_H
