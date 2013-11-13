#include "VideoFormat.h"
#include <QtCore/QVector>

#include "QtAV/QtAV_Compat.h"

// FF_API_PIX_FMT
#ifdef PixelFormat
#undef PixelFormat
#endif

namespace QtAV {

class VideoFormatPrivate : public QSharedData
{
public:
    VideoFormatPrivate(VideoFormat::PixelFormat fmt)
        : pixfmt(fmt)
        , pixfmt_ff(AV_PIX_FMT_NONE)
        , qpixfmt(QImage::Format_Invalid)
        , planes(0)
        , bpps(4)
        , bpps_pad(4)
        , pixdesc(0)
    {
        if (fmt == VideoFormat::Format_Invalid) {
            pixfmt_ff = AV_PIX_FMT_NONE;
            qpixfmt = QImage::Format_Invalid;
            return;
        }
        init();
    }
    VideoFormatPrivate(AVPixelFormat fmt)
        : pixfmt(VideoFormat::Format_Invalid)
        , pixfmt_ff(fmt)
        , qpixfmt(QImage::Format_Invalid)
        , bpps(4)
        , bpps_pad(4)
        , pixdesc(0)
    {
        init();
    }
    VideoFormatPrivate(QImage::Format fmt)
        : pixfmt(VideoFormat::Format_Invalid)
        , pixfmt_ff(AV_PIX_FMT_NONE)
        , qpixfmt(fmt)
        , bpps(4)
        , bpps_pad(4)
        , pixdesc(0)
    {
        init();
    }
    void init() {
        if (pixfmt_ff != AV_PIX_FMT_NONE) {
            pixfmt = VideoFormat::pixelFormatFromFFmpeg(pixfmt_ff);
            qpixfmt = VideoFormat::imageFormatFromPixelFormat(pixfmt);
        } else if (qpixfmt != QImage::Format_Invalid){
            pixfmt = VideoFormat::pixelFormatFromImageFormat(qpixfmt);
            pixfmt_ff = (AVPixelFormat)VideoFormat::pixelFormatToFFmpeg((VideoFormat::PixelFormat)pixfmt);
        } else {
            pixfmt_ff = (AVPixelFormat)VideoFormat::pixelFormatToFFmpeg((VideoFormat::PixelFormat)pixfmt);
            qpixfmt = VideoFormat::imageFormatFromPixelFormat(pixfmt);
        }
        planes = qMax(av_pix_fmt_count_planes(pixfmt_ff), 0);
        bpps.resize(planes);
        bpps_pad.resize(planes);
        if (pixfmt == VideoFormat::Format_Invalid) {
            return;
        }
        pixdesc = const_cast<AVPixFmtDescriptor*>(av_pix_fmt_desc_get(pixfmt_ff));
        if (!pixdesc)
            return;
        initBpp();
    }
    QString name() const {
        return av_get_pix_fmt_name(pixfmt_ff);
    }
    int flags() const {
        if (!pixdesc)
            return 0;
        return pixdesc->flags;
    }

