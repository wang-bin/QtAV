/******************************************************************************
	QtAV:  Media play library based on Qt and FFmpeg
	solve the version problem and diffirent api in FFmpeg and libav
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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
#ifndef QTAV_COMPAT_H
#define QTAV_COMPAT_H

/*!
  NOTE: include this at last
  TODO: runtime symble check use dllapi project? how ffmpeg version defined?
 */
#include "QtAV_Global.h"
#ifdef __cplusplus
extern "C"
{
/*UINT64_C: C99 math features, need -D__STDC_CONSTANT_MACROS in CXXFLAGS*/
#endif /*__cplusplus*/
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/log.h>
#include <libavutil/mathematics.h> //AV_ROUND_UP, av_rescale_rnd for libav
#include <libavutil/cpu.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

/* TODO: how to check whether we have swresample or not? how to check avresample?*/
#include <libavutil/samplefmt.h>
#if QTAV_HAVE(SWRESAMPLE)
#include <libswresample/swresample.h>
#ifndef LIBSWRESAMPLE_VERSION_INT //ffmpeg 0.9, swr 0.5
#define LIBSWRESAMPLE_VERSION_INT AV_VERSION_INT(LIBSWRESAMPLE_VERSION_MAJOR, LIBSWRESAMPLE_VERSION_MINOR, LIBSWRESAMPLE_VERSION_MICRO)
#endif //LIBSWRESAMPLE_VERSION_INT
//ffmpeg >= 0.11.x. swr0.6.100: ffmpeg-0.10.x
#define HAVE_SWR_GET_DELAY (LIBSWRESAMPLE_VERSION_INT > AV_VERSION_INT(0, 6, 100))
#endif //QTAV_HAVE(SWRESAMPLE)
#if QTAV_HAVE(AVRESAMPLE)
#include <libavresample/avresample.h>
#endif //QTAV_HAVE(AVRESAMPLE)

#if QTAV_HAVE(AVFILTER)
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#endif //QTAV_HAVE(AVFILTER)

#ifdef __cplusplus
}
#endif /*__cplusplus*/

/*!
 * Guide to uniform the api for different FFmpeg version(or other libraries)
 * We use the existing old api to simulater .
 * 1. The old version does not have this api: Just add it.
 * 2. The old version has similar api: Try using macro.
 * e.g. the old is bool my_play(char* data, size_t size)
 *      the new is bool my_play2(const ByteArray& data)
 * change:
 *    #define my_play2(data) my_play(data.data(), data.size());
 *
 * 3. The old version api is conflicted with the latest's. We can redefine the api
 * e.g. the old is bool my_play(char* data, size_t size)
 *      the new is bool my_play(const ByteArray& data)
 * change:
 *    typedef bool (*my_play_t)(const ByteArray&);
 *    static my_play_t my_play_ptr = my_play; //using the existing my_play(char*, size_t)
 *    #define my_play my_play_compat
 *    inline bool my_play_compat(const ByteArray& data)
 *    {
 *        return my_play_ptr(data.data(), data.size());
 *    }
 * 4. conflict macros
 * see av_err2str
 */

#ifndef AV_VERSION_INT
#define AV_VERSION_INT(a, b, c) (a<<16 | b<<8 | c)
#endif /*AV_VERSION_INT*/

void ffmpeg_version_print();

/*TODO: libav
avutil: error.h
*/
#if defined(Q_CC_MSVC) || !defined(av_err2str) || (GCC_VERSION_AT_LEAST(4, 7, 0) && __cplusplus)
#ifdef av_err2str
#undef av_err2str
/*#define av_make_error_string qtav_make_error_string*/
#else
/**
 * Fill the provided buffer with a string containing an error string
 * corresponding to the AVERROR code errnum.
 *
 * @param errbuf         a buffer
 * @param errbuf_size    size in bytes of errbuf
 * @param errnum         error code to describe
 * @return the buffer in input, filled with the error description
 * @see av_strerror()
 */
static av_always_inline char *av_make_error_string(char *errbuf, size_t errbuf_size, int errnum)
{
	av_strerror(errnum, errbuf, errbuf_size);
	return errbuf;
}
#endif /*av_err2str*/

