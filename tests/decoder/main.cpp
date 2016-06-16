#include <QCoreApplication>
#include <QtDebug>
#include <QtCore/QDateTime>
#include <QtCore/QQueue>
#include <QtCore/QStringList>
#include <QtAV/AVDemuxer.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/Packet.h>

using namespace QtAV;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QString file = QString::fromLatin1("test.avi");
    int idx = a.arguments().indexOf(QLatin1String("-f"));
    if (idx > 0)
        file = a.arguments().at(idx + 1);
    QString decName = QString::fromLatin1("FFmpeg");
    idx = a.arguments().indexOf(QLatin1String("-vc"));
    if (idx < 0)
        idx = a.arguments().indexOf(QLatin1String("-vd"));
    if (idx > 0)
        decName = a.arguments().at(idx + 1);

    QString opt;
    QVariantHash decopt;
    idx = decName.indexOf(QLatin1String(":"));
    if (idx > 0) {
        opt = decName.right(decName.size() - idx -1);
        decName = decName.left(idx);
        QStringList opts(opt.split(QString::fromLatin1(";")));
        QVariantHash subopt;
        foreach (QString o, opts) {
            idx = o.indexOf(QLatin1String(":"));
            subopt[o.left(idx)] = o.right(o.size() - idx - 1);
        }
        decopt[decName] = subopt;
    }
    qDebug() << decopt;

    VideoDecoder *dec = VideoDecoder::create(decName.toLatin1().constData());
    if (!dec) {
        fprintf(stderr, "Can not find decoder: %s\n", decName.toUtf8().constData());
        return 1;
    }
    if (!decopt.isEmpty())
        dec->setOptions(decopt);
    AVDemuxer demux;
    demux.setMedia(file);
    if (!demux.load()) {
        qWarning("Failed to load file: %s", file.toUtf8().constData());
        return 1;
    }

    dec->setCodecContext(demux.videoCodecContext());
    dec->open();
    int count = 0;
    int vstream = demux.videoStream();
    QQueue<qint64> t;
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();
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
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            const qint64 dt = now - t0;
            t.enqueue(now);
            printf("decode count: %d, elapsed: %lld, fps: %.1f/%.1f\r", count, dt, count*1000.0/dt, t.size()*1000.0/(now - t.first()));fflush(0);
            if (t.size() > 10)
                t.dequeue();
        }
    }
    return 0;
}
