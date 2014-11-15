/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include <cstdio>
#include <cstdlib>
#include <QApplication>

#include <QtDebug>
#include <QtCore/QDir>
#include <QMessageBox>

#include <QtAV/AVPlayer.h>
#include <QtAV/VideoRendererTypes.h>
#include <QtAV/VideoOutput.h>

#include "MainWindow.h"
#include "../common/common.h"

using namespace QtAV;

static FILE *sLogfile = 0; //'log' is a function in msvc math.h

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#define qInstallMessageHandler qInstallMsgHandler
void Logger(QtMsgType type, const char *msg)
{
#else
void Logger(QtMsgType type, const QMessageLogContext &, const QString& qmsg)
{
    const QByteArray msgArray = qmsg.toLocal8Bit();
    const char* msg = msgArray.constData();
#endif
	 switch (type) {
     case QtDebugMsg:
		 fprintf(stdout, "Debug: %s\n", msg);
         if (sLogfile)
            fprintf(sLogfile, "Debug: %s\n", msg);
         break;
     case QtWarningMsg:
		 fprintf(stdout, "Warning: %s\n", msg);
         if (sLogfile)
            fprintf(sLogfile, "Warning: %s\n", msg);
		 break;
     case QtCriticalMsg:
		 fprintf(stderr, "Critical: %s\n", msg);
         if (sLogfile)
            fprintf(sLogfile, "Critical: %s\n", msg);
		 break;
     case QtFatalMsg:
		 fprintf(stderr, "Fatal: %s\n", msg);
         if (sLogfile)
            fprintf(sLogfile, "Fatal: %s\n", msg);
		 abort();
     }
     fflush(0);
}

int main(int argc, char *argv[])
{
    // has no effect if qInstallMessageHandler() called
    //qSetMessagePattern("%{function} @%{line}: %{message}");

    QOptions options = get_common_options();
    options.add("player options")
            ("-vo", "gl", "video renderer engine. can be gl, qt, d2d, gdi, xv.")
            ("ao", "", "audio output. can be 'null'")
            ("no-ffmpeg-log", "disable ffmpeg log")
            ;
    options.parse(argc, argv);
    if (options.value("help").toBool()) {
        qDebug() << aboutQtAV_PlainText();
        options.print();
        return 0;
    }

    QApplication a(argc, argv);
    load_qm(QStringList() << "player", options.value("language").toString());

    sLogfile = fopen(QString(qApp->applicationDirPath() + "/log.txt").toUtf8().constData(), "w+");
    if (!sLogfile) {
        qWarning("Failed to open log file");
        sLogfile = stdout;
    }
    qInstallMessageHandler(Logger);

    QOption op = options.option("vo");
    QString vo = op.value().toString();
    if (!op.isSet()) {
        QString exe(a.arguments().at(0));
        int i = exe.lastIndexOf('-');
        if (i > 0) {
            vo = exe.mid(i+1, exe.indexOf('.') - i - 1);
        }
    }
    qDebug("vo: %s", vo.toUtf8().constData());
    vo = vo.toLower();
    if (vo != "gl" && vo != "d2d" && vo != "gdi" && vo != "xv" && vo != "qt")
        vo = "gl";
    QString title = "QtAV " /*+ vo + " "*/ + QtAV_Version_String_Long() + " wbsecg1@gmail.com";
#ifndef QT_NO_OPENGL
    VideoRendererId vid = VideoRendererId_GLWidget2;
#else
    VideoRendererId vid = VideoRendererId_Widget;
#endif
    // TODO: move to VideoRendererTypes or factory to query name
    struct {
        const char* name;
        VideoRendererId id;
    } vid_map[] = {
    { "gl", VideoRendererId_GLWidget2 },
    { "d2d", VideoRendererId_Direct2D },
    { "gdi", VideoRendererId_GDI },
    { "xv", VideoRendererId_XV },
    { "qt", VideoRendererId_Widget },
    { 0, 0 }
    };
    for (int i = 0; vid_map[i].name; ++i) {
        if (vo == vid_map[i].name) {
            vid = vid_map[i].id;
            break;
        }
    }
    VideoOutput *renderer = new VideoOutput(vid); //or VideoRenderer
    if (!renderer) {
        QMessageBox::critical(0, "QtAV", "vo '" + vo + "' not supported");
        return 1;
    }
    //renderer->scaleInRenderer(false);
    renderer->setOutAspectRatioMode(VideoRenderer::VideoAspectRatio);

    MainWindow window;
    window.show();
    window.setWindowTitle(title);
    window.setRenderer(renderer);
    int w = renderer->widget()->width();
    int h = renderer->widget()->width()*9/16;
    int x = window.x();
    int y = window.y();
    op = options.option("width");
    w = op.value().toInt();
    op = options.option("height");
    h = op.value().toInt();
    op = options.option("x");
    if (op.isSet())
        x = op.value().toInt();
    op = options.option("y");
    if (op.isSet())
        y = op.value().toInt();
    window.resize(w, h);
    window.move(x, y);
    if (options.value("fullscreen").toBool())
        window.showFullScreen();

    window.enableAudio(options.value("ao").toString() != "null");

    op = options.option("vd");
    if (op.isSet()) {
        QStringList vd = op.value().toString().split(";", QString::SkipEmptyParts);
        if (!vd.isEmpty())
            window.setVideoDecoderNames(vd);
    }

    if (options.value("no-ffmpeg-log").toBool())
        setFFmpegLogHandler(0);
    op = options.option("file");
    if (op.isSet()) {
        window.play(op.value().toString());
    } else {
        if (argc > 1 && !a.arguments().last().startsWith('-') && !a.arguments().at(argc-2).startsWith('-'))
            window.play(a.arguments().last());
    }
    int ret = a.exec();
    return ret;
}
