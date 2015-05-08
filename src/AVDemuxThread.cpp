/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "AVDemuxThread.h"
#include <limits>
#include "QtAV/AVClock.h"
#include "QtAV/AVDemuxer.h"
#include "QtAV/AVDecoder.h"
#include "VideoThread.h"
#include <QtCore/QTime>
#include "utils/Logger.h"

#define RESUME_ONCE_ON_SEEK 0

namespace QtAV {

class QueueEmptyCall : public PacketBuffer::StateChangeCallback
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
        mDemuxThread->updateBufferState(); // ensure detect buffering immediately
        AVThread *thread = mDemuxThread->videoThread();
        //qDebug("try wake up video queue");
        if (thread)
            thread->packetQueue()->blockFull(false);
        //qDebug("try wake up audio queue");
        thread = mDemuxThread->audioThread();
        if (thread)
            thread->packetQueue()->blockFull(false);
    }
private:
    AVDemuxThread *mDemuxThread;
};

AVDemuxThread::AVDemuxThread(QObject *parent) :
    QThread(parent)
  , paused(false)
  , user_paused(false)
  , end(false)
  , m_buffering(false)
  , m_buffer(0)
  , demuxer(0)
  , audio_thread(0)
  , video_thread(0)
  , nb_next_frame(0)
  , clock_type(-1)
{
    seek_tasks.setCapacity(1);
    seek_tasks.blockFull(false);
}

AVDemuxThread::AVDemuxThread(AVDemuxer *dmx, QObject *parent) :
    QThread(parent)
  , paused(false)
  , end(false)
  , m_buffering(false)
  , m_buffer(0)
  , audio_thread(0)
  , video_thread(0)
{
    setDemuxer(dmx);
    seek_tasks.setCapacity(1);
    seek_tasks.blockFull(false);
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
        disconnect(this, SLOT(onAVThreadQuit()));
    }
    pOld = pNew;
    if (!pNew)
        return;
    pOld->packetQueue()->setEmptyCallback(new QueueEmptyCall(this));
    connect(pOld, SIGNAL(finished()), SLOT(onAVThreadQuit()));
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

void AVDemuxThread::seek(qint64 pos, SeekType type)
{
    end = false;
    // queue maybe blocked by put()
    if (audio_thread) {
        audio_thread->packetQueue()->clear();
    }
    if (video_thread) {
        video_thread->packetQueue()->clear();
    }
    class SeekTask : public QRunnable {
    public:
        SeekTask(AVDemuxThread *dt, qint64 t, SeekType st)
            : demux_thread(dt)
            , type(st)
            , position(t)
        {}
        void run() {
            demux_thread->seekInternal(position, type);
        }
    private:
        AVDemuxThread *demux_thread;
        SeekType type;
        qint64 position;
    };
    newSeekRequest(new SeekTask(this, pos, type));
}

void AVDemuxThread::seekInternal(qint64 pos, SeekType type)
{
    AVThread* av[] = { audio_thread, video_thread};
    qDebug("seek to %s %lld ms (%f%%)", QTime(0, 0, 0).addMSecs(pos).toString().toUtf8().constData(), pos, double(pos - demuxer->startTime())/double(demuxer->duration())*100.0);
    demuxer->setSeekType(type);
    demuxer->seek(pos);
    // TODO: why queue may not empty?
    for (size_t i = 0; i < sizeof(av)/sizeof(av[0]); ++i) {
        AVThread *t = av[i];
        if (!t)
            continue;
        t->packetQueue()->clear();
        // TODO: the first frame (key frame) will not be decoded correctly if flush() is called.
        if (type == AccurateSeek) {
            Packet pkt;
            pkt.pts = qreal(pos)/1000.0;
            //qDebug("put seek packet. %d/%d-%d, progress: %.3f", pb->buffered(), pb->bufferValue(), pb->bufferMax(), pb->bufferProgress());
            t->packetQueue()->setBlocking(false); // aqueue bufferValue can be small (1), we can not put and take
            t->packetQueue()->put(pkt);
        }
        t->packetQueue()->setBlocking(true); // blockEmpty was false when eof is read.
    }
    if (isPaused() && (video_thread || audio_thread)) {
        AVThread *thread = video_thread ? video_thread : audio_thread;
        thread->pause(false);
        pauseInternal(false);
        emit requestClockPause(false); // need direct connection
        // direct connection is fine here
        connect(thread, SIGNAL(frameDelivered()), this, SLOT(frameDeliveredSeekOnPause()), Qt::DirectConnection);
    }
}

void AVDemuxThread::newSeekRequest(QRunnable *r)
{
    if (seek_tasks.size() >= seek_tasks.capacity()) {
        QRunnable *r = seek_tasks.take();
        if (r->autoDelete())
            delete r;
    }
    seek_tasks.put(r);
}

void AVDemuxThread::processNextSeekTask()
{
    if (seek_tasks.isEmpty())
        return;
    QRunnable *task = seek_tasks.take();
    if (!task)
        return;
    task->run();
    if (task->autoDelete())
        delete task;
}

