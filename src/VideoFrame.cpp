/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

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

// TODO: VideoFrame.copyPropertyies(VideoFrame) to avoid missing property copy
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
        const int offset_16 = (16 - ((uintptr_t)buf.constData() & 0x0f)) & 0x0f;
        // plane 1, 2... is aligned?
        uchar* plane_ptr = (uchar*)buf.constData() + offset_16;
        QVector<uchar*> dst(nb_planes, 0);
        for (int i = 0; i < nb_planes; ++i) {
            dst[i] = plane_ptr;
            // TODO: add VideoFormat::planeWidth/Height() ?
            // pitch instead of surface_width
            plane_ptr += pitch[i] * h[i];
            gpu_memcpy(dst[i], src[i], pitch[i]*h[i]);

        }
        frame = VideoFrame(width, height, fmt, buf);
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

void VideoFrame::copyPlane(quint8 *dst, size_t dst_stride, const quint8 *src, size_t src_stride, unsigned byteWidth, unsigned height)
{
    if (!dst || !src)
        return;
    if (dst_stride == src_stride && src_stride == byteWidth && height) {
        memcpy(dst, src, byteWidth*height);
        return;
    }
    for (; height > 0; --height) {
        memcpy(dst, src, byteWidth);
        src += src_stride;
        dst += dst_stride;
    }
}

class VideoFramePrivate : public FramePrivate
{
    Q_DISABLE_COPY(VideoFramePrivate)
public:
    VideoFramePrivate()
        : FramePrivate()
        , width(0)
        , height(0)
        , color_space(ColorSpace_Unknown)
        , color_range(ColorRange_Unknown)
        , displayAspectRatio(0)
        , format(VideoFormat::Format_Invalid)
    {}
    VideoFramePrivate(int w, int h, const VideoFormat& fmt)
        : FramePrivate()
        , width(w)
        , height(h)
        , color_space(ColorSpace_Unknown)
        , color_range(ColorRange_Unknown)
        , displayAspectRatio(0)
        , format(fmt)
    {
        if (!format.isValid())
            return;
        planes.resize(format.planeCount());
        line_sizes.resize(format.planeCount());
        planes.reserve(format.planeCount());
        line_sizes.reserve(format.planeCount());
    }
    ~VideoFramePrivate() {}
    int width, height;
    ColorSpace color_space;
    ColorRange color_range;
    float displayAspectRatio;
    VideoFormat format;
    QScopedPointer<QImage> qt_image;

    VideoSurfaceInteropPtr surface_interop;
};

VideoFrame::VideoFrame()
    : Frame(new VideoFramePrivate())
{
}

VideoFrame::VideoFrame(int width, int height, const VideoFormat &format, const QByteArray& data, int alignment)
    : Frame(new VideoFramePrivate(width, height, format))
{
    Q_D(VideoFrame);
    d->data = data;
    d->data_align = alignment;
}