    VideoFormat::PixelFormat pixfmt;
    AVPixelFormat pixfmt_ff;
    QImage::Format qpixfmt;
    int planes;
    int bpp;
    int bpp_pad;
    QVector<int> bpps;
    QVector<int> bpps_pad; //TODO: is it needed?

private:
    AVPixFmtDescriptor *pixdesc;
    // from libavutil/pixdesc.c
    void initBpp() {
        bpp = 0;
        bpp_pad = 0;
        int log2_pixels = pixdesc->log2_chroma_w + pixdesc->log2_chroma_h;
        for (int c = 0; c < pixdesc->nb_components; c++) {
            const AVComponentDescriptor *comp = &pixdesc->comp[c];
            int s = c == 1 || c == 2 ? 0 : log2_pixels; //?
            bpps[comp->plane] = (comp->depth_minus1 + 1) << s;
            bpps_pad[comp->plane] = (comp->step_minus1 + 1) << s;
            if(!(pixdesc->flags & AV_PIX_FMT_FLAG_BITSTREAM))
                bpps_pad[comp->plane] *= 8;
            bpp += bpps[comp->plane];
            bpp_pad += bpps_pad[comp->plane];
            bpps[comp->plane] >>= s;
            bpps_pad[comp->plane] >>= s;
        }
        bpp >>= log2_pixels;
        bpp_pad >>= log2_pixels;
    }
};

// TODO: use FFmpeg macros to get right endian
static const struct {
    VideoFormat::PixelFormat fmt;
    AVPixelFormat ff; //int
} pixfmt_map[] = {
    { VideoFormat::Format_YUV420P, AV_PIX_FMT_YUV420P },   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    { VideoFormat::Format_YUYV, AV_PIX_FMT_YUYV422 }, //??   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    { VideoFormat::Format_RGB24, AV_PIX_FMT_RGB24 },     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    { VideoFormat::Format_BGR24, AV_PIX_FMT_BGR24 },     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
    //{ VideoFormat::Format_YUV422P, AV_PIX_FMT_YUV422P},   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    //{ VideoFormat::Format_YUV444, AV_PIX_FMT_YUV444P },   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    //AV_PIX_FMT_YUV410P,   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    //AV_PIX_FMT_YUV411P,   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    //AV_PIX_FMT_GRAY8,     ///<        Y        ,  8bpp
    //AV_PIX_FMT_MONOWHITE, ///<        Y        ,  1bpp, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb
    //AV_PIX_FMT_MONOBLACK, ///<        Y        ,  1bpp, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb
    //AV_PIX_FMT_PAL8,      ///< 8 bit with PIX_FMT_RGB32 palette
    { VideoFormat::Format_YUV420P, AV_PIX_FMT_YUVJ420P },  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV420P and setting color_range
    //AV_PIX_FMT_YUVJ422P,  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV422P and setting color_range
    //AV_PIX_FMT_YUVJ444P,  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV444P and setting color_range
    //AV_PIX_FMT_XVMC_MPEG2_MC,///< XVideo Motion Acceleration via common packet passing
    //AV_PIX_FMT_XVMC_MPEG2_IDCT,
    { VideoFormat::Format_UYVY, AV_PIX_FMT_UYVY422 },   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
    //AV_PIX_FMT_UYYVYY411, ///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
    //AV_PIX_FMT_BGR8,      ///< packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
    //AV_PIX_FMT_BGR4,      ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1B 2G 1R(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
    //AV_PIX_FMT_BGR4_BYTE, ///< packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
    //AV_PIX_FMT_RGB8,      ///< packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
    //AV_PIX_FMT_RGB4,      ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1R 2G 1B(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
    //AV_PIX_FMT_RGB4_BYTE, ///< packed RGB 1:2:1,  8bpp, (msb)1R 2G 1B(lsb)
    { VideoFormat::Format_NV12, AV_PIX_FMT_NV12 },      ///< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
    { VideoFormat::Format_NV21, AV_PIX_FMT_NV21 },      ///< as above, but U and V bytes are swapped
    { VideoFormat::Format_ARGB32, AV_PIX_FMT_ARGB },      ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
    { VideoFormat::Format_RGB32, AV_PIX_FMT_RGB32 }, //auto endian
    { VideoFormat::Format_RGB32, AV_PIX_FMT_RGBA },      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
    //AV_PIX_FMT_ABGR,      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
    { VideoFormat::Format_BGRA32, AV_PIX_FMT_BGRA },      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...

    //AV_PIX_FMT_GRAY16BE,  ///<        Y        , 16bpp, big-endian
    //AV_PIX_FMT_GRAY16LE,  ///<        Y        , 16bpp, little-endian
    //AV_PIX_FMT_YUV440P,   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
    //AV_PIX_FMT_YUVJ440P,  ///< planar YUV 4:4:0 full scale (JPEG), deprecated in favor of PIX_FMT_YUV440P and setting color_range
    //AV_PIX_FMT_YUVA420P,  ///< planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
    /*
    AV_PIX_FMT_VDPAU_H264,///< H.264 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    AV_PIX_FMT_VDPAU_MPEG1,///< MPEG-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    AV_PIX_FMT_VDPAU_MPEG2,///< MPEG-2 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    AV_PIX_FMT_VDPAU_WMV3,///< WMV3 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    AV_PIX_FMT_VDPAU_VC1, ///< VC-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    */
    //AV_PIX_FMT_RGB48BE,   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as big-endian
    //AV_PIX_FMT_RGB48LE,   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as little-endian

    { VideoFormat::Format_RGB565, AV_PIX_FMT_RGB565BE },  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), big-endian
    { VideoFormat::Format_RGB565, AV_PIX_FMT_RGB565LE },  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), little-endian
    { VideoFormat::Format_RGB555, AV_PIX_FMT_RGB555BE },  ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian, most significant bit to 0
    { VideoFormat::Format_RGB555, AV_PIX_FMT_RGB555LE },  ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian, most significant bit to 0

    { VideoFormat::Format_BGR565, AV_PIX_FMT_BGR565BE },  ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), big-endian
    { VideoFormat::Format_BGR565, AV_PIX_FMT_BGR565LE },  ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), little-endian
    { VideoFormat::Format_BGR555, AV_PIX_FMT_BGR555BE },  ///< packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian, most significant bit to 1
    { VideoFormat::Format_BGR555, AV_PIX_FMT_BGR555LE },  ///< packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian, most significant bit to 1
