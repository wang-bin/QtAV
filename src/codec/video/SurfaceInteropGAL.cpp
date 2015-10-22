
#include "SurfaceInteropGAL.h"
#include "viv/GALScaler.h"
#include "QtAV/VideoFrame.h"

extern "C" {
void dma_copy_in_vmem(unsigned int dst, unsigned int src, int len);
void dma_copy_from_vmem(unsigned char* dst, unsigned int src, int len);
}
namespace QtAV {
namespace vpu {

InteropResource::InteropResource()
    : scaler(0)
{}

InteropResource::~InteropResource()
{
    if (scaler) {
        delete scaler;
        scaler = 0;
    }
}

bool InteropResource::map(const FBSurfacePtr &surface, VideoFrame *img, int)
{
    if (!scaler) {
        scaler = new GALScaler();
    }
    scaler->setInFormat(VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_YUV420P));
    scaler->setInSize(surface->fb.stride, surface->fb.height);
    scaler->setOutFormat(VideoFormat::pixelFormatToFFmpeg(img->pixelFormat()));
    scaler->setOutSize(img->width(), img->height());
    const quint8* src[] = {
        (quint8*)surface->fb.bufY,
        (quint8*)surface->fb.bufCb,
        (quint8*)surface->fb.bufCr
    };
    const int srcStride[] = {
        surface->fb.stride,
        surface->fb.stride/2,
        surface->fb.stride/2,
    };
    if (!scaler->convert(src, srcStride))
        return false;
    // dma copy. check img->stride
    if (img->bytesPerLine(0) == scaler->outLineSizes().at(0)) {
        // qMin(scaler->outHeight(), img->height)
        dma_copy_from_vmem(img->bits(0), (unsigned int)(quintptr)scaler->outPlanes().at(0), img->bytesPerLine(0)*img->height());
    } else {
        qWarning("different gpu/host_mem stride");
        for (int i = 0; i < img->height(); ++i)
            dma_copy_from_vmem(img->bits(0) + i*img->bytesPerLine(0), (unsigned int)(quintptr)scaler->outPlanes().at(0) + i*scaler->outLineSizes().at(0), img->bytesPerLine(0));
    }
    return true;
}

void SurfaceInteropGAL::setSurface(const FBSurfacePtr &surface, int frame_w, int frame_h)
{
    m_surface = surface;
    frame_width = frame_w;
    frame_height = frame_h;
}

void* SurfaceInteropGAL::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (type == HostMemorySurface) {
        return mapToHost(fmt, handle, plane);
    }
    return 0;
}

void* SurfaceInteropGAL::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    if (!format.isRGB())
        return 0;
    if (m_resource->map(m_surface, (VideoFrame*)handle, plane))
        return handle;
    return 0;
}
} //namespace vpu
} //namespace QtAV
