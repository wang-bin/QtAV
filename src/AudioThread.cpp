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

#include <QtAV/AudioThread.h>
#include <private/AVThread_p.h>
#include <QtAV/AudioDecoder.h>
#include <QtAV/Packet.h>
#include <QtAV/AudioOutput.h>
#include <QtAV/AVClock.h>
#include <QtAV/QtAV_Compat.h>
#include <QtCore/QCoreApplication>

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

/*
 *TODO:
 * if output is null or dummy, the use duration to wait
 */
void AudioThread::run()
{
    DPTR_D(AudioThread);
    //No decoder or output. No audio output is ok, just display picture
    if (!d.dec || !d.dec->isAvailable())
        return;
    resetState();
    Q_ASSERT(d.clock != 0);
    AudioDecoder *dec = static_cast<AudioDecoder*>(d.dec);
    AudioOutput *ao = static_cast<AudioOutput*>(d.writer);
    int sample_rate = dec->codecContext()->sample_rate;
    int channels = dec->codecContext()->channels;
    int csf = channels * sample_rate * sizeof(float);
    static const double max_len = 0.02;
    d.last_pts = 0;
    //TODO: bool need_sync in private class
    bool is_external_clock = d.clock->clockType() == AVClock::ExternalClock;
    while (!d.stop) {
        //TODO: why put it at the end of loop then playNextFrame() not work?
        if (tryPause()) { //DO NOT continue, or playNextFrame() will fail
            if (d.stop)
                break; //the queue is empty and may block. should setBlocking(false) wake up cond empty?
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
        if (!pkt.isValid()) {
            qDebug("Invalid packet! flush audio codec context!!!!!!!!");
            dec->flush();
            continue;
        }
        if (is_external_clock) {
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
        } else {
            d.clock->updateValue(pkt.pts);
        }
        //DO NOT decode and convert if ao is not available or mute!
        if (dec->decode(pkt.data)) {
            QByteArray decoded(dec->data());
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