/*
    AV_PIX_FMT_VAAPI_MOCO, ///< HW acceleration through VA API at motion compensation entry-point, Picture.data[3] contains a vaapi_render_state struct which contains macroblocks as well as various fields extracted from headers
    AV_PIX_FMT_VAAPI_IDCT, ///< HW acceleration through VA API at IDCT entry-point, Picture.data[3] contains a vaapi_render_state struct which contains fields extracted from headers
    AV_PIX_FMT_VAAPI_VLD,  ///< HW decoding through VA API, Picture.data[3] contains a vaapi_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers

    AV_PIX_FMT_YUV420P16LE,  ///< planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
    AV_PIX_FMT_YUV420P16BE,  ///< planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
    AV_PIX_FMT_YUV422P16LE,  ///< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
    AV_PIX_FMT_YUV422P16BE,  ///< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
    AV_PIX_FMT_YUV444P16LE,  ///< planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
    AV_PIX_FMT_YUV444P16BE,  ///< planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
    AV_PIX_FMT_VDPAU_MPEG4,  ///< MPEG4 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    AV_PIX_FMT_DXVA2_VLD,    ///< HW decoding through DXVA2, Picture.data[3] contains a LPDIRECT3DSURFACE9 pointer

    AV_PIX_FMT_RGB444LE,  ///< packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian, most significant bits to 0
    AV_PIX_FMT_RGB444BE,  ///< packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian, most significant bits to 0
    AV_PIX_FMT_BGR444LE,  ///< packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian, most significant bits to 1
    AV_PIX_FMT_BGR444BE,  ///< packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian, most significant bits to 1
    AV_PIX_FMT_GRAY8A,    ///< 8bit gray, 8bit alpha
    AV_PIX_FMT_BGR48BE,   ///< packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as big-endian
    AV_PIX_FMT_BGR48LE,   ///< packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as little-endian

    //the following 10 formats have the disadvantage of needing 1 format for each bit depth, thus
    //If you want to support multiple bit depths, then using AV_PIX_FMT_YUV420P16* with the bpp stored separately
    //is better
    AV_PIX_FMT_YUV420P9BE, ///< planar YUV 4:2:0, 13.5bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
    AV_PIX_FMT_YUV420P9LE, ///< planar YUV 4:2:0, 13.5bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
    AV_PIX_FMT_YUV420P10BE,///< planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
    AV_PIX_FMT_YUV420P10LE,///< planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
    AV_PIX_FMT_YUV422P10BE,///< planar YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
    AV_PIX_FMT_YUV422P10LE,///< planar YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
    AV_PIX_FMT_YUV444P9BE, ///< planar YUV 4:4:4, 27bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
    AV_PIX_FMT_YUV444P9LE, ///< planar YUV 4:4:4, 27bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
    AV_PIX_FMT_YUV444P10BE,///< planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
    AV_PIX_FMT_YUV444P10LE,///< planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
    AV_PIX_FMT_YUV422P9BE, ///< planar YUV 4:2:2, 18bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
    AV_PIX_FMT_YUV422P9LE, ///< planar YUV 4:2:2, 18bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
    AV_PIX_FMT_VDA_VLD,    ///< hardware decoding through VDA

#ifdef AV_PIX_FMT_ABI_GIT_MASTER
    AV_PIX_FMT_RGBA64BE,  ///< packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian
    AV_PIX_FMT_RGBA64LE,  ///< packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian
    AV_PIX_FMT_BGRA64BE,  ///< packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian
    AV_PIX_FMT_BGRA64LE,  ///< packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian
#endif
    AV_PIX_FMT_GBRP,      ///< planar GBR 4:4:4 24bpp
    AV_PIX_FMT_GBRP9BE,   ///< planar GBR 4:4:4 27bpp, big-endian
    AV_PIX_FMT_GBRP9LE,   ///< planar GBR 4:4:4 27bpp, little-endian
    AV_PIX_FMT_GBRP10BE,  ///< planar GBR 4:4:4 30bpp, big-endian
    AV_PIX_FMT_GBRP10LE,  ///< planar GBR 4:4:4 30bpp, little-endian
    AV_PIX_FMT_GBRP16BE,  ///< planar GBR 4:4:4 48bpp, big-endian
    AV_PIX_FMT_GBRP16LE,  ///< planar GBR 4:4:4 48bpp, little-endian
*/
    /**
     * duplicated pixel formats for compatibility with libav.
     * FFmpeg supports these formats since May 8 2012 and Jan 28 2012 (commits f9ca1ac7 and 143a5c55)
     * Libav added them Oct 12 2012 with incompatible values (commit 6d5600e85)
     */
