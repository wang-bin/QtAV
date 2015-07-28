/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include "QtAV/VideoFrame.h"
#include "QtAV/private/Frame_p.h"
#include "QtAV/SurfaceInterop.h"
#include "ImageConverter.h"
#include <QtCore/QSharedPointer>
#include <QtGui/QImage>
#include "QtAV/private/AVCompat.h"
#include "utils/GPUMemCopy.h"
#include "utils/Logger.h"

namespace QtAV {
namespace{
static const struct RegisterMetaTypes
{
    inline RegisterMetaTypes()
    {
        qRegisterMetaType<QtAV::VideoFrame>("QtAV::VideoFrame");
    }
} _registerMetaTypes;
}

VideoFrame VideoFrame::fromGPU(const VideoFormat& fmt, int width, int height, int surface_h, quint8 *src[], int pitch[], bool optimized, bool swapUV)
{
    Q_ASSERT(src[0] && pitch[0] > 0 && "VideoFrame::fromGPU: src[0] and pitch[0] must be set");
    const int nb_planes = fmt.planeCount();
    const int chroma_pitch = nb_planes > 1 ? fmt.bytesPerLine(pitch[0], 1) : 0;
    const int chroma_h = fmt.chromaHeight(surface_h);
    int h[] = { surface_h, 0, 0};
    for (int i = 1; i < nb_planes; ++i) {
        h[i] = chroma_h;
        // set chroma address and pitch if not set
        if (pitch[i] <= 0)
            pitch[i] = chroma_pitch;
        if (!src[i])
            src[i] = src[i-1] + pitch[i-1]*h[i-1];
    }
    if (swapUV) {
        std::swap(src[1], src[2]);
        std::swap(pitch[1], pitch[2]);
    }
    VideoFrame frame;
    if (optimized) {
        int yuv_size = 0;
        for (int i = 0; i < nb_planes; ++i) {
            yuv_size += pitch[i]*h[i];
        }
        // additional 15 bytes to ensure 16 bytes aligned
        QByteArray buf(15 + yuv_size, 0);
        const int offset_16 = (16 - ((uintptr_t)buf.data() & 0x0f)) & 0x0f;
        // plane 1, 2... is aligned?
        uchar* plane_ptr = (uchar*)buf.data() + offset_16;
        QVector<uchar*> dst(nb_planes, 0);
        for (int i = 0; i < nb_planes; ++i) {
            dst[i] = plane_ptr;
            // TODO: add VideoFormat::planeWidth/Height() ?
            // pitch instead of surface_width
            plane_ptr += pitch[i] * h[i];
            gpu_memcpy(dst[i], src[i], pitch[i]*h[i]);

        }
        frame = VideoFrame(buf, width, height, fmt);
        frame.setBits(dst);
        frame.setBytesPerLine(pitch);
    } else {
        frame = VideoFrame(width, height, fmt);
        frame.setBits(src);
        frame.setBytesPerLine(pitch);
        // TODO: why clone is faster()?
        // TODO: buffer pool and create VideoFrame when needed to avoid copy? also for other va
        frame = frame.clone();
    }
    return frame;
}

class VideoFramePrivate : public FramePrivate
{
    Q_DISABLE_COPY(VideoFramePrivate)
public:
    VideoFramePrivate()
        : FramePrivate()
        , width(0)
        , height(0)
        , color_space(ColorSpace_Unknow)
        , displayAspectRatio(0)
        , format(VideoFormat::Format_Invalid)
        , textures(4, 0)
    {}
    VideoFramePrivate(int w, int h, const VideoFormat& fmt)
        : FramePrivate()
        , width(w)
        , height(h)
        , color_space(ColorSpace_Unknow)
        , displayAspectRatio(0)
        , format(fmt)
        , textures(4, 0)
    {
        if (!format.isValid())
            return;
        planes.resize(format.planeCount());
        line_sizes.resize(format.planeCount());
        textures.resize(format.planeCount());
        planes.reserve(format.planeCount());
        line_sizes.reserve(format.planeCount());
        textures.reserve(format.planeCount());
    }
    ~VideoFramePrivate() {}
    int width, height;
    ColorSpace color_space;
    float displayAspectRatio;
    VideoFormat format;
    QVector<int> textures;

