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
#include <QtGui/QImage>
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

double VideoThread::currentPts() const
{
    return d_func().pts;
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

//TODO: if output is null or dummy, the use duration to wait
void VideoThread::run()
{
    DPTR_D(VideoThread);
    //TODO: no d.writer is ok, just a audio player
    if (!d.dec || !d.dec->isAvailable() || !d.outputSet)// || !d.conv)
        return;
    resetState();
    Q_ASSERT(d.clock != 0);
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
        pkt = d.packets.take(); //wait to dequeue
        //Compare to the clock
        if (!pkt.isValid()) {
            qDebug("Invalid packet! flush video codec context!!!!!!!!!!");
            dec->flush();
            continue;
        }
        d.delay = pkt.pts  - d.clock->value();
        /*
         *after seeking forward, a packet may be the old, v packet may be
         *the new packet, then the d.delay is very large, omit it.
         *TODO: 1. how to choose the value
         * 2. use last delay when seeking
        */
        if (qAbs(d.delay) < 2.718) {
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
                msleep(64);
            } else {
                //audio packet not cleaned up?
                continue;
            }
        }
        d.clock->updateVideoPts(pkt.pts); //here?
        if (d.stop) {
            qDebug("video thread stop before decode()");
            break;
        }
        QMutexLocker locker(&d.mutex);
        Q_UNUSED(locker);
        //still decode, we may need capture. TODO: decode only if existing a capture request if no vo
        if (dec->decode(pkt.data)) {
            d.pts = pkt.pts;
            if (d.capture) {
                d.capture->setRawImage(dec->data(), dec->width(), dec->height());
            }
            VideoFrame frame = dec->frame();
            if (!frame.isValid())
                continue;
            d.conv->setInFormat(frame.pixelFormatFFmpeg());
            d.conv->setInSize(frame.width(), frame.height());
            frame.setImageConverter(d.conv);
            if (d.statistics) {
                d.statistics->video.current_time = QTime(0, 0, 0).addMSecs(int(pkt.pts * 1000.0)); //TODO: is it expensive?
                if (!d.filters.isEmpty()) {
                    //sort filters by format. vo->defaultFormat() is the last
                    foreach (Filter *filter, d.filters) {
                        if (d.stop) {
                            break;
                        }
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
            frame.convertTo(VideoFormat(VideoFormat::Format_RGB32));
            d.outputSet->sendVideoFrame(frame); //TODO: group by format, convert group by group
        }
    }
    qDebug("Video thread stops running...");
}

} //namespace QtAV
