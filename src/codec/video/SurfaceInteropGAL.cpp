
#include "SurfaceInteropGAL.h"
#include "viv/GALScaler.h"
#include "QtAV/VideoFrame.h"
#include "QtAV/private/AVCompat.h"
#include <QtDebug>

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

bool InteropResource::map(const FBSurfacePtr &surface, const VideoFormat &format, VideoFrame *img, int frame_w, int frame_h, int)
{
    // TODO: use mutex to avoid VideoCapture(async mode)/VideoRenderer scale the same frame at the same time
    if (!scaler) {
        scaler = new GALScaler();
    }
    scaler->setHostSource(!surface->isValid());
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
    const int planes_out = fmt.planeCount();
    if (!img->constBits(0)) { //interop in VideoFrame.to() and format is set
        scaler->setHostTarget(true);
        int out_size = 0;
        QVector<int> offset(planes_out);
        QVector<int> out_pitch(planes_out);
        for (int i = 0; i < planes_out; ++i) {
            offset[i] = out_size;
            out_pitch[i] = fmt.width(FFALIGN(w*fmt.bytesPerPixel(i), 16), i);
            out_size += out_pitch.at(i) * fmt.height(h, i);
        }
        QByteArray host_buf(out_size, 0);
        *img = VideoFrame(w, h, fmt, host_buf);
        img->setBytesPerLine(out_pitch);
        for (int i = 0; i < planes_out; ++i) {
            img->setBits((quint8*)host_buf.constData() + offset[i], i);
        }
        qDebug() << fmt << " pitch host: " << out_pitch;
    }
    // must be the same stride. height can be smaller than algined value and use the smaller one
    if (img->bytesPerLine(0) == srcStride[0] && FFALIGN(h, 16) == surface->fb.height
            && fmt.pixelFormat() == VideoFormat::Format_YUV420P) {
        // if in/out parameters are the same, no scale is required
        for (int p = 0; p < planes_out; ++p) {
            // dma copy. check img->stride
            if (img->bytesPerLine(p) == srcStride[p]) {
                dma_copy_from_vmem(img->bits(p), (unsigned int)(quintptr)src[p], img->bytesPerLine(p)*img->planeHeight(p));
            } else {
                qWarning("different stride @plane%d. vmem: %d, host: %d", p, srcStride[p], img->bytesPerLine(p));
                for (int i = img->planeHeight(p) - 1; i >= 0; --i)
                    dma_copy_from_vmem(img->bits(p) + i*img->bytesPerLine(p), (unsigned int)(quintptr)src[p] + i*srcStride[p], img->bytesPerLine(p));
            }
        }
    } else {
        scaler->setInFormat(VideoFormat::pixelFormatToFFmpeg(VideoFormat::Format_YUV420P));
        scaler->setInSize(frame_w, frame_h);
        scaler->setOutFormat(fmt.pixelFormatFFmpeg());
        scaler->setOutSize(w, h);
        QVector<quint8*> dst(planes_out);
        QVector<int> dstStride(planes_out);
        for (int i = 0; i < planes_out; ++i) {
            dst[i] = img->bits(i);
            dstStride[i] = img->bytesPerLine(i);
        }
        if (!scaler->convert(src, srcStride, dst.data(), dstStride.data()))
            return false;
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
    if (!fmt.isRGB() && fmt.pixelFormat() != VideoFormat::Format_YUV420P)
        return 0;
    if (type == HostMemorySurface
            || type == UserSurface) {
        if (m_resource->map(m_surface, fmt, (VideoFrame*)handle, frame_width, frame_height, plane))
            return handle;
    }
    return 0;
}
} //namespace vpu
} //namespace QtAV
