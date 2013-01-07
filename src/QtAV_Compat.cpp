#include <QtAV/QtAV_Compat.h>
#include "prepost.h"

void ffmpeg_version_print()
{
    printf("FFMpeg/Libav version:\n"
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

