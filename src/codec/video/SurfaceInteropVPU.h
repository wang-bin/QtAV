
#ifndef QTAV_SURFACEINTEROPVPU_H
#define QTAV_SURFACEINTEROPVPU_H
#include <QtAV/SurfaceInterop.h>
#include "coda/vpuapi/vpuapi.h"
struct XImage;

namespace QtAV {
class gcScaler;

class FBSurface {
    DecHandle handle;
public:
    FBSurface(DecHandle h)
        : handle(h)
        , index(0)
    {
        memset(&fb, 0, sizeof(fb));
    }
    ~FBSurface() {
        if (check_VSYNC_flag())
            clear_VSYNC_flag();
        VPU_DecClrDispFlag(handle, index);
    }
    int index;
    FrameBuffer fb;
};
typedef QSharedPointer<FBSurface> FBSurfacePtr;

namespace vpu {

class InteropResource
{
public:
    InteropResource();
    ~InteropResource();
    /*!
     * \brief map
     * \param surface vpu decoded surface
     * \param tex opengl texture
     * \param w frame width(visual width) without alignment, <= vpu surface width
     * \param h frame height(visual height)
     * \param plane useless now
     * \return true if success
     */
    bool map(const FBSurfacePtr& surface, XImage* ximg, int plane);
    bool unmap(XImage *) { return true;}
protected:
    gcScaler *scaler;
};
typedef QSharedPointer<InteropResource> InteropResourcePtr;

class SurfaceInteropVIV Q_DECL_FINAL: public VideoSurfaceInterop
{
public:
    SurfaceInteropVIV(const InteropResourcePtr& res) : m_surface(0), m_resource(res), frame_width(0), frame_height(0) {}
    ~SurfaceInteropVIV();
    /*!
     * \brief setSurface
     * \param surface vpu decoded surface
     * \param frame_w frame width(visual width) without alignment, <= vpu surface width
     * \param frame_h frame height(visual height)
     */
    void setSurface(const FBSurfacePtr& surface, int frame_w, int frame_h);
    /// GLTextureSurface only supports rgb32
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
protected:
    /// copy from gpu (optimized if possible) and convert to target format if necessary
    void* mapToHost(const VideoFormat &format, void *handle, int plane);
private:
    InteropResourcePtr m_resource;
    FBSurfacePtr m_surface;
    int frame_width, frame_height;
};
