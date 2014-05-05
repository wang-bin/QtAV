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

#ifndef QTAV_VIDEOFORMAT_H
#define QTAV_VIDEOFORMAT_H

#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtGui/QImage>
#include <QtAV/QtAV_Global.h>

// FF_API_PIX_FMT
#ifdef PixelFormat
#undef PixelFormat
#endif

class QDebug;
namespace QtAV {

class VideoFormatPrivate;
class Q_AV_EXPORT VideoFormat
{
public:
    enum PixelFormat {
        Format_Invalid = -1,
        Format_ARGB32,
        Format_ARGB32_Premultiplied,
        Format_RGB32,
        Format_RGB24,
        Format_RGB565,
        Format_RGB555, //?
        Format_ARGB8565_Premultiplied,
        Format_BGRA32,
        Format_BGRA32_Premultiplied,
        Format_BGR32,
        Format_BGR24,
        Format_BGR565,
        Format_BGR555,
        Format_BGRA5658_Premultiplied,

        //http://www.fourcc.org/yuv.php
        Format_AYUV444,
        Format_AYUV444_Premultiplied,
        Format_YUV444,
        Format_YUV420P,
        Format_YV12,
        Format_UYVY,
        Format_YUYV,
        Format_NV12,
        Format_NV21,
        Format_IMC1,
        Format_IMC2,
        Format_IMC3, //same as IMC1, swap U V
        Format_IMC4, //same as IMC2, swap U V
        Format_Y8, //GREY. single 8 bit Y plane
        Format_Y16, //single 16 bit Y plane. LE

        Format_Jpeg,

        Format_CameraRaw,
        Format_AdobeDng,

        Format_YUV420P9LE,
        Format_YUV422P9LE,
        Format_YUV444P9LE,
        Format_YUV420P10LE,
        Format_YUV422P10LE,
        Format_YUV444P10LE,
        Format_YUV420P12LE,
        Format_YUV422P12LE,
        Format_YUV444P12LE,
        Format_YUV420P14LE,
        Format_YUV422P14LE,
        Format_YUV444P14LE,
        Format_YUV420P16LE,
        Format_YUV422P16LE,
        Format_YUV444P16LE,
        Format_YUV420P9BE,
        Format_YUV422P9BE,
        Format_YUV444P9BE,
        Format_YUV420P10BE,
        Format_YUV422P10BE,
        Format_YUV444P10BE,
        Format_YUV420P12BE,
        Format_YUV422P12BE,
        Format_YUV444P12BE,
        Format_YUV420P14BE,
        Format_YUV422P14BE,
        Format_YUV444P14BE,
        Format_YUV420P16BE,
        Format_YUV422P16BE,
        Format_YUV444P16BE,
        Format_User
    };

    static PixelFormat pixelFormatFromImageFormat(QImage::Format format);
    static QImage::Format imageFormatFromPixelFormat(PixelFormat format);
    static PixelFormat pixelFormatFromFFmpeg(int ff); //AVPixelFormat
    static int pixelFormatToFFmpeg(PixelFormat fmt);

    VideoFormat(PixelFormat format = Format_Invalid);
    VideoFormat(int formatFF);
    VideoFormat(QImage::Format fmt);
    VideoFormat(const QString& name);
    VideoFormat(const VideoFormat &other);
    ~VideoFormat();

    VideoFormat& operator=(const VideoFormat &other);
    VideoFormat& operator=(VideoFormat::PixelFormat pixfmt);
    VideoFormat& operator=(QImage::Format qpixfmt);
    VideoFormat& operator=(int ffpixfmt);
    bool operator==(const VideoFormat &other) const;
    bool operator==(VideoFormat::PixelFormat pixfmt) const;
    bool operator==(QImage::Format qpixfmt) const;
    bool operator==(int ffpixfmt) const;
    bool operator!=(const VideoFormat &other) const;
    bool operator!=(VideoFormat::PixelFormat pixfmt) const;
    bool operator!=(QImage::Format qpixfmt) const;
    bool operator!=(int ffpixfmt) const;

    bool isValid() const;

    PixelFormat pixelFormat() const;
    int pixelFormatFFmpeg() const;
    QImage::Format imageFormat() const;
    QString name() const;
    /*!
     * \brief setPixelFormat set pixel format to format. other information like bpp will be updated
     * \param format
     */
    void setPixelFormat(PixelFormat format);
    void setPixelFormatFFmpeg(int format);

    /*!
     * \brief channels
     * \return number of channels(components). e.g. RGBA has 4 channels, NV12 is 3
     */
    int channels() const;
    /*!
     * \brief planeCount
     * \return -1 if not a valid format
     */
    int planeCount() const;
    /*!
     * https://wiki.videolan.org/YUV
     *  YUV420P: 1pix = 4Y+U+V
     */
    int bitsPerPixel() const;
    int bitsPerPixel(int plane) const;
    int bitsPerPixelPadded() const;
    int bitsPerPixelPadded(int plane) const;
    int bytesPerPixel() const;
    int bytesPerPixel(int plane) const;

    // return line size with given width
    int bytesPerLine(int width, int plane) const;
    /*!
     * \brief chromaWidth
     * \param lumaWidth
     * \return U, V component (or channel) width for the given luma width.
     */
    int chromaWidth(int lumaWidth) const;
    int chromaHeight(int lumaHeight) const;
    //TODO: add planeWidth()/planeHeight()
    // test AV_PIX_FMT_FLAG_XXX
    bool isBigEndian() const;
    bool hasPalette() const;
    bool isPseudoPaletted() const;
    /**
     * All values of a component are bit-wise packed end to end.
     */
    bool isBitStream() const;
    /**
     * Pixel format is an HW accelerated format.
     */
    bool isHWAccelerated() const;
    /*!
     * \brief isPlanar
     * \return true if is planar or semi planar
     *
     * Semi-planar: 2 planes instead of 3, one plane for luminance, and one plane for both chrominance components.
     * They are also sometimes referred to as biplanar formats also
     * Packed: 1 plane
     * Planar: 1 plane for each component (channel)
     */
    bool isPlanar() const;
    bool isRGB() const;
    bool hasAlpha() const;

    static bool isPlanar(PixelFormat pixfmt);
    static bool isRGB(PixelFormat pixfmt);
    static bool hasAlpha(PixelFormat pixfmt);

private:
    QSharedDataPointer<VideoFormatPrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
Q_AV_EXPORT QDebug operator<<(QDebug debug, const VideoFormat &fmt);
Q_AV_EXPORT QDebug operator<<(QDebug debug, VideoFormat::PixelFormat pixFmt);
#endif

} //namespace QtAV

Q_DECLARE_METATYPE(QtAV::VideoFormat)
Q_DECLARE_METATYPE(QtAV::VideoFormat::PixelFormat)

#endif // QTAV_VIDEOFORMAT_H