void AVDemuxThread::pauseInternal(bool value)
{
    paused = value;
}

bool AVDemuxThread::isPaused() const
{
    return paused;
}

bool AVDemuxThread::isEnd() const
{
    return end;
}

PacketBuffer* AVDemuxThread::buffer()
{
    return m_buffer;
}

void AVDemuxThread::updateBufferState()
{
    if (!m_buffer)
        return;
    if (m_buffering) { // always report progress when buffering
        Q_EMIT bufferProgressChanged(m_buffer->bufferProgress());
    }
    if (m_buffering == m_buffer->isBuffering())
        return;
    m_buffering = m_buffer->isBuffering();
    Q_EMIT mediaStatusChanged(m_buffering ? QtAV::BufferingMedia : QtAV::BufferedMedia);
    // state change to buffering, report progress immediately. otherwise we have to wait to read 1 packet.
    if (m_buffering) {
        Q_EMIT bufferProgressChanged(m_buffer->bufferProgress());
    }
}

//No more data to put. So stop blocking the queue to take the reset elements
void AVDemuxThread::stop()
{
    //this will not affect the pause state if we pause the output
    //TODO: why remove blockFull(false) can not play another file?
    AVThread* av[] = { audio_thread, video_thread};
    for (size_t i = 0; i < sizeof(av)/sizeof(av[0]); ++i) {
        AVThread* t = av[i];
        if (!t)
            continue;
        t->packetQueue()->clear();
        t->packetQueue()->blockFull(false); //??
        while (t->isRunning()) {
            qDebug() << "stopping thread " << t;
            t->stop();
            t->wait(500);
        }
    }
    pause(false);
    cond.wakeAll();
    qDebug("all avthread finished. try to exit demux thread<<<<<<");
    end = true;
}

void AVDemuxThread::pause(bool p)
{
    if (paused == p)
        return;
    paused = p;
    user_paused = paused;
    if (!paused)
        cond.wakeAll();
}

void AVDemuxThread::nextFrame()
{
    // clock type will be wrong if no lock because slot frameDeliveredNextFrame() is in video thread
    QMutexLocker locker(&next_frame_mutex);
    Q_UNUSED(locker);
    pause(true); // must pause AVDemuxThread (set user_paused true)
    AVThread *t = video_thread;
    bool connected = false;
    if (t) {
        // set clock first
        if (clock_type < 0)
            clock_type = (int)t->clock()->isClockAuto() + 2*(int)t->clock()->clockType();
        t->clock()->setClockType(AVClock::VideoClock);
        ((VideoThread*)t)->scheduleFrameDrop(false);
        t->pause(false);
        t->packetQueue()->blockFull(false);
        if (!connected) {
            connect(t, SIGNAL(frameDelivered()), this, SLOT(frameDeliveredNextFrame()), Qt::DirectConnection);
            connected = true;
        }
    }
    t = audio_thread;
    if (t) {
        if (clock_type < 0)
            clock_type = (int)t->clock()->isClockAuto() + 2*(int)t->clock()->clockType();
        t->clock()->setClockType(AVClock::VideoClock);
        t->pause(false);
        t->packetQueue()->blockFull(false);
        if (!connected) {
            connect(t, SIGNAL(frameDelivered()), this, SLOT(frameDeliveredNextFrame()), Qt::DirectConnection);
            connected = true;
        }
    }
    emit requestClockPause(false);
    nb_next_frame.ref();
    pauseInternal(false);
}

void AVDemuxThread::frameDeliveredSeekOnPause()
{
    AVThread *thread = video_thread ? video_thread : audio_thread;
    Q_ASSERT(thread);
    disconnect(thread, SIGNAL(frameDelivered()), this, SLOT(frameDeliveredSeekOnPause()));
    if (user_paused) {
        pause(true); // restore pause state
        emit requestClockPause(true); // need direct connection
    // pause video/audio thread
        thread->pause(true);
    }
}

void AVDemuxThread::frameDeliveredNextFrame()
{
    AVThread *thread = video_thread ? video_thread : audio_thread;
    Q_ASSERT(thread);
    if (nb_next_frame.deref()) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0) || QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
        Q_ASSERT_X((int)nb_next_frame > 0, "frameDeliveredNextFrame", "internal error. frameDeliveredNextFrame must be > 0");
#else
        Q_ASSERT_X((int)nb_next_frame.load() > 0, "frameDeliveredNextFrame", "internal error. frameDeliveredNextFrame must be > 0");
#endif
        return;
    }
    QMutexLocker locker(&next_frame_mutex);
    Q_UNUSED(locker);
    disconnect(thread, SIGNAL(frameDelivered()), this, SLOT(frameDeliveredNextFrame()));
    if (user_paused) {
        pause(true); // restore pause state
        emit requestClockPause(true); // need direct connection
    // pause both video and audio thread
        if (video_thread)
            video_thread->pause(true);
        if (audio_thread)
            audio_thread->pause(true);
    }
    if (clock_type >= 0) {
        thread->clock()->setClockAuto(clock_type & 1);
        thread->clock()->setClockType(AVClock::ClockType(clock_type/2));
        clock_type = -1;
    }
}

