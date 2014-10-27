#include <QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include <QtAV/AVDemuxer.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/Packet.h>

using namespace QtAV;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QString file = "test.avi";
    int idx = a.arguments().indexOf("-f");
    if (idx > 0)
        file = a.arguments().at(idx + 1);
    QString decName("FFmpeg");
    idx = a.arguments().indexOf("-vc");
    if (idx > 0)
        decName = a.arguments().at(idx + 1);

    VideoDecoderId cid = VideoDecoderFactory::id(decName.toStdString());
    if (cid <= 0) {
        qWarning("Can not find decoder: %s", decName.toUtf8().constData());
        return 1;
    }
    VideoDecoder *dec = VideoDecoderFactory::create(cid);
    AVDemuxer demux;
    if (!demux.loadFile(file)) {
        qWarning("Failed to load file: %s", file.toUtf8().constData());
        return 1;
    }

    dec->setCodecContext(demux.videoCodecContext());
    dec->prepare();
    dec->open();
    QElapsedTimer timer;
    timer.start();
    int count = 0;
    int vstream = demux.videoStream();
    while (!demux.atEnd()) {
        if (!demux.readFrame())
            continue;
        if (demux.stream() != vstream)
            continue;
        if (dec->decode(demux.packet()->data)) {
            /*
             * TODO: may contains more than 1 frames
             * map from gpu or not?
             */
            //frame = dec->frame().clone();
            count++;
        }
    }
    qint64 elapsed = timer.elapsed();
    int msec = elapsed/1000LL;
    qDebug("decoded frames: %d, time: %d, average speed: %d", count, msec, count/msec);
    return 0;
}
