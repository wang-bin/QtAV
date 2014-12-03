/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2014 Wang Bin <wbsecg1@gmail.com>
    Miroslav Bendik <miroslav.bendik@gmail.com>

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

#include "QtAV/VideoDecoder.h"
#include "QtAV/private/VideoDecoder_p.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/prepost.h"
#include <libavcodec/avcodec.h>
extern "C" {
#include <libcedarv/libcedarv.h>
}
#include "utils/Logger.h"

// TODO: neon+nv12+opengl crash

#ifndef NO_NEON_OPT //Don't HAVE_NEON
extern "C" {
// from libvdpau-sunxi
void tiled_to_planar(void *src, void *dst, unsigned int dst_pitch,
                     unsigned int width, unsigned int height);

void tiled_deinterleave_to_planar(void *src, void *dst1, void *dst2,
                                  unsigned int dst_pitch,
                                  unsigned int width, unsigned int height);
}
#endif //NO_NEON_OPT
namespace QtAV {

class VideoDecoderCedarvPrivate;
class VideoDecoderCedarv : public VideoDecoder
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderCedarv)
#ifndef NO_NEON_OPT //Don't HAVE_NEON
    Q_PROPERTY(bool neon READ neon WRITE setNeon NOTIFY neonChanged)
#endif
    Q_PROPERTY(PixFmt outputPixelFormat READ outputPixelFormat WRITE setOutputPixelFormat NOTIFY outputPixelFormatChanged)
    Q_ENUMS(PixFmt)
public:
    enum PixFmt {
        YUV420p,
        NV12
    };
    VideoDecoderCedarv();
    virtual VideoDecoderId id() const;
    virtual QString description() const{
        return "Allwinner A10 CedarX video hardware acceleration";
    }
    bool prepare();
    bool decode(const QByteArray &encoded) Q_DECL_FINAL;
    bool decode(const Packet& packet) Q_DECL_FINAL;
    VideoFrame frame();

    //properties
    void setNeon(bool value);
    bool neon() const;
    void setOutputPixelFormat(PixFmt value);
    PixFmt outputPixelFormat() const;
Q_SIGNALS:
    void neonChanged();
    void outputPixelFormatChanged();
};

extern VideoDecoderId VideoDecoderId_Cedarv;
FACTORY_REGISTER_ID_AUTO(VideoDecoder, Cedarv, "Cedarv")

void RegisterVideoDecoderCedarv_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoDecoder, Cedarv, "Cedarv")
}

// source data is colum major. every block is 32x32
static void map32x32_to_yuv_Y(void* srcY, void* tarY, unsigned int dst_pitch, unsigned int coded_width, unsigned int coded_height)
{
    unsigned long offset;
    unsigned char *ptr = (unsigned char *)srcY;
    const unsigned int mb_width = (coded_width+15) >> 4;
    const unsigned int mb_height = (coded_height+15) >> 4;
    const unsigned int twomb_line = (mb_height+1) >> 1;
    const unsigned int recon_width = (mb_width+1) & 0xfffffffe;

    for (unsigned int i = 0; i < twomb_line; i++) {
        const unsigned int M = 32*i;
        for (unsigned int j = 0; j < recon_width; j+=2) {
            const unsigned int n = j*16;
            offset = M*dst_pitch + n;
            for (unsigned int l = 0; l < 32; l++) {
                if (M+l < coded_height) {
                    if (n+16 < coded_width) {
                        //1st & 2nd mb
                        memcpy((unsigned char *)tarY+offset, ptr, 32);
                    } else if (n<coded_width) {
                        // 1st mb
                        memcpy((unsigned char *)tarY+offset, ptr, 16);
                    }
                    offset += dst_pitch;
                }
                ptr += 32;
            }
        }
    }
}

