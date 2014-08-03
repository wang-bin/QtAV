/******************************************************************************
	QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>
    
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


#ifndef QTAV_AUDIOOUTPUT_P_H
#define QTAV_AUDIOOUTPUT_P_H

#include <QtAV/private/AVOutput_p.h>
#include <QtAV/AudioFrame.h>
#include <QtCore/QQueue>
#include <QtCore/QVector>
#include <limits>
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
#include <QtCore/QElapsedTimer>
#else
#include <QtCore/QTime>
typedef QTime QElapsedTimer;
#endif
#define AO_USE_TIMER 1
// TODO:writeChunk(), write(QByteArray) { while writeChunk() }, users define writeChunk()
namespace QtAV {

// chunk
const int kBufferSize = 1024*4;
const int kBufferCount = 8;

class Q_AV_PRIVATE_EXPORT AudioOutputPrivate : public AVOutputPrivate
{
public:
    AudioOutputPrivate():
        mute(false)
      , vol(1)
      , speed(1.0)
      , nb_buffers(8)
      , buffer_size(kBufferSize)
      , control(0)
      , features(0)
      , play_pos(0)
      , processed_remain(0)
      , index_enqueue(-1)
      , index_deuqueue(-1)
    {
        frame_infos.resize(nb_buffers);
    }
    virtual ~AudioOutputPrivate(){}

    void onCallback() { cond.wakeAll();}
    virtual void uwait(qint64 us) {
        QMutexLocker lock(&mutex);
        Q_UNUSED(lock);
        cond.wait(&mutex, (us+500LL)/1000LL);
    }

    int bufferSizeTotal() { return nb_buffers * kBufferSize; }
    typedef struct {
        qreal timestamp;
        int data_size;
    } FrameInfo;

    FrameInfo& currentEnqueueInfo() {
        Q_ASSERT(index_enqueue >= 0);
        return frame_infos[index_enqueue%frame_infos.size()];
    }
    FrameInfo& nextEnqueueInfo() { return frame_infos[(index_enqueue + 1)%frame_infos.size()]; }
    const FrameInfo& nextEnqueueInfo() const { return frame_infos[(index_enqueue + 1)%frame_infos.size()]; }
    FrameInfo& currentDequeueInfo() {
        Q_ASSERT(index_deuqueue >= 0);
        return frame_infos[index_deuqueue%frame_infos.size()];
    }
    FrameInfo& nextDequeueInfo() { return frame_infos[(index_deuqueue+1)%frame_infos.size()]; }
    const FrameInfo& nextDequeueInfo() const { return frame_infos[(index_deuqueue+1)%frame_infos.size()]; }
    void resetBuffers() {
        index_enqueue = -1;
        index_deuqueue = -1;
    }
    bool isBufferReseted() const { return index_enqueue == -1 && index_deuqueue == -1; }
    bool canAddBuffer() {
        return isBufferReseted() || (index_enqueue - index_deuqueue  < frame_infos.size()&& index_deuqueue >= 0);
        return isBufferReseted() || (!!((index_enqueue - index_deuqueue + 1) % frame_infos.size()) && index_deuqueue >= 0);
    }
    void bufferAdded() {
        if (index_enqueue < 0 || index_enqueue == std::numeric_limits<int>::max())
            index_enqueue = 0;
        else
            ++index_enqueue;
        return;
        index_enqueue = (index_enqueue + 1) % frame_infos.size();
    }
    bool canRemoveBuffer() {
        return index_enqueue > index_deuqueue;
    }
    void bufferRemoved() {
        if (index_deuqueue == index_enqueue)
            return;
        if (index_deuqueue < 0 || index_deuqueue == std::numeric_limits<int>::max())
            index_deuqueue = 0;
        else
            ++index_deuqueue;
        return;
        index_deuqueue = (index_deuqueue + 1) % frame_infos.size();
    }
    void resetStatus() {
        play_pos = 0;
        processed_remain = 0;
#if AO_USE_TIMER
        timer.invalidate();
#endif
        resetBuffers();
        frame_infos.clear();
        frame_infos.resize(nb_buffers);
    }

    bool mute;
    qreal vol;
    qreal speed;
    AudioFormat format;
    QByteArray data;
    AudioFrame audio_frame;
    quint32 nb_buffers;
    qint32 buffer_size;
    int control;
    int features;
    int play_pos; // index or bytes
    int processed_remain;
#if AO_USE_TIMER
    QElapsedTimer timer;
#endif
private:
    // the index of current enqueue/dequeue
    int index_enqueue, index_deuqueue;
    QVector<FrameInfo> frame_infos;
};

} //namespace QtAV
#endif // QTAV_AUDIOOUTPUT_P_H
