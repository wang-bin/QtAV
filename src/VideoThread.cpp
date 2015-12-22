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

#include "VideoThread.h"
#include "AVThread_p.h"
#include "QtAV/Packet.h"
#include "QtAV/AVClock.h"
#include "QtAV/VideoCapture.h"
#include "QtAV/VideoDecoder.h"
#include "QtAV/VideoRenderer.h"
#include "QtAV/Statistics.h"
#include "QtAV/Filter.h"
#include "QtAV/FilterContext.h"
#include "output/OutputSet.h"
#include "VideoFilterThread.h"
#include "QtAV/private/AVCompat.h"
#include <QtCore/QFileInfo>
#include "utils/Logger.h"

namespace QtAV {

class VideoThreadPrivate : public AVThreadPrivate
{
public:
    VideoThreadPrivate():
        AVThreadPrivate()
      , force_fps(0)
      , force_dt(0)
      , capture(0)
    {
    }

    VideoFrameConverter conv;
    qreal force_fps; // <=0: try to use pts. if no pts in stream(guessed by 5 packets), use |force_fps|
    // not const.
    int force_dt; //unit: ms. force_fps = 1/force_dt.

    double pts; //current decoded pts. for capture. TODO: remove
    VideoCapture *capture;
    VideoFrame displayed_frame;
    VideoFilterThread *filter_thread;
};

VideoThread::VideoThread(QObject *parent) :
    AVThread(*new VideoThreadPrivate(), parent)
{
    d_func().filter_thread = new VideoFilterThread(this);
    connect(d_func().filter_thread, SIGNAL(frameDelivered()), this , SIGNAL(frameDelivered()), Qt::DirectConnection);
}

//it is called in main thread usually, but is being used in video thread,
VideoCapture* VideoThread::setVideoCapture(VideoCapture *cap)
{
    qDebug("setCapture %p", cap);
    DPTR_D(VideoThread);
    QMutexLocker locker(&d.mutex);
    VideoCapture *old = d.capture;
    d.capture = cap;
    if (old)
        disconnect(old, SIGNAL(requested()), this, SLOT(addCaptureTask()));
    if (cap)
        connect(cap, SIGNAL(requested()), this, SLOT(addCaptureTask()));
    if (cap->autoSave() && cap->name.isEmpty()) {
        // statistics is already set by AVPlayer
        cap->setCaptureName(QFileInfo(d.statistics->url).completeBaseName());
    }
    return old;
}

VideoCapture* VideoThread::videoCapture() const
{
    return d_func().capture;
}

void VideoThread::addCaptureTask()
{
    if (!isRunning())
        return;
    class CaptureTask : public QRunnable {
    public:
        CaptureTask(VideoThread *vt) : vthread(vt) {}
        void run() {
            VideoCapture *vc = vthread->videoCapture();
            if (!vc)
                return;
            VideoFrame frame(vthread->displayedFrame());
            //vthread->applyFilters(frame);
            vc->setVideoFrame(frame);
            vc->start();
        }
    private:
        VideoThread *vthread;
    };
    scheduleTask(new CaptureTask(this));
}

VideoFrame VideoThread::displayedFrame() const
{
    return d_func().displayed_frame;
}

void VideoThread::setFrameRate(qreal value)
{
    DPTR_D(VideoThread);
    d.force_fps = value;
    if (d.force_fps != 0.0) {
        d.force_dt = int(1000.0/d.force_fps);
    } else {
        d.force_dt = 0;
    }
}

void VideoThread::setBrightness(int val)
{
    setEQ(val, 101, 101);
}

void VideoThread::setContrast(int val)
{
    setEQ(101, val, 101);
}

void VideoThread::setSaturation(int val)
{
    setEQ(101, 101, val);
}

void VideoThread::setEQ(int b, int c, int s)
{
    class EQTask : public QRunnable {
    public:
        EQTask(VideoFrameConverter *c)
            : brightness(0)
            , contrast(0)
            , saturation(0)
            , conv(c)
        {
            //qDebug("EQTask tid=%p", QThread::currentThread());
        }
        void run() {
            conv->setEq(brightness, contrast, saturation);
        }
        int brightness, contrast, saturation;
    private:
        VideoFrameConverter *conv;
    };
    DPTR_D(VideoThread);
    EQTask *task = new EQTask(&d.conv);
    task->brightness = b;
    task->contrast = c;
    task->saturation = s;
    if (isRunning()) {
        scheduleTask(task);
    } else {
        task->run();
        delete task;
    }
}

void VideoThread::prepareSeek()
{
    DPTR_D(VideoThread);
    if (d.filter_thread) {
        d.filter_thread->frameQueue()->clear();
    }
}

void VideoThread::onUpdateFilters()
{
    qDebug("update filters");
    DPTR_D(VideoThread);
    d.filter_thread->setFilters(d.filters);
}

void VideoThread::onStop()
{
    DPTR_D(VideoThread);
    // stop by user. clear frame queue to avoid blocking at put()
    d.filter_thread->frameQueue()->clear();
}

//TODO: if output is null or dummy, the use duration to wait
void VideoThread::run()
{
    DPTR_D(VideoThread);
    if (!d.dec || !d.dec->isAvailable() || !d.outputSet)
        return;
    d.filter_thread->setClock(d.clock);
    d.filter_thread->setStatistics(d.statistics);
    d.filter_thread->setOutputSet(d.outputSet);
    d.filter_thread->frameQueue()->clear();
    d.filter_thread->start();

    resetState();
    if (d.capture->autoSave()) {
        d.capture->setCaptureName(QFileInfo(d.statistics->url).completeBaseName());
    }
    //not neccesary context is managed by filters.
    VideoDecoder *dec = static_cast<VideoDecoder*>(d.dec);
    Packet pkt;
    QVariantHash *dec_opt = &d.dec_opt_normal; //TODO: restore old framedrop option after seek
    /*!
     * if we skip some frames(e.g. seek, drop frames to speed up), then then first frame to decode must
     * be a key frame for hardware decoding. otherwise may crash
     */
    bool wait_key_frame = false;
    int nb_dec_slow = 0;
    int nb_dec_fast = 0;

    qint32 seek_count = 0; // wm4 says: 1st seek can not use frame drop for decoder
    // TODO: kNbSlowSkip depends on video fps, ensure slow time <= 2s
    /* kNbSlowSkip: if video frame slow count >= kNbSlowSkip, skip decoding all frames until next keyframe reaches.
     * if slow count > kNbSlowSkip/2, skip rendering every 3 or 6 frames
     */
    const int kNbSlowSkip = 120; // about 1s for 120fps video
    // kNbSlowFrameDrop: if video frame slow count > kNbSlowFrameDrop, skip decoding nonref frames. only some of ffmpeg based decoders support it.
    const int kNbSlowFrameDrop = 10;
    bool sync_audio = d.clock->clockType() == AVClock::AudioClock;
    bool sync_video = d.clock->clockType() == AVClock::VideoClock; // no frame drop
    const qint64 start_time = QDateTime::currentMSecsSinceEpoch();
    bool skip_render = false; // keep true if decoded frame does not reach desired time
    qreal v_a = 0;
    int nb_no_pts = 0;
    //bool wait_audio_drain
    const char* pkt_data = NULL; // workaround for libav9 decode fail but error code >= 0
    qint64 last_deliver_time = 0;
    while (!d.stop) {
        processNextTask();
        //TODO: why put it at the end of loop then stepForward() not work?
        //processNextTask tryPause(timeout) and  and continue outter loop
        if (tryPause()) { //DO NOT continue, or stepForward() will fail
        } else {
            if (isPaused())
                continue; //timeout. process pending tasks
        }
        if(!pkt.isValid() && !pkt.isEOF()) { // can't seek back if eof packet is read
            pkt = d.packets.take(); //wait to dequeue
           // TODO: push pts history here and reorder
        }
        if (pkt.isEOF()) {
            d.render_pts0 = -1;
            wait_key_frame = false;
            qDebug("video thread gets an eof packet.");
        } else {
            if (d.stop) // user stop
                break;
            //qDebug() << pkt.position << " pts:" <<pkt.pts;
            //Compare to the clock
            if (!pkt.isValid()) {
                // may be we should check other information. invalid packet can come from
                wait_key_frame = true;
                qDebug("Invalid packet! flush video codec context!!!!!!!!!! video packet queue size: %d", d.packets.size());  
                d.dec->flush(); //d.dec instead of dec because d.dec maybe changed in processNextTask() but dec is not
                d.render_pts0 = pkt.pts;
                d.pts_history = ring<qreal>(d.pts_history.capacity(), -1);
                v_a = 0;
                d.filter_thread->frameQueue()->clear();
                continue;
            }
        }
        if (pkt.pts <= 0) {
            nb_no_pts++;
        } else {
            nb_no_pts = 0;
        }
        if (nb_no_pts > 5) {
            qDebug("the stream may have no pts. force fps to: %f", d.force_fps < 0 ? -d.force_fps : 24);
            d.clock->setClockAuto(false);
            d.clock->setClockType(AVClock::VideoClock);
            if (d.force_fps < 0)
                setFrameRate(-d.force_fps);
            else if (d.force_fps == 0)
                setFrameRate(24);
        }

        if (d.clock->clockType() == AVClock::AudioClock) {
            sync_audio = true;
            sync_video = false;
        } else if (d.clock->clockType() == AVClock::VideoClock) {
            sync_audio = false;
            sync_video = true;
        } else {
            sync_audio = false;
            sync_video = false;
        }
        const qreal dts = pkt.dts; //FIXME: pts and dts
        // TODO: delta ref time
        // if dts is invalid, diff can be very small (<0) and video will be decoded and rendered(display_wait is disabled for now) immediately
        qreal diff = dts > 0 ? dts - d.clock->value() + v_a : v_a;
        if (pkt.isEOF())
            diff = qMin<qreal>(1.0, qMax<qreal>(d.delay, 1.0/d.statistics->video_only.currentDisplayFPS()));
        if (diff < 0 && sync_video)
            diff = 0; // this ensures no frame drop
#if 1
        // can not reset diff to avoid breaking frame drop logic
         //qDebug("vthread diff: %.3f v-a: %.3f, frame_dec_time: %.3f, frame_filter_time: %.3f", diff, v_a, frame_dec_time, frame_filter_time);
         //if (d.outputSet->videFrameRequired()) { //|| d.filter_thread->delay() < 0 || d.filter_thread->frameQueue().size() < 2
              //qDebug("video frame required");
              //diff = 0;
          //}
#endif
        if (diff > kSyncThreshold) {
            nb_dec_fast++;
        } else {
            nb_dec_fast /= 2;
        }
        bool seeking = d.render_pts0 >= 0.0;
        if (seeking) {
            nb_dec_slow = 0;
            nb_dec_fast = 0;
        }
        //qDebug("nb_fast/slow: %d/%d. diff: %f, delay: %f, dts: %f, clock: %f", nb_dec_fast, nb_dec_slow, diff, d.delay, dts, clock()->value());
        if (d.delay < -0.5 && d.delay > diff) {
            if (!seeking) {
                // ensure video will not later than 2s
                if (diff < -2 || (nb_dec_slow > kNbSlowSkip && diff < -1.0 && !pkt.hasKeyFrame)) {
                    qDebug("video is too slow. skip decoding until next key frame.");
                    // TODO: when to reset so frame drop flag can reset?
                    nb_dec_slow = 0;
                    wait_key_frame = true;
                    pkt = Packet();
                    // TODO: use discard flag
                    continue;
                } else {
                    nb_dec_slow++;
                    qDebug("frame slow count: %d. v-a: %.3f", nb_dec_slow, diff);
                }
            }
        } else {
            if (nb_dec_slow > kNbSlowFrameDrop) {
                qDebug("decrease 1 slow frame: %d", nb_dec_slow);
                nb_dec_slow = qMax(0, nb_dec_slow-1); // nb_dec_slow < kNbSlowFrameDrop will reset decoder frame drop flag
            }
        }
        // can not change d.delay after! we need it to comapre to next loop
        d.delay = diff;
        /*
         *after seeking forward, a packet may be the old, v packet may be
         *the new packet, then the d.delay is very large, omit it.
        */
        if (seeking)
            diff = 0; // TODO: here?
        if (!sync_audio && diff > 0) {
            // wait to dts reaches
            // d.force_fps>0: wait after decoded before deliver
            //if (d.force_fps <= 0)// || !qFuzzyCompare(d.clock->speed(), 1.0))
              //  waitAndCheck(diff*1000UL, dts); // TODO: count decoding and filter time, or decode immediately but wait for display
            diff = 0; // TODO: can not change delay!
        }
        // update here after wait. TODO: use decoded timestamp/guessed next pts?
        d.clock->updateVideoTime(dts); // FIXME: dts or pts?
        if (qAbs(diff) < 0.5) {
            if (diff < -kSyncThreshold) { //Speed up. drop frame?
                //continue;
            }
        } else if (!seeking) { //when to drop off?
            qDebug("delay %fs @%fs", diff, d.clock->value());
            if (diff < 0) {
                if (!pkt.hasKeyFrame) {
                    // if continue without decoding, we must wait to the next key frame, then we may skip to many frames
                    //wait_key_frame = true;
                    //pkt = Packet();
                    //continue;
                }
                skip_render = !pkt.hasKeyFrame;
                if (skip_render) {
                    if (nb_dec_slow < kNbSlowSkip/2) {
                        skip_render = false;
                    } else if (nb_dec_slow < kNbSlowSkip) {
                        const int skip_every = kNbSlowSkip >= 60 ? 2 : 4;
                        skip_render = nb_dec_slow % skip_every; // skip rendering every 3 frames
                    }
                }
            } else {
                const double s = qMin<qreal>(0.01*(nb_dec_fast>>1), diff);
                qWarning("video too fast!!! sleep %.2f s, nb fast: %d, v_a: %.4f", s, nb_dec_fast, v_a);
                //waitAndCheck(s*1000UL, dts);
                diff = 0;
            }
        }
        //audio packet not cleaned up?
        if (diff > 0 && diff < 1.0 && !seeking) {
            // can not change d.delay here! we need it to comapre to next loop
            //waitAndCheck(diff*1000UL, dts);
        }
        if (wait_key_frame) {
            if (!pkt.hasKeyFrame) {
                qDebug("waiting for key frame. queue size: %d. pkt.size: %d", d.packets.size(), pkt.data.size());
                pkt = Packet();
                continue;
            }
            wait_key_frame = false;
        }
        QVariantHash *dec_opt_old = dec_opt;
        if (d.drop_frame_seek) {
            if (!seeking) { // MAYBE not seeking
                if (nb_dec_slow < kNbSlowFrameDrop) {
                    if (dec_opt == &d.dec_opt_framedrop) {
                        qDebug("frame drop normal. nb_dec_slow: %d. not seeking", nb_dec_slow);
                        dec_opt = &d.dec_opt_normal;
                    }
                } else {
                    if (dec_opt == &d.dec_opt_normal) {
                        qDebug("frame drop noref. nb_dec_slow: %d too slow. not seeking", nb_dec_slow);
                        dec_opt = &d.dec_opt_framedrop;
                    }
                }
            } else { // seeking
                if (seek_count > 0) {
                    if (dec_opt == &d.dec_opt_normal) {
                        qDebug("seeking... frame drop noref. nb_dec_slow: %d", nb_dec_slow);
                        dec_opt = &d.dec_opt_framedrop;
                    }
                } else {
                    seek_count = -1;
                }
            }
        } else {
            dec_opt = &d.dec_opt_normal;
        }

        // decoder maybe changed in processNextTask(). code above MUST use d.dec but not dec
        if (dec != static_cast<VideoDecoder*>(d.dec)) {
            dec = static_cast<VideoDecoder*>(d.dec);
            if (!pkt.hasKeyFrame) {
                wait_key_frame = true;
                continue;
            }
            qDebug("decoder changed. decoding key frame");
        }
        if (dec_opt != dec_opt_old)
            dec->setOptions(*dec_opt);
        if (!dec->decode(pkt)) {
            d.pts_history.push_back(d.pts_history.back());
            qWarning("Decode video failed. undecoded: %d", dec->undecodedSize());
            if (pkt.isEOF()) {
                qDebug("decode eof done");
                break;
            }
            pkt = Packet();
            continue;
        }
        // reduce here to ensure to decode the rest data in the next loop
        if (!pkt.isEOF())
            pkt.data = QByteArray::fromRawData(pkt.data.constData() + pkt.data.size() - dec->undecodedSize(), dec->undecodedSize());
        VideoFrame frame = dec->frame();
        if (!frame.isValid()) {
            qWarning("invalid video frame from decoder. undecoded data size: %d", pkt.data.size());
            if (pkt_data == pkt.data.constData()) //FIXME: for libav9. what about other versions?
                pkt = Packet();
            else
                pkt_data = pkt.data.constData();
            continue;
        }
        pkt_data = pkt.data.constData();
        if (frame.timestamp() <= 0)
            frame.setTimestamp(pkt.pts); // pkt.pts is wrong. >= real timestamp
        const qreal pts = frame.timestamp();
        d.pts_history.push_back(pts);
        // seek finished because we can ensure no packet before seek decoded when render_pts0 is set
        //qDebug("pts0: %f, pts: %f, clock: %d", d.render_pts0, pts, d.clock->clockType());
        if (d.render_pts0 >= 0.0) {
            if (pts < d.render_pts0) {
                if (!pkt.isEOF())
                    pkt = Packet();
                continue;
            }
            d.render_pts0 = -1;
            qDebug("seek finished @%f", pts);
            Q_EMIT seekFinished(qint64(pts*1000.0));
            if (seek_count == -1)
                seek_count = 1;
            else if (seek_count > 0)
                seek_count++;
            skip_render = false;
            d.filter_thread->frameQueue()->clear();
        }
        if (skip_render) {
            qDebug("skip rendering @%.3f", pts);
            pkt = Packet();
        } else {
            // TODO: reorder is required by VT
            qDebug("frame queue size: %d", d.filter_thread->frameQueue()->size());
            d.filter_thread->frameQueue()->put(frame);
            qDebug("frame queueded");
        }
        continue;
        //Q_ASSERT(d.statistics);
        // TODO: current_dec_time
        //d.statistics->video.current_time = QTime(0, 0, 0).addMSecs(int(pts * 1000.0)); //TODO: is it expensive?

        //qDebug("force fps: %f dt: %d", d.force_fps, d.force_dt);
        if (d.force_dt > 0) {// && qFuzzyCompare(d.clock->speed(), 1.0)) {
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            const qint64 delta = qint64(d.force_dt) - (now - last_deliver_time);
            if (frame.timestamp() <= 0) {
                // TODO: what if seek happens during playback?
                const int msecs_started(now + qMax(0LL, delta) - start_time);
                frame.setTimestamp(qreal(msecs_started)/1000.0);
                clock()->updateValue(frame.timestamp());
            }
            if (delta > 0LL) { // limit up bound?
                waitAndCheck((ulong)delta, -1); // wait and not compare pts-clock
            }
        } else if (false) { //FIXME: may block a while when seeking
            const qreal display_wait = pts - clock()->value();
            if (!seeking && display_wait > 0.0) {
                // wait to pts reaches. TODO: count rendering time
                //qDebug("wait %f to display for pts %f-%f", display_wait, pts, clock()->value());
                if (display_wait < 1.0)
                    waitAndCheck(display_wait*1000UL, pts); // TODO: count decoding and filter time
            }
        }
        // no return even if d.stop is true. ensure frame is displayed. otherwise playing an image may be failed to display
        //if (!deliverVideoFrame(frame))
          //  continue;
        if (d.force_dt > 0)
            last_deliver_time = QDateTime::currentMSecsSinceEpoch();
        // TODO: store original frame. now the frame is filtered and maybe converted to renderer perferred format
        d.displayed_frame = frame;
    }
    d.packets.clear();
    d.filter_thread->frameQueue()->put(VideoFrame()); //ensure no block in take()
    qDebug("Video thread stops running...");
}

} //namespace QtAV