static void map32x32_to_yuv_C(void* srcC, void* tarCb, void* tarCr, unsigned int dst_pitch, unsigned int coded_width, unsigned int coded_height)
{
    coded_width /= 2; // libvdpau-sunxi compatible
    unsigned char line[32];
    unsigned long offset;
    unsigned char *ptr = (unsigned char *)srcC;
    const unsigned int mb_width = (coded_width+7) >> 3;
    const unsigned int mb_height = (coded_height+7) >> 3;
    const unsigned int fourmb_line = (mb_height+3) >> 2;
    const unsigned int recon_width = (mb_width+1) & 0xfffffffe;

    for (unsigned int i = 0; i < fourmb_line; i++) {
        const int M = i*32;
        for (unsigned int j = 0; j < recon_width; j+=2) {
            const unsigned int n = j*8;
            offset = M*dst_pitch + n;
            for (unsigned int l = 0; l < 32; l++) {
                if (M+l < coded_height) {
                    if (n+8 < coded_width) {
                        // 1st & 2nd mb
                        memcpy(line, ptr, 32);
                        //unsigned char *line = ptr;
                        for (int k = 0; k < 16; k++) {
                            *((unsigned char *)tarCb + offset + k) = line[2*k];
                            *((unsigned char *)tarCr + offset + k) = line[2*k+1];
                        }
                    } else if (n < coded_width) {
                        // 1st mb
                        memcpy(line, ptr, 16);
                        //unsigned char *line = ptr;
                        for (int k = 0; k < 8; k++) {
                            *((unsigned char *)tarCb + offset + k) = line[2*k];
                            *((unsigned char *)tarCr + offset + k) = line[2*k+1];
                        }
                    }
                    offset += dst_pitch;
                }
                ptr += 32;
            }
        }
    }
}

