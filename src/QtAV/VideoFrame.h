/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_VIDEOFRAME_H
#define QTAV_VIDEOFRAME_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/Frame.h>
#include <QtCore/QSize>

/*
 * TODO: move PixelFormat to VideoFormat class
 * bpp used and not used
 */

namespace QtAV {

template<int N> struct Log2 { enum { value = 1 + Log2<N/2>::value }; };
template<> struct Log2<0> { enum { value = 0 }; }; //also for N=1
template<int N> struct ULog2 { enum { value = (N>>Log2<N>::value) ? Log2<N>::value+1 : Log2<N>::value }; };

class ImageConverter;
class VideoFramePrivate;
class Q_AV_EXPORT VideoFrame : public Frame
{
    DPTR_DECLARE_PRIVATE(VideoFrame)
public:
    /*!
     * \brief The PixelFormat enum
     *      currently from QVideoFrame
     *      the value is N*100 + bpp, so bpp() == PixelFormat % 100
     *      where kMaxBPP < 100.
     *      in general, let m = ceiling(kMaxBPP/100), then
     *  PixelFormat value = N*100^m+bpp
     *      or
     *  PixelFormat value = N*(kMaxBPP+1)+bpp
     *  bpp == PixelFormat % (kMaxBPP+1)
     *      or for small kMaxBPP(i.e. Log2(N)+kBPPS<16), the best is
     *  value = N<<ceiling(log_2(kMaxBPP))+bpp
     *  bpp() == PixelFormat & (1<<kBPPS)-1 (i.e. last kBPPS bits are 1)
     *      and all values are different
     * 2013-10-30
     *
     * TODO: what about rgb555 and sth. like that? how to calculate the values auto by compiler with a simpe expression?
     */
    // TODO: VideoFormat class like AudioFormat?
    static const int kMaxBPP = 64;
    static const int kBPPS = ULog2<kMaxBPP+1>::value;
    static const int kBPPMask = (1<<kBPPS) - 1;
    /*!
     * we can use an array to store the map from PixelFormat to bpp. But by giving special enum
     * values described here, we can save both mem and cpu
     */
    // TODO: ffmpeg formats. e.g. dxva
    enum PixelFormat {
        Format_Invalid = 0,
        Format_ARGB32 = (1<<kBPPS) + 32,
        Format_ARGB32_Premultiplied = (2<<kBPPS) + 32,
        Format_RGB32 = (3<<kBPPS) + 32,
        Format_RGB24 = (4<<kBPPS) + 24,
        Format_RGB565 = (5<<kBPPS) + 16,
        Format_RGB555 = (6<<kBPPS) + 15, //?
        Format_ARGB8565_Premultiplied = (7<<kBPPS) + 24,
        Format_BGRA32 = (8<<kBPPS) + 32,
        Format_BGRA32_Premultiplied = (9<<kBPPS) + 32,
        Format_BGR32 = (10<<kBPPS) + 32,
        Format_BGR24 = (11<<kBPPS) + 24,
        Format_BGR565 = (12<<kBPPS) + 16,
        Format_BGR555 = (13<<kBPPS) + 15, //?
        Format_BGRA5658_Premultiplied = (14<<kBPPS) + 24,

        //http://www.fourcc.org/yuv.php
        Format_AYUV444 = (15<<kBPPS) + 12,
        Format_AYUV444_Premultiplied = (16<<kBPPS) + 12,
        Format_YUV444 = (17<<kBPPS) + 12,
        Format_YUV420P = (18<<kBPPS) + 12,
        Format_YV12 = (19<<kBPPS) + 12,
        Format_UYVY = (20<<kBPPS) + 16,
        Format_YUYV = (21<<kBPPS) + 16,
        Format_NV12 = (22<<kBPPS) + 12,
        Format_NV21 = (23<<kBPPS) + 12,
        Format_IMC1 = (24<<kBPPS) + 12,
        Format_IMC2 = (25<<kBPPS) + 12,
        Format_IMC3 = (26<<kBPPS) + 12,
        Format_IMC4 = (27<<kBPPS) + 12,
        Format_Y8 = (28<<kBPPS) + 8,
        Format_Y16 = (28<<kBPPS) + 16,

        Format_Jpeg,

        Format_CameraRaw,
        Format_AdobeDng,

        Format_User = 1000<<kBPPS
    };

    VideoFrame(int width, int height, PixelFormat pixFormat);
    VideoFrame(int width, int height, int pixFormatFFmpeg);
    virtual ~VideoFrame();

    PixelFormat pixelFormat() const;
    int pixelFormatFFmpeg() const;

    /*!
     * \brief bytesPerLine bytes/line for the given pixel format and size
     * \return
     */
    virtual int bytesPerLine(int plane) const;
    /*!
     * \brief samplesPerPixel
     * https://wiki.videolan.org/YUV
     *  YUV420P: 1pix = 4Y+U+V
     *
     * \return
     */
    int bitsPerPixel(int plane) const;

    int bpp() const;
    int bytesPerPixel() const;
    QSize size() const;
    int width() const;
    int height() const;

    void setImageConverter(ImageConverter *conv);
    // if use gpu to convert, mapToDevice() first
    bool convertTo(PixelFormat fmt);
    bool convertTo(PixelFormat fmt, const QSizeF& dstSize, const QRectF& roi);

    //upload to GPU. return false if gl(or other, e.g. cl) not supported
    bool mapToDevice();
    //copy to host. Used if gpu filter not supported. To avoid copy too frequent, sort the filters first?
    bool mapToHost();
    /*!
       texture in FBO. we can use texture in FBO through filter pipeline then switch to window context to display
       return -1 if no texture, not uploaded
     */
    int texture(int plane = 0) const;

protected:
    //QExplicitlySharedDataPointer<QVideoFramePrivate> d;
};

} //namespace QtAV

#endif // QTAV_VIDEOFRAME_H
