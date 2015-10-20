
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

bool InteropResource::map(const QSharedPointer<FBSurface> &surface, XImage *ximg, int plane)
{
    if (!scaler) {
        scaler = new gcScaler();
        scaler->setInFormat(VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_YUV420P));
    }
    //scaler->setOutFormat(VideoFormat::pixelFormatToFFmpeg(ximg.));
    scaler->setOutFormat(VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_RGB32));
    scaler->setInSize(surface->fb.stride, surface->fb.height);
    scaler->setOutSize(ximg->width, ximg->height);
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
    // dma copy. check ximg->bytes_per_line
    if (ximg->bytes_per_line == scaler->outLineSizes().at(0)) {
        // qMin(scaler->outHeight(), ximg->height)
        dma_copy_from_vmem(ximg->data, scaler->outPlanes().at(0), ximg->bytes_per_line*ximg->height);
    } else {
        qWarning("different gpu/host_mem stride");
        for (int i = 0; i < ximg->height; ++i)
            dma_copy_from_vmem(ximg->data + i*ximg->bytes_per_line, scaler->outPlanes().at(0) + i*scaler->outLineSizes().at(0), ximg->bytes_per_line);
    }
    return true;
}

bool SurfaceInteropVIV::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (type == HostMemorySurface) {
        return mapToHost(fmt, handle, plane);
    }
}

bool SurfaceInteropVIV::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    return m_resource->map(m_surface, (XImage*)handle, plane);
}

}
}
Q_DECLARE_METATYPE(QtAV::FBSurface)