#if 0
static void map32x32_to_nv12_UV(void* srcC, void* tarUV, unsigned int dst_pitch, unsigned int coded_width, unsigned int coded_height)
{
    coded_width /= 2; // libvdpau-sunxi compatible
    unsigned long offset;
    unsigned char *ptr = (unsigned char *)srcC;
    const unsigned int mb_width = (coded_width+7) >> 3;
    const unsigned int mb_height = (coded_height+7) >> 3;
    const unsigned int fourmb_line = (mb_height+3) >> 2;
    const unsigned int recon_width = (mb_width+1) & 0xfffffffe;
    for (unsigned int i = 0; i < fourmb_line; i++) {
        const int M = i*32;
        for (unsigned int j = 0; j < recon_width; j+=2) {
            const unsigned int n = j*16; // 1 logical pixel 2 channel
            offset = M*dst_pitch + n;
            for (unsigned int l = 0; l < 32; l++) { // copy 16 logical pixels every time. 16/2*recon_width ~ coded_width
                if (M+l < coded_height) {
                    if (n+8 < coded_width) {
                        // 1st & 2nd mb
                        memcpy((unsigned char *)tarUV + offset, ptr, 32);
                    } else if (n < coded_width) {
                        // 1st mb
                        memcpy((unsigned char *)tarUV + offset, ptr, 16);
                    }
                    offset += dst_pitch;
                }
                ptr += 32;
            }
        }
    }
}
#endif
#ifndef NO_NEON_OPT //Don't HAVE_NEON
// use tiled_to_planar instead
static void map32x32_to_yuv_Y_neon(void* srcY, void* tarY, unsigned int dst_pitch, unsigned int coded_width,unsigned int coded_height)
{
    unsigned long offset;
    unsigned char *dst_asm,*src_asm;

    unsigned char *ptr = (unsigned char *)srcY;
    const unsigned int mb_width = (coded_width+15) >> 4;
    const unsigned int mb_height = (coded_height+15) >> 4;
    const unsigned int twomb_line = (mb_height+1) >> 1;
    const unsigned int twomb_width = mb_width/2;
    for (unsigned int i = 0; i < twomb_line; i++) {
        const int M = i*32;
        for (unsigned int j = 0; j < twomb_width; j++) {
            const unsigned int n= j*32;
            offset = M*dst_pitch + n;
            for (unsigned int l=0;l<32;l++) {
                //first mb
                dst_asm = (unsigned char *)tarY + offset;
                src_asm = ptr;
                asm volatile (
                        "vld1.8         {d0 - d3}, [%[src_asm]]              \n\t"
                        "vst1.8         {d0 - d3}, [%[dst_asm]]              \n\t"
                        : [dst_asm] "+r" (dst_asm), [src_asm] "+r" (src_asm)
                        :  //[srcY] "r" (srcY)
                        : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
                        );
                offset += dst_pitch;
                ptr += 32;
            }
        }

        //LOGV("mb_width:%d",mb_width);
        if(mb_width & 1) {
            unsigned int j = mb_width-1;
            const unsigned int n = j*16;
            offset = M*dst_pitch + n;
            for (unsigned int l = 0; l < 32; l++) {
                //first mb
                if (M+l<coded_height && n<coded_width) {
                    dst_asm = (unsigned char *)tarY + offset;
                    src_asm = ptr;
                    asm volatile (
                            "vld1.8         {d0 - d1}, [%[src_asm]]              \n\t"
                            "vst1.8         {d0 - d1}, [%[dst_asm]]              \n\t"
                            : [dst_asm] "+r" (dst_asm), [src_asm] "+r" (src_asm)
                            :  //[srcY] "r" (srcY)
                            : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
                            );
                }
                offset += dst_pitch;
                ptr += 32;
            }
        }
    }
}
// use tiled_deinterleave_to_planar instead
static void map32x32_to_yuv_C_neon(void* srcC, void* tarCb, void* tarCr, unsigned int dst_pitch, unsigned int coded_width, unsigned int coded_height)
{
    coded_width /= 2; // libvdpau-sunxi compatible
    unsigned int j,l,k;
    unsigned long offset;
    unsigned char *dst0_asm,*dst1_asm,*src_asm;
    unsigned char line[16];
    int dst_stride = FFALIGN(coded_width, 16);

    unsigned char *ptr = (unsigned char *)srcC;
    const unsigned int mb_width = (coded_width+7)>>3;
    const unsigned int mb_height = (coded_height+7)>>3;
    const unsigned int fourmb_line = (mb_height+3)>>2;
    const unsigned int twomb_width = mb_width/2;

    for (unsigned int i = 0; i < fourmb_line; i++) {
        const int M = i*32;
        for (j = 0; j < twomb_width; j++) {
            const unsigned int n = j*16;
            offset = M*dst_stride + n;
            for (l = 0; l < 32; l++) {
                //first mb
                if (M+l<coded_height && n<coded_width) {
                    dst0_asm = (unsigned char *)tarCb + offset;
                    dst1_asm = (unsigned char *)tarCr + offset;
                    src_asm = ptr;
//                    for(k=0;k<16;k++)
//                    {
//                        dst0_asm[k] = src_asm[2*k];
//                        dst1_asm[k] = src_asm[2*k+1];
//                    }
                    asm volatile (
                            "vld1.8         {d0 - d3}, [%[src_asm]]              \n\t"
                            "vuzp.8         d0, d1              \n\t"
                            "vuzp.8         d2, d3              \n\t"
                            "vst1.8         {d0}, [%[dst0_asm]]!              \n\t"
                            "vst1.8         {d2}, [%[dst0_asm]]!              \n\t"
                            "vst1.8         {d1}, [%[dst1_asm]]!              \n\t"
                            "vst1.8         {d3}, [%[dst1_asm]]!              \n\t"
                             : [dst0_asm] "+r" (dst0_asm), [dst1_asm] "+r" (dst1_asm), [src_asm] "+r" (src_asm)
                             :  //[srcY] "r" (srcY)
                             : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
                             );
                }
                offset += dst_pitch;
                ptr += 32;
            }
        }
        if (mb_width & 1) {
            j = mb_width-1;
            for (l = 0; l < 32; l++) {
                const unsigned int n = j*8;
                offset = M*dst_stride + n;
                if (M+l<coded_height && n<coded_width) {
                    memcpy(line, ptr, 16);
                    for(k = 0; k < 8; k++) {
                        *((unsigned char *)tarCb + offset + k) = line[2*k];
                        *((unsigned char *)tarCr + offset + k) = line[2*k+1];
                    }
                }
                offset += dst_stride;
                ptr += 32;
            }
        }
    }
}
#endif

typedef struct {
    enum AVCodecID id;
    cedarv_stream_format_e format;
    cedarv_sub_format_e sub_format;
} avcodec_cedarv;

