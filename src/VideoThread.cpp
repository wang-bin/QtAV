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

#include "VideoThread.h"
#include "AVThread_p.h"
#include "QtAV/Packet.h"
#include "QtAV/AVClock.h"
#include "QtAV/VideoCapture.h"
#include "QtAV/VideoDecoder.h"
#include "QtAV/VideoRenderer.h"
#include "QtAV/ImageConverter.h"
#include "QtAV/Statistics.h"
#include "QtAV/Filter.h"
#include "QtAV/FilterContext.h"
#include "QtAV/ImageConverterTypes.h"
#include "output/OutputSet.h"
#include "QtAV/private/AVCompat.h"
#include <QtCore/QFileInfo>
#include "utils/Logger.h"

namespace QtAV {

class VideoThreadPrivate : public AVThreadPrivate
{
public:
    VideoThreadPrivate():
        AVThreadPrivate()
      , conv(0)
      , capture(0)
      , filter_context(0)
    {
        conv = ImageConverterFactory::create(ImageConverterId_FF); //TODO: set in AVPlayer
        conv->setOutFormat(VideoFormat::Format_RGB32); //vo->defaultFormat

        QVariantHash opt;
        opt["skip_frame"] = 8; // 8 for "avcodec", "NoRef" for "FFmpeg". see AVDiscard
        dec_opt_framedrop["avcodec"] = opt;
        opt["skip_frame"] = 0; // 0 for "avcodec", "Default" for "FFmpeg". see AVDiscard
        dec_opt_normal["avcodec"] = opt; // avcodec need correct string or value in libavcodec
    }
    ~VideoThreadPrivate() {
        if (conv) {
            delete conv;
            conv = 0;
        }
        //not neccesary context is managed by filters.
        filter_context = 0;
    }