/*
    AV_PIX_FMT_YUVA422P_LIBAV,  ///< planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
    AV_PIX_FMT_YUVA444P_LIBAV,  ///< planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)

    AV_PIX_FMT_YUVA420P9BE,  ///< planar YUV 4:2:0 22.5bpp, (1 Cr & Cb sample per 2x2 Y & A samples), big-endian
    AV_PIX_FMT_YUVA420P9LE,  ///< planar YUV 4:2:0 22.5bpp, (1 Cr & Cb sample per 2x2 Y & A samples), little-endian
    AV_PIX_FMT_YUVA422P9BE,  ///< planar YUV 4:2:2 27bpp, (1 Cr & Cb sample per 2x1 Y & A samples), big-endian
    AV_PIX_FMT_YUVA422P9LE,  ///< planar YUV 4:2:2 27bpp, (1 Cr & Cb sample per 2x1 Y & A samples), little-endian
    AV_PIX_FMT_YUVA444P9BE,  ///< planar YUV 4:4:4 36bpp, (1 Cr & Cb sample per 1x1 Y & A samples), big-endian
    AV_PIX_FMT_YUVA444P9LE,  ///< planar YUV 4:4:4 36bpp, (1 Cr & Cb sample per 1x1 Y & A samples), little-endian
    // Y: 2bytes/pix U: 1, V: 1
    AV_PIX_FMT_YUVA420P10BE, ///< planar YUV 4:2:0 25bpp, (1 Cr & Cb sample per 2x2 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA420P10LE, ///< planar YUV 4:2:0 25bpp, (1 Cr & Cb sample per 2x2 Y & A samples, little-endian)
    AV_PIX_FMT_YUVA422P10BE, ///< planar YUV 4:2:2 30bpp, (1 Cr & Cb sample per 2x1 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA422P10LE, ///< planar YUV 4:2:2 30bpp, (1 Cr & Cb sample per 2x1 Y & A samples, little-endian)
    AV_PIX_FMT_YUVA444P10BE, ///< planar YUV 4:4:4 40bpp, (1 Cr & Cb sample per 1x1 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA444P10LE, ///< planar YUV 4:4:4 40bpp, (1 Cr & Cb sample per 1x1 Y & A samples, little-endian)
    AV_PIX_FMT_YUVA420P16BE, ///< planar YUV 4:2:0 40bpp, (1 Cr & Cb sample per 2x2 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA420P16LE, ///< planar YUV 4:2:0 40bpp, (1 Cr & Cb sample per 2x2 Y & A samples, little-endian)
    AV_PIX_FMT_YUVA422P16BE, ///< planar YUV 4:2:2 48bpp, (1 Cr & Cb sample per 2x1 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA422P16LE, ///< planar YUV 4:2:2 48bpp, (1 Cr & Cb sample per 2x1 Y & A samples, little-endian)
    AV_PIX_FMT_YUVA444P16BE, ///< planar YUV 4:4:4 64bpp, (1 Cr & Cb sample per 1x1 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA444P16LE, ///< planar YUV 4:4:4 64bpp, (1 Cr & Cb sample per 1x1 Y & A samples, little-endian)
*/
    //AV_PIX_FMT_VDPAU,     ///< HW acceleration through VDPAU, Picture.data[3] contains a VdpVideoSurface
