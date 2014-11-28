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

#ifndef QTAV_AVDECODER_P_H
#define QTAV_AVDECODER_P_H

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include "QtAV/private/AVCompat.h"

namespace QtAV {

class Q_AV_PRIVATE_EXPORT AVDecoderPrivate : public DPtrPrivate<AVDecoder>
{
public:
    AVDecoderPrivate():
        codec_ctx(0)
      , available(true)
      , is_open(false)
      , frame(0)
      , got_frame_ptr(0)
      , undecoded_size(0)
      , dict(0)
    {
        frame = av_frame_alloc();
    }
    virtual ~AVDecoderPrivate() {
        if (frame) {
            av_frame_free(&frame);
            frame = 0;
        }
        if (dict) {
            av_dict_free(&dict);
        }
    }
    virtual bool open() {return true;}
    virtual void close() {}

    AVCodecContext *codec_ctx; //set once and not change
    bool available; //TODO: true only when context(and hw ctx) is ready
    bool is_open;
    AVFrame *frame; //set once and not change
    QByteArray decoded;
    int got_frame_ptr;
    int undecoded_size;
    QMutex mutex;
    QString codec_name;
    QVariantHash options;
    AVDictionary *dict;
};

} //namespace QtAV
#endif // QTAV_AVDECODER_P_H
