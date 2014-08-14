#ifndef QTAV_SURFACEINTEROPVAAPI_H
#define QTAV_SURFACEINTEROPVAAPI_H

#include <QtCore/QMap>
#include "vaapi_helper.h"
#include "QtAV/SurfaceInterop.h"

namespace QtAV {
namespace vaapi {

class SurfaceInteropVAAPI : public VideoSurfaceInterop, public VAAPI_GLX
{
public:
    SurfaceInteropVAAPI();
    ~SurfaceInteropVAAPI();
    void setSurface(const surface_ptr& surface) { m_surface = surface; }
    // return glx surface
    surface_glx_ptr createGLXSurface(void* handle);
    virtual void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane);
    virtual void unmap(void *handle);
    virtual void* createHandle(SurfaceType type, const VideoFormat& fmt, int plane = 0);
private:
    surface_ptr m_surface;
    QMap<GLuint*,surface_glx_ptr> glx_surfaces, tmp_surfaces;
};

} //namespace QtAV
} //namespace vaapi
#endif // QTAV_SURFACEINTEROPVAAPI_H
