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

#ifndef QTAV_VIDEOFRAME_H
#define QTAV_VIDEOFRAME_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/Frame.h>
#include <QtAV/VideoFormat.h>
#include <QtCore/QSize>

namespace QtAV {

class ImageConverter;
class VideoFramePrivate;
class Q_AV_EXPORT VideoFrame : public Frame
{
    Q_DECLARE_PRIVATE(VideoFrame)
public:
    VideoFrame();
    //must set planes and linesize manually
    VideoFrame(int width, int height, const VideoFormat& format);
    //set planes and linesize manually or call init
    VideoFrame(const QByteArray& data, int width, int height, const VideoFormat& format);
    VideoFrame(const QVector<int>& textures, int width, int height, const VideoFormat& format);
    VideoFrame(const QImage& image);
    VideoFrame(const VideoFrame &other);
    virtual ~VideoFrame();

    VideoFrame &operator =(const VideoFrame &other);

    /*!
     * Deep copy. Given the format, width and height, plane addresses and line sizes.
     */
    VideoFrame clone() const;
    /*!
     * Allocate memory with given format, width and height. planes and bytesPerLine will be set internally.
     * The memory can be initialized by user
     */
    virtual int allocate();
    VideoFormat format() const;
    VideoFormat::PixelFormat pixelFormat() const;
    QImage::Format imageFormat() const;
    int pixelFormatFFmpeg() const;

    bool isValid() const;

    QSize size() const;
    //int width(int plane = 0) const?
    int width() const;
    int height() const;
    // plane width without padded bytes.
    int effectivePlaneWidth(int plane) const;
    // plane width with padded bytes for alignment.
    int planeWidth(int plane) const;
    int planeHeight(int plane) const;

    float displayAspectRatio() const;
    void setDisplayAspectRatio(float displayAspectRatio);

    // no padded bytes
    int effectiveBytesPerLine(int plane) const;

    //use ptr instead of ImageConverterId to avoid allocating memory
    // Id can be used in VideoThread
    void setImageConverter(ImageConverter *conv);
    // if use gpu to convert, mapToDevice() first
    /*!
     * \brief convertTo
     * You may clone the frame first because VideoFrame is explicitly shared
     */
    bool convertTo(const VideoFormat& fmt);
    bool convertTo(VideoFormat::PixelFormat fmt);
    bool convertTo(QImage::Format fmt);
    bool convertTo(int fffmt);
    bool convertTo(const VideoFormat& fmt, const QSizeF& dstSize, const QRectF& roi);

    //upload to GPU. return false if gl(or other, e.g. cl) not supported
    bool mapToDevice();
    //copy to host. Used if gpu filter not supported. To avoid copy too frequent, sort the filters first?
    bool mapToHost();
    /*!
       texture in FBO. we can use texture in FBO through filter pipeline then switch to window context to display
       return -1 if no texture, not uploaded
     */
    int texture(int plane = 0) const;
private:
    /*
     * call this only when setBytesPerLine() and setBits() will not be called
     */
    void init();
};

} //namespace QtAV

#endif // QTAV_VIDEOFRAME_H