/*
#ifndef AV_PIX_FMT_ABI_GIT_MASTER
    AV_PIX_FMT_RGBA64BE=0x123,  ///< packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian
    AV_PIX_FMT_RGBA64LE,  ///< packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian
    AV_PIX_FMT_BGRA64BE,  ///< packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian
    AV_PIX_FMT_BGRA64LE,  ///< packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian
#endif
    AV_PIX_FMT_0RGB=0x123+4,      ///< packed RGB 8:8:8, 32bpp, 0RGB0RGB...
    AV_PIX_FMT_RGB0,      ///< packed RGB 8:8:8, 32bpp, RGB0RGB0...
    AV_PIX_FMT_0BGR,      ///< packed BGR 8:8:8, 32bpp, 0BGR0BGR...
    AV_PIX_FMT_BGR0,      ///< packed BGR 8:8:8, 32bpp, BGR0BGR0...
    AV_PIX_FMT_YUVA444P,  ///< planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
    AV_PIX_FMT_YUVA422P,  ///< planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)

    AV_PIX_FMT_YUV420P12BE, ///< planar YUV 4:2:0,18bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
    AV_PIX_FMT_YUV420P12LE, ///< planar YUV 4:2:0,18bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
    AV_PIX_FMT_YUV420P14BE, ///< planar YUV 4:2:0,21bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
    AV_PIX_FMT_YUV420P14LE, ///< planar YUV 4:2:0,21bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
    AV_PIX_FMT_YUV422P12BE, ///< planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
    AV_PIX_FMT_YUV422P12LE, ///< planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
    AV_PIX_FMT_YUV422P14BE, ///< planar YUV 4:2:2,28bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
    AV_PIX_FMT_YUV422P14LE, ///< planar YUV 4:2:2,28bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
    AV_PIX_FMT_YUV444P12BE, ///< planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
    AV_PIX_FMT_YUV444P12LE, ///< planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
    AV_PIX_FMT_YUV444P14BE, ///< planar YUV 4:4:4,42bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
    AV_PIX_FMT_YUV444P14LE, ///< planar YUV 4:4:4,42bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
    AV_PIX_FMT_GBRP12BE,    ///< planar GBR 4:4:4 36bpp, big-endian
    AV_PIX_FMT_GBRP12LE,    ///< planar GBR 4:4:4 36bpp, little-endian
    AV_PIX_FMT_GBRP14BE,    ///< planar GBR 4:4:4 42bpp, big-endian
    AV_PIX_FMT_GBRP14LE,    ///< planar GBR 4:4:4 42bpp, little-endian
*/
    { VideoFormat::Format_Invalid, AV_PIX_FMT_NONE },
};

