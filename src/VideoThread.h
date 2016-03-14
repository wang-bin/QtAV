/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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


#ifndef QTAV_VIDEOTHREAD_H
#define QTAV_VIDEOTHREAD_H

#include "AVThread.h"
#include <QtCore/QSize>

namespace QtAV {

class VideoCapture;
class VideoFrame;
class VideoThreadPrivate;
class VideoThread : public AVThread
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoThread)
public:
    explicit VideoThread(QObject *parent = 0);
    VideoCapture *setVideoCapture(VideoCapture* cap); //ensure thread safe
    VideoCapture *videoCapture() const;
    VideoFrame displayedFrame() const;
    void setFrameRate(qreal value);
    //virtual bool event(QEvent *event);
    void setBrightness(int val);
    void setContrast(int val);
    void setSaturation(int val);
    void setEQ(int b, int c, int s);

public Q_SLOTS:
    void addCaptureTask();
    void clearRenderers();

protected:
    void applyFilters(VideoFrame& frame);
    // deliver video frame to video renderers. frame may be converted to a suitable format for renderer
    bool deliverVideoFrame(VideoFrame &frame);
    virtual void run();
    // wait for value msec. every usleep is a small time, then process next task and get new delay
};


} //namespace QtAV
#endif // QTAV_VIDEOTHREAD_H
