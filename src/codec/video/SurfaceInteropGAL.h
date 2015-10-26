
#ifndef QTAV_SURFACEINTEROPGAL_H
#define QTAV_SURFACEINTEROPGAL_H
#include <QtAV/SurfaceInterop.h>
#include "coda/vpuapi/vpuapi.h"
namespace QtAV {
class GALScaler;
class VideoFrame;
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
     * \param format works only for VideoFormat.to(). VideoFormat.map() use the VideoFrame.format()
     * \param img contains the target frame information. if the parameter is not valid, use the input value. If plane(0) address is null, create host memory
     * \param plane useless now
     * \return true if success
     */
    bool map(const FBSurfacePtr& surface, const VideoFormat& format, VideoFrame* img, int plane);
protected:
    GALScaler *scaler;
};
typedef QSharedPointer<InteropResource> InteropResourcePtr;

class SurfaceInteropGAL Q_DECL_FINAL: public VideoSurfaceInterop
{
public:
    SurfaceInteropGAL(const InteropResourcePtr& res) :  m_resource(res), m_surface(0),frame_width(0), frame_height(0) {}
    /*!
     * \brief setSurface
     * \param surface vpu decoded surface
     * \param frame_w frame width(visual width) without alignment, <= vpu surface width
     * \param frame_h frame height(visual height)
     */
    void setSurface(const FBSurfacePtr& surface, int frame_w, int frame_h);
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *) Q_DECL_OVERRIDE {}
protected:
    /// copy from gpu (optimized if possible) and convert to target format + scaled to target size if necessary
    void* mapToHost(const VideoFormat &format, void *handle, int plane);
private:
    InteropResourcePtr m_resource;
    FBSurfacePtr m_surface;
    int frame_width, frame_height;
};
} //namespace vpu
} //namespace QtAV
#endif //QTAV_SURFACEINTEROPGAL_H
