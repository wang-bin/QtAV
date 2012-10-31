/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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

#ifndef QTAV_AVDECODER_P_H
#define QTAV_AVDECODER_P_H

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
}
#endif //__cplusplus

#if CONFIG_EZX
#define PIX_FMT PIX_FMT_BGR565
#else
#define PIX_FMT PIX_FMT_RGB32
#endif //CONFIG_EZX

#include <qbytearray.h>
#include <QtCore/QMutex>

namespace QtAV {

class AVDecoderPrivate
{
public:
    AVDecoderPrivate():codec_ctx(0),frame(0) {
        frame = avcodec_alloc_frame();
    }
    ~AVDecoderPrivate() {
        if (frame) {
            av_free(frame);
            frame = 0;
        }
    }

    AVCodecContext *codec_ctx; //set once and not change
    AVFrame *frame; //set once and not change
    QByteArray decoded;
    int got_frame_ptr;
    QMutex mutex;
};

} //namespace QtAV
#endif // QTAV_AVDECODER_P_H
