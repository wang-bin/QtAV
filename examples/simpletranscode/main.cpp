/******************************************************************************
    Simple Transcode:  this file is part of QtAV examples
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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
#include <QtCore/QDir>
#include <QtAV>
#include <QtAV/AVTranscoder.h>
#include <QtDebug>

using namespace QtAV;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qDebug("QtAV simpletranscode");
    qDebug("./simpletranscode -i infile -o outfile [-async] [-c:v video_codec (default: libx264)] [-hwdev dev] [-f format] [-an] [-ss HH:mm:ss.z]");
    qDebug("-an: disable audio");
    qDebug() << "examples:\n"
             << "./simpletranscode -i test.mp4 -o /tmp/test-%05d.png -f image2 -c:v png\n"
             << "./simpletranscode -i test.mp4 -o /tmp/bbb%04d.ts -f segment\n"
             << "./simpletranscode -i test.mp4 -o /tmp/test.mkv\n"
             ;
    if (a.arguments().contains(QString::fromLatin1("-h"))) {
        return 0;
    }
    QString file = QString::fromLatin1("test.avi");
    int idx = a.arguments().indexOf(QLatin1String("-i"));
    if (idx > 0)
        file = a.arguments().at(idx + 1);
    QString outFile = QString::fromLatin1("/tmp/out.mp4");
    idx = a.arguments().indexOf(QLatin1String("-o"));
    if (idx > 0)
        outFile = a.arguments().at(idx + 1);
    QDir().mkpath(outFile.left(outFile.lastIndexOf(QLatin1Char('/'))+1));

    QString cv = QString::fromLatin1("libx264");
    idx = a.arguments().indexOf(QLatin1String("-c:v"));
    if (idx > 0)
        cv = a.arguments().at(idx + 1);
    QString ca = QString::fromLatin1("aac");
    idx = a.arguments().indexOf(QLatin1String("-c:a"));
    if (idx > 0)
        ca = a.arguments().at(idx + 1);
    QString fmt;
    idx = a.arguments().indexOf(QLatin1String("-f"));
    if (idx > 0)
        fmt = a.arguments().at(idx + 1);
    QString hwdev;
    idx = a.arguments().indexOf(QLatin1String("-hwdev"));
    if (idx > 0)
        hwdev = a.arguments().at(idx + 1);
    const bool an = a.arguments().contains(QLatin1String("-an"));
    const bool async = a.arguments().contains(QLatin1String("-async"));
    qint64 ss = 0;
    idx = a.arguments().indexOf(QLatin1String("-ss"));
    if (idx > 0)
        ss = QTime(0, 0, 0).msecsTo(QTime::fromString(a.arguments().at(idx + 1), QLatin1String("HH:mm:ss.z")));
    QVariantHash muxopt, avfopt;
    avfopt[QString::fromLatin1("segment_time")] = 4;
    avfopt[QString::fromLatin1("segment_list_size")] = 0;
    avfopt[QString::fromLatin1("segment_list")] = outFile.left(outFile.lastIndexOf(QLatin1Char('/'))+1).arg(QString::fromLatin1("index.m3u8"));
    avfopt[QString::fromLatin1("segment_format")] = QString::fromLatin1("mpegts");
    muxopt[QString::fromLatin1("avformat")] = avfopt;

    AVPlayer player;
    player.setFile(file);
    player.setFrameRate(10000.0); // as fast as possible. FIXME: why 1000 may block source player?
    player.audio()->setBackends(QStringList() << QString::fromLatin1("null"));
    AVTranscoder avt;
    if (ss > 0)
        avt.setStartTime(ss);
    avt.setMediaSource(&player);
    avt.setOutputMedia(outFile);
    avt.setOutputOptions(muxopt);
    if (!fmt.isEmpty())
        avt.setOutputFormat(fmt); // segment, image2
    if (!avt.createVideoEncoder()) {
        qWarning("Failed to create video encoder");
        return 1;
    }
    VideoEncoder *venc = avt.videoEncoder();
    venc->setCodecName(cv); // "png"
    venc->setBitRate(1024*1024);
    if (!hwdev.isEmpty())
        venc->setProperty("hwdevice", hwdev);
    if (fmt == QLatin1String("image2"))
        venc->setPixelFormat(VideoFormat::Format_RGBA32);
    if (an) {
        avt.sourcePlayer()->setAudioStream(-1);
    } else {
        if (!avt.createAudioEncoder()) {
            qWarning("Failed to create audio encoder");
            return 1;
        }
        AudioEncoder *aenc = avt.audioEncoder();
        aenc->setCodecName(ca);
    }

    QObject::connect(&avt, SIGNAL(stopped()), qApp, SLOT(quit()));
    avt.setAsync(async);
    avt.start(); //start transcoder first
    player.play();

    return a.exec();
}
