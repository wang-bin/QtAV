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
public:
    AVDecoder();
    virtual ~AVDecoder();
    void setCodecContext(AVCodecContext* codecCtx); //protected
    AVCodecContext* codecContext() const;

    virtual bool decode(const QByteArray& encoded) = 0; //decode AVPacket?
    QByteArray data() const; //decoded data

protected:
    AVDecoder(AVDecoderPrivate& d);

    Q_DECLARE_PRIVATE(AVDecoder)
    AVDecoderPrivate *d_ptr;
};

} //namespace QtAV
#endif // QAV_DECODER_H
