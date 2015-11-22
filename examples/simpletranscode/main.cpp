/******************************************************************************
    Simple Transcode:  this file is part of QtAV examples
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include <QApplication>
#include <QtCore/QDir>
#include <QtAV>
#include <QtAV/AVTranscoder.h>
#include <QtDebug>

using namespace QtAV;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qDebug("QtAV simpletranscode");
    qDebug("./simpletranscode -i infile -o outfile [-c:v video_codec (default: libx264)] [-f format]");
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
    QVariantHash muxopt, avfopt;
    avfopt[QString::fromLatin1("segment_time")] = 4;
    avfopt[QString::fromLatin1("segment_list_size")] = 0;
    avfopt[QString::fromLatin1("segment_list")] = outFile.left(outFile.lastIndexOf(QLatin1Char('/'))+1).arg(QString::fromLatin1("index.m3u8"));
    avfopt[QString::fromLatin1("segment_format")] = QString::fromLatin1("mpegts");
    muxopt[QString::fromLatin1("avformat")] = avfopt;

    AVPlayer player;
    player.setFile(file);
    player.setFrameRate(1000.0); // as fast as possible
    player.audio()->setBackends(QStringList() << QString::fromLatin1("null"));
    AVTranscoder avt;
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
    if (fmt == QLatin1String("image2"))
        venc->setPixelFormat(VideoFormat::Format_RGBA32);

    QObject::connect(&avt, SIGNAL(stopped()), qApp, SLOT(quit()));
    avt.start(); //start transcoder first
    player.play();

    return a.exec();
}
