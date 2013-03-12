/******************************************************************************
    Simple Player:  this file is part of QtAV examples
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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

#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QMessageBox>

#include <QtAV/AVPlayer.h>
#include <QtAV/VideoRendererTypes.h>
#include <QtAV/WidgetRenderer.h>
#include <QtAV/GLWidgetRenderer.h>
#include <QtAV/Direct2DRenderer.h>
#include <QtAV/GDIRenderer.h>

using namespace QtAV;

static FILE *sLogfile = 0; //'log' is a function in msvc math.h

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#define qInstallMessageHandler qInstallMsgHandler
void Logger(QtMsgType type, const char *msg)
{
#else
void Logger(QtMsgType type, const QMessageLogContext &, const QString& qmsg)
{
	const char* msg = qPrintable(qmsg);
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
    QApplication a(argc, argv);
    QTranslator ts;
    if (ts.load(qApp->applicationDirPath() + "/i18n/QtAV_" + QLocale::system().name()))
        a.installTranslator(&ts);
    QTranslator qtts;
    if (qtts.load("qt_" + QLocale::system().name()))
        a.installTranslator(&qtts);

    sLogfile = fopen(QString(qApp->applicationDirPath() + "/log.txt").toUtf8().constData(), "w+");
    if (!sLogfile) {
        qWarning("Failed to open log file");
        sLogfile = stdout;
    }
    qInstallMessageHandler(Logger);

    QString vo("qpainter");
    int idx = a.arguments().indexOf("-vo");
    if (idx > 0) {
        vo = a.arguments().at(idx+1);
    }
    QString fileName;
    if (argc > idx + 2 ) { //>-1+2=1
        fileName = a.arguments().last();
    }
    vo = vo.toLower();
    if (vo != "gl" && vo != "d2d" && vo != "gdi")
        vo = "qpainter";
    QString title = "QtAV " + vo + " " QTAV_VERSION_STR_LONG " wbsecg1@gmail.com";
    VideoRenderer *renderer = 0;
    if (vo == "gl") {
        GLWidgetRenderer *r = static_cast<GLWidgetRenderer*>(VideoRendererFactory::create(VideoRendererId_GLWidget));
        if (r) {
            r->show();
            r->setWindowTitle(title);
        }
        renderer = r;
    } else if (vo == "d2d") {
        Direct2DRenderer *r = static_cast<Direct2DRenderer*>(VideoRendererFactory::create(VideoRendererId_Direct2D));
        if (r) { //may not support
            r->show();
            r->setWindowTitle(title);
        }
        renderer = r;
    } else if (vo == "gdi") {
        GDIRenderer *r = static_cast<GDIRenderer*>(VideoRendererFactory::create(VideoRendererId_GDI));
        if (r) {
            r->show();
            r->setWindowTitle(title);
        }
        renderer = r;
    } else {
        WidgetRenderer *r = static_cast<WidgetRenderer*>(VideoRendererFactory::create(VideoRendererId_Widget));
        if (r) {
            r->show();
            r->setWindowTitle(title);
        }
        renderer = r;
    }
    if (!renderer) {
        QMessageBox::critical(0, "QtAV", "vo '" + vo + "' not supported");
        return 1;
    }
    renderer->setOutAspectRatioMode(VideoRenderer::VideoAspectRatio);
    AVPlayer player;
    player.setRenderer(renderer);
    if (!fileName.isEmpty()) {
        player.play(fileName);
    }

    int ret = a.exec();
    delete renderer;
    return ret;
}
