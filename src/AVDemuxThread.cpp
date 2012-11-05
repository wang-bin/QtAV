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

#include <QtAV/AVDemuxThread.h>
#include <QtAV/AVDemuxer.h>
#include <QtAV/Packet.h>
#include <QtAV/AVThread.h>

namespace QtAV {

const int kPktBufferSize = 512;

AVDemuxThread::AVDemuxThread(QObject *parent) :
    QThread(parent),end(false),demuxer(0),audio_thread(0),video_thread(0)
{
}

AVDemuxThread::AVDemuxThread(AVDemuxer *dmx, QObject *parent) :
    QThread(parent),end(false),audio_thread(0),video_thread(0)
{
    setDemuxer(dmx);
}

void AVDemuxThread::setDemuxer(AVDemuxer *dmx)
{
    demuxer = dmx;
    connect(dmx, SIGNAL(finished()), this, SLOT(stop()), Qt::QueuedConnection);
}

void AVDemuxThread::setAudioThread(AVThread *thread)
{
    if (audio_thread) {
        audio_thread->stop();
        delete audio_thread;
        audio_thread = 0;
    }
    audio_thread = thread;
    audio_thread->setDemuxThread(this);
}

void AVDemuxThread::setVideoThread(AVThread *thread)
{
    if (video_thread) {
        video_thread->stop();
        delete video_thread;
        video_thread = 0;
    }
    video_thread = thread;
    video_thread->setDemuxThread(this);
}

void AVDemuxThread::readMoreFrames()
{
    buffer_cond.wakeAll();
}

void AVDemuxThread::stop()
{
    end = true;
}

void AVDemuxThread::run()
{
    end = false;
    Q_ASSERT(audio_thread != 0);
    Q_ASSERT(video_thread != 0);
    if (!audio_thread->isRunning())
        audio_thread->start(QThread::HighPriority);
    if (!video_thread->isRunning())
        video_thread->start();

    audio_stream = demuxer->audioStream();
    video_stream = demuxer->videoStream();
    //demuxer->read()
    //enqueue()
    int index = 0;
    Packet pkt;
    while (!end) {
        buffer_mutex.lock();
        if (audio_thread->packetQueue()->size() > kPktBufferSize
                && video_thread->packetQueue()->size() > kPktBufferSize) {
            buffer_cond.wait(&buffer_mutex);
        }
        if (!demuxer->readFrame()) {
            buffer_mutex.unlock();
            continue;
        }
        index = demuxer->stream();
        pkt = *demuxer->packet(); //TODO: how to avoid additional copy?
        if (index == audio_stream) {
            audio_thread->packetQueue()->enqueue(pkt);
            audio_thread->wakeAll();
        } else if (index == video_stream) {
            video_thread->packetQueue()->enqueue(pkt);
            video_thread->wakeAll(); //need it?
        } else { //subtitle
            buffer_mutex.unlock();
            continue;
        }
        buffer_mutex.unlock();
    }
    qDebug("Demux thread stops running....");
}

} //namespace QtAV
