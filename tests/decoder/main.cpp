#include <QCoreApplication>
#include <QtDebug>
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
    if (idx < 0)
        idx = a.arguments().indexOf("-vd");
    if (idx > 0)
        decName = a.arguments().at(idx + 1);

    QString opt;
    QVariantHash decopt;
    idx = decName.indexOf(":");
    if (idx > 0) {
        opt = decName.right(decName.size() - idx -1);
        decName = decName.left(idx);
        QStringList opts(opt.split(";"));
        QVariantHash subopt;
        foreach (QString o, opts) {
            idx = o.indexOf(":");
            subopt[o.left(idx)] = o.right(o.size() - idx - 1);
        }
        decopt[decName] = subopt;
    }
    qDebug() << decopt;

    VideoDecoderId cid = VideoDecoderFactory::id(decName.toStdString());
    if (cid <= 0) {
        qWarning("Can not find decoder: %s", decName.toUtf8().constData());
        return 1;
    }
    VideoDecoder *dec = VideoDecoderFactory::create(cid);
    if (!decopt.isEmpty())
        dec->setOptions(decopt);
    AVDemuxer demux;
    demux.setMedia(file);
    if (!demux.load()) {
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
        const Packet pkt = demux.packet();
        if (dec->decode(pkt)) {
            VideoFrame frame = dec->frame(); // why is faster to call frame() for hwdec? no frame() is very slow for VDA
            Q_UNUSED(frame);
            count++;
            printf("decode count: %d\r", count);fflush(0);
        }
    }
    qint64 elapsed = timer.elapsed();
    int msec = elapsed/1000LL+1;
    qDebug("decoded frames: %d, time: %d, average speed: %d", count, msec, count/msec);
    return 0;
}
