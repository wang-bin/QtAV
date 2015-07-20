/******************************************************************************
    transcode:  this file is part of QtAV examples
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

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

#include <QCoreApplication>
#include <QtDebug>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include <QtAV>
#include <QtAV/VideoEncoder.h>
#include <QtAV/AVMuxer.h>

using namespace QtAV;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QString file = "test.avi";
    int idx = a.arguments().indexOf("-i");
    if (idx > 0)
        file = a.arguments().at(idx + 1);
    QString decName("FFmpeg");
    idx = a.arguments().indexOf("-d:v");
    if (idx > 0)
        decName = a.arguments().at(idx + 1);
    QString outFile("/tmp/out.mp4");
    idx = a.arguments().indexOf("-o");
    if (idx > 0)
        outFile = a.arguments().at(idx + 1);
    QDir().mkpath(outFile.left(outFile.lastIndexOf('/')+1));

    QString cv("libx264");
    idx = a.arguments().indexOf("-c:v");
    if (idx > 0)
        cv = a.arguments().at(idx + 1);

    QString fmt;
    idx = a.arguments().indexOf("-f");
    if (idx > 0)
        fmt = a.arguments().at(idx + 1);


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
    dec->open();
    QElapsedTimer timer;
    timer.start();
    int count = 0;
    int vstream = demux.videoStream();
    VideoEncoder *venc = VideoEncoder::create("FFmpeg");
    venc->setCodecName(cv);
    //venc->setCodecName("png");
    venc->setBitRate(1024*1024);
    //venc->setPixelFormat(VideoFormat::Format_RGBA32);
    AVMuxer mux;
    //mux.setMedia("/Users/wangbin/Movies/m3u8/bbb%05d.ts");
    //mux.setMedia("/Users/wangbin/Movies/img2/bbb%05d.png");
    mux.setMedia(outFile);
    QVariantHash muxopt, avfopt;
    avfopt["segment_time"] = 4;
    avfopt["segment_list_size"] = 0;
    avfopt["segment_list"] = outFile.left(outFile.lastIndexOf('/')+1) + "index.m3u8";// "//Users/wangbin/Movies/m3u8/bbb.m3u8";
    avfopt["segment_format"] = "mpegts";
    muxopt["avformat"] = avfopt;
    qreal fps = 0;
    while (!demux.atEnd()) {
        if (!demux.readFrame())
            continue;
        if (demux.stream() != vstream)
            continue;
        const Packet pkt = demux.packet();
        if (dec->decode(pkt)) {
            VideoFrame frame = dec->frame(); // why is faster to call frame() for hwdec? no frame() is very slow for VDA
            if (!frame)
                continue;
            if (!venc->isOpen()) {
                venc->setWidth(frame.width());
                venc->setHeight(frame.height());
                if (!venc->open()) {
                    qWarning("failed to open encoder");
                    return 1;
                }
            }
            if (!mux.isOpen()) {
                mux.copyProperties(venc);
                mux.setOptions(muxopt);
                if (!fmt.isEmpty())
                    mux.setFormat(fmt);
                //mux.setFormat("segment");
               // mux.setFormat("image2");
                if (!mux.open()) {
                    qWarning("failed to open muxer");
                    return 1;
                }
                //mux.setOptions(muxopt);
            }
            if (frame.pixelFormat() != venc->pixelFormat())
                frame = frame.to(venc->pixelFormat());
            if (venc->encode(frame)) {
                Packet pkt(venc->encoded());
                mux.writeVideo(pkt);
                count++;
                if (count%20 == 0) {
                    fps = qreal(count*1000)/qreal(timer.elapsed());
                }
                printf("decode count: %d, fps: %.2f frame size: %dx%d %d\r", count, fps, frame.width(), frame.height(), frame.data().size());fflush(0);
            }
        }
    }
    // get delayed frames
    while (venc->encode()) {
        qDebug("encode delayed frames...\r");
        Packet pkt(venc->encoded());
        mux.writeVideo(pkt);
    }
    qint64 elapsed = timer.elapsed();
    int msec = elapsed/1000LL+1;
    qDebug("decoded frames: %d, time: %d, average speed: %d", count, msec, count/msec);
    venc->close();
    mux.close();
    return 0;
}