#define AV_ERROR_MAX_STRING_SIZE 64
av_always_inline char* av_err2str(int errnum)
{
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

/**
 * Convenience macro, the return value should be used only directly in
 * function arguments but never stand-alone.
 */
/*GCC: taking address of temporary array*/
/*
#define av_err2str(errnum) \
    av_make_error_string((char[AV_ERROR_MAX_STRING_SIZE]){0}, AV_ERROR_MAX_STRING_SIZE, errnum)
*/
#endif /*!defined(av_err2str) || GCC_VERSION_AT_LEAST(4, 7, 2)*/

#if (LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(52,23,0))
#define avcodec_decode_audio3(avctx, samples, frame_size_ptr, avpkt) \
    avcodec_decode_audio2(avctx, samples, frame_size_ptr, (*avpkt).data, (*avpkt).size);

#endif /*AV_VERSION_INT(52,23,0)*/

#if (LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(52,101,0))
#define av_dump_format(...) dump_format(__VA_ARGS__)
#endif /*AV_VERSION_INT(52,101,0)*/

#if QTAV_HAVE(SWRESAMPLE) && (LIBSWRESAMPLE_VERSION_INT <= AV_VERSION_INT(0, 5, 0))
#define swresample_version() LIBSWRESAMPLE_VERSION_INT //we can not know the runtime version, so just use build time version
#define swresample_configuration() "Not available."
#define swresample_license() "Not available."
#endif

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51, 32, 0)
int64_t av_get_default_channel_layout(int nb_channels);
#endif
/*
 * mapping avresample to swresample
 * https://github.com/xbmc/xbmc/commit/274679d
 */
#if (QTAV_HAVE(SWR_AVR_MAP) || !QTAV_HAVE(SWRESAMPLE)) && QTAV_HAVE(AVRESAMPLE)
#ifndef SWR_CH_MAX
#ifdef AVRESAMPLE_MAX_CHANNELS
#define SWR_CH_MAX AVRESAMPLE_MAX_CHANNELS
#else
#define SWR_CH_MAX 32
#endif //AVRESAMPLE_MAX_CHANNELS
#endif //SWR_CH_MAX
#define SwrContext AVAudioResampleContext
#define swr_init(ctx) avresample_open(ctx)
//free context and set pointer to null. see swresample
#define swr_free(ctx) \
    if (ctx && *ctx) { \
        avresample_close(*ctx); \
        *ctx = 0; \
    }
#define swr_get_class() avresample_get_class()
#define swr_alloc() avresample_alloc_context()
//#define swr_next_pts()
#define swr_set_compensation() avresample_set_compensation()
#define swr_set_channel_mapping(ctx, map) avresample_set_channel_mapping(ctx, map)
#define swr_set_matrix(ctx, matrix, stride) avresample_set_matrix(ctx, matrix, stride)
//#define swr_drop_output(ctx, count)
//#define swr_inject_silence(ctx, count)
#define swr_get_delay(ctx, ...) avresample_get_delay(ctx)
#if LIBAVRESAMPLE_VERSION_INT >= AV_VERSION_INT(1, 0, 0) //ffmpeg >= 1.1
#define swr_convert(ctx, out, out_count, in, in_count) \
    avresample_convert(ctx, out, 0, out_count, in, 0, in_count)
#else
#define swr_convert(ctx, out, out_count, in, in_count) \
    avresample_convert(ctx, (void**)out, 0, out_count, (void**)in, 0, in_count)
#endif
struct SwrContext *swr_alloc_set_opts(struct SwrContext *s, int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate, int64_t in_ch_layout, enum AVSampleFormat in_sample_fmt, int in_sample_rate, int log_offset, void *log_ctx);
#define swresample_version() avresample_version()
#define swresample_configuration() avresample_configuration()
#define swresample_license() avresample_license()
#endif //MAP_SWR_AVR


/* For FFmpeg < 2.0
 * FF_API_PIX_FMT macro?
 * 51.42.0: PIX_FMT_* -> AV_PIX_FMT_*, PixelFormat -> AVPixelFormat
 * so I introduce QTAV_PIX_FMT_C(X) for internal use
 * FFmpeg n1.1 AVPixelFormat
 */
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 13, 100) //(51, 42, 0)
typedef enum PixelFormat AVPixelFormat; // so we must avoid using  enum AVPixelFormat
#define QTAV_PIX_FMT_C(X) PIX_FMT_##X
#else //FFmpeg >= 2.0
typedef enum AVPixelFormat AVPixelFormat;
#define QTAV_PIX_FMT_C(X) AV_PIX_FMT_##X
#endif //AV_VERSION_INT(51, 42, 0)

