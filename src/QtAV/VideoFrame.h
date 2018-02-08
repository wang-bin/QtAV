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

#ifndef QTAV_VIDEOFRAME_H
#define QTAV_VIDEOFRAME_H

#include <QtAV/Frame.h>
#include <QtAV/VideoFormat.h>
#include <QtCore/QSize>
/// TODO: fromAVFrame(const AVFrame* f);
namespace QtAV {

/// metadata: pallete for pal8
class VideoFramePrivate;
class Q_AV_EXPORT VideoFrame : public Frame
{
    Q_DECLARE_PRIVATE(VideoFrame)
public:
    /*!
     * \brief fromGPU
     * Make a VideoFrame with data on host memory from GPU resource
     * \param fmt video format of GPU resource
     * \param width frame width
     * \param height frame height
     * \param surface_h surface height. Can be greater than visual frame height because of alignment
     * \param src CPU accessible address of frame planes on GPU. src[0] must be valid. src[i>0] will be filled depending on pixel format, pitch and surface_h if it's NULL.
     * \param pitch plane pitch on GPU. pitch[0] must be valid. pitch[i>0] will be filled depending on pixel format, pitch[0] and surface_h if it's NULL.
     * \param optimized try to use SIMD to copy from GPU. otherwise use memcpy
     * \param swapUV it's required if u/v src are null
     */
    static VideoFrame fromGPU(const VideoFormat& fmt, int width, int height, int surface_h, quint8 *src[], int pitch[], bool optimized = true, bool swapUV = false);
    static void copyPlane(quint8 *dst, size_t dst_stride, const quint8 *src, size_t src_stride, unsigned byteWidth, unsigned height);

    VideoFrame();
    //must set planes and linesize manually if data is empty
    // must set planes and linesize manually
    // alignment: data ptr alignment
    VideoFrame(int width, int height, const VideoFormat& format, const QByteArray& data = QByteArray(), int alignment = 1);
    VideoFrame(const QImage& image);
    VideoFrame(const VideoFrame &other);
    ~VideoFrame();

    VideoFrame &operator =(const VideoFrame &other);

    int channelCount() const Q_DECL_OVERRIDE;
    /*!
     * Deep copy. Given the format, width and height, plane addresses and line sizes.
     */
    VideoFrame clone() const;
    VideoFormat format() const;
    VideoFormat::PixelFormat pixelFormat() const;
    QImage::Format imageFormat() const;
    int pixelFormatFFmpeg() const;

    bool isValid() const;
    operator bool() const { return isValid();}

    QSize size() const;
    //int width(int plane = 0) const?
    int width() const;
    int height() const;
    /*!
     * \brief effectiveBytesPerLine
     * The plane bytes contains valid image data without padded data for alignment reason
     */
    int effectiveBytesPerLine(int plane) const;
    // plane width with padded bytes for alignment.
    int planeWidth(int plane) const;
    int planeHeight(int plane) const;
    // display attributes
    float displayAspectRatio() const;
    void setDisplayAspectRatio(float displayAspectRatio);
    // TODO: pixel aspect ratio
    ColorSpace colorSpace() const;
    void setColorSpace(ColorSpace value);
    ColorRange colorRange() const;
    void setColorRange(ColorRange value);
    /*!
     * \brief toImage
     * Return a QImage of current video frame, with given format, image size and region of interest.
     * If VideoFrame is constructed from an QImage, the target format, size and roi are the same, then no data copy.
     * \param dstSize result image size
     * \param roi NOT implemented!
     */
    QImage toImage(QImage::Format fmt = QImage::Format_ARGB32, const QSize& dstSize = QSize(), const QRectF& roi = QRect()) const;
    /*!
     * \brief to
     * The result frame data is always on host memory. If video frame data is already in host memory, and the target parameters are the same, then return the current frame.
     * \param pixfmt target pixel format
     * \param dstSize target frame size
     * \param roi interested region of source frame
     */
    VideoFrame to(VideoFormat::PixelFormat pixfmt, const QSize& dstSize = QSize(), const QRectF& roi = QRect()) const;
    VideoFrame to(const VideoFormat& fmt, const QSize& dstSize = QSize(), const QRectF& roi = QRect()) const;
    bool to(VideoFormat::PixelFormat pixfmt, quint8 *const dst[], const int dstStride[], const QSize& dstSize = QSize(), const QRectF& roi = QRect()) const;
    bool to(const VideoFormat& fmt, quint8 *const dst[], const int dstStride[], const QSize& dstSize = QSize(), const QRectF& roi = QRect()) const;
    /*!
     * map a gpu frame to opengl texture or d3d texture or other handle.
     * handle: given handle. can be gl texture (& GLuint), d3d texture, or 0 if create a new handle
     * return the result handle or 0 if not supported
     */
    void* map(SurfaceType type, void* handle, int plane = 0);
    void* map(SurfaceType type, void* handle, const VideoFormat& fmt, int plane = 0);
    void unmap(void* handle);
    /*!
     * \brief createInteropHandle
     * \param handle input/output handle
     * \return null on error. otherwise return the input handle
     */
    void* createInteropHandle(void* handle, SurfaceType type, int plane);
};

class ImageConverter;
class Q_AV_EXPORT VideoFrameConverter
{
public:
    VideoFrameConverter();
    ~VideoFrameConverter();
    /// value out of [-100, 100] will be ignored
    void setEq(int brightness, int contrast, int saturation);
    /*!
     * \brief convert
     * return a frame with a given format from a given source frame. The result frame data is always on host memory.
     */
    VideoFrame convert(const VideoFrame& frame, const VideoFormat& fmt) const;
    VideoFrame convert(const VideoFrame& frame, VideoFormat::PixelFormat fmt) const;
    VideoFrame convert(const VideoFrame& frame, QImage::Format fmt) const;
    VideoFrame convert(const VideoFrame& frame, int fffmt) const;
private:
    mutable ImageConverter *m_cvt;
    int m_eq[3];
};
} //namespace QtAV

Q_DECLARE_METATYPE(QtAV::VideoFrame)
#endif // QTAV_VIDEOFRAME_H
