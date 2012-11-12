/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

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

#if CONFIG_EZX
#include <ZApplication.h>
#else
#include <qapplication.h>
typedef QApplication ZApplication;
#endif //CONFIG_EZX

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QtOpenGL/QGLWidget>

#include <QtAV/AVPlayer.h>
#include <QtAV/WidgetRenderer.h>
#include <QtAV/GraphicsItemRenderer.h>
using namespace QtAV;

int main(int argc, char *argv[])
{
	ZApplication a(argc, argv);
    
    AVPlayer player;
    if (argc > 1)
        player.setFile(a.arguments().at(1));
    else
        QMessageBox::warning(0, "Usage", QString("Command line: %1 path/of/video\nPress \"O\" to open a file").arg(qApp->arguments().at(0)));
#if 0
    QGraphicsScene s;
    s.setSceneRect(0, 0, 800, 600);
    QGraphicsView v(&s);
    //QGLWidget gw;
    //v.setViewport(&gw); //may flick

    GraphicsItemRenderer g;
    g.resizeVideo(800, 600);
    player.setRenderer(&g);
    s.addItem(&g);
    v.show();
#else
    WidgetRenderer w;
    w.setWindowTitle(a.arguments().at(1));
    player.setRenderer(&w);
    w.showMaximized();
 #endif
    player.play();
    return a.exec();
}
