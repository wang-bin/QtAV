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

#include <QtAV/VideoThread.h>
#include <private/AVThread_p.h>
#include <QtAV/Packet.h>
#include <QtAV/AVClock.h>
#include <QtAV/VideoCapture.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/VideoRenderer.h>
#include <QtAV/ImageConverter.h>
#include <QtCore/QFileInfo>
#include <QtAV/Statistics.h>
#include <QtAV/Filter.h>
#include <QtAV/FilterContext.h>
#include <QtAV/OutputSet.h>
#include <QtAV/ImageConverterTypes.h>
#include <QtAV/QtAV_Compat.h>

#define PIX_FMT PIX_FMT_RGB32 //PIX_FMT_YUV420P

namespace QtAV {

class VideoThreadPrivate : public AVThreadPrivate
{
public:
    VideoThreadPrivate():
        conv(0)
      , capture(0)
    {
        conv = ImageConverterFactory::create(ImageConverterId_FF); //TODO: set in AVPlayer
        conv->setOutFormat(PIX_FMT); //vo->defaultFormat
    }
    ~VideoThreadPrivate() {
        if (conv) {
            delete conv;
            conv = 0;
        }
    }

    ImageConverter *conv;
    double pts; //current decoded pts. for capture. TODO: remove
    //QImage image; //use QByteArray? Then must allocate a picture in ImageConverter, see VideoDecoder
    VideoCapture *capture;
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
    DPTR_D(VideoThread);
    setEQ(val, 101, 101);
}

void VideoThread::setContrast(int val)
{
    DPTR_D(VideoThread);
    setEQ(101, val, 101);
}

void VideoThread::setSaturation(int val)
{
    DPTR_D(VideoThread);
    setEQ(101, 101, val);
}

void VideoThread::setEQ(int b, int c, int s)
{
    class EQTask : public QRunnable {
    public:
        EQTask(ImageConverter *c)
            : conv(c)
            , brightness(0)
            , contrast(0)
            , saturation(0)
        {
            qDebug("EQTask tid=%p", QThread::currentThread());
        }
        void run() {
            // check here not in ctor because it may called later in video thread
            if (brightness < -100 || brightness > 100)
                brightness = conv->brightness();
            if (contrast < -100 || contrast > 100)
                contrast = conv->contrast();
            if (saturation < -100 || saturation > 100)
                saturation = conv->saturation();
            qDebug("EQTask::run() tid=%p (b: %d, c: %d, s: %d)"
                   , QThread::currentThread(), brightness, contrast, saturation);
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
    VideoDecoder *dec = static_cast<VideoDecoder*>(d.dec);
    if (dec) {
        //used to initialize the decoder's frame size
        dec->resizeVideoFrame(0, 0);
    }
    Packet pkt;
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
            qDebug("Invalid packet! flush video codec context!!!!!!!!!!");
            dec->flush();
            continue;
        }
        qreal pts = pkt.pts;
        d.delay = pts - d.clock->value();
        /*
         *after seeking forward, a packet may be the old, v packet may be
         *the new packet, then the d.delay is very large, omit it.
         *TODO: 1. how to choose the value
         * 2. use last delay when seeking
         * 3. compute average decode time
        */
        bool skip_render = false;
        if (qAbs(d.delay) < 0.5) {
            if (d.delay < -kSyncThreshold) { //Speed up. drop frame?
                //continue;
            }
            while (d.delay > kSyncThreshold) { //Slow down
                //d.delay_cond.wait(&d.mutex, d.delay*1000); //replay may fail. why?
                //qDebug("~~~~~wating for %f msecs", d.delay*1000);
                usleep(kSyncThreshold * 1000000UL);
                if (d.stop)
                    d.delay = 0;
                else
                    d.delay -= kSyncThreshold;
            }
            if (d.delay > 0)
                usleep(d.delay * 1000000UL);
        } else { //when to drop off?
            qDebug("delay %f/%f", d.delay, d.clock->value());
            if (d.delay > 0) {
                msleep(40);
            } else {
                // FIXME: if continue without decoding, hw decoding may crash, why?
                //audio packet not cleaned up?
                if (!pkt.hasKeyFrame) {
                    qDebug("No key frame. Skip decoding");
                    //pkt = Packet();
                    //continue; //may crash if hw
                }
                skip_render = !pkt.hasKeyFrame;
            }
        }
        d.clock->updateVideoPts(pts); //here?
        if (d.stop) {
            qDebug("video thread stop before decode()");
            break;
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

        if (skip_render)
            continue;
        VideoFrame frame = dec->frame();
        if (!frame.isValid())
            continue;
        d.conv->setInFormat(frame.pixelFormatFFmpeg());
        d.conv->setInSize(frame.width(), frame.height());
        d.conv->setOutSize(frame.width(), frame.height());
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
                    if (!filter->isEnabled())
                        continue;
                    filter->process(d.filter_context, d.statistics, &frame);
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
        frame.convertTo(VideoFormat::Format_RGB32);
        d.outputSet->sendVideoFrame(frame); //TODO: group by format, convert group by group
        d.capture->setPosition(pts);
        if (d.capture->isRequested()) {
            bool auto_name = d.capture->name.isEmpty() && d.capture->autoSave();
            if (auto_name) {
                QString cap_name;
                if (d.statistics)
                    cap_name = QFileInfo(d.statistics->url).completeBaseName();
                d.capture->setCaptureName(cap_name + "_" + QString::number(pts, 'f', 3));
            }
            //TODO: what if not rgb32 now? detach the frame
            //FIXME: why frame.data() may crash?
            d.capture->setRawImage(frame.frameData(), frame.width(), frame.height(), frame.imageFormat());
            d.capture->start();
            if (auto_name)
                d.capture->setCaptureName("");
        }
    }
    d.capture->cancel();
    qDebug("Video thread stops running...");
}

} //namespace QtAV
