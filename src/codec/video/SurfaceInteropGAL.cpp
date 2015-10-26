
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
    // TODO: use mutex to avoid VideoCapture(async mode)/VideoRenderer scale the same frame at the same time
    if (!scaler) {
        scaler = new GALScaler();
    }
    const VideoFormat fmt(img->pixelFormat() == VideoFormat::Format_Invalid ? format.pixelFormat() : img->pixelFormat());
    int w = img->width(), h = img->height();
    // If out size not set, use gal out size which is aligned to 16
    if (w <= 0)
        w = FFALIGN(surface->fb.stride, 16); // TODO: use frame width?
    if (h <= 0)
        h = FFALIGN(surface->fb.height, 16);
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
    QVector<quint8*> dma_bits(fmt.planeCount());
    QVector<int> dma_pitch(fmt.planeCount());
    // if in/out parameters are the same, no scale is required
    if (w == surface->fb.stride && h == surface->fb.height && fmt.pixelFormat() == VideoFormat::Format_YUV420P) {
        for (int i = 0; i < fmt.planeCount(); ++i) {
            dma_bits[i] = (quint8*)src[i];
            dma_pitch[i] = srcStride[i];
        }
    } else {
        scaler->setInFormat(VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_YUV420P));
        scaler->setInSize(surface->fb.stride, surface->fb.height);
        scaler->setOutFormat(fmt.pixelFormatFFmpeg());
        scaler->setOutSize(w, h);
        if (!scaler->convert(src, srcStride))
            return false;
        dma_bits = scaler->outPlanes();
        dma_pitch = scaler->outLineSizes();
    }
    QVector<int> out_pitch(fmt.planeCount());
    if (!img->constBits(0)) { //interop in VideoFrame.to() and format is set
        int out_size = 0;
        QVector<int> offset(fmt.planeCount());
        for (int i = 0; i < fmt.planeCount(); ++i) {
            offset[i] = out_size;
            out_pitch[i] = fmt.width(w*fmt.bytesPerPixel(i), i);
            out_size += out_pitch.at(i) * fmt.height(h, i);
        }
        QByteArray host_buf(out_size, 0);
        *img = VideoFrame(w, h, fmt, host_buf);
        img->setBytesPerLine(out_pitch);
        for (int i = 0; i < fmt.planeCount(); ++i) {
            img->setBits((quint8*)host_buf.constData() + offset[i], i);
        }
        qDebug() << fmt;
        qDebug() << "pitch host: " << out_pitch << " vmem: " << dma_pitch;
    }
    for (int p = 0; p < fmt.planeCount(); ++p) {
        // dma copy. check img->stride
        if (img->bytesPerLine(p) == dma_pitch.at(p)) {
            // qMin(scaler->outHeight(), img->height)
            dma_copy_from_vmem(img->bits(p), (unsigned int)(quintptr)dma_bits.at(p), img->bytesPerLine(p)*img->planeHeight(p));
        } else {
            qWarning("different stride @plane%d. vmem: %d, host: %d", p, dma_pitch.at(p), img->bytesPerLine(p));
            for (int i = img->planeHeight(p) - 1; i >= 0; --i)
                dma_copy_from_vmem(img->bits(p) + i*img->bytesPerLine(p), (unsigned int)(quintptr)dma_bits.at(p) + i*dma_pitch.at(p), img->bytesPerLine(p));
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
