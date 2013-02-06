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

#ifndef QAV_DEMUXTHREAD_H
#define QAV_DEMUXTHREAD_H

#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtAV/QtAV_Global.h>

namespace QtAV {

class AVDemuxer;
class AVThread;
class Q_EXPORT AVDemuxThread : public QThread
{
    Q_OBJECT
public:
    explicit AVDemuxThread(QObject *parent = 0);
    explicit AVDemuxThread(AVDemuxer *dmx, QObject *parent = 0);
    void setDemuxer(AVDemuxer *dmx);
    void setAudioThread(AVThread *thread);
    void setVideoThread(AVThread *thread);
    void seek(qreal pos);
    void seekForward();
    void seekBackward();
    //AVDemuxer* demuxer
    bool isPaused() const;
public slots:
    void stop();
    void pause(bool p);

protected:
    virtual void run();
    /*
     * If the pause state is true setted by pause(true), then block the thread and wait for pause state changed, i.e. pause(false)
     * and return true. Otherwise, return false immediatly.
     */
    bool tryPause();

private:
    bool paused, seeking;
    volatile bool end;
    AVDemuxer *demuxer;
    AVThread *audio_thread, *video_thread;
    int audio_stream, video_stream;
    QMutex buffer_mutex;
    QWaitCondition cond, seek_cond;
};

} //namespace QtAV
#endif // QAV_DEMUXTHREAD_H