VideoFormat::PixelFormat VideoFormat::pixelFormatFromFFmpeg(int ff)
{
    for (int i = 0; i < sizeof(pixfmt_map)/sizeof(pixfmt_map[0]); ++i) {
        if (pixfmt_map[i].ff == ff)
            return pixfmt_map[i].fmt;
    }
    return VideoFormat::Format_Invalid;
}

int VideoFormat::pixelFormatToFFmpeg(VideoFormat::PixelFormat fmt)
{
    for (int i = 0; i < sizeof(pixfmt_map)/sizeof(pixfmt_map[0]); ++i) {
        if (pixfmt_map[i].fmt == fmt)
            return pixfmt_map[i].ff;
    }
    return AV_PIX_FMT_NONE;
}

/*!
    Returns a video pixel format equivalent to an image \a format.  If there is no equivalent
    format VideoFormat::InvalidType is returned instead.

    \note In general \l QImage does not handle YUV formats.

*/
static const struct {
    VideoFormat::PixelFormat fmt;
    QImage::Format qfmt;
} qpixfmt_map[] = {
    { VideoFormat::Format_RGB32,QImage::Format_RGB32 },
    { VideoFormat::Format_ARGB32, QImage::Format_ARGB32 },
    { VideoFormat::Format_ARGB32_Premultiplied, QImage::Format_ARGB32_Premultiplied },
    { VideoFormat::Format_RGB565, QImage::Format_RGB16 },
    { VideoFormat::Format_ARGB8565_Premultiplied, QImage::Format_ARGB8565_Premultiplied },
    { VideoFormat::Format_RGB555, QImage::Format_RGB555 },
    { VideoFormat::Format_RGB24, QImage::Format_RGB888 },
    { VideoFormat::Format_Invalid, QImage::Format_Invalid }
};

VideoFormat::PixelFormat VideoFormat::pixelFormatFromImageFormat(QImage::Format format)
{
    for (int i = 0; qpixfmt_map[i].fmt != Format_Invalid; ++i) {
        if (qpixfmt_map[i].qfmt == format)
            return qpixfmt_map[i].fmt;
    }
    return Format_Invalid;
}

QImage::Format VideoFormat::imageFormatFromPixelFormat(PixelFormat format)
{
    for (int i = 0; qpixfmt_map[i].fmt != Format_Invalid; ++i) {
        if (qpixfmt_map[i].fmt == format)
            return qpixfmt_map[i].qfmt;
    }
    return QImage::Format_Invalid;
}


VideoFormat::VideoFormat(PixelFormat format)
    :d(new VideoFormatPrivate(format))
{
}

VideoFormat::VideoFormat(int formatFF)
    :d(new VideoFormatPrivate((AVPixelFormat)formatFF))
{
}

VideoFormat::VideoFormat(QImage::Format fmt)
    :d(new VideoFormatPrivate(fmt))
{
}

VideoFormat::VideoFormat(const QString &name)
    :d(new VideoFormatPrivate(av_get_pix_fmt(name.toUtf8().constData())))
{
}

VideoFormat::VideoFormat(const VideoFormat &other)
    :d(other.d)
{
}

VideoFormat::~VideoFormat()
{

}


/*!
    Assigns \a other to this VideoFormat implementation.
*/
VideoFormat& VideoFormat::operator=(const VideoFormat &other)
{
    d = other.d;
    return *this;
}

VideoFormat& VideoFormat::operator =(VideoFormat::PixelFormat fmt)
{
    d->pixfmt = fmt;
    d->init();
    return *this;
}

VideoFormat& VideoFormat::operator =(QImage::Format qpixfmt)
{
    d->qpixfmt = qpixfmt;
    d->init();
    return *this;
}

VideoFormat& VideoFormat::operator =(int fffmt)
{
    d->pixfmt_ff = (AVPixelFormat)fffmt;
    d->init();
    return *this;
}

