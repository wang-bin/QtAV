/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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

#include <QtAV/QtAV_Compat.h>
#include "prepost.h"

void ffmpeg_version_print()
{
    printf("FFmpeg/Libav version:\n"
           "libavcodec-%d.%d.%d\n"
           "libavformat-%d.%d.%d\n"
//	       "libavdevice-%d.%d.%d\n"
           "libavutil-%d.%d.%d\n"
           "libswscal-%d.%d.%d\n"
           , LIBAVCODEC_VERSION_MAJOR, LIBAVCODEC_VERSION_MINOR, LIBAVCODEC_VERSION_MICRO//,avcodec_version()
           , LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO //avformat_version()
//	       ,avdevice_version()
           , LIBAVUTIL_VERSION_MAJOR, LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO //avutil_version()
           , LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO
           );
    fflush(0);
}

PRE_FUNC_ADD(ffmpeg_version_print);

#ifndef av_err2str

#endif //av_err2str

