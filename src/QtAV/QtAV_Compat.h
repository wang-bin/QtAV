/******************************************************************************
	QtAV:  Media play library based on Qt and FFmpeg
	solve the version problem and diffirent api in FFmpeg and libav
	Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef QTAV_COMPAT_H
#define QTAV_COMPAT_H

#include <qglobal.h>
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#ifdef __cplusplus
}
#endif //__cplusplus

//TODO: libav
//avutil: error.h
#ifndef av_err2str
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
//GCC: taking address of temporary array
//#define av_err2str(errnum) \
//	av_make_error_string((char[AV_ERROR_MAX_STRING_SIZE]){0}, AV_ERROR_MAX_STRING_SIZE, errnum)

#endif //av_err2str

#endif