bool VideoFormat::operator==(const VideoFormat &other) const
{
    return d->pixfmt_ff == other.d->pixfmt_ff;
}

bool VideoFormat::operator==(VideoFormat::PixelFormat fmt) const
{
    return d->pixfmt == fmt;
}

bool VideoFormat::operator==(QImage::Format qpixfmt) const
{
    return d->qpixfmt == qpixfmt;
}

bool VideoFormat::operator==(int fffmt) const
{
    return d->pixfmt_ff == fffmt;
}

bool VideoFormat::operator!=(const VideoFormat& other) const
{
    return !(*this == other);
}

bool VideoFormat::operator!=(VideoFormat::PixelFormat fmt) const
{
    return d->pixfmt != fmt;
}

bool VideoFormat::operator!=(QImage::Format qpixfmt) const
{
    return d->qpixfmt != qpixfmt;
}

bool VideoFormat::operator!=(int fffmt) const
{
    return d->pixfmt_ff != fffmt;
}

bool VideoFormat::isValid() const
{
    return d->pixfmt_ff != AV_PIX_FMT_NONE || d->pixfmt != Format_Invalid;
}

VideoFormat::PixelFormat VideoFormat::pixelFormat() const
{
    return d->pixfmt;
}

int VideoFormat::pixelFormatFFmpeg() const
{
    return d->pixfmt_ff;
}

QImage::Format VideoFormat::imageFormat() const
{
    return d->qpixfmt;
}

QString VideoFormat::name() const
{
    return d->name();
}

void VideoFormat::setPixelFormat(PixelFormat format)
{
    d->pixfmt = format;
    d->init();
}

void VideoFormat::setPixelFormatFFmpeg(int format)
{
    d->pixfmt_ff = (AVPixelFormat)format;
    d->init();
}

int VideoFormat::planeCount() const
{
    return d->planes;
}

int VideoFormat::bitsPerPixel() const
{
    return d->bpp;
}

int VideoFormat::bitsPerPixelPadded() const
{
    return d->bpp_pad;
}

int VideoFormat::bitsPerPixel(int plane) const
{
    //must be a valid index position in the vector
    if (plane >= d->bpps.size())
        return 0;
    return d->bpps[plane];
}

int VideoFormat::bytesPerPixel() const
{
    return (bitsPerPixel() + 7) >> 3;
}

int VideoFormat::bytesPerPixel(int plane) const
{
    return (bitsPerPixel(plane) + 7) >> 3;
}

// test AV_PIX_FMT_FLAG_XXX
bool VideoFormat::isBigEndian() const
{
    return (d->flags() & AV_PIX_FMT_FLAG_BE) == AV_PIX_FMT_FLAG_BE;
}

bool VideoFormat::hasPalette() const
{
    return (d->flags() & AV_PIX_FMT_FLAG_PAL) == AV_PIX_FMT_FLAG_PAL;
}

bool VideoFormat::isPseudoPaletted() const
{
    return (d->flags() & AV_PIX_FMT_FLAG_PSEUDOPAL) == AV_PIX_FMT_FLAG_PSEUDOPAL;
}

bool VideoFormat::isBitStream() const
{
    return (d->flags() & AV_PIX_FMT_FLAG_BITSTREAM) == AV_PIX_FMT_FLAG_BITSTREAM;
}

bool VideoFormat::isHWAccelerated() const
{
    return (d->flags() & AV_PIX_FMT_FLAG_HWACCEL) == AV_PIX_FMT_FLAG_HWACCEL;
}

bool VideoFormat::isPlanar() const
{
    return (d->flags() & AV_PIX_FMT_FLAG_PLANAR) == AV_PIX_FMT_FLAG_PLANAR;
}

bool VideoFormat::isRGB() const
{
    return (d->flags() & AV_PIX_FMT_FLAG_RGB) == AV_PIX_FMT_FLAG_RGB;
}

bool VideoFormat::hasAlpha() const
{
    return (d->flags() & AV_PIX_FMT_FLAG_ALPHA) == AV_PIX_FMT_FLAG_ALPHA;
}

} //namespace QtAV
