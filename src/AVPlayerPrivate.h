/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2014-2017 Wang Bin <wbsecg1@gmail.com>

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

    bool checkSourceChange();
    void updateNotifyInterval();
    void applyFrameRate();
    void initStatistics();
    void initBaseStatistics();
    void initCommonStatistics(int s, Statistics::Common* st, AVCodecContext* avctx);
    void initAudioStatistics(int s);
    void initVideoStatistics(int s);
    void initSubtitleStatistics(int s);
    QVariantList getTracksInfo(AVDemuxer* demuxer, AVDemuxer::StreamType st);

    bool applySubtitleStream(int n, AVPlayer *player);
    bool setupAudioThread(AVPlayer *player);
    bool setupVideoThread(AVPlayer *player);
    bool tryApplyDecoderPriority(AVPlayer *player);
    // TODO: what if buffer mode changed during playback?
    void updateBufferValue(PacketBuffer *buf);
    void updateBufferValue();
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
    bool relative_time_mode;
    qint64 media_start_pts; // read from media stream
    qint64 media_end;
    bool reset_state;
    qint64 start_position, stop_position;
    qint64 start_position_norm, stop_position_norm; // real position
    qint64 last_known_good_pts;
    bool was_stepping;
    int repeat_max, repeat_current;
    int timer_id; //notify position change and check AB repeat range. active when playing

    int audio_track, video_track, subtitle_track;
    QVariantList subtitle_tracks;
    QVariantList video_tracks;
    QString external_audio;
    AVDemuxer audio_demuxer;
    QVariantList external_audio_tracks;
    QVariantList audio_tracks;
    BufferMode buffer_mode;
    qint64 buffer_value;
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
    OutputSet *vos, *aos;
    QVector<VideoDecoderId> vc_ids;
    int brightness, contrast, saturation;

    QVariantHash ac_opt, vc_opt;

    bool seeking;
    SeekType seek_type;
    qint64 interrupt_timeout;

    qreal force_fps;
    // timerEvent interval in ms. can divide 1000. depends on media duration, fps etc.
    // <0: auto compute internally, |notify_interval| is the real interval
    int notify_interval;
    MediaStatus status; // status changes can be from demuxer or demux thread
    AVPlayer::State state;
    MediaEndAction end_action;
    QMutex load_mutex;
};

} //namespace QtAV
#endif // QTAV_AVPLAYER_PRIVATE_H
