namespace QtAV
{

class VideoFormat
{
%TypeHeaderCode
#include <QtAV/VideoFormat.h>
%End

public:
    enum PixelFormat {
        Format_Invalid = -1,
        Format_ARGB32, // AARRGGBB or 00RRGGBB, check hasAlpha is required
        Format_BGRA32, //BBGGRRAA
        Format_ABGR32, // QImage.RGBA8888 le
        Format_RGBA32, // QImage. no
        Format_RGB32, // 0xAARRGGBB native endian. same as QImage::Format_ARGB32. be: ARGB32, le: BGRA32
        Format_BGR32, // 0xAABBGGRR native endian
        Format_RGB24,
        Format_BGR24,
        Format_RGB565,
        Format_BGR565,
        Format_RGB555,
        Format_BGR555,

        //http://www.fourcc.org/yuv.php
        Format_AYUV444,
        Format_YUV444P,
        Format_YUV422P,
        Format_YUV420P,
        Format_YUV411P,
        Format_YUV410P,
        Format_YV12,
        Format_UYVY, //422
        Format_VYUY, //not in ffmpeg. OMX_COLOR_FormatCrYCbY
        Format_YUYV, //422, aka yuy2
        Format_YVYU, //422
        Format_NV12,
        Format_NV21,
        Format_IMC1,
        Format_IMC2,
        Format_IMC3, //same as IMC1, swap U V
        Format_IMC4, //same as IMC2, swap U V
        Format_Y8, //GREY. single 8 bit Y plane
        Format_Y16, //single 16 bit Y plane. LE

        Format_Jpeg, //yuvj

        //Format_CameraRaw,
        //Format_AdobeDng,

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

        Format_RGB48, // native endian
        Format_RGB48LE,
        Format_RGB48BE,
        Format_BGR48,
        Format_BGR48LE,
        Format_BGR48BE,
        Format_RGBA64, //native endian
        Format_RGBA64LE,
        Format_RGBA64BE,
        Format_BGRA64, //native endian
        Format_BGRA64LE,
        Format_BGRA64BE,

        Format_VYU, // for rgb422_apple texture, the layout is like rgb24: (v, y, u, )
        Format_XYZ12,
        Format_XYZ12LE,
        Format_XYZ12BE,
        Format_User
    };

    static PixelFormat pixelFormatFromImageFormat(QImage::Format format);
    static QImage::Format imageFormatFromPixelFormat(PixelFormat format);
    static PixelFormat pixelFormatFromFFmpeg(int ff); //AVPixelFormat
    static int pixelFormatToFFmpeg(PixelFormat fmt);
    static QVector<int> pixelFormatsFFmpeg();

    VideoFormat(PixelFormat format = Format_Invalid);
    VideoFormat(int formatFF /Constrained/);
    VideoFormat(QImage::Format fmt /Constrained/);
    VideoFormat(const QString& name);
    VideoFormat(const VideoFormat &other /Constrained/);
    ~VideoFormat();

    // NOTE: We don't expose the assignment operators.
    //VideoFormat& operator=(const VideoFormat &other);
    //VideoFormat& operator=(VideoFormat::PixelFormat pixfmt);
    //VideoFormat& operator=(QImage::Format qpixfmt);
    //VideoFormat& operator=(int ffpixfmt);
    bool operator==(const VideoFormat &other) const;
    bool operator==(VideoFormat::PixelFormat pixfmt /Constrained/) const;
    bool operator==(QImage::Format qpixfmt /Constrained/) const;
    bool operator==(int ffpixfmt /Constrained/) const;
    bool operator!=(const VideoFormat &other) const;
    bool operator!=(VideoFormat::PixelFormat pixfmt /Constrained/) const;
    bool operator!=(QImage::Format qpixfmt /Constrained/) const;
    bool operator!=(int ffpixfmt /Constrained/) const;

    bool isValid() const;

    PixelFormat pixelFormat() const;
    int pixelFormatFFmpeg() const;
    QImage::Format imageFormat() const;
    QString name() const;

    void setPixelFormat(PixelFormat format);
    void setPixelFormatFFmpeg(int format);

    int channels() const;
    int channels(int plane) const;
    int planeCount() const;
    int bitsPerPixel() const;
    /// nv12: 16 for uv plane
    int bitsPerPixel(int plane) const;
    /// bgr24 is 24 not 32
    int bitsPerPixelPadded() const;
    int bytesPerPixel() const;
    int bytesPerPixel(int plane) const;
    int bitsPerComponent() const;

    int bytesPerLine(int width, int plane) const;
    int chromaWidth(int lumaWidth) const;
    int chromaHeight(int lumaHeight) const;
    int width(int lumaWidth, int plane) const;
    int height(int lumaHeight, int plane) const;
    qreal normalizedWidth(int plane) const;
    qreal normalizedHeight(int plane) const;
    bool isBigEndian() const;
    bool hasPalette() const;
    bool isPseudoPaletted() const;
    bool isBitStream() const;
    bool isHWAccelerated() const;
    bool isPlanar() const;
    bool isRGB() const;
    bool isXYZ() const;
    bool hasAlpha() const;

    static bool isPlanar(PixelFormat pixfmt);
    static bool isRGB(PixelFormat pixfmt);
    static bool hasAlpha(PixelFormat pixfmt);
};

//QDebug operator<<(QDebug debug, const VideoFormat &fmt);
//QDebug operator<<(QDebug debug, VideoFormat::PixelFormat pixFmt);

};