// AV_PIX_FMT_FLAG_XXX was PIX_FMT_XXX before FFmpeg 2.0
// AV_PIX_FMT_FLAG_ALPHA was added at 52.2.0. but version.h not changed
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 2, 1) //git cbe5a60c9d495df0fb4775b064f06719b70b9952
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51, 22, 1) //git 38d553322891c8e47182f05199d19888422167dc
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51, 19, 0) //git 6b0768e2021b90215a2ab55ed427bce91d148148
#define PIX_FMT_PLANAR   16 ///< At least one pixel component is not in the first data plane
#define PIX_FMT_RGB      32 ///< The pixel format contains RGB-like data (as opposed to YUV/grayscale)
#endif //AV_VERSION_INT(51, 19, 0)
#define PIX_FMT_PSEUDOPAL 64  //why not defined in FFmpeg 0.9 lavu51.32.0 but git log says 51.22.1 defined it?
#endif //AV_VERSION_INT(51, 22, 1)
#define PIX_FMT_ALPHA   128 ///< The pixel format has an alpha channel
#endif //AV_VERSION_INT(52, 2, 1)

#ifndef PIX_FMT_PLANAR
#define PIX_FMT_PLANAR 16
#endif //PIX_FMT_PLANAR
#ifndef PIX_FMT_RGB
#define PIX_FMT_RGB 32
#endif //PIX_FMT_RGB
#ifndef PIX_FMT_PSEUDOPAL
#define PIX_FMT_PSEUDOPAL 64
#endif //PIX_FMT_PSEUDOPAL
#ifndef PIX_FMT_ALPHA
#define PIX_FMT_ALPHA 128
#endif //PIX_FMT_ALPHA

/*
 * rename PIX_FMT_* flags to AV_PIX_FMT_FLAG_*. git e6c4ac7b5f038be56dfbb0171f5dd0cb850d9b28
 */
//#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 11, 0)
#ifndef AV_PIX_FMT_FLAG_BE
#define AV_PIX_FMT_FLAG_BE           PIX_FMT_BE
#define AV_PIX_FMT_FLAG_PAL          PIX_FMT_PAL
#define AV_PIX_FMT_FLAG_BITSTREAM    PIX_FMT_BITSTREAM
#define AV_PIX_FMT_FLAG_HWACCEL      PIX_FMT_HWACCEL

// FFmpeg >=  0.9, libav >= 0.8.8(51,22,1)
#define AV_PIX_FMT_FLAG_PLANAR       PIX_FMT_PLANAR
#define AV_PIX_FMT_FLAG_RGB          PIX_FMT_RGB

// FFmpeg >= 1.0, libav >= 9.7
#define AV_PIX_FMT_FLAG_PSEUDOPAL    PIX_FMT_PSEUDOPAL
// FFmpeg >= 1.1, libav >= 9.7
#define AV_PIX_FMT_FLAG_ALPHA        PIX_FMT_ALPHA
#endif //AV_PIX_FMT_FLAG_BE
//#endif //AV_VERSION_INT(52, 11, 0)
// FFmpeg >= 1.1, but use internal av_pix_fmt_descriptors. FFmpeg < 1.1 has extern av_pix_fmt_descriptors
// used by av_pix_fmt_count_planes
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 13, 100)
const AVPixFmtDescriptor *av_pix_fmt_desc_get(AVPixelFormat pix_fmt);
#endif //AV_VERSION_INT(52, 13, 100)

/*
 * lavu 52.9.0 git 2c328a907978b61949fd20f7c991803174337855
 * FFmpeg >= 2.0.
 */
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 38, 100)
int av_pix_fmt_count_planes(AVPixelFormat pix_fmt);
#endif //AV_VERSION_INT(52, 38, 100)

// FFmpeg < 1.0 has no av_samples_copy
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51, 73, 101)
/**
 * Copy samples from src to dst.
 *
 * @param dst destination array of pointers to data planes
 * @param src source array of pointers to data planes
 * @param dst_offset offset in samples at which the data will be written to dst
 * @param src_offset offset in samples at which the data will be read from src
 * @param nb_samples number of samples to be copied
 * @param nb_channels number of audio channels
 * @param sample_fmt audio sample format
 */
int av_samples_copy(uint8_t **dst, uint8_t * const *src, int dst_offset,
                    int src_offset, int nb_samples, int nb_channels,
                    enum AVSampleFormat sample_fmt);
#endif //AV_VERSION_INT(51, 73, 101)

#if LIBAVCODEC_VERSION_MAJOR < 54
typedef enum CodecID AVCodecID;
#endif

#endif //QTAV_COMPAT_H
