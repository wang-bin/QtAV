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
#include <libavutil/mathematics.h> //AV_ROUND_UP, av_rescale_rnd for libav
#include <libavutil/error.h>
#include <libavutil/opt.h>

/* TODO: how to check whether we have swresample or not? how to check avresample?*/
#include <libavutil/samplefmt.h>
#if QTAV_HAVE(SWRESAMPLE)
#include <libswresample/swresample.h>
#ifndef LIBSWRESAMPLE_VERSION_INT //ffmpeg 0.9, swr 0.5
#define LIBSWRESAMPLE_VERSION_INT AV_VERSION_INT(LIBSWRESAMPLE_VERSION_MAJOR, LIBSWRESAMPLE_VERSION_MINOR, LIBSWRESAMPLE_VERSION_MICRO)
#endif
#define HAVE_SWR_GET_DELAY (LIBSWRESAMPLE_VERSION_INT > AV_VERSION_INT(0, 5, 0))
#endif //QTAV_HAVE(SWRESAMPLE)
#if QTAV_HAVE(AVRESAMPLE)
#include <libavresample/avresample.h>
#endif //QTAV_HAVE(AVRESAMPLE)
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
#if defined(Q_CC_MSVC) || !defined(av_err2str) || (GCC_VERSION_AT_LEAST(4, 7, 2) && __cplusplus)
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

#endif

#if (LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(52,101,0))
#define av_dump_format(...) dump_format(__VA_ARGS__)
#endif

#if QTAV_HAVE(SWRESAMPLE) && (LIBSWRESAMPLE_VERSION_INT <= AV_VERSION_INT(0, 5, 0))
#define swresample_version() LIBSWRESAMPLE_VERSION_INT //we can not know the runtime version, so just use build time version
#define swresample_configuration() "Not available."
#define swresample_license() "Not available."
#endif
#endif

/*
 * mapping avresample to swresample
 * https://github.com/xbmc/xbmc/commit/274679d
 */
#if (QTAV_HAVE(SWR_AVR_MAP) || !QTAV_HAVE(SWRESAMPLE)) && QTAV_HAVE(AVRESAMPLE)
#define SwrContext AVAudioResampleContext
#define swr_init(ctx) avresample_open(ctx)
#define swr_free(ctx) avresample_close(*ctx)
#define swr_get_class() avresample_get_class()
#define swr_alloc() avresample_alloc_context()
//#define swr_next_pts()
#define swr_set_compensation() avresample_set_compensation()
#define swr_set_channel_mapping(ctx, map) avresample_set_channel_mapping(ctx, map)
#define swr_set_matrix(ctx, matrix, stride) avresample_set_matrix(ctx, matrix, stride)
//#define swr_drop_output(ctx, count)
//#define swr_inject_silence(ctx, count)
#define swr_get_delay(ctx, ...) avresample_get_delay(ctx)
#define swr_convert(ctx, out, out_count, in, in_count) \
    avresample_convert(ctx, out, 0, out_count, const_cast<uint8_t**>(in), 0, in_count)
struct SwrContext *swr_alloc_set_opts(struct SwrContext *s, int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate, int64_t in_ch_layout, enum AVSampleFormat in_sample_fmt, int in_sample_rate, int log_offset, void *log_ctx);
#define swresample_version() avresample_version()
#define swresample_configuration() avresample_configuration()
#define swresample_license() avresample_license()
#endif //MAP_SWR_AVR
