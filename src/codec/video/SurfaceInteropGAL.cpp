
#include "SurfaceInteropGAL.h"
#include "viv/GALScaler.h"

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

bool InteropResource::map(const FBSurfacePtr &surface, ImageDesc *img, int)
{
    if (!scaler) {
        qDebug("create GALScaler");
        scaler = new GALScaler();
    }
    qDebug("map in surface->fb.stride:%d fmt:%d; out: %d-%dx%d ", surface->fb.stride, VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_YUV420P), img->stride, img->width, img->height);
    scaler->setInFormat(VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_YUV420P));
    scaler->setInSize(surface->fb.stride, surface->fb.height);
    //scaler->setOutFormat(VideoFormat::pixelFormatToFFmpeg(img.));
    scaler->setOutFormat(VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_RGB32));
    qDebug("set fmt: %d", VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_RGB32));
    scaler->setOutSize(img->width, img->height);
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
    if (img->stride == scaler->outLineSizes().at(0)) {
        qWarning("same gpu/host_mem stride. dma_copy_from_vmem");
        // qMin(scaler->outHeight(), img->height)
        dma_copy_from_vmem(img->data, (unsigned int)(quintptr)scaler->outPlanes().at(0), img->stride*img->height);
    } else {
        qWarning("different gpu/host_mem stride");
        for (int i = 0; i < img->height; ++i)
            dma_copy_from_vmem(img->data + i*img->stride, (unsigned int)(quintptr)scaler->outPlanes().at(0) + i*scaler->outLineSizes().at(0), img->stride);
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
        qDebug("HostMemorySurface");
        return mapToHost(fmt, handle, plane);
    }
    return 0;
}

void* SurfaceInteropGAL::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    if (!format.isRGB())
        return 0;
    if (m_resource->map(m_surface, (ImageDesc*)handle, plane))
        return handle;
    return 0;
}

} //namespace vpu
} //namespace QtAV