static const avcodec_cedarv avcodec_cedarv_map[] = {
    { QTAV_CODEC_ID(MPEG1VIDEO), CEDARV_STREAM_FORMAT_MPEG2, CEDARV_MPEG2_SUB_FORMAT_MPEG1 },
    { QTAV_CODEC_ID(MPEG2VIDEO), CEDARV_STREAM_FORMAT_MPEG2, CEDARV_MPEG2_SUB_FORMAT_MPEG2 },
    { QTAV_CODEC_ID(H263), CEDARV_STREAM_FORMAT_MPEG4, CEDARV_MPEG4_SUB_FORMAT_H263 },
    { QTAV_CODEC_ID(H264), CEDARV_STREAM_FORMAT_H264, CEDARV_SUB_FORMAT_UNKNOW }, //CEDARV_CONTAINER_FORMAT_TS
    { QTAV_CODEC_ID(VP6F), CEDARV_STREAM_FORMAT_MPEG4, CEDARV_MPEG4_SUB_FORMAT_VP6 },
    { QTAV_CODEC_ID(WMV1), CEDARV_STREAM_FORMAT_MPEG4, CEDARV_MPEG4_SUB_FORMAT_WMV1 },
    { QTAV_CODEC_ID(WMV2), CEDARV_STREAM_FORMAT_MPEG4, CEDARV_MPEG4_SUB_FORMAT_WMV2 },
    { QTAV_CODEC_ID(WMV3), CEDARV_STREAM_FORMAT_VC1, CEDARV_SUB_FORMAT_UNKNOW },
    { QTAV_CODEC_ID(VC1), CEDARV_STREAM_FORMAT_VC1, CEDARV_SUB_FORMAT_UNKNOW },
    { QTAV_CODEC_ID(MJPEG), CEDARV_STREAM_FORMAT_MJPEG, CEDARV_SUB_FORMAT_UNKNOW },
    { QTAV_CODEC_ID(VP8), CEDARV_STREAM_FORMAT_VP8, CEDARV_SUB_FORMAT_UNKNOW },
    { QTAV_CODEC_ID(MSMPEG4V1), CEDARV_STREAM_FORMAT_MPEG4, CEDARV_MPEG4_SUB_FORMAT_DIVX1 },
    { QTAV_CODEC_ID(MSMPEG4V2), CEDARV_STREAM_FORMAT_MPEG4, CEDARV_MPEG4_SUB_FORMAT_DIVX2 },
    { QTAV_CODEC_ID(MSMPEG4V3), CEDARV_STREAM_FORMAT_MPEG4, CEDARV_MPEG4_SUB_FORMAT_DIVX3 },
    { QTAV_CODEC_ID(FLV1), CEDARV_STREAM_FORMAT_MPEG4, CEDARV_MPEG4_SUB_FORMAT_SORENSSON_H263 },
    { QTAV_CODEC_ID(MPEG4), CEDARV_STREAM_FORMAT_MPEG4, CEDARV_MPEG4_SUB_FORMAT_XVID },
    { QTAV_CODEC_ID(RV10), CEDARV_STREAM_FORMAT_REALVIDEO, CEDARV_SUB_FORMAT_UNKNOW },
    { QTAV_CODEC_ID(RV20), CEDARV_STREAM_FORMAT_REALVIDEO, CEDARV_SUB_FORMAT_UNKNOW },
    { QTAV_CODEC_ID(RV30), CEDARV_STREAM_FORMAT_REALVIDEO, CEDARV_SUB_FORMAT_UNKNOW },
    { QTAV_CODEC_ID(RV40), CEDARV_STREAM_FORMAT_REALVIDEO, CEDARV_SUB_FORMAT_UNKNOW },
    { QTAV_CODEC_ID(NONE), CEDARV_STREAM_FORMAT_UNKNOW, CEDARV_SUB_FORMAT_UNKNOW }
};

typedef struct {
    enum AVPixelFormat fpf;
    cedarv_pixel_format_e cpf;
} ff_cedarv_pixfmt;