    ImageConverter *conv;
    double pts; //current decoded pts. for capture. TODO: remove
    //QImage image; //use QByteArray? Then must allocate a picture in ImageConverter, see VideoDecoder
    VideoCapture *capture;
    VideoFilterContext *filter_context;//TODO: use own smart ptr. QSharedPointer "=" is ugly
    VideoFrame displayed_frame;
    static QVariantHash dec_opt_framedrop, dec_opt_normal;
};

QVariantHash VideoThreadPrivate::dec_opt_framedrop;
QVariantHash VideoThreadPrivate::dec_opt_normal;

VideoThread::VideoThread(QObject *parent) :
    AVThread(*new VideoThreadPrivate(), parent)
{
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
        EQTask(ImageConverter *c)
            : brightness(0)
            , contrast(0)
            , saturation(0)
            , conv(c)
        {
            //qDebug("EQTask tid=%p", QThread::currentThread());
        }
        void run() {
            // check here not in ctor because it may called later in video thread
            if (brightness < -100 || brightness > 100)
                brightness = conv->brightness();
            if (contrast < -100 || contrast > 100)
                contrast = conv->contrast();
            if (saturation < -100 || saturation > 100)
                saturation = conv->saturation();
            //qDebug("EQTask::run() tid=%p (b: %d, c: %d, s: %d)"
               //    , QThread::currentThread(), brightness, contrast, saturation);
            conv->setBrightness(brightness);
            conv->setContrast(contrast);
            conv->setSaturation(saturation);
        }
        int brightness, contrast, saturation;
    private:
        ImageConverter *conv;
    };
    DPTR_D(VideoThread);
    EQTask *task = new EQTask(d.conv);
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

void VideoThread::scheduleFrameDrop(bool value)
{
    class FrameDropTask : public QRunnable {
        AVDecoder *decoder;
        bool drop;
    public:
        FrameDropTask(AVDecoder *dec, bool value) : decoder(dec), drop(value) {}
        void run() Q_DECL_OVERRIDE {
            if (!decoder)
                return;
            if (drop)
                decoder->setOptions(VideoThreadPrivate::dec_opt_framedrop);
            else
                decoder->setOptions(VideoThreadPrivate::dec_opt_normal);
        }
    };
    scheduleTask(new FrameDropTask(decoder(), value));
}

void VideoThread::applyFilters(VideoFrame &frame)
{
    DPTR_D(VideoThread);
    QMutexLocker locker(&d.mutex);
    Q_UNUSED(locker);
    if (!d.filters.isEmpty()) {
        //sort filters by format. vo->defaultFormat() is the last
        foreach (Filter *filter, d.filters) {
            VideoFilter *vf = static_cast<VideoFilter*>(filter);
            if (!vf->isEnabled())
                continue;
            vf->prepareContext(d.filter_context, d.statistics, &frame);
            vf->apply(d.statistics, &frame);
            //frame may be changed
            frame.setImageConverter(d.conv);
        }
    }
}

// filters on vo will not change video frame, so it's safe to protect frame only in every individual vo
bool VideoThread::deliverVideoFrame(VideoFrame &frame)
{
    DPTR_D(VideoThread);
    /*
     * TODO: video renderers sorted by preferredPixelFormat() and convert in AVOutputSet.
     * Convert only once for the renderers has the same preferredPixelFormat().
     */
    d.outputSet->lock();
    QList<AVOutput *> outputs = d.outputSet->outputs();
    if (outputs.size() > 1) { //FIXME!
        if (!frame.convertTo(VideoFormat::Format_RGB32)) {
            /*
             * use VideoFormat::Format_User to deliver user defined frame
             * renderer may update background but no frame to graw, so flickers
             * may crash for some renderer(e.g. d2d) without validate and render an invalid frame
             */
            d.outputSet->unlock();
            return false;
        }
    } else {
        VideoRenderer *vo = 0;
        if (!outputs.isEmpty())
            vo = static_cast<VideoRenderer*>(outputs.first());
        if (vo && (!vo->isSupported(frame.pixelFormat())
                || (vo->isPreferredPixelFormatForced() && vo->preferredPixelFormat() != frame.pixelFormat())
                )) {
            if (!frame.convertTo(vo->preferredPixelFormat())) {
                /*
                 * use VideoFormat::Format_User to deliver user defined frame
                 * renderer may update background but no frame to graw, so flickers
                 * may crash for some renderer(e.g. d2d) without validate and render an invalid frame
                 */
                d.outputSet->unlock();
                return false;
            }
        }
    }
    d.outputSet->sendVideoFrame(frame); //TODO: group by format, convert group by group
    d.outputSet->unlock();

    emit frameDelivered();
    return true;
}

//TODO: if output is null or dummy, the use duration to wait
void VideoThread::run()
{
    DPTR_D(VideoThread);
    if (!d.dec || !d.dec->isAvailable() || !d.outputSet)// || !d.conv)
        return;
    resetState();
    if (d.capture->autoSave()) {
        d.capture->setCaptureName(QFileInfo(d.statistics->url).completeBaseName());
    }
    //not neccesary context is managed by filters.
    d.filter_context = 0;
    VideoDecoder *dec = static_cast<VideoDecoder*>(d.dec);
    //used to initialize the decoder's frame size
    dec->resizeVideoFrame(0, 0);
    Packet pkt;
    QVariantHash *dec_opt = &d.dec_opt_normal; //TODO: restore old framedrop option after seek
    /*!
     * if we skip some frames(e.g. seek, drop frames to speed up), then then first frame to decode must
     * be a key frame for hardware decoding. otherwise may crash
     */
    bool wait_key_frame = false;
    int nb_dec_slow = 0;
    int nb_dec_fast = 0;

    bool is_pkt_bf_seek = true; // when seeking, a packet token from queue maybe before queue.clear() and packet is old. we may skip compare, wait and decode.
    qint32 seek_count = 0; // wm4 says: 1st seek can not use frame drop for decoder
    // TODO: kNbSlowSkip depends on video fps, ensure slow time <= 2s
    /* kNbSlowSkip: if video frame slow count >= kNbSlowSkip, skip decoding all frames until next keyframe reaches.
     * if slow count > kNbSlowSkip/2, skip rendering every 3 or 6 frames
     */
    const int kNbSlowSkip = 120; // about 1s for 120fps video
    // kNbSlowFrameDrop: if video frame slow count > kNbSlowFrameDrop, skip decoding nonref frames. only some of ffmpeg based decoders support it.
    const int kNbSlowFrameDrop = 10;
    int seek_done_count = 0; // seek_done_count > 1 means seeking is really finished. theorically can't be >1 if seeking!
    bool sync_audio = d.clock->clockType() == AVClock::AudioClock;
    bool sync_video = d.clock->clockType() == AVClock::VideoClock; // no frame drop
    while (!d.stop) {
        processNextTask();
        //TODO: why put it at the end of loop then playNextFrame() not work?
        //processNextTask tryPause(timeout) and  and continue outter loop
        if (tryPause()) { //DO NOT continue, or playNextFrame() will fail

        } else {
            if (isPaused())
                continue; //timeout. process pending tasks
        }
        if (d.packets.isEmpty() && !d.stop) {
            d.stop = d.demux_end;
        }
        if (d.stop && d.packets.isEmpty()) { // must stop here. otherwise thread will be blocked at d.packets.take()
            qDebug("video thread stop before take packet. packet queue is empty.");
            break;
        }
        if(!pkt.isValid()) {
            pkt = d.packets.take(); //wait to dequeue
        }
        //Compare to the clock
        if (!pkt.isValid()) {
            // may be we should check other information. invalid packet can come from
            wait_key_frame = true;
            qDebug("Invalid packet! flush video codec context!!!!!!!!!! video packet queue size: %d", d.packets.size());
            dec->flush();
            continue;
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
        const qreal pts = pkt.dts; //FIXME: pts and dts
        // TODO: delta ref time
        qreal diff = pts - d.clock->value();
        if (diff < 0 && sync_video)
            diff = 0; // this ensures no frame drop
        if (diff > kSyncThreshold) {
            nb_dec_fast++;
        } else {
            nb_dec_fast /= 2;
        }
        bool seeking = !qFuzzyIsNull(d.render_pts0);
        if (seeking) {
            nb_dec_slow = 0;
            nb_dec_fast = 0;
        }
        //qDebug("nb_fast: %d", nb_dec_fast);
        if (d.delay < -0.5 && d.delay > diff) {
            seeking = !qFuzzyIsNull(d.render_pts0);
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
                    qDebug("frame slow count: %d. a-v: %.3f", nb_dec_slow, diff);
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
        bool skip_render = pts < d.render_pts0;
        if (!sync_audio && diff > 0) {
            waitAndCheck(diff*1000UL, pts); // TODO: count decoding and filter time
            diff = 0; // TODO: can not change delay!
        }
        // update here after wait
        d.clock->updateVideoTime(pts); // dts or pts?
        seeking = !qFuzzyIsNull(d.render_pts0);
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
                // video too fast if old packet before seek backward compared with new audio packet after seek backward
                // what about video too late?
                if (is_pkt_bf_seek && diff > 2.0) {
                    // if seeking, we can not continue without decoding
                    // FIXME: also happens after seek but not seeking. so must check seek_done_count
                    is_pkt_bf_seek = false;
                    if (seek_done_count <= 1) { // is seeking
                        qDebug("old video packet before seek detected!!!!!!!!!!!!!");
                        pkt = Packet();
                        continue; // seeking and this v packet is before seeking
                    }
                }
                const double s = qMin<qreal>(0.01*(nb_dec_fast>>1), diff);
                qWarning("video too fast!!! sleep %.2f s, nb fast: %d", s, nb_dec_fast);
                waitAndCheck(s*1000UL, pts);
                diff = 0;
            }
        }
        seeking = !qFuzzyIsNull(d.render_pts0);
        //qDebug("d.render_pts0: %f, seeking: %d", d.render_pts0, seeking);
        //audio packet not cleaned up?
        if (diff > 0 && diff < 1.0 && !seeking) {
            // can not change d.delay here! we need it to comapre to next loop
            waitAndCheck(diff*1000UL, pts);
        }
        if (wait_key_frame) {
            if (pkt.hasKeyFrame)
                wait_key_frame = false;
            else {
                qDebug("waiting for key frame. queue size: %d. pkt.size: %d", d.packets.size(), pkt.data.size());
                pkt = Packet();
                continue;
            }
        }
        seeking = !qFuzzyIsNull(d.render_pts0);
        QVariantHash *dec_opt_old = dec_opt;
        if (!seeking) { // MAYBE not seeking
            if (nb_dec_slow < kNbSlowFrameDrop) {
                if (dec_opt == &d.dec_opt_framedrop) {
                    qDebug("frame drop normal. nb_dec_slow: %d, line: %d", nb_dec_slow, __LINE__);
                    dec_opt = &d.dec_opt_normal;
                }
            } else {
                if (dec_opt == &d.dec_opt_normal) {
                    qDebug("frame drop noref. nb_dec_slow: %d, line: %d", nb_dec_slow, __LINE__);
                    dec_opt = &d.dec_opt_framedrop;
                }
            }
        } else { // seeking
            if (seek_count > 0) {
                if (dec_opt == &d.dec_opt_normal) {
                    qDebug("seeking... frame drop noref. nb_dec_slow: %d, line: %d", nb_dec_slow, __LINE__);
                    dec_opt = &d.dec_opt_framedrop;
                }
            } else {
                seek_count = -1;
            }
        }
        if (dec_opt != dec_opt_old)
            dec->setOptions(*dec_opt);
        if (!dec->decode(pkt)) {
            pkt = Packet();
            continue;
        } else {
            int undecoded = dec->undecodedSize();
            if (undecoded > 0) {
                qDebug("undecoded size: %d", undecoded);
                const int remove = pkt.data.size() - undecoded;
                if (remove > 0)
                    pkt.data.remove(0, pkt.data.size() - undecoded);
            } else {
                pkt = Packet();
            }
        }
        VideoFrame frame = dec->frame();
        if (!frame.isValid()) {
            pkt = Packet(); //mark invalid to take next
            qWarning() << "invalid video frame from decoder";
            continue;
        }
        is_pkt_bf_seek = true;
        if (skip_render) {
            pkt = Packet(); //mark invalid to take next
            continue;
        }
        // can not check only pts > render_pts0 for seek backward. delta can not be too small(smaller than 1/fps)
        // FIXME: what if diff too large?
        if (d.render_pts0 > 0 && pts > d.render_pts0) {
            if (seek_done_count > 1)
                seek_done_count = 0;
            seek_done_count++;
            if (seek_done_count > 1) { // theorically can't be >1 if seeking!
                qDebug("reset render_pts0");
                d.render_pts0 = 0;
            }
        }
        if (seek_count == -1 && seek_done_count > 0)
            seek_count = 1;
        else if (seek_count > 0)
            seek_count++;
        if (frame.timestamp() == 0)
            frame.setTimestamp(pkt.pts); // pkt.pts is wrong. >= real timestamp
        frame.setImageConverter(d.conv);
        Q_ASSERT(d.statistics);
        d.statistics->video.current_time = QTime(0, 0, 0).addMSecs(int(pts * 1000.0)); //TODO: is it expensive?
        applyFilters(frame);

        //while can pause, processNextTask, not call outset.puase which is deperecated
        while (d.outputSet->canPauseThread()) {
            d.outputSet->pauseThread(100);
            //tryPause(100);
            processNextTask();
        }

        // no return even if d.stop is true. ensure frame is displayed. otherwise playing an image may be failed to display
        if (!deliverVideoFrame(frame))
            continue;
        d.statistics->video_only.frameDisplayed(frame.timestamp());
        // TODO: store original frame. now the frame is filtered and maybe converted to renderer perferred format
        d.displayed_frame = frame;
    }
    d.packets.clear();
    d.outputSet->sendVideoFrame(VideoFrame()); // TODO: let user decide what to display
    qDebug("Video thread stops running...");
}

} //namespace QtAV
