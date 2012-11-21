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

#include <QtCore/QByteArray>
#include <QtCore/QSize>
#include <QtAV/AVOutput.h>

struct AVCodecContext;
struct AVFrame;
class QImage;
namespace QtAV {

class EventFilter;
class VideoRendererPrivate;
class Q_EXPORT VideoRenderer : public AVOutput
{
    DPTR_DECLARE_PRIVATE(VideoRenderer)
public:
    VideoRenderer();
    virtual ~VideoRenderer() = 0;
    //TODO: unregister
    virtual void registerEventFilter(EventFilter* filter);
    virtual bool open();
    virtual bool close();
    virtual QImage currentFrameImage() const = 0; //const QImage& const?
    void resizeVideo(const QSize& size);
    void resizeVideo(int width, int height);
    QSize videoSize() const;
    int videoWidth() const;
    int videoHeight() const;

protected:
    VideoRenderer(VideoRendererPrivate &d);
};

} //namespace QtAV
#endif // QAV_VIDEORENDERER_H