static const ff_cedarv_pixfmt pixel_format_map[] = {
    { QTAV_PIX_FMT_C(RGB565), CEDARV_PIXEL_FORMAT_RGB565 },
    { QTAV_PIX_FMT_C(RGB24), CEDARV_PIXEL_FORMAT_RGB888 },
    { QTAV_PIX_FMT_C(RGB32), CEDARV_PIXEL_FORMAT_ARGB8888 },
    { QTAV_PIX_FMT_C(YUV444P), CEDARV_PIXEL_FORMAT_YUV444 },
    { QTAV_PIX_FMT_C(YUV422P), CEDARV_PIXEL_FORMAT_YUV422 },
    { QTAV_PIX_FMT_C(YUV420P), CEDARV_PIXEL_FORMAT_YUV420 },
    { QTAV_PIX_FMT_C(YUV411P), CEDARV_PIXEL_FORMAT_YUV411 },
    //CEDARV_PIXEL_FORMAT_CSIRGB     = 0xf,
    { QTAV_PIX_FMT_C(NV12), CEDARV_PIXEL_FORMAT_AW_YUV420 },
    //{ QTAV_PIX_FMT_C(YUV422P), CEDARV_PIXEL_FORMAT_AW_YUV422 },
    //{ QTAV_PIX_FMT_C(YUV411P), CEDARV_PIXEL_FORMAT_AW_YUV411 },
};

void format_from_avcodec(enum AVCodecID id, cedarv_stream_format_e* format, cedarv_sub_format_e *sub_format)
{
    size_t i = 0;
    while (avcodec_cedarv_map[i].id != QTAV_CODEC_ID(NONE)) {
        if (avcodec_cedarv_map[i].id == id) {
            *format = avcodec_cedarv_map[i].format;
            *sub_format = avcodec_cedarv_map[i].sub_format;
            return;
        }
        ++i;
    }
    *format = CEDARV_STREAM_FORMAT_UNKNOW;
    *sub_format = CEDARV_SUB_FORMAT_UNKNOW;
}

enum AVPixelFormat pixel_format_from_cedarv(cedarv_pixel_format_e cpf)
{
    size_t i = 0;
    while (i < sizeof(pixel_format_map)/sizeof(pixel_format_map[0])) {
        if (pixel_format_map[i].cpf == cpf)
            return pixel_format_map[i].fpf;
        ++i;
    }
    return QTAV_PIX_FMT_C(NONE);
}

class VideoDecoderCedarvPrivate : public VideoDecoderPrivate
{
public:
    VideoDecoderCedarvPrivate()
        : VideoDecoderPrivate()
        , cedarv(0)
        , map_y(map32x32_to_yuv_Y)
        , map_c(map32x32_to_yuv_C)
        , pixfmt(VideoDecoderCedarv::NV12)
    {}
    ~VideoDecoderCedarvPrivate() {
        if (!cedarv)
            return;
        cedarv->ioctrl(cedarv, CEDARV_COMMAND_STOP, 0);
        // FIXME: why crash?
        //cedarv->close(cedarv);
        //libcedarv_exit(cedarv);
        cedarv = NULL;
    }

    CEDARV_DECODER *cedarv;
    cedarv_picture_t cedarPicture;
    typedef void (*map_y_t)(void* src, void* dst, unsigned int dst_pitch, unsigned int w, unsigned int h);
    map_y_t map_y;
    typedef void (*map_c_t)(void* src, void* dst1, void* dst2, unsigned int dst_pitch, unsigned int w, unsigned int h);
    map_c_t map_c;
    VideoDecoderCedarv::PixFmt pixfmt;
};

VideoDecoderCedarv::VideoDecoderCedarv()
    : VideoDecoder(*new VideoDecoderCedarvPrivate())
{
}

VideoDecoderId VideoDecoderCedarv::id() const
{
    return VideoDecoderId_Cedarv;
}

void VideoDecoderCedarv::setNeon(bool value)
{
    if (value == neon())
        return;
    DPTR_D(VideoDecoderCedarv);
    if (value) {
#ifndef NO_NEON_OPT //Don't HAVE_NEON
        d.map_y = tiled_to_planar;// map32x32_to_yuv_Y_neon;
        d.map_c = tiled_deinterleave_to_planar;// map32x32_to_yuv_C_neon;
#endif
    } else {
        d.map_y = map32x32_to_yuv_Y;
        d.map_c = map32x32_to_yuv_C;
    }
    emit neonChanged();
}

void VideoDecoderCedarv::setOutputPixelFormat(PixFmt value)
{
    if (outputPixelFormat() == value)
        return;
    d_func().pixfmt = value;
    emit outputPixelFormatChanged();
}

VideoDecoderCedarv::PixFmt VideoDecoderCedarv::outputPixelFormat() const
{
    return d_func().pixfmt;
}