void AVDemuxThread::onAVThreadQuit()
{
    AVThread* av[] = { audio_thread, video_thread};
    for (size_t i = 0; i < sizeof(av)/sizeof(av[0]); ++i) {
        if (!av[i])
            continue;
        if (av[i]->isRunning())
            return;
    }
    end = true; //(!audio_thread || !audio_thread->isRunning()) &&
}

void AVDemuxThread::run()
{
    m_buffering = false;
    end = false;
    if (audio_thread && !audio_thread->isRunning())
        audio_thread->start(QThread::HighPriority);
    if (video_thread && !video_thread->isRunning())
        video_thread->start();

    int index = 0;
    Packet pkt;
    pause(false);
    qDebug("get av queue a/v thread = %p %p", audio_thread, video_thread);
    PacketBuffer *aqueue = audio_thread ? audio_thread->packetQueue() : 0;
    PacketBuffer *vqueue = video_thread ? video_thread->packetQueue() : 0;
    // aqueue as a primary buffer: music with/without cover
    AVThread* thread = !video_thread || (audio_thread && demuxer->hasAttacedPicture()) ? audio_thread : video_thread;
    m_buffer = thread->packetQueue();
    const int buf2 = aqueue ? aqueue->bufferValue() : 1; // TODO: may be changed by user
    if (aqueue) {
        aqueue->clear();
        aqueue->setBlocking(true);
    }
    if (vqueue) {
        vqueue->clear();
        vqueue->setBlocking(true);
    }
    connect(thread, SIGNAL(seekFinished(qint64)), this, SIGNAL(seekFinished(qint64)), Qt::DirectConnection);
    seek_tasks.clear();
    bool was_end = false;
    while (!end) {
        processNextSeekTask();
        if (demuxer->atEnd()) {
            if (!was_end) {
                if (aqueue) {
                    aqueue->put(Packet::createEOF());
                    aqueue->blockEmpty(false); // do not block if buffer is not enough. block again on seek
                }
                if (vqueue) {
                    vqueue->put(Packet::createEOF());
                    vqueue->blockEmpty(false);
                }
            }
            m_buffering = false;
            Q_EMIT mediaStatusChanged(QtAV::BufferedMedia);
            was_end = true;
            // wait for a/v thread finished
            msleep(100);
            continue;
        }
        was_end = false;
        if (tryPause()) {
            continue; //the queue is empty and will block
        }
        updateBufferState();
        QMutexLocker locker(&buffer_mutex);
        Q_UNUSED(locker);
        if (!demuxer->readFrame()) {
            continue;
        }
        index = demuxer->stream();
        pkt = demuxer->packet();
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
        if (index == demuxer->audioStream()) {
            /* if vqueue if not blocked and full, and aqueue is empty, then put to
             * vqueue will block demuex thread
             */
            if (aqueue) {
                if (!audio_thread || !audio_thread->isRunning()) {
                    aqueue->clear();
                    continue;
                }
                if (m_buffer != aqueue) {
                    if (m_buffer->isBuffering()) {
                        aqueue->setBufferValue(std::numeric_limits<int>::max());
                    } else {
                        aqueue->setBufferValue(buf2);
                    }
                }
                // always block full if no vqueue because empty callback may set false
                // attached picture is cover for song, 1 frame
                aqueue->blockFull(!video_thread || !video_thread->isRunning() || !vqueue || demuxer->hasAttacedPicture());
                aqueue->put(pkt); //affect video_thread
            }
        } else if (index == demuxer->videoStream()) {
            if (vqueue) {
                if (!video_thread || !video_thread->isRunning()) {
                    vqueue->clear();
                    continue;
                }
                vqueue->blockFull(!audio_thread || !audio_thread->isRunning() || !aqueue || aqueue->isEnough());
                vqueue->put(pkt); //affect audio_thread
            }
        } else { //subtitle
            continue;
        }
    }
    m_buffering = false;
    m_buffer = 0;
    while (audio_thread && audio_thread->isRunning()) {
        qDebug("waiting audio thread.......");
        aqueue->blockEmpty(false); //FIXME: why need this
        audio_thread->wait(500);
    }
    while (video_thread && video_thread->isRunning()) {
        qDebug("waiting video thread.......");
        vqueue->blockEmpty(false);
        video_thread->wait(500);
    }
    disconnect(this, SIGNAL(seekFinished(qint64)));
    qDebug("Demux thread stops running....");
    emit mediaStatusChanged(QtAV::EndOfMedia);
}

bool AVDemuxThread::tryPause(unsigned long timeout)
{
    if (!paused)
        return false;
    QMutexLocker lock(&buffer_mutex);
    Q_UNUSED(lock);
    cond.wait(&buffer_mutex, timeout);
    return true;
}

} //namespace QtAV
