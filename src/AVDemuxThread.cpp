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

#include <QtAV/AVDemuxThread.h>
#include <QtAV/AVDemuxer.h>
#include <QtAV/AVDecoder.h>
#include <QtAV/Packet.h>
#include <QtAV/AVThread.h>
#include <QtCore/QTimer>
#include <QtCore/QEventLoop>

#define CORRECT_END 1

namespace QtAV {

class QueueEmptyCall : public PacketQueue::StateChangeCallback
{
public:
    QueueEmptyCall(AVDemuxThread* thread):
        mDemuxThread(thread)
    {}
    virtual void call() {
        if (!mDemuxThread)
            return;
        if (mDemuxThread->isEnd())
            return;
        AVThread *thread = mDemuxThread->videoThread();
        qDebug("try wake up video queue");
        if (thread)
            thread->packetQueue()->blockFull(false);
        qDebug("try wake up audio queue");
        thread = mDemuxThread->audioThread();
        if (thread)
            thread->packetQueue()->blockFull(false);
    }
private:
    AVDemuxThread *mDemuxThread;
};

AVDemuxThread::AVDemuxThread(QObject *parent) :
    QThread(parent),paused(false),seeking(false),end(true)
    ,demuxer(0)
    ,audio_thread(0),video_thread(0)
{
}

AVDemuxThread::AVDemuxThread(AVDemuxer *dmx, QObject *parent) :
    QThread(parent),paused(false),seeking(false),end(true)
    ,audio_thread(0),video_thread(0)
{
    setDemuxer(dmx);
}

void AVDemuxThread::setDemuxer(AVDemuxer *dmx)
{
    demuxer = dmx;
}

void AVDemuxThread::setAVThread(AVThread*& pOld, AVThread *pNew)
{
    if (pOld == pNew)
        return;
    if (pOld) {
        if (pOld->isRunning())
            pOld->stop();
#if CORRECT_END
        disconnect(pOld, SIGNAL(terminated()), this, SLOT(notifyEnd()));
        disconnect(pOld, SIGNAL(finished()), this, SLOT(notifyEnd()));
#endif //CORRECT_END
    }
    pOld = pNew;
    if (!pNew)
        return;
#if CORRECT_END
    connect(pOld, SIGNAL(terminated()), this, SLOT(notifyEnd()), Qt::DirectConnection);
    connect(pOld, SIGNAL(finished()), this, SLOT(notifyEnd()), Qt::DirectConnection);
#endif //CORRECT_END
    pOld->packetQueue()->setEmptyCallback(new QueueEmptyCall(this));
}

void AVDemuxThread::setAudioThread(AVThread *thread)
{
    setAVThread(audio_thread, thread);
}

void AVDemuxThread::setVideoThread(AVThread *thread)
{
    setAVThread(video_thread, thread);
}

AVThread* AVDemuxThread::videoThread()
{
    return video_thread;
}

AVThread* AVDemuxThread::audioThread()
{
    return audio_thread;
}

void AVDemuxThread::seek(qreal pos)
{
    qDebug("demux thread start to seek...");
    seeking = true;
    end = false;
    if (audio_thread) {
        audio_thread->setDemuxEnded(false);
        audio_thread->packetQueue()->clear();
    }
    if (video_thread) {
        video_thread->setDemuxEnded(false);
        video_thread->packetQueue()->clear();
    }
    demuxer->seek(pos);
    if (audio_thread)
        audio_thread->packetQueue()->put(Packet());
    if (video_thread)
        video_thread->packetQueue()->put(Packet());
    //if (subtitle_thread)
    //    subtitle_thread->packetQueue()->put(Packet());

    seeking = false;
    seek_cond.wakeAll();
    if (isPaused()) {
        pause(false);
        AVThread *avthread = video_thread;
        if (!avthread)
            avthread = audio_thread;
        avthread->pause(false);
        QEventLoop loop;
        QTimer::singleShot(40, &loop, SLOT(quit()));
        loop.exec();
        pause(true);
        avthread->pause(true);
    }
}

bool AVDemuxThread::isPaused() const
{
    return paused;
}

bool AVDemuxThread::isEnd() const
{
    return end;
}

//No more data to put. So stop blocking the queue to take the reset elements
void AVDemuxThread::stop()
{
    qDebug("void AVDemuxThread::stop()");
    end = true;
    //this will not affect the pause state if we pause the output
    //TODO: why remove blockFull(false) can not play another file?
    if (audio_thread) {
        audio_thread->setDemuxEnded(true);
        audio_thread->packetQueue()->clear();
        audio_thread->packetQueue()->blockFull(false); //??
    }
    if (video_thread) {
        video_thread->setDemuxEnded(true);
        video_thread->packetQueue()->clear();
        video_thread->packetQueue()->blockFull(false); //?
    }
    pause(false);
    seek_cond.wakeAll();
}

void AVDemuxThread::pause(bool p)
{
    qDebug("demux thread pause %d", p);
    if (paused == p)
        return;
    paused = p;
    if (!paused)
        cond.wakeAll();
}

void AVDemuxThread::notifyEnd()
{
    qDebug("avthread end. notify demux thread<<<<<<");
    end = true;
    seek_cond.wakeAll();
    cond.wakeAll();
}

void AVDemuxThread::run()
{
    qDebug("demux thread start running...");
    end = false;
    if (audio_thread && !audio_thread->isRunning())
        audio_thread->start(QThread::HighPriority);
    if (video_thread && !video_thread->isRunning())
        video_thread->start();

    audio_stream = demuxer->audioStream();
    video_stream = demuxer->videoStream();
    int index = 0;
    Packet pkt;
    pause(false);
    qDebug("get av queue a/v thread = %p %p", audio_thread, video_thread);
    PacketQueue *aqueue = audio_thread ? audio_thread->packetQueue() : 0;
    PacketQueue *vqueue = video_thread ? video_thread->packetQueue() : 0;
    if (aqueue) {
        aqueue->clear();
        aqueue->setBlocking(true);
    }
    if (vqueue) {
        vqueue->clear();
        vqueue->setBlocking(true);
    }
    while (true) {
        if (tryPause())
            continue; //the queue is empty and will block
        QMutexLocker locker(&buffer_mutex);
        Q_UNUSED(locker);
        if (end) {
#if CORRECT_END
            if ((audio_thread && audio_thread->isRunning())
                    || (video_thread && video_thread->isRunning())) {
                qDebug("demux read end but avthread still running. waiting for finish...");
                seek_cond.wait(&buffer_mutex);
            }
            if (end)
#endif //CORRECT_END
                break;
        }
        if (seeking) {
            qDebug("Demuxer is seeking... wait for seek end");
            if (!seek_cond.wait(&buffer_mutex, 200)) { //will return the same state(i.e. lock)
                qWarning("seek timed out");
            }
        }
        if (!demuxer->readFrame()) {
            continue;
        }
        index = demuxer->stream();
        pkt = *demuxer->packet(); //TODO: how to avoid additional copy?
        //connect to stop is ok too
        if (pkt.isEnd()) {
            qDebug("read end packet %d A:%d V:%d", index, audio_stream, video_stream);
            end = true;
            //avthread can stop. do not clear queue, make sure all data are played
            if (audio_thread) {
                audio_thread->setDemuxEnded(true);
            }
            if (video_thread) {
                video_thread->setDemuxEnded(true);
            }
        }
        /*1 is empty but another is enough, then do not block to
          ensure the empty one can put packets immediatly.
          But usually it will not happen, why?
        */
        /* demux thread will be blocked only when 1 queue is full and still put
         * if vqueue is full and aqueue becomes empty, then demux thread
         * will be blocked. so we should wake up another queue when empty(or threshold?).
         * TODO: the video stream and audio stream may be group by group. provide it
         * stream data: aaaaaaavvvvvvvaaaaaaaavvvvvvvvvaaaaaa, it happens
         * stream data: aavavvavvavavavavavavavavvvaavavavava, it's ok
         */
        //TODO: use cache queue, take from cache queue if not empty?
        if (index == audio_stream) {
            /* if vqueue if not blocked and full, and aqueue is empty, then put to
             * vqueue will block demuex thread
             */
            if (aqueue) {
                if (vqueue)
                    aqueue->blockFull(vqueue->isEnough());
                aqueue->put(pkt); //affect video_thread
            }
        } else if (index == video_stream) {
            if (vqueue) {
                if (aqueue)
                    vqueue->blockFull(aqueue->isEnough());
                vqueue->put(pkt); //affect audio_thread
            }
        } else { //subtitle
            continue;
        }
    }
    //flush. seeking will be omitted when stopped
    if (aqueue)
        aqueue->put(Packet());
    if (vqueue)
        vqueue->put(Packet());
    qDebug("Demux thread stops running....");
}

bool AVDemuxThread::tryPause()
{
    if (!paused)
        return false;
    QMutexLocker lock(&buffer_mutex);
    Q_UNUSED(lock);
    cond.wait(&buffer_mutex);
    return true;
}


} //namespace QtAV
