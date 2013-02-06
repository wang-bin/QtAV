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

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtOpenGL/QGLWidget>

#include <QtAV/AVPlayer.h>
#include <QtAV/WidgetRenderer.h>
#include <QtAV/GraphicsItemRenderer.h>
//#include <QtAV/GLWidgetRenderer.h>
using namespace QtAV;

FILE *log = 0;

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
		 if (log)
			fprintf(log, "Debug: %s\n", msg);
         break;
     case QtWarningMsg:
		 fprintf(stdout, "Warning: %s\n", msg);
		 if (log)
			fprintf(log, "Warning: %s\n", msg);
		 break;
     case QtCriticalMsg:
		 fprintf(stderr, "Critical: %s\n", msg);
		 if (log)
			fprintf(log, "Critical: %s\n", msg);
		 break;
     case QtFatalMsg:
		 fprintf(stderr, "Fatal: %s\n", msg);
		 if (log)
			fprintf(log, "Fatal: %s\n", msg);
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

    log = fopen(qApp->applicationDirPath() + "/log.txt", "w+");
    if (!log) {
        qWarning("Failed to open log file");
        log = stdout;
    }
    qInstallMessageHandler(Logger);
#if 0
    QGraphicsScene s;
    s.setSceneRect(0, 0, 800, 600);
    QGraphicsView w(&s);
	w.showMaximized();

#ifndef QT_NO_OPENGL
    QGLWidget *glw = new QGLWidget(QGLFormat(QGL::SampleBuffers));
    glw->setAutoFillBackground(false);
    w.setCacheMode(QGraphicsView::CacheNone);
    w.setViewport(glw);
#else
    w.setCacheMode(QGraphicsView::CacheBackground);
#endif
	GraphicsItemRenderer renderer;
    renderer.resizeVideo(800, 600);
    s.addItem(&renderer);
#else
    WidgetRenderer renderer;
    renderer.show();
    renderer.setWindowTitle("QtAV " QTAV_VERSION_STR_LONG " wbsecg1@gmail.com");
    //renderer.resize(800, 600);
#endif
    QString fileName;
	if (argc > 1)
		fileName = a.arguments().at(1);

	AVPlayer player;
    player.setRenderer(&renderer);
    if (!fileName.isEmpty()) {
        player.play(fileName);
    }

    return a.exec();
}
