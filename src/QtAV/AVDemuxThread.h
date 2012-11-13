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

#ifndef QAV_DEMUXTHREAD_H
#define QAV_DEMUXTHREAD_H

#include <QtCore/QMutex>
#include <QtCore/QThread>
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

public slots:
    void stop();

protected:
    virtual void run();
    
private:
    volatile bool end;
    AVDemuxer *demuxer;
    AVThread *audio_thread, *video_thread;
    int audio_stream, video_stream;
    QMutex buffer_mutex;
};

} //namespace QtAV
#endif // QAV_DEMUXTHREAD_H
