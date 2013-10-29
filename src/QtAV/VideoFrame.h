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

namespace QtAV {

template<int N>
struct Log2 {
    enum {
        value = 1 + Log2<N/2>::value
    };
};
template<>
struct Log2<0> { //also for N=1
    enum {
        value = 0
    };
};
template<bool Y> struct If {
    template<int A> struct Then {
        template<int B> struct Else {
            enum {
                value = Y ? A : B
            };
        };
    };
};

template<int N> struct ULog2 {
    enum {
        value = If<(N>>Log2<N>::value)>::Then<Log2<N>::value+1>::Else<Log2<N>::value>::value
    };
};

#define VIEW_TEMPLATE_VAL(expr) void view_template_val_##__LINE__() { \
    const int x = expr; \
    switch (x) {case expr:;case expr:;} \
}

class VideoFramePrivate;
class Q_AV_EXPORT VideoFrame : public Frame
{
    DPTR_DECLARE_PRIVATE(VideoFrame)
public:
    /*!
     * \brief The PixelFormat enum
     *      currently from QVideoFrame
     *      the value is N*10 + bytesPerPixel, so bytesPerPixel() == PixelFormat % 10
     *      where kMaxBPP < 10.
     *      in general, let m = ceiling(kMaxBPP/10), then
     *  PixelFormat value = N*10^m+bytesPerPixel
     *      or
     *  PixelFormat value = N*(kMaxBPP+1)+bytesPerPixel
     *  bytesPerPixel() == PixelFormat % (kMaxBPP+1)
     *      or for small kMaxBPP(i.e. Log2(N)+kBPPS<16), the best is
     *  value = N<<ceiling(log_2(kMaxBPP))+bytesPerPixel
     *  bytesPerPixel() == PixelFormat & bytesPerPixel
     *      and all values are different
     * 2013-10-30
     *
     * TODO: what about rgb555 and sth. like that? how to calculate the values auto by compiler with a simpe expression?
     */
    // TODO: VideoFormat class like AudioFormat?
    static const int kMaxBPP = 4;
    static const int kBPPS = ULog2<kMaxBPP+1>::value;
    /*!
     * we can use an array to store the map from PixelFormat to bytesPerPixel. But by giving special enum
     * values described here, we can save both mem and cpu
     */
    enum PixelFormat {
        Format_Invalid = 0,
        Format_ARGB32 = (1<<kBPPS) + 4,
        Format_ARGB32_Premultiplied = (2<<kBPPS) + 4,
        Format_RGB32 = (3<<kBPPS) + 4,
        Format_RGB24 = (4<<kBPPS) + 3,
        Format_RGB565 = (5<<kBPPS) + 2,
        Format_RGB555 = (6<<kBPPS) + 2, //?
        Format_ARGB8565_Premultiplied = (7<<kBPPS) + 3,
        Format_BGRA32 = (8<<kBPPS) + 4,
        Format_BGRA32_Premultiplied = (9<<kBPPS) + 4,
        Format_BGR32 = (10<<kBPPS) + 4,
        Format_BGR24 = (11<<kBPPS) + 3,
        Format_BGR565 = (12<<kBPPS) + 2,
        Format_BGR555 = (13<<kBPPS) + 2, //?
        Format_BGRA5658_Premultiplied = (14<<kBPPS) + 3,

        Format_AYUV444 = (15<<kBPPS) + 3,
        Format_AYUV444_Premultiplied = (16<<kBPPS) + 3,
        Format_YUV444 = (17<<kBPPS) + 3,
        Format_YUV420P = (18<<kBPPS),
        Format_YV12,
        Format_UYVY,
        Format_YUYV,
        Format_NV12,
        Format_NV21,
        Format_IMC1,
        Format_IMC2,
        Format_IMC3,
        Format_IMC4,
        Format_Y8,
        Format_Y16,

        Format_Jpeg,

        Format_CameraRaw,
        Format_AdobeDng,

        Format_User = 1000<<kBPPS
    };

    VideoFrame(int width, int height, PixelFormat pixFormat);
    virtual ~VideoFrame();

    /*!
     * \brief bytesPerLine bytes/line for the given pixel format and size
     * \return
     */
    virtual int bytesPerLine(int plane) const;

    int bytesPerPixel() const;
    QSize size() const;
    int width() const;
    int height() const;

protected:
    //QExplicitlySharedDataPointer<QVideoFramePrivate> d;
};

} //namespace QtAV

#endif // QTAV_VIDEOFRAME_H