    VideoSurfaceInteropPtr surface_interop;
};

VideoFrame::VideoFrame()
    : Frame(new VideoFramePrivate())
{
}

VideoFrame::VideoFrame(int width, int height, const VideoFormat &format)
    : Frame(new VideoFramePrivate(width, height, format))
{
}

VideoFrame::VideoFrame(const QByteArray& data, int width, int height, const VideoFormat &format)
    : Frame(new VideoFramePrivate(width, height, format))
{
    Q_D(VideoFrame);
    d->data = data;
}

VideoFrame::VideoFrame(const QVector<int>& textures, int width, int height, const VideoFormat &format)
    : Frame(new VideoFramePrivate(width, height, format))
{
    Q_D(VideoFrame);
    d->textures = textures;
}

VideoFrame::VideoFrame(const QImage& image)
    : Frame(new VideoFramePrivate(image.width(), image.height(), VideoFormat(image.format())))
{
    // TODO: call const image.bits()?
    setBits((uchar*)image.bits(), 0);
    setBytesPerLine(image.bytesPerLine(), 0);
}

/*!
    Constructs a shallow copy of \a other.  Since VideoFrame is
    explicitly shared, these two instances will reflect the same frame.

*/
VideoFrame::VideoFrame(const VideoFrame &other)
    : Frame(other)
{
}

/*!
    Assigns the contents of \a other to this video frame.  Since VideoFrame is
    explicitly shared, these two instances will reflect the same frame.

*/
VideoFrame &VideoFrame::operator =(const VideoFrame &other)
{
    d_ptr = other.d_ptr;
    return *this;
}

VideoFrame::~VideoFrame()
{
}

int VideoFrame::channelCount() const
{
    Q_D(const VideoFrame);
    if (!d->format.isValid())
        return 0;
    return d->format.channels();
}

VideoFrame VideoFrame::clone() const
{
    Q_D(const VideoFrame);
    if (!d->format.isValid())
        return VideoFrame();

    // data may be not set (ff decoder)
    if (d->planes.isEmpty() || !d->planes.at(0)) {//d->data.size() < width()*height()) { // at least width*height
        // maybe in gpu memory, then bits() is not set
        qDebug("frame data not valid. size: %d", d->data.size());
        VideoFrame f(width(), height(), d->format);
        f.d_ptr->metadata = d->metadata; // need metadata?
        f.setTimestamp(d->timestamp);
        f.setDisplayAspectRatio(d->displayAspectRatio);
        return f;
    }
    int bytes = 0;
    for (int i = 0; i < d->format.planeCount(); ++i) {
        bytes += bytesPerLine(i)*planeHeight(i);
    }

    QByteArray buf(bytes, 0);
    char *dst = buf.data(); //must before buf is shared, otherwise data will be detached.
    VideoFrame f(buf, width(), height(), d->format);
    const int nb_planes = d->format.planeCount();
    for (int i = 0; i < nb_planes; ++i) {
        f.setBits((quint8*)dst, i);
        f.setBytesPerLine(bytesPerLine(i), i);
        const int plane_size = bytesPerLine(i)*planeHeight(i);
        memcpy(dst, bits(i), plane_size);
        dst += plane_size;
    }
    f.d_ptr->metadata = d->metadata; // need metadata?
    f.setTimestamp(d->timestamp);
    f.setDisplayAspectRatio(d->displayAspectRatio);
    f.setColorSpace(d->color_space);
    return f;
}

int VideoFrame::allocate()
{
    Q_D(VideoFrame);
    if (pixelFormatFFmpeg() == QTAV_PIX_FMT_C(NONE) || width() <=0 || height() <= 0) {
        qWarning("Not valid format(%s) or size(%dx%d)", qPrintable(format().name()), width(), height());
        return 0;
    }
#if 0
    const int align = 16;
    int bytes = av_image_get_buffer_size((AVPixelFormat)d->format.pixelFormatFFmpeg(), width(), height(), align);
    d->data.resize(bytes);
    av_image_fill_arrays(d->planes.data(), d->line_sizes.data()
                         , (const uint8_t*)d->data.constData()
                         , (AVPixelFormat)d->format.pixelFormatFFmpeg()
                         , width(), height(), align);
    return bytes;
#endif
    int bytes = avpicture_get_size((AVPixelFormat)pixelFormatFFmpeg(), width(), height());
    if (d->data.size() < bytes) {
        d->data = QByteArray(bytes, 0);
    }
    init();
    return bytes;
}

VideoFormat VideoFrame::format() const
{
    return d_func()->format;
}

VideoFormat::PixelFormat VideoFrame::pixelFormat() const
{
    return d_func()->format.pixelFormat();
}

QImage::Format VideoFrame::imageFormat() const
{
    return d_func()->format.imageFormat();
}

int VideoFrame::pixelFormatFFmpeg() const
{
    return d_func()->format.pixelFormatFFmpeg();
}

bool VideoFrame::isValid() const
{
    Q_D(const VideoFrame);
    return d->width > 0 && d->height > 0 && d->format.isValid(); //data not empty?
}

QSize VideoFrame::size() const
{
    Q_D(const VideoFrame);
    return QSize(d->width, d->height);
}

int VideoFrame::width() const
{
    return d_func()->width;
}

int VideoFrame::height() const
{
    return d_func()->height;
}

int VideoFrame::effectivePlaneWidth(int plane) const
{
    Q_D(const VideoFrame);
    return effectiveBytesPerLine(plane)/d->format.bytesPerPixel(plane); //padded bpl?
}

int VideoFrame::planeWidth(int plane) const
{
    Q_D(const VideoFrame);
    return bytesPerLine(plane)/d->format.bytesPerPixel(plane);
}

int VideoFrame::planeHeight(int plane) const
{
    Q_D(const VideoFrame);
    if (plane == 0)
        return d->height;
    return d->format.chromaHeight(d->height);
}

float VideoFrame::displayAspectRatio() const
{
    Q_D(const VideoFrame);
    if (d->displayAspectRatio > 0)
        return d->displayAspectRatio;

    if (d->width > 0 && d->height > 0)
        return (float)d->width / (float)d->height;

    return 0;
}

void VideoFrame::setDisplayAspectRatio(float displayAspectRatio)
{
    d_func()->displayAspectRatio = displayAspectRatio;
}

ColorSpace VideoFrame::colorSpace() const
{
    return d_func()->color_space;
}

void VideoFrame::setColorSpace(ColorSpace value)
{
    d_func()->color_space = value;
}

int VideoFrame::effectiveBytesPerLine(int plane) const
{
    Q_D(const VideoFrame);
    return d->format.bytesPerLine(width(), plane);
}

QImage VideoFrame::toImage(QImage::Format fmt, const QSize& dstSize, const QRectF &roi) const
{
    VideoFrame f(to(VideoFormat(VideoFormat::pixelFormatFromImageFormat(fmt)), dstSize, roi));
    if (!f)
        return QImage();
    QImage image((const uchar*)f.frameData().constData(), f.width(), f.height(), f.bytesPerLine(0), fmt);
    return image.copy();
}

VideoFrame VideoFrame::to(const VideoFormat &fmt, const QSize& dstSize, const QRectF& roi) const
{
    if (!isValid() || !bits(0)) {// hw surface. map to host. only supports rgb packed formats now
        Q_D(const VideoFrame);
        const QVariant v = d->metadata.value("surface_interop");
        if (!v.isValid())
            return VideoFrame();
        VideoSurfaceInteropPtr si = v.value<VideoSurfaceInteropPtr>();
        if (!si)
            return VideoFrame();
        VideoFrame f;
        f.setDisplayAspectRatio(displayAspectRatio());
        f.setTimestamp(timestamp());
        if (si->map(HostMemorySurface, fmt, &f)) {
            if ((!dstSize.isValid() ||dstSize == QSize(width(), height())) && (!roi.isValid() || roi == QRectF(0, 0, width(), height()))) //roi is not supported now
                return f;
            return f.to(fmt, dstSize, roi);
        }
        return VideoFrame();
    }
    if (fmt.pixelFormatFFmpeg() == pixelFormatFFmpeg())
        return *this;
    Q_D(const VideoFrame);
    ImageConverterSWS conv;
    conv.setInFormat(pixelFormatFFmpeg());
    conv.setOutFormat(fmt.pixelFormatFFmpeg());
    conv.setInSize(width(), height());
    int w = width(), h = height();
    if (dstSize.width() > 0)
        w = dstSize.width();
    if (dstSize.height() > 0)
        h = dstSize.height();
    conv.setOutSize(w, h);
    if (!conv.convert(d->planes.constData(), d->line_sizes.constData())) {
        qWarning() << "VideoFrame::to error: " << format() << "=>" << fmt;
        return VideoFrame();
    }
    VideoFrame f(conv.outData(), w, h, fmt);
    f.setBits(conv.outPlanes());
    f.setBytesPerLine(conv.outLineSizes());
    if (fmt.isRGB()) {
        f.setColorSpace(fmt.isPlanar() ? ColorSpace_GBR : ColorSpace_RGB);
    } else {
        f.setColorSpace(ColorSpace_Unknow);
    }
    f.setTimestamp(timestamp());
    f.setDisplayAspectRatio(displayAspectRatio());
    f.d_ptr->metadata = d->metadata; // need metadata?
    return f;
}

VideoFrame VideoFrame::to(VideoFormat::PixelFormat pixfmt, const QSize& dstSize, const QRectF &roi) const
{
    return to(VideoFormat(pixfmt), dstSize, roi);
}

void *VideoFrame::map(SurfaceType type, void *handle, int plane)
{
    Q_D(VideoFrame);
    const QVariant v = d->metadata.value("surface_interop");
    if (!v.isValid())
        return 0;
    d->surface_interop = v.value<VideoSurfaceInteropPtr>();
    if (!d->surface_interop)
        return 0;
    if (plane > planeCount())
        return 0;
    return d->surface_interop->map(type, format(), handle, plane);
}

void VideoFrame::unmap(void *handle)
{
    Q_D(VideoFrame);
    if (!d->surface_interop)
        return;
    d->surface_interop->unmap(handle);
}

void* VideoFrame::createInteropHandle(void* handle, SurfaceType type, int plane)
{
    Q_D(VideoFrame);
    const QVariant v = d->metadata.value("surface_interop");
    if (!v.isValid())
        return 0;
    d->surface_interop = v.value<VideoSurfaceInteropPtr>();
    if (!d->surface_interop)
        return 0;
    if (plane > planeCount())
        return 0;
    return d->surface_interop->createHandle(handle, type, format(), plane, planeWidth(plane), planeHeight(plane));
}

int VideoFrame::texture(int plane) const
{
    Q_D(const VideoFrame);
    if (d->textures.size() <= plane)
        return -1;
    return d->textures[plane];
}

void VideoFrame::init()
{
    Q_D(VideoFrame);
    AVPicture picture;
    AVPixelFormat fff = (AVPixelFormat)d->format.pixelFormatFFmpeg();
    //int bytes = avpicture_get_size(fff, width(), height());
    //d->data.resize(bytes);
    avpicture_fill(&picture, (uint8_t*)d->data.constData(), fff, width(), height());
    setBits(picture.data);
    setBytesPerLine(picture.linesize);
}

VideoFrameConverter::VideoFrameConverter()
    : m_cvt(0)
{
    memset(m_eq, 0, sizeof(m_eq));
}

VideoFrameConverter::~VideoFrameConverter()
{
    if (m_cvt) {
        delete m_cvt;
        m_cvt = 0;
    }
}

void VideoFrameConverter::setEq(int brightness, int contrast, int saturation)
{
    if (brightness >= -100 && brightness <= 100)
        m_eq[0] = brightness;
    if (contrast >= -100 && contrast <= 100)
        m_eq[1] = contrast;
    if (saturation >= -100 && saturation <= 100)
        m_eq[2] = saturation;
}

VideoFrame VideoFrameConverter::convert(const VideoFrame& frame, const VideoFormat &fmt) const
{
    return convert(frame, fmt.pixelFormatFFmpeg());
}

VideoFrame VideoFrameConverter::convert(const VideoFrame &frame, VideoFormat::PixelFormat fmt) const
{
    return convert(frame, VideoFormat::pixelFormatToFFmpeg(fmt));
}

VideoFrame VideoFrameConverter::convert(const VideoFrame& frame, QImage::Format fmt) const
{
    return convert(frame, VideoFormat::pixelFormatFromImageFormat(fmt));
}

VideoFrame VideoFrameConverter::convert(const VideoFrame &frame, int fffmt) const
{
    if (!frame.isValid() || fffmt == QTAV_PIX_FMT_C(NONE))
        return VideoFrame();
    if (!frame.bits(0)) // hw surface
        return frame.to(VideoFormat::pixelFormatFromFFmpeg(fffmt));
    const VideoFormat format(frame.format());
    //if (fffmt == format.pixelFormatFFmpeg())
      //  return *this;
    if (!m_cvt) {
        m_cvt = new ImageConverterSWS();
    }
    m_cvt->setBrightness(m_eq[0]);
    m_cvt->setContrast(m_eq[1]);
    m_cvt->setSaturation(m_eq[2]);
    m_cvt->setInFormat(format.pixelFormatFFmpeg());
    m_cvt->setOutFormat(fffmt);
    m_cvt->setInSize(frame.width(), frame.height());
    m_cvt->setOutSize(frame.width(), frame.height());
    QVector<const uchar*> pitch(format.planeCount());
    QVector<int> stride(format.planeCount());
    for (int i = 0; i < format.planeCount(); ++i) {
        pitch[i] = frame.bits(i);
        stride[i] = frame.bytesPerLine(i);
    }
    if (!m_cvt->convert(pitch.constData(), stride.constData())) {
        return VideoFrame();
    }
    const VideoFormat fmt(fffmt);
    VideoFrame f(m_cvt->outData(), frame.width(), frame.height(), fmt);
    f.setBits(m_cvt->outPlanes());
    f.setBytesPerLine(m_cvt->outLineSizes());
    f.setTimestamp(frame.timestamp());
    f.setDisplayAspectRatio(frame.displayAspectRatio());
    // metadata?
    if (fmt.isRGB()) {
        f.setColorSpace(fmt.isPlanar() ? ColorSpace_GBR : ColorSpace_RGB);
    } else {
        f.setColorSpace(ColorSpace_Unknow);
    }
    return f;
}

} //namespace QtAV
