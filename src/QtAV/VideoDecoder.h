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

#ifndef QTAV_VIDEODECODER_H
#define QTAV_VIDEODECODER_H

#include <QtAV/AVDecoder.h>

class QSize;
struct SwsContext;
namespace QtAV {
class VideoDecoderPrivate;
class Q_EXPORT VideoDecoder : public AVDecoder
{
    DPTR_DECLARE_PRIVATE(VideoDecoder)
public:
    VideoDecoder();
    //virtual bool prepare();
    virtual bool decode(const QByteArray &encoded);
    //TODO: new api: originalVideoSize()(inSize()), decodedVideoSize()(outSize())
    //size: the decoded(actually then resized in ImageConverter) frame size
    void resizeVideoFrame(const QSize& size);
    void resizeVideoFrame(int width, int height);
    //TODO: decodedSize()
    int width() const;
    int height() const;
};

} //namespace QtAV
#endif // QTAV_VIDEODECODER_H