bool VideoDecoderCedarv::neon() const
{
    return d_func().map_y != map32x32_to_yuv_Y;
}

bool VideoDecoderCedarv::prepare()
{
    DPTR_D(VideoDecoderCedarv);
    cedarv_stream_format_e format;
    cedarv_sub_format_e sub_format;
    /* format check */
    format_from_avcodec(d.codec_ctx->codec_id, &format, &sub_format);
    if (format == CEDARV_STREAM_FORMAT_UNKNOW) {
        qWarning("CedarV: codec not supported '%s'", avcodec_get_name(d.codec_ctx->codec_id));
        return false;
    }
    if (!d.cedarv) {
        int ret;
        d.cedarv = libcedarv_init(&ret);
        if (ret < 0 || d.cedarv == NULL)
            return false;
    }

    d.codec_ctx->opaque = &d; //is it ok?

    cedarv_stream_info_t cedarStreamInfo;
    memset(&cedarStreamInfo, 0, sizeof cedarStreamInfo);
    cedarStreamInfo.format = format;
    cedarStreamInfo.sub_format = sub_format;
    cedarStreamInfo.video_width = d.codec_ctx->coded_width; //coded_width?
    cedarStreamInfo.video_height = d.codec_ctx->coded_height;
    if (d.codec_ctx->extradata_size) {
        cedarStreamInfo.init_data = d.codec_ctx->extradata;
        cedarStreamInfo.init_data_len = d.codec_ctx->extradata_size;
    }

    int cedarvRet;
    cedarvRet = d.cedarv->set_vstream_info(d.cedarv, &cedarStreamInfo);
    if (cedarvRet < 0) {
        qWarning("CedarV: set_vstream_info error: %d", cedarvRet);
        return false;
    }
    cedarvRet = d.cedarv->open(d.cedarv);
    if (cedarvRet < 0) {
        qWarning("CedarV: set_vstream_info error: %d", cedarvRet);
        return false;
    }
    d.cedarv->ioctrl(d.cedarv, CEDARV_COMMAND_RESET, 0);
    d.cedarv->ioctrl(d.cedarv, CEDARV_COMMAND_PLAY, 0);

    return true;
}

bool VideoDecoderCedarv::decode(const QByteArray &encoded)
{
    DPTR_D(VideoDecoderCedarv);
    if (encoded.isEmpty())
        return true;
    //d.cedarv->ioctrl(d.cedarv, CEDARV_COMMAND_JUMP, 0);
    u32 bufsize0, bufsize1;
    u8 *buf0, *buf1;
    int ret = d.cedarv->request_write(d.cedarv, encoded.size(), &buf0, &bufsize0, &buf1, &bufsize1);
    if (ret < 0) {
        qWarning("CedarV: request_write failed (%d)", ret);
        return false;
    }
    memcpy(buf0, encoded.constData(), bufsize0);
    if ((u32)encoded.size() > bufsize0)
        memcpy(buf1, encoded.constData() + bufsize0, bufsize1);
    cedarv_stream_data_info_t stream_data_info;
    stream_data_info.type = 0;
    stream_data_info.lengh = encoded.size();
    stream_data_info.pts = 0; //packet.pts;
    stream_data_info.flags = CEDARV_FLAG_FIRST_PART | CEDARV_FLAG_LAST_PART | CEDARV_FLAG_PTS_VALID;
    if ((ret = d.cedarv->update_data(d.cedarv, &stream_data_info)) < 0) {
        qWarning("CedarV: update_data failed (%d)", ret);
        return false;
    }
    if ((ret = d.cedarv->decode(d.cedarv)) < 0) {
       qWarning("CedarV: decode failed (%d)", ret);
       return false;
    }
    ret = d.cedarv->display_request(d.cedarv, &d.cedarPicture);
    if (ret > 3 || ret < 0) {
       qWarning("CedarV: display_request failed (%d)", ret);
       if (d.cedarPicture.id) {
           d.cedarv->display_release(d.cedarv, d.cedarPicture.id);
           d.cedarPicture.id = 0;
       }
       return false;
    }
    return true;
}

