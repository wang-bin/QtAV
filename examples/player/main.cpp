/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
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

#include <cstdio>
#include <cstdlib>
#include <QApplication>
#include <QtDebug>
#include <QtCore/QDir>
#include <QMessageBox>

#include <QtAV/AVPlayer.h>
#include <QtAV/VideoOutput.h>
#include <QtAVWidgets>
#include "MainWindow.h"
#include "../common/common.h"

using namespace QtAV;

static FILE *sLogfile = 0; //'log' is a function in msvc math.h

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
class QMessageLogContext {};
typedef void (*QtMessageHandler)(QtMsgType, const QMessageLogContext &, const QString &);
QtMsgHandler qInstallMessageHandler(QtMessageHandler h) {
    static QtMessageHandler hh;
    hh = h;
    struct MsgHandlerWrapper {
        static void handler(QtMsgType type, const char *msg) {
            static QMessageLogContext ctx;
            hh(type, ctx, msg);
        }
    };
    return qInstallMsgHandler(MsgHandlerWrapper::handler);
}
#endif
void Logger(QtMsgType type, const QMessageLogContext &, const QString& qmsg)
{
    const QByteArray msgArray = qmsg.toLocal8Bit();
    const char* msg = msgArray.constData();
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
    QApplication a(argc, argv);
    QDir::setCurrent(qApp->applicationDirPath());
    QOptions options = get_common_options();
    options.add(QString::fromLatin1("player options"))
            ("-vo", QString::fromLatin1("gl"), QString::fromLatin1("video renderer engine. can be gl, qt, d2d, gdi, xv."))
            ("no-ffmpeg-log", QString::fromLatin1("disable ffmpeg log"))
            ("logfile", QString::fromLatin1("log.txt"), QString::fromLatin1("log to file. Set empty to disable log file (-logfile '')"))
            ;
    options.parse(argc, argv);
    if (options.value(QString::fromLatin1("help")).toBool()) {
        qDebug() << aboutQtAV_PlainText();
        options.print();
        return 0;
    }

    set_opengl_backend(options.option(QString::fromLatin1("gl")).value().toString(), a.arguments().first());
    load_qm(QStringList() << QString::fromLatin1("player"), options.value(QString::fromLatin1("language")).toString());

    QString logfile(options.option(QString::fromLatin1("logfile")).value().toString());
    if (!logfile.isEmpty()) {
        sLogfile = fopen(logfile.toUtf8().constData(), "w+");
        if (!sLogfile) {
            qWarning("Failed to open log file");
            sLogfile = stdout;
        }
        qInstallMessageHandler(Logger);
    }

    qDebug() <<a.arguments();
    QOption op = options.option(QString::fromLatin1("vo"));
    QString vo = op.value().toString();
    if (!op.isSet()) {
        QString exe(a.arguments().at(0));
        int i = exe.lastIndexOf(QLatin1Char('-'));
        if (i > 0) {
            vo = exe.mid(i+1, exe.indexOf(QLatin1Char('.')) - i - 1);
        }
    }
    qDebug("vo: %s", vo.toUtf8().constData());
    vo = vo.toLower();
    if (vo != QLatin1String("opengl") && vo != QLatin1String("gl") && vo != QLatin1String("d2d") && vo != QLatin1String("gdi") && vo != QLatin1String("xv") && vo != QLatin1String("qt"))
#ifdef Q_OS_ANDROID
        vo = "opengl"; // qglwidget is not suitable for android
#else
        vo = QString::fromLatin1("gl");
#endif
    QString title = QString::fromLatin1("QtAV %1 wbsecg1@gmail.com").arg(QtAV_Version_String_Long());
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
    { "opengl", VideoRendererId_OpenGLWidget },
    { "gl", VideoRendererId_GLWidget2 },
    { "d2d", VideoRendererId_Direct2D },
    { "gdi", VideoRendererId_GDI },
    { "xv", VideoRendererId_XV },
    { "qt", VideoRendererId_Widget },
    { 0, 0 }
    };
    for (int i = 0; vid_map[i].name; ++i) {
        if (vo == QLatin1String(vid_map[i].name)) {
            vid = vid_map[i].id;
            break;
        }
    }
    VideoOutput *renderer = new VideoOutput(vid); //or VideoRenderer
    if (!renderer) {
        QMessageBox::critical(0, QString::fromLatin1("QtAV"), QString::fromLatin1("vo '%1' not supported").arg(vo));
        return 1;
    }
    //renderer->scaleInRenderer(false);
    renderer->setOutAspectRatioMode(VideoRenderer::VideoAspectRatio);

    MainWindow window;
    AppEventFilter ae(&window);
    qApp->installEventFilter(&ae);

    window.show();
    window.setWindowTitle(title);
    window.setRenderer(renderer);
    int w = renderer->widget()->width();
    int h = renderer->widget()->width()*9/16;
    int x = window.x();
    int y = window.y();
    op = options.option(QString::fromLatin1("width"));
    w = op.value().toInt();
    op = options.option(QString::fromLatin1("height"));
    h = op.value().toInt();
    op = options.option(QString::fromLatin1("x"));
    if (op.isSet())
        x = op.value().toInt();
    op = options.option(QString::fromLatin1("y"));
    if (op.isSet())
        y = op.value().toInt();
    window.resize(w, h);
    window.move(x, y);
    if (options.value(QString::fromLatin1("fullscreen")).toBool())
        window.showFullScreen();

    op = options.option(QString::fromLatin1("ao"));
    if (op.isSet()) {
        QString aos(op.value().toString());
        QStringList ao;
        if (aos.contains(QString::fromLatin1(";")))
            ao = aos.split(QString::fromLatin1(";"), QString::SkipEmptyParts);
        else
            ao = aos.split(QString::fromLatin1(","), QString::SkipEmptyParts);
        window.setAudioBackends(ao);
    }

    op = options.option(QString::fromLatin1("vd"));
    if (op.isSet()) {
        QStringList vd = op.value().toString().split(QString::fromLatin1(";"), QString::SkipEmptyParts);
        if (!vd.isEmpty())
            window.setVideoDecoderNames(vd);
    }

    if (options.value(QString::fromLatin1("no-ffmpeg-log")).toBool())
        setFFmpegLogHandler(0);
    op = options.option(QString::fromLatin1("file"));
    if (op.isSet()) {
        qDebug() << "-f set: " << op.value().toString();
        window.play(op.value().toString());
    } else {
        if (argc > 1 && !a.arguments().last().startsWith(QLatin1Char('-')) && !a.arguments().at(argc-2).startsWith(QLatin1Char('-')))
            window.play(a.arguments().last());
    }
    int ret = a.exec();
    return ret;
}
