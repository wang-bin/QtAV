/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/ImageConverter.h"
#include "QtAV/ImageConverterTypes.h"
#include "QtAV/SurfaceInterop.h"
#include "QtAV/private/AVCompat.h"
#include <QtCore/QSharedPointer>
#include <QtGui/QImage>

// FF_API_PIX_FMT
#ifdef PixelFormat
#undef PixelFormat
#endif

namespace
{
class RegisterMetaTypes
{
public:
    RegisterMetaTypes()
    {
        qRegisterMetaType<QtAV::VideoFrame>("QtAV::VideoFrame");
    }
} _registerMetaTypes;
}

namespace QtAV {

class VideoFramePrivate : public FramePrivate
{
public:
    VideoFramePrivate()
        : FramePrivate()
        , width(0)
        , height(0)
        , displayAspectRatio(0)
        , format(VideoFormat::Format_Invalid)
        , textures(4, 0)
        , conv(0)
    {}
    VideoFramePrivate(int w, int h, const VideoFormat& fmt)
        : FramePrivate()
        , width(w)
        , height(h)
        , displayAspectRatio(0)
        , format(fmt)
        , textures(4, 0)
        , conv(0)
    {
        planes.resize(format.planeCount());
        line_sizes.resize(format.planeCount());
        textures.resize(format.planeCount());
    }
    ~VideoFramePrivate() {}
    bool convertTo(const VideoFormat& fmt) {
        return convertTo(fmt.pixelFormatFFmpeg());
    }
    bool convertTo(VideoFormat::PixelFormat fmt) {
        return convertTo(VideoFormat::pixelFormatToFFmpeg(fmt));
    }
    bool convertTo(QImage::Format fmt) {
        return convertTo(VideoFormat::pixelFormatFromImageFormat(fmt));
    }
    bool convertTo(int fffmt) {
        if (fffmt == format.pixelFormatFFmpeg())
            return true;
        if (!conv)
            return false;
        conv->setInFormat(format.pixelFormatFFmpeg());
        conv->setOutFormat(fffmt);
        conv->setInSize(width, height);
        conv->setOutSize(width, height);
        if (!conv->convert(planes.constData(), line_sizes.constData())) {
            format.setPixelFormat(VideoFormat::Format_Invalid);
            return false;
        }
        format.setPixelFormatFFmpeg(fffmt);
        data = conv->outData();
        planes = conv->outPlanes();
        line_sizes = conv->outLineSizes();

        planes.resize(format.planeCount());
        line_sizes.resize(format.planeCount());
        textures.resize(format.planeCount());

        return true;
    }
    bool convertTo(const VideoFormat& fmt, const QSizeF &dstSize, const QRectF &roi) {
        if (fmt == format.pixelFormatFFmpeg()
                && roi == QRectF(0, 0, width, height)
                && dstSize == roi.size())
            return true;
        if (!conv) {
            format.setPixelFormat(VideoFormat::Format_Invalid);
            return false;
        }
        format = fmt;
        data = conv->outData();
        planes = conv->outPlanes();
        line_sizes = conv->outLineSizes();

        planes.resize(fmt.planeCount());
        line_sizes.resize(fmt.planeCount());
        textures.resize(fmt.planeCount());

        return false;
    }

    int width, height;
    float displayAspectRatio;
    VideoFormat format;
    QVector<int> textures;

    ImageConverter *conv;
    VideoSurfaceInteropPtr surface_interop;
};

VideoFrame::VideoFrame()
    : Frame(*new VideoFramePrivate())
{
}

VideoFrame::VideoFrame(int width, int height, const VideoFormat &format)
    : Frame(*new VideoFramePrivate(width, height, format))
{
}

VideoFrame::VideoFrame(const QByteArray& data, int width, int height, const VideoFormat &format)
    : Frame(*new VideoFramePrivate(width, height, format))
{
    Q_D(VideoFrame);
    d->data = data;
}

VideoFrame::VideoFrame(const QVector<int>& textures, int width, int height, const VideoFormat &format)
    : Frame(*new VideoFramePrivate(width, height, format))
{
    Q_D(VideoFrame);
    d->textures = textures;
}

VideoFrame::VideoFrame(const QImage& image)
    : Frame(*new VideoFramePrivate(image.width(), image.height(), VideoFormat(image.format())))
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
    int bytes = 0;
    for (int i = 0; i < d->format.planeCount(); ++i) {
        bytes += bytesPerLine(i)*planeHeight(i);
    }
    QByteArray buf(bytes, 0);
    char *dst = buf.data(); //must before buf is shared, otherwise data will be detached.
    VideoFrame f(buf, width(), height(), d->format);
    for (int i = 0; i < d->format.planeCount(); ++i) {
        f.setBits((quint8*)dst, i);
        f.setBytesPerLine(bytesPerLine(i), i);
        const int plane_size = bytesPerLine(i)*planeHeight(i);
        memcpy(dst, bits(i), plane_size);
        dst += plane_size;
    }
    f.setDisplayAspectRatio(d->displayAspectRatio);
    //f.setImageConverter(d->conv);
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

    return 1;
}

void VideoFrame::setDisplayAspectRatio(float displayAspectRatio)
{
    d_func()->displayAspectRatio = displayAspectRatio;
}

int VideoFrame::effectiveBytesPerLine(int plane) const
{
    Q_D(const VideoFrame);
    return d->format.bytesPerLine(width(), plane);
}

void VideoFrame::setImageConverter(ImageConverter *conv)
{
    d_func()->conv = conv;
}

bool VideoFrame::convertTo(const VideoFormat& fmt)
{
    Q_D(VideoFrame);
    if (fmt == d->format) //TODO: check whether own the data
        return true;
    return d->convertTo(fmt);
}

bool VideoFrame::convertTo(VideoFormat::PixelFormat fmt)
{
    return d_func()->convertTo(fmt);
}

bool VideoFrame::convertTo(QImage::Format fmt)
{
    return d_func()->convertTo(fmt);
}

bool VideoFrame::convertTo(int fffmt)
{
    return d_func()->convertTo(fffmt);
}

bool VideoFrame::convertTo(const VideoFormat& fmt, const QSizeF &dstSize, const QRectF &roi)
{
    return d_func()->convertTo(fmt, dstSize, roi);
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

} //namespace QtAV
