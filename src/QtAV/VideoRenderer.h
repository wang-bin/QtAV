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

#ifndef QAV_VIDEORENDERER_H
#define QAV_VIDEORENDERER_H

#include <qbytearray.h>
#include <qsize.h>
#include <QtAV/AVOutput.h>

struct AVCodecContext;
struct AVFrame;

namespace QtAV {
class VideoRendererPrivate;
class Q_EXPORT VideoRenderer : public AVOutput
{
public:
    VideoRenderer();
    virtual ~VideoRenderer() = 0;

    virtual bool open() {return true;}
    virtual bool close() {return true;}
    void resizeVideo(const QSize& size);
    void resizeVideo(int width, int height);
    QSize videoSize() const;
    int videoWidth() const;
    int videoHeight() const;

protected:
    Q_DECLARE_PRIVATE(VideoRenderer)
};
}

#endif // QAV_VIDEORENDERER_H