VideoFrame::VideoFrame(const QImage& image)
    : Frame(new VideoFramePrivate(image.width(), image.height(), VideoFormat(image.format())))
{
    setBits((uchar*)image.constBits(), 0);
    setBytesPerLine(image.bytesPerLine(), 0);
    d_func()->qt_image.reset(new QImage(image));
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
    VideoFrame f(width(), height(), d->format, buf);
    const int nb_planes = d->format.planeCount();
    for (int i = 0; i < nb_planes; ++i) {
        f.setBits((quint8*)dst, i);
        f.setBytesPerLine(bytesPerLine(i), i);
        const int plane_size = bytesPerLine(i)*planeHeight(i);
        memcpy(dst, constBits(i), plane_size);
        dst += plane_size;
    }
    f.d_ptr->metadata = d->metadata; // need metadata?
    f.setTimestamp(d->timestamp);
    f.setDisplayAspectRatio(d->displayAspectRatio);
    f.setColorSpace(d->color_space);
    f.setColorRange(d->color_range);
    return f;
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

int VideoFrame::planeWidth(int plane) const
{
    Q_D(const VideoFrame);
    return d->format.width(width(), plane);
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

ColorRange VideoFrame::colorRange() const
{
    return d_func()->color_range;
}

void VideoFrame::setColorRange(ColorRange value)
{
    d_func()->color_range = value;
}

int VideoFrame::effectiveBytesPerLine(int plane) const
{
    Q_D(const VideoFrame);
    return d->format.bytesPerLine(width(), plane);
}

QImage VideoFrame::toImage(QImage::Format fmt, const QSize& dstSize, const QRectF &roi) const
{
    Q_D(const VideoFrame);
    if (!d->qt_image.isNull()
            && fmt == d->qt_image->format()
            && dstSize == d->qt_image->size()
            && (!roi.isValid() || roi == d->qt_image->rect())) {
        return *d->qt_image.data();
    }
    VideoFrame f(to(VideoFormat(VideoFormat::pixelFormatFromImageFormat(fmt)), dstSize, roi));
    if (!f)
        return QImage();
    QImage image(f.frameDataPtr(), f.width(), f.height(), f.bytesPerLine(0), fmt);
    return image.copy();
}

VideoFrame VideoFrame::to(const VideoFormat &fmt, const QSize& dstSize, const QRectF& roi) const
{
    if (!isValid() || !constBits(0)) {// hw surface. map to host. only supports rgb packed formats now
        Q_D(const VideoFrame);
        const QVariant v = d->metadata.value(QStringLiteral("surface_interop"));
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
    const int w = dstSize.width() > 0 ? dstSize.width() : width();
    const int h = dstSize.height() > 0 ? dstSize.height() : height();
    if (fmt.pixelFormatFFmpeg() == pixelFormatFFmpeg()
            && w == width() && h == height()
            // TODO: roi check.
            )
        return *this;
    Q_D(const VideoFrame);
    ImageConverterSWS conv;
    conv.setInFormat(pixelFormatFFmpeg());
    conv.setOutFormat(fmt.pixelFormatFFmpeg());
    conv.setInSize(width(), height());
    conv.setOutSize(w, h);
    conv.setInRange(colorRange());
    if (!conv.convert(d->planes.constData(), d->line_sizes.constData())) {
        qWarning() << "VideoFrame::to error: " << format() << "=>" << fmt;
        return VideoFrame();
    }
    VideoFrame f(w, h, fmt, conv.outData(), ImageConverter::DataAlignment);
    f.setBits(conv.outPlanes());
    f.setBytesPerLine(conv.outLineSizes());
    if (fmt.isRGB()) {
        f.setColorSpace(fmt.isPlanar() ? ColorSpace_GBR : ColorSpace_RGB);
    } else {
        f.setColorSpace(ColorSpace_Unknown);
    }
    // TODO: color range
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
    return map(type, handle, format(), plane);
}

void *VideoFrame::map(SurfaceType type, void *handle, const VideoFormat& fmt, int plane)
{
    Q_D(VideoFrame);
    const QVariant v = d->metadata.value(QStringLiteral("surface_interop"));
    if (!v.isValid())
        return 0;
    d->surface_interop = v.value<VideoSurfaceInteropPtr>();
    if (!d->surface_interop)
        return 0;
    if (plane > planeCount())
        return 0;
    return d->surface_interop->map(type, fmt, handle, plane);
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
    const QVariant v = d->metadata.value(QStringLiteral("surface_interop"));
    if (!v.isValid())
        return 0;
    d->surface_interop = v.value<VideoSurfaceInteropPtr>();
    if (!d->surface_interop)
        return 0;
    if (plane > planeCount())
        return 0;
    return d->surface_interop->createHandle(handle, type, format(), plane, planeWidth(plane), planeHeight(plane));
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
    if (!frame.constBits(0)) // hw surface
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
    m_cvt->setInRange(frame.colorRange());
    const int pal = format.hasPalette();
    QVector<const uchar*> pitch(format.planeCount() + pal);
    QVector<int> stride(format.planeCount() + pal);
    for (int i = 0; i < format.planeCount(); ++i) {
        pitch[i] = frame.constBits(i);
        stride[i] = frame.bytesPerLine(i);
    }
    const QByteArray paldata(frame.metaData(QStringLiteral("pallete")).toByteArray());
    if (pal > 0) {
        pitch[1] = (const uchar*)paldata.constData();
        stride[1] = paldata.size();
    }
    if (!m_cvt->convert(pitch.constData(), stride.constData())) {
        return VideoFrame();
    }
    const VideoFormat fmt(fffmt);
    VideoFrame f(frame.width(), frame.height(), fmt, m_cvt->outData());
    f.setBits(m_cvt->outPlanes());
    f.setBytesPerLine(m_cvt->outLineSizes());
    f.setTimestamp(frame.timestamp());
    f.setDisplayAspectRatio(frame.displayAspectRatio());
    // metadata?
    if (fmt.isRGB()) {
        f.setColorSpace(fmt.isPlanar() ? ColorSpace_GBR : ColorSpace_RGB);
    } else {
        f.setColorSpace(ColorSpace_Unknown);
    }
    // TODO: color range
    return f;
}

} //namespace QtAV
