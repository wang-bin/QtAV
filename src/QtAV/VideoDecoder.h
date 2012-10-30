/******************************************************************************
    VideoDecoder.h: description
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


#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <qsize.h>
#include <QtAV/AVDecoder.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif //__cplusplus

struct SwsContext;
namespace QtAV {
class VideoDecoderPrivate;
class Q_EXPORT VideoDecoder : public AVDecoder
{
public:
    VideoDecoder();
    virtual bool decode(const QByteArray &encoded);

    void resizeVideo(const QSize& size);
    void resizeVideo(int width, int height);

private:
    Q_DECLARE_PRIVATE(VideoDecoder)
};

} //namespace QtAV
#endif // VIDEODECODER_H
