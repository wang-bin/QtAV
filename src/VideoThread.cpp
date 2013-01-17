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

#include <QtAV/VideoThread.h>
#include <private/AVThread_p.h>
#include <QtAV/Packet.h>
#include <QtAV/AVClock.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/VideoRenderer.h>
#include <QtAV/ImageConverter.h>
#include <QtGui/QImage>

namespace QtAV {

const double kSyncThreshold = 0.005; // 5 ms

class VideoThreadPrivate : public AVThreadPrivate
{
public:
    VideoThreadPrivate():conv(0),delay(0){}
    ImageConverter *conv;
    double delay;
    double pts; //current decoded pts. for capture
    QByteArray decoded_data; //for capture. cow
    int width, height;
    QImage image; //use QByteArray? Then must allocate a picture in ImageConverter, see VideoDecoder
};

VideoThread::VideoThread(QObject *parent) :
    AVThread(*new VideoThreadPrivate(), parent)
{
}

ImageConverter* VideoThread::setImageConverter(ImageConverter *converter)
{
    DPTR_D(VideoThread);
    QtAV::ImageConverter* old = d.conv;
    d.conv = converter;
    return old;
}

ImageConverter* VideoThread::imageConverter() const
{
    return d_func().conv;
}

QByteArray VideoThread::currentRawImage() const
{
    return d_func().decoded_data;
}

QSize VideoThread::currentRawImageSize() const
{
    DPTR_D(const VideoThread);
    return QSize(d.width, d.height);
}

double VideoThread::currentPts() const
{
    return d_func().pts;
}

//TODO: if output is null or dummy, the use duration to wait
void VideoThread::run()
{
    DPTR_D(VideoThread);
    //TODO: no d.writer is ok, just a audio player
    if (!d.dec || !d.dec->isAvailable() || !d.writer)// || !d.conv)
        return;
    resetState();
    Q_ASSERT(d.clock != 0);
    VideoDecoder *dec = static_cast<VideoDecoder*>(d.dec);
    VideoRenderer* vo = static_cast<VideoRenderer*>(d.writer);
    while (!d.stop) {
        if (tryPause())
            continue; //the queue is empty and may block. should setBlocking(false) wake up cond empty?
        QMutexLocker locker(&d.mutex);
        Q_UNUSED(locker);
        if (d.packets.isEmpty() && !d.stop) {
            d.stop = d.demux_end;
            if (d.stop) {
                break;
            }
        }
        Packet pkt = d.packets.take(); //wait to dequeue
        //Compare to the clock
        if (pkt.pts <= 0 || pkt.data.isNull()) {
            qDebug("Invalid pts or empty packet!");
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
            if (d.delay > kSyncThreshold) { //Slow down
                //d.delay_cond.wait(&d.mutex, d.delay*1000); //replay may fail. why?
                //qDebug("~~~~~wating for %f msecs", d.delay*1000);
                usleep(d.delay * 1000000);
            } else if (d.delay < -kSyncThreshold) { //Speed up. drop frame?
                //continue;
            }
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
        //DO NOT decode and convert if vo is not available or null!
        //if ()
        if (d.dec->decode(pkt.data)) {
            d.pts = pkt.pts;
            d.width = dec->width();
            d.height = dec->height();
            d.decoded_data = d.dec->data();
            //TODO: Add filters here
            /*if (d.image.width() != d.width || d.image.height() != d.height)
                d.image = QImage(d.width, d.height, QImage::Format_RGB32);
            d.conv->setInSize(d.width, d.height);
            if (!d.conv->convert(d.decoded_data.constData(), d.image.bits())) {

            }*/
            if (vo && vo->isAvailable()) {
                vo->setSourceSize(dec->width(), dec->height());
                vo->writeData(d.decoded_data);
            }
        }
    }
    qDebug("Video thread stops running...");
}

} //namespace QtAV
