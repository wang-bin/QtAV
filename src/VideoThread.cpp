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
};

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
    return old;
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

//TODO: if output is null or dummy, the use duration to wait
void VideoThread::run()
{
    DPTR_D(VideoThread);
    //TODO: no d.writer is ok, just a audio player
    if (!d.dec || !d.dec->isAvailable() || !d.outputSet)// || !d.conv)
        return;
    resetState();
    //not neccesary context is managed by filters.
    d.filter_context = 0;
    VideoDecoder *dec = static_cast<VideoDecoder*>(d.dec);
    //used to initialize the decoder's frame size
    dec->resizeVideoFrame(0, 0);
    Packet pkt;
    /*!
     * if we skip some frames(e.g. seek, drop frames to speed up), then then first frame to decode must
     * be a key frame for hardware decoding. otherwise may crash
     */
    bool wait_key_frame = false;
    int nb_dec_slow = 0;
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
        if (d.stop) {
            qDebug("video thread stop before take packet");
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
        qreal pts = pkt.pts;
        // TODO: delta ref time
        qreal new_delay = pts - d.clock->value();
        if (d.delay < -0.5 && d.delay > new_delay) {
            qDebug("video becomes slower. force reduce video delay");
            // skip decoding
            // TODO: force fit min fps
            if (nb_dec_slow > 10 && !pkt.hasKeyFrame) {
                nb_dec_slow = 0;
                wait_key_frame = true;
                pkt = Packet();
                continue;
            } else {
                nb_dec_slow++;
            }
        } else {
            d.delay = new_delay;
        }
        /*
         *after seeking forward, a packet may be the old, v packet may be
         *the new packet, then the d.delay is very large, omit it.
         *TODO: 1. how to choose the value
         * 2. use last delay when seeking
         * 3. compute average decode time
        */
        bool skip_render = pts < d.render_pts0;
        // TODO: check frame type(after decode) and skip decoding some frames to speed up
        if (skip_render) {
            d.clock->updateVideoPts(pts); //here?
            //qDebug("skip video render at %f/%f", pkt.pts, d.render_pts0);
        }
        if (qAbs(d.delay) < 0.5) {
            if (d.delay < -kSyncThreshold) { //Speed up. drop frame?
                //continue;
            }
        } else if (qFuzzyIsNull(d.render_pts0)){ //when to drop off?
            qDebug("delay %f/%f", d.delay, d.clock->value());
            if (d.delay < 0) {
                if (!pkt.hasKeyFrame) {
                    // if continue without decoding, we must wait to the next key frame, then we may skip to many frames
                    //wait_key_frame = true;
                    //pkt = Packet();
                    //continue;
                }
                skip_render = !pkt.hasKeyFrame;
            }
        }
        //audio packet not cleaned up?
        if (d.delay < 1.0 && qFuzzyIsNull(d.render_pts0)) {
            while (d.delay > kSyncThreshold) { //Slow down
                //d.delay_cond.wait(&d.mutex, d.delay*1000); //replay may fail. why?
                //qDebug("~~~~~wating for %f msecs", d.delay*1000);
                usleep(kSyncThreshold * 1000000UL);
                if (d.stop)
                    d.delay = 0;
                else
                    d.delay -= kSyncThreshold;
                d.delay = qMax(0.0, qMin(d.delay, pts - d.clock->value()));
                processNextTask();
            }
            if (d.delay > 0)
                usleep(d.delay * 1000000UL);
            d.clock->updateVideoPts(pts); //here?
            if (d.stop) {
                qDebug("video thread stop before decode()");
                break;
            }
        } else if (qFuzzyIsNull(d.render_pts0)) {
            if (d.delay > 0)
                msleep(40);
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
        if (!dec->decode(pkt.data)) {
            pkt = Packet();
            continue;
        } else {
            int undecoded = dec->undecodedSize();
            if (undecoded > 0) {
                pkt.data.remove(0, pkt.data.size() - undecoded);
            } else {
                pkt = Packet();
            }
        }
        VideoFrame frame = dec->frame();
        if (!frame.isValid()) {
            pkt = Packet(); //mark invalid to take next
            qWarning("invalid video frame");
            continue;
        }
        if (skip_render) {
            pkt = Packet(); //mark invalid to take next
            continue;
        }
        d.render_pts0 = 0;
        frame.setTimestamp(pts);
        frame.setImageConverter(d.conv);
        Q_ASSERT(d.statistics);
        d.statistics->video.current_time = QTime(0, 0, 0).addMSecs(int(pts * 1000.0)); //TODO: is it expensive?
        //TODO: add current time instead of pts
        d.statistics->video_only.putPts(pts);
        {
            QMutexLocker locker(&d.mutex);
            Q_UNUSED(locker);
            if (!d.filters.isEmpty()) {
                //sort filters by format. vo->defaultFormat() is the last
                foreach (Filter *filter, d.filters) {
                    if (d.stop) {
                        break;
                    }
                    VideoFilter *vf = static_cast<VideoFilter*>(filter);
                    if (!vf->isEnabled())
                        continue;
                    vf->prepareContext(d.filter_context, d.statistics, &frame);
                    vf->apply(d.statistics, &frame);
                    //frame may be changed
                    frame.setImageConverter(d.conv);
                    frame.setTimestamp(pts);
                }
            }
        }

        //while can pause, processNextTask, not call outset.puase which is deperecated
        while (d.outputSet->canPauseThread()) {
            if (d.stop) {
                break;
            }
            d.outputSet->pauseThread(100);
            //tryPause(100);
            processNextTask();
        }

        if (d.stop) {
            qDebug("video thread stop before send decoded data");
            break;
        }

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
                continue;
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
                    continue;
                }
            }
        }
        d.outputSet->sendVideoFrame(frame); //TODO: group by format, convert group by group
        d.outputSet->unlock();

        emit frameDelivered();

        d.capture->setPosition(pts);
        // TODO: capture yuv frames
        if (d.capture->isRequested()) {
            bool auto_name = d.capture->name.isEmpty() && d.capture->autoSave();
            if (auto_name) {
                QString cap_name;
                if (d.statistics)
                    cap_name = QFileInfo(d.statistics->url).completeBaseName();
                d.capture->setCaptureName(cap_name + "_" + QString::number(pts, 'f', 3));
            }
            //TODO: what if not rgb32 now? detach the frame
            d.capture->setVideoFrame(frame);
            d.capture->start();
            if (auto_name)
                d.capture->setCaptureName("");
        }
    }
    d.packets.clear();
    d.capture->cancel();
    d.outputSet->sendVideoFrame(VideoFrame());
    qDebug("Video thread stops running...");
}

} //namespace QtAV
