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
#include <QDateTime>
namespace QtAV {

const double kSyncThreshold = 0.005; // 5 ms

class VideoThreadPrivate : public AVThreadPrivate
{
public:
    VideoThreadPrivate():delay(0){}
    double delay;
    double pts; //current decoded pts. for capture
    QByteArray decoded_data; //for capture. cow
    int width, height;
};

VideoThread::VideoThread(QObject *parent) :
    AVThread(*new VideoThreadPrivate(), parent)
{
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
    if (!d.dec || !d.writer)
        return;
    resetState();
    Q_ASSERT(d.clock != 0);
    VideoDecoder *dec = static_cast<VideoDecoder*>(d.dec);
    while (!d.stop) {
        d.mutex.lock();
        if (d.packets.isEmpty() && !d.stop) {
            d.stop = d.demux_end;
            if (d.stop) {
                d.mutex.unlock();
                break;
            }
        }
        Packet pkt = d.packets.take(); //wait to dequeue
        //Compare to the clock
        if (pkt.pts <= 0 || pkt.data.isNull()) {
            qDebug("Invalid pts or empty packet!");
            d.mutex.unlock();
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
                //d.mutex.unlock();
                //continue;
            }
        } else { //when to drop off?
            qDebug("delay %f/%f", d.delay, d.clock->value());
            if (d.delay > 0) {
                msleep(64);
            } else {
                //audio packet not cleaned up?
                d.mutex.unlock();
                continue;
            }
        }
        d.clock->updateVideoPts(pkt.pts); //here?
        //DO NOT decode and convert if vo is not available or null!
        if (d.dec->decode(pkt.data)) {
            d.pts = pkt.pts;
            d.width = dec->width();
            d.height = dec->height();
            d.decoded_data = d.dec->data();
            if (d.writer) {
                ((VideoRenderer*)d.writer)->setSourceSize(dec->width(), dec->height());
				d.writer->writeData(d.dec->data());
            }
            //qApp->processEvents(QEventLoop::AllEvents);
        }
        d.mutex.unlock();
    }
    qDebug("Video thread stops running...");
}

} //namespace QtAV
