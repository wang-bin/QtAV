
#include "SurfaceInteropVPU.h"
#include "viv/gcScaler.h"

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

bool InteropResource::map(const QSharedPointer<FBSurface> &surface, ImageDesc *img, int plane)
{
    if (!scaler) {
        scaler = new gcScaler();
        scaler->setInFormat(VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_YUV420P));
    }
    //scaler->setOutFormat(VideoFormat::pixelFormatToFFmpeg(img.));
    scaler->setOutFormat(VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_RGB32));
    scaler->setInSize(surface->fb.stride, surface->fb.height);
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
        // qMin(scaler->outHeight(), img->height)
        dma_copy_from_vmem(img->data, scaler->outPlanes().at(0), img->stride*img->height);
    } else {
        qWarning("different gpu/host_mem stride");
        for (int i = 0; i < img->height; ++i)
            dma_copy_from_vmem(img->data + i*img->stride, scaler->outPlanes().at(0) + i*scaler->outLineSizes().at(0), img->stride);
    }
    return true;
}

bool SurfaceInteropGAL::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (type == HostMemorySurface) {
        return mapToHost(fmt, handle, plane);
    }
}

bool SurfaceInteropGAL::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    if (!format.isRGB())
        return false;
    return m_resource->map(m_surface, (ImageDesc*)handle, plane);
}

}
}
Q_DECLARE_METATYPE(QtAV::FBSurface)
