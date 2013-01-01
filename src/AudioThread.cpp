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
        if (pkt.data.isNull()) {
            qDebug("Empty packet!");
            d.mutex.unlock();
            continue;
        }
        d.clock->updateValue(pkt.pts);
        if (d.dec->decode(pkt.data)) {
            QByteArray decoded(d.dec->data());
            int decodedSize = decoded.size();
            int decodedPos = 0;
            qreal delay =0;
            while (decodedSize > 0) {
                int chunk = qMin(decodedSize, int(max_len*csf));
                QByteArray decodedChunk(chunk, 0); //volume == 0 || mute
                if (ao) {
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
                    msleep((qreal)chunk/(qreal)csf * 1000);
                }
                d.clock->updateDelay(delay += (qreal)chunk/(qreal)csf);
                decodedPos += chunk;
                decodedSize -= chunk;
            }
		}
        d.mutex.unlock();
    }
    qDebug("Audio thread stops running...");
}

} //namespace QtAV