bool VideoDecoderCedarv::decode(const Packet &packet)
{
    DPTR_D(VideoDecoderCedarv);
    if (packet.data.isEmpty())
        return true;
    //d.cedarv->ioctrl(d.cedarv, CEDARV_COMMAND_JUMP, 0);
    u32 bufsize0, bufsize1;
    u8 *buf0, *buf1;
    int ret = d.cedarv->request_write(d.cedarv, packet.data.size(), &buf0, &bufsize0, &buf1, &bufsize1);
    if (ret < 0) {
        qWarning("CedarV: request_write failed (%d)", ret);
        return false;
    }
    memcpy(buf0, packet.data.constData(), bufsize0);
    if ((u32)packet.data.size() > bufsize0)
        memcpy(buf1, packet.data.constData() + bufsize0, bufsize1);
    cedarv_stream_data_info_t stream_data_info;
    stream_data_info.type = 0; // TODO
    stream_data_info.lengh = packet.data.size();
    stream_data_info.pts = packet.pts * 1000.0;
    stream_data_info.flags = CEDARV_FLAG_FIRST_PART | CEDARV_FLAG_LAST_PART | CEDARV_FLAG_PTS_VALID;
    if ((ret = d.cedarv->update_data(d.cedarv, &stream_data_info)) < 0) {
        qWarning("CedarV: update_data failed (%d)", ret);
        return false;
    }
    if ((ret = d.cedarv->decode(d.cedarv)) < 0) {
       qWarning("CedarV: decode failed (%d)", ret);
       return false;
    }
    ret = d.cedarv->display_request(d.cedarv, &d.cedarPicture);
    if (ret > 3 || ret < 0) {
       qWarning("CedarV: display_request failed (%d)", ret);
       if (d.cedarPicture.id) {
           d.cedarv->display_release(d.cedarv, d.cedarPicture.id);
           d.cedarPicture.id = 0;
       }
       return false;
    }
    return true;
}


VideoFrame VideoDecoderCedarv::frame()
{
    DPTR_D(VideoDecoderCedarv);
    if (!d.cedarPicture.id)
        return VideoFrame();
    d.cedarPicture.display_height = FFALIGN(d.cedarPicture.display_height, 8);
    const int display_h_align = FFALIGN(d.cedarPicture.display_height, 2); // already aligned to 8!
    const int display_w_align = FFALIGN(d.cedarPicture.display_width, 16);
    const int dst_y_stride = display_w_align;
    const int dst_y_size = dst_y_stride * display_h_align;
    const int dst_c_stride = FFALIGN(d.cedarPicture.display_width/2, 16);
    const int dst_c_size = dst_c_stride * (display_h_align/2);
    const int alloc_size = dst_y_size + dst_c_size * 2;
    const unsigned int offset_y = 0;
    const unsigned int offset_u = offset_y + dst_y_size;
    const unsigned int offset_v = offset_u + dst_c_size;

    const bool nv12 = outputPixelFormat() == NV12;
    QByteArray buf(alloc_size, 0);
    buf.reserve(alloc_size);
    quint8 *dst = reinterpret_cast<quint8*>(buf.data());
    quint8 *plane[] = {
        dst + offset_y,
        dst + offset_u,
        dst + offset_v
    };
    int pitch[] = {
        dst_y_stride,
        dst_c_stride,
        dst_c_stride
    };
    if (nv12)
        pitch[1] = dst_y_stride;

    d.map_y(d.cedarPicture.y, plane[0], display_w_align, pitch[0], display_h_align);
    if (nv12)
        d.map_y(d.cedarPicture.u, plane[1], pitch[1], display_w_align, display_h_align/2);
    else
        d.map_c(d.cedarPicture.u, plane[1], plane[2], pitch[1], display_w_align, display_h_align/2); //vdpau use w, h/2

    const VideoFormat fmt(nv12 ? VideoFormat::Format_NV12 : VideoFormat::Format_YUV420P);
    VideoFrame frame(buf, display_w_align, display_h_align, fmt);
    frame.setBits(plane);
    frame.setBytesPerLine(pitch);
    // TODO: timestamp
    d.cedarv->display_release(d.cedarv, d.cedarPicture.id);
    d.cedarPicture.id = 0;
    return frame;
}

} // namespace QtAV
#include "VideoDecoderCedarv.moc"
