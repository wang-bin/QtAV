/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#include "AudioThread.h"
#include "AVThread_p.h"
#include "QtAV/AudioDecoder.h"
#include "QtAV/Packet.h"
#include "QtAV/AudioFormat.h"
#include "QtAV/AudioOutput.h"
#include "QtAV/AudioResampler.h"
#include "QtAV/AVClock.h"
#include "QtAV/Filter.h"
#include "output/OutputSet.h"
#include "QtAV/private/AVCompat.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include "utils/Logger.h"

namespace QtAV {

class AudioThreadPrivate : public AVThreadPrivate
{
public:
    void init() {
        resample = false;
        last_pts = 0;
    }

    bool resample;
    qreal last_pts; //used when audio output is not available, to calculate the aproximate sleeping time
};

AudioThread::AudioThread(QObject *parent)
    :AVThread(*new AudioThreadPrivate(), parent)
{
}

void AudioThread::applyFilters(AudioFrame &frame)
{
    DPTR_D(AudioThread);
    //QMutexLocker locker(&d.mutex);
    //Q_UNUSED(locker);
    if (!d.filters.isEmpty()) {
        //sort filters by format. vo->defaultFormat() is the last
        foreach (Filter *filter, d.filters) {
            AudioFilter *af = static_cast<AudioFilter*>(filter);
            if (!af->isEnabled())
                continue;
            af->apply(d.statistics, &frame);
        }
    }
}
/*
 *TODO:
 * if output is null or dummy, the use duration to wait
 */
void AudioThread::run()
{
    DPTR_D(AudioThread);
    //No decoder or output. No audio output is ok, just display picture
    if (!d.dec || !d.dec->isAvailable() || !d.outputSet)
        return;
    // resetState(); // we can't reset the thread state from here
    Q_ASSERT(d.clock != 0);
    d.init();
    Packet pkt;
    qint64 fake_duration = 0LL;
    qint64 fake_pts = 0LL;
    int sync_id = 0;
    while (!d.stop) {
        processNextTask();
        if (d.render_pts0 < 0) { // no pause when seeking
            if (tryPause()) { //DO NOT continue, or stepForward() will fail
                if (d.stop)
                    break; //the queue is empty and may block. should setBlocking(false) wake up cond empty?
            } else {
                if (isPaused())
                    continue;
            }
        }
        if (d.seek_requested) {
            d.seek_requested = false;
            qDebug("request seek audio thread");
            pkt = Packet(); // last decode failed and pkt is valid, reset pkt to force take the next packet if seek is requested
            msleep(1);
        } else {
            // d.render_pts0 < 0 means seek finished here
            if (d.clock->syncId() > 0) {
                qDebug("audio thread wait to sync end for sync id: %d", d.clock->syncId());
                if (d.render_pts0 < 0 && sync_id > 0) {
                    msleep(10);
                    continue;
                }
            } else {
                sync_id = 0;
            }
        }
        if (!pkt.isValid()) {
            // can't seek back if eof packet is read
            //qDebug("eof pkt: %d valid: %d, aqueue size: %d, abuffer: %d %.3f %d, fake_duration: %lld", pkt.isEOF(), pkt.isValid(), d.packets.size(), d.packets.bufferValue(), d.packets.bufferMax(), d.packets.isFull(), fake_duration);
            // If seek requested but last decode failed
            if (!pkt.isEOF() && (fake_duration <= 0 || !d.packets.isEmpty())) {
                pkt = d.packets.take(); //wait to dequeue
            }
            if (pkt.isEOF()) {
                fake_duration = 0; //avoid endless wait
                qDebug("audio thread gets an eof packet. pkt.pts: %.3f, d.render_pts0:%.3f", pkt.pts, d.render_pts0);
            }
            if (!pkt.isValid()) {
                if (pkt.pts >= 0) { // check seek first
                    qDebug("Invalid packet! flush audio codec context!!!!!!!! audio queue size=%d", d.packets.size());
                    QMutexLocker locker(&d.mutex);
                    Q_UNUSED(locker);
                    if (d.dec) //maybe set to null in setDecoder()
                        d.dec->flush();
                    d.render_pts0 = pkt.pts;
                    sync_id = pkt.position;
                    qDebug("audio seek: %.3f, id: %d", d.render_pts0, sync_id);
                    pkt = Packet(); //mark invalid to take next
                    if (fake_duration > 0) {
                        //qDebug("fake_duration update on seek: %ul + %ul - %.3f", fake_duration, fake_pts, d.render_pts0);
                        fake_duration = fake_duration + fake_pts - d.render_pts0*1000.0;
                        fake_pts = d.render_pts0*1000.0;
                    }
                    d.clock->updateValue(d.render_pts0);
                    d.clock->updateDelay(0);
                    continue;
                }
                if (pkt.duration > 0) {
                    fake_duration = pkt.duration * 1000.0;
                    fake_pts = d.last_pts*1000.0;
                    pkt = Packet(); //mark invalid to avoid run here in the next loop
                    //qDebug("get fake apkt: %.3f+%ul", pkt.pts, fake_duration);
                    continue;
                }
            }
            if (fake_duration > 0) {
                static const ulong kSleepMs = 20;
                const ulong ms = qMin<qint64>(fake_duration, kSleepMs);
                fake_duration -= ms;
                fake_pts += ms;
                //qDebug("fake_wait: %ul, fake_duration: %lld, delay: %.3f", ms, fake_duration, d.clock->delay());
                d.clock->updateDelay(d.clock->delay() + qreal(ms)/1000.0);
                msleep(ms);
                continue;
            }
        }
        if (!pkt.isValid() && !pkt.isEOF()) // decode it will cause crash
            continue;
        qreal dts = pkt.dts; //FIXME: pts and dts
        // no key frame for audio. so if pts reaches, try decode and skip render if got frame pts does not reach
        bool skip_render = pkt.pts >= 0 && pkt.pts < d.render_pts0; // if audio stream is too short, seeking will fail and d.render_pts0 keeps >0
        // audio has no key frame, skip rendering equals to skip decoding
        if (skip_render) {
            d.clock->updateValue(pkt.pts);
            /*
             * audio may be too fast than video if skip without sleep
             * a frame is about 20ms. sleep time must be << frame time
             */
            qreal a_v = dts - d.clock->videoTime();
            qDebug("skip audio decode at %f/%f v=%f a-v=%fms", dts, d.render_pts0, d.clock->videoTime(), a_v*1000.0);
            if (a_v > 0) {
                msleep(qMin((ulong)20, ulong(a_v*1000.0)));
            } else {
                // audio maybe too late compared with video packet before seeking backword. so just ignore
                msleep(0); //wait video seek done if audio done early
            }
            pkt = Packet(); //mark invalid to take next
            continue;
        }
        const bool is_external_clock = d.clock->clockType() == AVClock::ExternalClock;
        if (is_external_clock && !pkt.isEOF()) {
            d.delay = dts - d.clock->value();
            /*
             *after seeking forward, a packet may be the old, v packet may be
             *the new packet, then the d.delay is very large, omit it.
             *TODO: 1. how to choose the value
             * 2. use last delay when seeking
            */
            if (qAbs(d.delay) < 2.0) {
                if (d.delay < -kSyncThreshold) { //Speed up. drop frame? resample?
                    qDebug("audio is late compared with external clock. skip decoding. %.3f-%.3f=%.3f", dts, d.clock->value(), d.delay);
                    pkt = Packet(); //mark invalid to take next
                    continue;
                }
                if (d.delay > 0)
                    waitAndCheck(d.delay, dts);
            } else { //when to drop off?
                if (d.delay > 0) {
                    msleep(64);
                } else {
                    //audio packet not cleaned up?
                    qDebug("audio is too late compared with external clock. skip decoding. %.3f-%.3f=%.3f", dts, d.clock->value(), d.delay);
                    pkt = Packet(); //mark invalid to take next
                    continue;
                }
            }
        }
        /* lock here to ensure decoder and ao can complete current work before they are changed
         * current packet maybe not supported by new decoder
         */
        // TODO: smaller scope
        QMutexLocker locker(&d.mutex);
        Q_UNUSED(locker);
        AudioDecoder *dec = static_cast<AudioDecoder*>(d.dec);
        if (!dec) {
            pkt = Packet(); //mark invalid to take next
            continue;
        }
        AudioOutput *ao = 0;
        // first() is not null even if list empty
        if (!d.outputSet->outputs().isEmpty())
            ao = static_cast<AudioOutput*>(d.outputSet->outputs().first());

        //DO NOT decode and convert if ao is not available or mute!
        bool has_ao = ao && ao->isAvailable();
        //if (!has_ao) {//do not decode?
        // TODO: move resampler to AudioFrame, like VideoFrame does
        if (has_ao && dec->resampler()) {
            if (dec->resampler()->speed() != ao->speed()
                    || dec->resampler()->outAudioFormat() != ao->audioFormat()) {
                //resample later to ensure thread safe. TODO: test
                if (d.resample) {
                    qDebug() << "ao.format " << ao->audioFormat();
                    qDebug() << "swr.format " << dec->resampler()->outAudioFormat();
                    qDebug("decoder set speed: %.2f", ao->speed());
                    dec->resampler()->setOutAudioFormat(ao->audioFormat());
                    dec->resampler()->setSpeed(ao->speed());
                    dec->resampler()->prepare();
                    d.resample = false;
                } else {
                    d.resample = true;
                }
            }
        } else {
            if (dec->resampler() && dec->resampler()->speed() != d.clock->speed()) {
                if (d.resample) {
                    qDebug("decoder set speed: %.2f", d.clock->speed());
                    dec->resampler()->setSpeed(d.clock->speed());
                    dec->resampler()->prepare();
                    d.resample = false;
                } else {
                    d.resample = true;
                }
            }
        }
        if (d.stop) {
            qDebug("audio thread stop before decode()");
            break;
        }
        //qDebug("apkt: %.3f, %lld %p", pkt.pts, pkt.asAVPacket()->pts, pkt.asAVPacket()->data);
        if (!dec->decode(pkt)) {
            qWarning("Decode audio failed. undecoded: %d", dec->undecodedSize());
            if (pkt.isEOF()) {
                qDebug("audio decode eof done");
                Q_EMIT eofDecoded();
                if (d.render_pts0 >= 0) {
                    qDebug("audio seek done at eof pts: %.3f. id: %d", pkt.pts, sync_id);
                    d.render_pts0 = -1;
                    d.clock->syncEndOnce(sync_id);
                    Q_EMIT seekFinished(qint64(pkt.pts*1000.0)); //TODO: pts
                }
                if (!pkt.position)
                    break;
            }
            qreal dt = dts - d.last_pts;
            if (dt > 0.5 || dt < 0) {
                dt = 0;
            }
            if (!qFuzzyIsNull(dt)) {
                msleep((unsigned long)(dt*1000.0));
            }
            pkt = Packet();
            d.last_pts = d.clock->value(); //not pkt.pts! the delay is updated!
            continue;
        }
        // reduce here to ensure to decode the rest data in the next loop
        if (!pkt.isEOF())
            pkt.skip(pkt.data.size() - dec->undecodedSize());
#if USE_AUDIO_FRAME
        AudioFrame frame(dec->frame());
        if (!frame)
            continue; //pkt data is updated after decode, no reset here
        if (frame.timestamp() <= 0)
            frame.setTimestamp(pkt.pts); // pkt.pts is wrong. >= real timestamp
        if (d.render_pts0 >= 0.0) { // seeking
            d.clock->updateValue(frame.timestamp());
            if (frame.timestamp() < d.render_pts0) {
                qDebug("skip audio rendering: %f-%f", frame.timestamp(), d.render_pts0);
                continue; //pkt data is updated after decode, no reset here
            }
            qDebug("audio seek finished @%.3f. id: %d", frame.timestamp(), sync_id);
            d.render_pts0 = -1.0;
            d.clock->syncEndOnce(sync_id);
            Q_EMIT seekFinished(qint64(frame.timestamp()*1000.0));
            if (has_ao) {
                ao->clear();
            }
        }
        if (has_ao) {
            applyFilters(frame);
            frame.setAudioResampler(dec->resampler()); //!!!
            // FIXME: resample ONCE is required for audio frames from ffmpeg
            //if (ao->audioFormat() != frame.format()) {
                frame = frame.to(ao->audioFormat());
            //}
        }
        QByteArray decoded(frame.data());
#else
        QByteArray decoded(dec->data());
#endif
        int decodedSize = decoded.size();
        int decodedPos = 0;
        qreal delay = 0;
        const qreal byte_rate = frame.format().bytesPerSecond();
        qreal pts = frame.timestamp();
        //qDebug("frame samples: %d @%.3f+%lld", frame.samplesPerChannel()*frame.channelCount(), frame.timestamp(), frame.duration()/1000LL);
        while (decodedSize > 0) {
            if (d.stop) {
                qDebug("audio thread stop after decode()");
                break;
            }
            const int chunk = qMin(decodedSize, has_ao ? ao->bufferSize() : 512*frame.format().bytesPerFrame());//int(max_len*byte_rate));
            //AudioFormat.bytesForDuration
            const qreal chunk_delay = (qreal)chunk/(qreal)byte_rate;
            if (has_ao && ao->isOpen()) {
                QByteArray decodedChunk = QByteArray::fromRawData(decoded.constData() + decodedPos, chunk);
                //qDebug("ao.timestamp: %.3f, pts: %.3f, pktpts: %.3f", ao->timestamp(), pts, pkt.pts);
                ao->play(decodedChunk, pts);
                if (!is_external_clock && ao->timestamp() > 0) {//TODO: clear ao buffer
                   // const qreal da = qAbs(pts - ao->timestamp());
                   // if (da > 1.0) { // what if frame duration is long?
                   // }
                    // TODO: check seek_requested(atomic bool)
                    d.clock->updateValue(ao->timestamp());
                }
            } else {
                d.clock->updateDelay(delay += chunk_delay);
            /*
             * why need this even if we add delay? and usleep sounds weird
             * the advantage is if no audio device, the play speed is ok too
             * So is portaudio blocking the thread when playing?
             */
                //TODO: avoid acummulative error. External clock?
                msleep((unsigned long)(chunk_delay * 1000.0));
            }
            decodedPos += chunk;
            decodedSize -= chunk;
            pts += chunk_delay;
            pkt.pts += chunk_delay; // packet not fully decoded, use new pts in the next decoding
            pkt.dts += chunk_delay;
        }
        if (has_ao)
            emit frameDelivered();
        d.last_pts = d.clock->value(); //not pkt.pts! the delay is updated!
    }
    d.packets.clear();
    qDebug("Audio thread stops running...");
}

} //namespace QtAV
