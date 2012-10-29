#include <QtAV/AudioThread.h>
#include <private/AVThread_p.h>
#include <QtAV/AudioDecoder.h>
#include <QtAV/QAVPacket.h>
#include <QtAV/AudioOutput.h>
#include <QtCore/QCoreApplication>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
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

void AudioThread::run()
{
    //No decoder or output, no audio
    if (!d_ptr->dec || !d_ptr->writer)
        return;
	Q_ASSERT(d_ptr->packets != 0);
	while (true) {
        d_ptr->mutex.lock();
		int i = 0;
		while (d_ptr->packets->isEmpty()) {
            d_ptr->not_empty_cond.wait(&d_ptr->mutex, 1000); //why sometines return immediatly?
			qDebug("audio data size: %d, loop %d", d_ptr->packets->size(), ++i);
        }
        QAVPacket pkt = d_ptr->packets->dequeue();
		//qDebug("audio data size after dequeue: %d", d_ptr->packets->size());
        if (d_ptr->dec->decode(pkt.data)) {
            //play sound
            QByteArray decoded(d_ptr->dec->data());
            int decodedSize = decoded.size();
            int decodedPos = 0;
            while (decodedSize > 0) {
                static int sample_rate = d_ptr->dec->codecContext()->sample_rate;
                static int channels = d_ptr->dec->codecContext()->channels;
                const double max_len = /*playC.frame_last_delay > 0.0 ? qMin( playC.frame_last_delay, 0.02 ) :*/ 0.02;
                const int chunk = qMin(decodedSize, int(ceil(sample_rate * max_len) * channels * sizeof(float)));

                QByteArray decodedChunk;
                //if ( playC.vol == 0.0 || playC.muted )
                //  decodedChunk.fill( 0, chunk );
                //else
                decodedChunk = QByteArray::fromRawData(decoded.data() + decodedPos, chunk);

                decodedPos += chunk;
                decodedSize -= chunk;
				d_func()->writer->write(decodedChunk);
				//qApp->processEvents(QEventLoop::AllEvents);
            }
		}
        d_ptr->mutex.unlock();
    }
}

}
