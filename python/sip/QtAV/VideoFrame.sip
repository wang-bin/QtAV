namespace QtAV
{

class VideoFrame : public QtAV::Frame
{
%TypeHeaderCode
#include <QtAV/VideoFrame.h>
%End

public:
    //static VideoFrame fromGPU(const VideoFormat& fmt, int width, int height, int surface_h, quint8 *src[], int pitch[], bool optimized = true, bool swapUV = false);
    //static void copyPlane(quint8 *dst, size_t dst_stride, const quint8 *src, size_t src_stride, unsigned byteWidth, unsigned height);

public:

    VideoFrame();
    VideoFrame(int width, int height, const VideoFormat& format, const QByteArray& data = QByteArray(), int alignment = 1);
    VideoFrame(const QImage& image);
    VideoFrame(const VideoFrame &other);
    ~VideoFrame();

    // NOTE: We don't expose the assignment operators.
    //VideoFrame &operator=(const VideoFrame &other);

    int channelCount() const;
    VideoFrame clone() const;
    VideoFormat format() const;
    VideoFormat::PixelFormat pixelFormat() const;
    QImage::Format imageFormat() const;
    int pixelFormatFFmpeg() const;

    bool isValid() const;
    operator bool() const;

    QSize size() const;
    int width() const;
    int height() const;
    int effectiveBytesPerLine(int plane) const;
    int planeWidth(int plane) const;
    int planeHeight(int plane) const;
    float displayAspectRatio() const;
    void setDisplayAspectRatio(float displayAspectRatio);

    // TODO: After we expose ColorSpace and ColorRange exposed.
    //ColorSpace colorSpace() const;
    //void setColorSpace(ColorSpace value);
    //ColorRange colorRange() const;
    //void setColorRange(ColorRange value);

    QImage toImage(QImage::Format fmt = QImage::Format_ARGB32, const QSize& dstSize = QSize(), const QRectF& roi = QRect()) const;
    VideoFrame to(VideoFormat::PixelFormat pixfmt, const QSize& dstSize = QSize(), const QRectF& roi = QRect()) const;
    VideoFrame to(const VideoFormat& fmt, const QSize& dstSize = QSize(), const QRectF& roi = QRect()) const;

    // TODO
    //bool to(VideoFormat::PixelFormat pixfmt, quint8 *const dst[], const int dstStride[], const QSize& dstSize = QSize(), const QRectF& roi = QRect()) const;
    //bool to(const VideoFormat& fmt, quint8 *const dst[], const int dstStride[], const QSize& dstSize = QSize(), const QRectF& roi = QRect()) const;

    //void* map(SurfaceType type, void* handle, int plane = 0);
    //void* map(SurfaceType type, void* handle, const VideoFormat& fmt, int plane = 0);

    void unmap(void* handle);

    //void* createInteropHandle(void* handle, SurfaceType type, int plane);
};


class VideoFrameConverter
{
public:
    VideoFrameConverter();
    ~VideoFrameConverter();
    void setEq(int brightness, int contrast, int saturation);
    VideoFrame convert(const VideoFrame& frame, const VideoFormat& fmt /Constrained/) const;
    VideoFrame convert(const VideoFrame& frame, VideoFormat::PixelFormat fmt /Constrained/) const;
    VideoFrame convert(const VideoFrame& frame, QImage::Format fmt /Constrained/) const;
    VideoFrame convert(const VideoFrame& frame, int fffmt /Constrained/) const;
};

};
