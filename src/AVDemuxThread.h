/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QRunnable>
#include "PacketBuffer.h"
#include <QTimer>

namespace QtAV {

class AVDemuxer;
class AVThread;
class AVDemuxThread : public QThread
{
    Q_OBJECT
public:
    explicit AVDemuxThread(QObject *parent = 0);
    explicit AVDemuxThread(AVDemuxer *dmx, QObject *parent = 0);
    void setDemuxer(AVDemuxer *dmx);
    void setAudioDemuxer(AVDemuxer *demuxer); //not thread safe
    void setAudioThread(AVThread *thread);
    AVThread* audioThread();
    void setVideoThread(AVThread *thread);
    AVThread* videoThread();
    void stepForward(); // show next video frame and pause
    void stepBackward();
    void seek(qint64 external_pos, qint64 pos, SeekType type); //ms
    //AVDemuxer* demuxer
    bool isPaused() const;
    bool isEnd() const;
    bool atEndOfMedia() const;
    PacketBuffer* buffer();
    void updateBufferState();
    void stop(); //TODO: remove it?
    void pause(bool p, bool wait = false);

    MediaEndAction mediaEndAction() const;
    void setMediaEndAction(MediaEndAction value);
    bool waitForStarted(int msec = -1);
    qint64 lastSeekPos();
    bool hasSeekTasks();
Q_SIGNALS:
    void requestClockPause(bool value);
    void mediaStatusChanged(QtAV::MediaStatus);
    void bufferProgressChanged(qreal);
    void seekFinished(qint64 timestamp);
    void stepFinished();
    void internalSubtitlePacketRead(int index, const QtAV::Packet& packet);
private slots:
    void finishedStepBackward();
    void seekOnPauseFinished();
    void frameDeliveredOnStepForward();
    void eofDecodedOnStepForward();
    void stepForwardDone();
    void onAVThreadQuit();

protected:
    virtual void run();
    /*
     * If the pause state is true setted by pause(true), then block the thread and wait for pause state changed, i.e. pause(false)
     * and return true. Otherwise, return false immediatly.
     */
    bool tryPause(unsigned long timeout = 100);

private:
    void setAVThread(AVThread *&pOld, AVThread* pNew);
    void newSeekRequest(QRunnable *r);
    void processNextSeekTask();
    void seekInternal(qint64 pos, SeekType type, qint64 external_pos = std::numeric_limits < qint64 >::min()); //must call in AVDemuxThread
    void pauseInternal(bool value);

    bool paused;
    bool user_paused;
    volatile bool end;
    MediaEndAction end_action;
    bool m_buffering;
    PacketBuffer *m_buffer;
    AVDemuxer *demuxer;
    AVDemuxer *ademuxer;
    AVThread *audio_thread, *video_thread;
    int audio_stream, video_stream;
    QMutex buffer_mutex;
    QWaitCondition cond;
    BlockingQueue<QRunnable*> seek_tasks;
    qint64 last_seek_pos;
    QRunnable *current_seek_task;
    bool stepping;
    qint64 stepping_timeout_time;
        
    QSemaphore sem;
    QMutex next_frame_mutex;
    int clock_type; // change happens in different threads(direct connection)
    friend class SeekTask;
    friend class stepBackwardTask;
};

} //namespace QtAV
#endif // QAV_DEMUXTHREAD_H
