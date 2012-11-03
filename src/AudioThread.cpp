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
#include <QtAV/AVDemuxThread.h>
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

const int kAPktBufferMin = 256;

class AudioThreadPrivate : public AVThreadPrivate
{
public:

};

AudioThread::AudioThread(QObject *parent)
    :AVThread(*new AudioThreadPrivate(), parent)
{
}

void AudioThread::run()
{
    //No decoder or output, no audio
    if (!d_ptr->dec || !d_ptr->writer)
        return;
    Q_ASSERT(d_ptr->clock != 0);
    Q_ASSERT(d_ptr->demux_thread != 0);
    int sample_rate = d_ptr->dec->codecContext()->sample_rate;
    int channels = d_ptr->dec->codecContext()->channels;
    int csf = channels * sample_rate * sizeof(float);
    const double max_len = 0.02;
    while (!d_ptr->stop) {
        d_ptr->mutex.lock();
        while (d_ptr->packets.isEmpty() && !d_ptr->stop) {
            d_ptr->condition.wait(&d_ptr->mutex); //why sometines return immediatly?
            qDebug("audio data size: %d", d_ptr->packets.size());
        }
        if (d_ptr->stop) {
            d_ptr->mutex.unlock();
            break;
        }
        Packet pkt = d_ptr->packets.dequeue();
        if (d_ptr->packets.size() < kAPktBufferMin) {
            d_ptr->demux_thread->readMoreFrames();
        }
        d_ptr->clock->updateValue(pkt.pts);
        if (d_ptr->dec->decode(pkt.data)) {
            QByteArray decoded(d_ptr->dec->data());
            int decodedSize = decoded.size();
            int decodedPos = 0;
            while (decodedSize > 0) {
                const int chunk = qMin(decodedSize, int(max_len*csf));
                QByteArray decodedChunk(chunk, 0); //volume == 0 || mute
                decodedChunk = QByteArray::fromRawData(decoded.constData() + decodedPos, chunk);
                decodedPos += chunk;
                decodedSize -= chunk;
                d_ptr->clock->updateDelay(chunk/csf);
                if (d_func()->writer)
                    d_func()->writer->write(decodedChunk);
				//qApp->processEvents(QEventLoop::AllEvents);
            }
		}
        d_ptr->mutex.unlock();
    }
    qDebug("Audio thread stops running...");
}

} //namespace QtAV
