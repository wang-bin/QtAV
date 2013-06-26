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
#include "QtAV/version.h"
#include "prepost.h"

void ffmpeg_version_print()
{
    struct _component {
        const char* lib;
        unsigned build_version;
        unsigned rt_version;
    } components[] = {
        { "avcodec", LIBAVCODEC_VERSION_INT, avcodec_version()},
        { "avformat", LIBAVFORMAT_VERSION_INT, avformat_version()},
        { "avutil", LIBAVUTIL_VERSION_INT, avutil_version()},
        { "swscale", LIBSWSCALE_VERSION_INT, swscale_version()},
#if QTAV_HAVE(SWRESAMPLE)
        //{ "swresample", LIBSWRESAMPLE_VERSION_INT, swresample_version()}, //swresample_version not declared in 0.9
#endif //QTAV_HAVE(SWRESAMPLE)
#if QTAV_HAVE(AVRESAMPLE)
        { "avresample", LIBAVRESAMPLE_VERSION_INT, avresample_version()},
#endif //QTAV_HAVE(AVRESAMPLE)
        { 0, 0, 0}
    };
    for (int i = 0; components[i].lib != 0; ++i) {
        printf("Build with lib%s-%u.%u.%u\n"
               , components[i].lib
               , QTAV_VERSION_MAJOR(components[i].build_version)
               , QTAV_VERSION_MINOR(components[i].build_version)
               , QTAV_VERSION_PATCH(components[i].build_version)
               );
        unsigned rt_version = components[i].rt_version;
        if (components[i].build_version != rt_version) {
            fprintf(stderr, "Warning: %s runtime version %u.%u.%u mismatch!\n"
                    , components[i].lib
                    , QTAV_VERSION_MAJOR(rt_version)
                    , QTAV_VERSION_MINOR(rt_version)
                    , QTAV_VERSION_PATCH(rt_version)
                    );
        }
    }
    fflush(0);
}

PRE_FUNC_ADD(ffmpeg_version_print);

#ifndef av_err2str

#endif //av_err2str

