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

#include <QtAV/AudioThread.h>
#include <private/AVThread_p.h>
#include <QtAV/AudioDecoder.h>
#include <QtAV/Packet.h>
#include <QtAV/AudioOutput.h>
#include <QtAV/AVClock.h>
#include <QtCore/QCoreApplication>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
}
#endif //__cplusplus

namespace QtAV {

class AudioThreadPrivate : public AVThreadPrivate
{
public:
    qreal last_pts; //used when audio output is not available, to calculate the aproximate sleeping time
};

AudioThread::AudioThread(QObject *parent)
    :AVThread(*new AudioThreadPrivate(), parent)
{
}

//TODO: if output is null or dummy, the use duration to wait
void AudioThread::run()
{
    DPTR_D(AudioThread);
    //No decoder or output. No audio output is ok, just display picture
    if (!d.dec)
        return;
    resetState();
    Q_ASSERT(d.clock != 0);
    AudioOutput *ao = static_cast<AudioOutput*>(d.writer);
    int sample_rate = d.dec->codecContext()->sample_rate;
    int channels = d.dec->codecContext()->channels;
    int csf = channels * sample_rate * sizeof(float);
    static const double max_len = 0.02;
    d.last_pts = 0;
    while (!d.stop) {
        tryPause();
        if (d.stop) { //the queue is empty and may block. should setBlocking(false) wake up cond empty?
            break;
        }
        QMutexLocker locker(&d.mutex);
        Q_UNUSED(locker);
        if (d.packets.isEmpty() && !d.stop) {
            d.stop = d.demux_end;
            if (d.stop) {
                break;
            }
        }
        Packet pkt = d.packets.take(); //wait to dequeue
        if (pkt.data.isNull()) {
            qDebug("Empty packet!");
            continue;
        }
        d.clock->updateValue(pkt.pts);
        //DO NOT decode and convert if ao is not available or mute!
        if (d.dec->decode(pkt.data)) {
            QByteArray decoded(d.dec->data());
            int decodedSize = decoded.size();
            int decodedPos = 0;
            qreal delay =0;
            while (decodedSize > 0) {
                int chunk = qMin(decodedSize, int(max_len*csf));
                d.clock->updateDelay(delay += (qreal)chunk/(qreal)csf);
                QByteArray decodedChunk(chunk, 0); //volume == 0 || mute
                if (ao && ao->isAvailable()) {
                    if (!ao->isMute()) {
                        decodedChunk = QByteArray::fromRawData(decoded.constData() + decodedPos, chunk);
                        qreal vol = ao->volume();
                        if (vol != 1.0) {
                            int len = decodedChunk.size()/sizeof(float); //TODO: why???
                            float *data = (float*)decodedChunk.data();
                            for (int i = 0; i < len; ++i)
                                data[i] *= vol;
                        }
                    }
                    ao->writeData(decodedChunk);
                } else {
                /*
                 * why need this even if we add delay? and usleep sounds weird
                 * the advantage is if no audio device, the play speed is ok too
                 * So is portaudio blocking the thread when playing?
                 */
                    static bool sWarn_no_ao = true; //FIXME: no warning when replay. warn only once
                    if (sWarn_no_ao) {
                        qDebug("Audio output not available! msleep(%lu)", (unsigned long)((qreal)chunk/(qreal)csf * 1000));
                        sWarn_no_ao = false;
                    }
                    //TODO: avoid acummulative error. External clock?
                    msleep((unsigned long)((qreal)chunk/(qreal)csf * 1000.0));
                }
                decodedPos += chunk;
                decodedSize -= chunk;
            }
        } else {
            //qWarning("Decode audio failed");
            qreal dt = pkt.pts - d.last_pts;
            if (abs(dt) > 0.618 || dt < 0) {
                dt = 0;
            }
            //qDebug("sleep %f", dt);
            //TODO: avoid acummulative error. External clock?
            msleep((unsigned long)(dt*1000.0));
        }
        d.last_pts = d.clock->value(); //not pkt.pts! the delay is updated!
    }
    qDebug("Audio thread stops running...");
}

} //namespace QtAV
