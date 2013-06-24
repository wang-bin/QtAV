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

#ifndef QAV_DECODER_H
#define QAV_DECODER_H

#include <QtAV/QtAV_Global.h>

class QByteArray;
struct AVCodecContext;
struct AVFrame;

namespace QtAV {

class AVDecoderPrivate;
class Q_EXPORT AVDecoder
{
    DPTR_DECLARE_PRIVATE(AVDecoder)
public:
    AVDecoder();
    virtual ~AVDecoder();
    void flush();
    void setCodecContext(AVCodecContext* codecCtx); //protected
    AVCodecContext* codecContext() const;
    /*not available if AVCodecContext == 0*/
    bool isAvailable() const;
    virtual bool prepare(); //if resampler or image converter set, call it
    virtual bool decode(const QByteArray& encoded) = 0; //decode AVPacket?
    QByteArray data() const; //decoded data

protected:
    AVDecoder(AVDecoderPrivate& d);

    DPTR_DECLARE(AVDecoder)
};

} //namespace QtAV
#endif // QAV_DECODER_H
