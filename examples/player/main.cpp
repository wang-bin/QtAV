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

#include <QtCore/QDir>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QMessageBox>

#include <QtAV/AVPlayer.h>
#include <QtAV/VideoRendererTypes.h>
#include <QtAV/WidgetRenderer.h>
#include <QtAV/GLWidgetRenderer.h>
#include <QtAV/Direct2DRenderer.h>
#include <QtAV/GDIRenderer.h>
#include <QtAV/XVRenderer.h>
#include <QtAV/OSDFilter.h>

#include "MainWindow.h"

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
    if (a.arguments().contains("-h") || a.arguments().contains("--help")) {
        qDebug("Usage: %s [-vo qt/gl/d2d/gdi] [url/path]filename", a.applicationFilePath().section(QDir::separator(), -1).toUtf8().constData());
        qDebug("\n%s", aboutQtAV_PlainText().toUtf8().constData());
        return 0;
    }
    QTranslator ts;
    if (ts.load(qApp->applicationDirPath() + "/i18n/QtAV_" + QLocale::system().name())) {
        a.installTranslator(&ts);
    } else {
        if (ts.load(":/i18n/QtAV_" + QLocale::system().name()))
            a.installTranslator(&ts);
    }
    QTranslator qtts;
    if (qtts.load("qt_" + QLocale::system().name()))
        a.installTranslator(&qtts);

    sLogfile = fopen(QString(qApp->applicationDirPath() + "/log.txt").toUtf8().constData(), "w+");
    if (!sLogfile) {
        qWarning("Failed to open log file");
        sLogfile = stdout;
    }
    qInstallMessageHandler(Logger);

    QString vo;
    int idx = a.arguments().indexOf("-vo");
    if (idx > 0) {
        vo = a.arguments().at(idx+1);
    } else {
        QString exe(a.arguments().at(0));
        qDebug("exe: %s", exe.toUtf8().constData());
        int i = exe.lastIndexOf('-');
        if (i > 0) {
            vo = exe.mid(i+1, exe.indexOf('.') - i - 1);
        }
    }
    qDebug("vo: %s", vo.toUtf8().constData());
    QString media_file;
    if (argc > idx + 2 ) { //>-1+2=1
        media_file = a.arguments().last();
    }
    vo = vo.toLower();
    if (vo != "gl" && vo != "d2d" && vo != "gdi" && vo != "xv")
        vo = "qpainter";
    QString title = "QtAV " /*+ vo + " "*/ + QtAV_Version_String_Long() + " wbsecg1@gmail.com";
    VideoRenderer *renderer = 0;
    if (vo == "gl") {
        renderer = VideoRendererFactory::create(VideoRendererId_GLWidget);
    } else if (vo == "d2d") {
        renderer = VideoRendererFactory::create(VideoRendererId_Direct2D);
    } else if (vo == "gdi") {
        renderer = VideoRendererFactory::create(VideoRendererId_GDI);
    } else if (vo == "xv") {
        renderer = VideoRendererFactory::create(VideoRendererId_XV);
    } else {
        renderer = VideoRendererFactory::create(VideoRendererId_Widget);
    }
    if (!renderer) {
        QMessageBox::critical(0, "QtAV", "vo '" + vo + "' not supported");
        return 1;
    }
    renderer->widget()->show();
    renderer->widget()->setWindowTitle(title);
    renderer->osdFilter()->setShowType(OSD::ShowNone);
    //renderer->scaleInRenderer(false);
    renderer->setOutAspectRatioMode(VideoRenderer::VideoAspectRatio);

    MainWindow window;
    window.show();
    window.setWindowTitle(title);
    window.setRenderer(renderer);
    if (!media_file.isEmpty()) {
        window.play(media_file);
    }
    int ret = a.exec();
    delete renderer;
    return ret;
}
