/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_AVPLAYER_PRIVATE_H
#define QTAV_AVPLAYER_PRIVATE_H

#include <limits>
#include "QtAV/AVDemuxer.h"
#include "QtAV/AVPlayer.h"
#include "AudioThread.h"
#include "VideoThread.h"
#include "AVDemuxThread.h"
#include "utils/Logger.h"

namespace QtAV {

static const qint64 kInvalidPosition = std::numeric_limits<qint64>::max();
class AVPlayer::Private
{
public:
    Private();
    ~Private();

    void initStatistics(AVPlayer *player);
    bool setupAudioThread(AVPlayer *player);
    bool setupVideoThread(AVPlayer *player);


    //TODO: addAVOutput()
    template<class Out>
    void setAVOutput(Out *&pOut, Out *pNew, AVThread *thread) {
        Out *old = pOut;
        bool delete_old = false;
        if (pOut == pNew) {
            qDebug("output not changed: %p", pOut);
            if (thread && thread->output() == pNew) {//avthread already set that output
                qDebug("avthread already set that output");
                return;
            }
        } else {
            pOut = pNew;
            delete_old = true;
        }
        if (!thread) {
            qDebug("avthread not ready. can not set output.");
            //no avthread, we can delete it safely
            //AVOutput must be allocated in heap. Just like QObject's children.
            if (delete_old) {
                delete old;
                old = 0;
            }
            return;
        }
        //FIXME: what if isPaused()==false but pause(true) in another thread?
        //bool need_lock = isPlaying() && !thread->isPaused();
        //if (need_lock)
        //    thread->lock();
        qDebug("set AVThread output");
        thread->setOutput(pOut);
        if (pOut) {
            pOut->setStatistics(&statistics);
            //if (need_lock)
            //    thread->unlock(); //??why here?
        }
        //now the old avoutput is not used by avthread, we can delete it safely
        //AVOutput must be allocated in heap. Just like QObject's children.
        if (delete_old) {
            delete old;
            old = 0;
        }
    }


    bool auto_load;
    bool async_load;
    // can be QString, QIODevice*
    QVariant current_source, pendding_source;
    bool loaded; // for current source
    AVFormatContext	*fmt_ctx; //changed when reading a packet
    qint64 media_end;
    /*
     * unit: s. 0~1. stream's start time/duration(). or last position/duration() if change to new stream
     * auto set to 0 if stop(). to stream start time if load()
     *
     * -1: used by play() to get current playing position
     */
    qint64 last_position; //last_pos
    bool reset_state;
    qint64 start_position, stop_position;
    int repeat_max, repeat_current;
    int timer_id; //notify position change and check AB repeat range. active when playing

    //the following things are required and must be set not null
    AVDemuxer demuxer;
    AVDemuxThread *read_thread;
    AVClock *clock;
    VideoRenderer *vo; //list? // TODO: remove
    AudioOutput *ao; // TODO: remove
    AudioDecoder *adec;
    VideoDecoder *vdec;
    AudioThread *athread;
    VideoThread *vthread;

    VideoCapture *vcapture;
    Statistics statistics;
    qreal speed;
    bool ao_enabled;
    OutputSet *vos, *aos;
    QVector<VideoDecoderId> vc_ids;
    QVector<AudioOutputId> ao_ids;
    int brightness, contrast, saturation;

    QVariantHash ac_opt, vc_opt;

    bool seeking;
    qint64 seek_target;
    qint64 interrupt_timeout;
    bool mute;
};

} //namespace QtAV
#endif // QTAV_AVPLAYER_PRIVATE_H
