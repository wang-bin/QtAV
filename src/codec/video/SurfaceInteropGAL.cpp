
#include "SurfaceInteropGAL.h"
#include "viv/GALScaler.h"
#include "QtAV/VideoFrame.h"
#include "QtAV/private/AVCompat.h"
#include <QtDebug>

#define FFALIGN(x, a) (((x)+(a)-1)&~((a)-1))
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

bool InteropResource::map(const FBSurfacePtr &surface, const VideoFormat &format, VideoFrame *img, int)
{
    if (!scaler) {
        scaler = new GALScaler();
    }
    scaler->setInFormat(VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_YUV420P));
    scaler->setInSize(surface->fb.stride, surface->fb.height);
    const VideoFormat fmt(img->pixelFormat() == VideoFormat::Format_Invalid ? format.pixelFormat() : img->pixelFormat());
    int w = img->width(), h = img->height();
    // If out size not set, use gal out size which is aligned to 16
    if (w <= 0)
        w = FFALIGN(surface->fb.stride, 16); // TODO: use frame width?
    if (h <= 0)
        h = FFALIGN(surface->fb.height, 16);
    scaler->setOutFormat(fmt.pixelFormatFFmpeg());
    scaler->setOutSize(w, h);
    const quint8* src[] = {
        (quint8*)(quintptr)surface->fb.bufY,
        (quint8*)(quintptr)surface->fb.bufCb,
        (quint8*)(quintptr)surface->fb.bufCr
    };
    const int srcStride[] = {
        surface->fb.stride,
        surface->fb.stride/2,
        surface->fb.stride/2,
    };
    if (!scaler->convert(src, srcStride))
        return false;
    QVector<int> out_pitch(fmt.planeCount());
    if (!img->constBits(0)) { //interop in VideoFrame.to() and format is set
        int data_size = 0;
        QVector<int> offset(fmt.planeCount());
        for (int i = 0; i < fmt.planeCount(); ++i) {
            offset[i] = data_size;
            out_pitch[i] = fmt.width(w*fmt.bytesPerPixel(), i);
            data_size += out_pitch.at(i) * fmt.height(h, i);
        }
        QByteArray data(data_size, 0);
        *img = VideoFrame(w, h, fmt, data);
        img->setBytesPerLine(out_pitch);
        for (int i = 0; i < fmt.planeCount(); ++i) {
            img->setBits((quint8*)data.constData() + offset[i], i);
        }
        qDebug() << fmt;
        qDebug() << "pitch host: " << out_pitch << " vmem: " << scaler->outLineSizes();
    }
    for (int p = 0; p < fmt.planeCount(); ++p) {
        // dma copy. check img->stride
        if (img->bytesPerLine(p) == scaler->outLineSizes().at(p)) {
            // qMin(scaler->outHeight(), img->height)
            dma_copy_from_vmem(img->bits(p), (unsigned int)(quintptr)scaler->outPlanes().at(p), img->bytesPerLine(p)*img->planeHeight(p));
        } else {
            qWarning("different stride @plane%d. vmem: %d, host: %d", p, scaler->outLineSizes().at(p), img->bytesPerLine(p));
            for (int i = img->planeHeight(p) - 1; i >= 0; --i)
                dma_copy_from_vmem(img->bits(p) + i*img->bytesPerLine(p), (unsigned int)(quintptr)scaler->outPlanes().at(p) + i*scaler->outLineSizes().at(p), img->bytesPerLine(p));
        }
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
    if (!format.isRGB() && format.pixelFormat() != VideoFormat::Format_YUV420P)
        return 0;
    if (m_resource->map(m_surface, format, (VideoFrame*)handle, plane))
        return handle;
    return 0;
}
} //namespace vpu
} //namespace QtAV
