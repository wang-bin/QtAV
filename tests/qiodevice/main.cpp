/******************************************************************************
    VideoGroup:  this file is part of QtAV examples
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtAV>
#include <QtAVWidgets>
#include <QFile>
#include <QBuffer>

using namespace QtAV;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile vidfile(a.arguments().last());

    if (!vidfile.open(QIODevice::ReadOnly))
        return 1;

    AVPlayer player[2];
    WidgetRenderer renderer[2];
    renderer[0].show();
    renderer[0].widget()->setWindowTitle(QString::fromLatin1("Test QFile"));
    renderer[1].show();
    renderer[1].widget()->setWindowTitle(QString::fromLatin1("Test QBuffer. Play <=1M video from memory"));
    player[0].addVideoRenderer(&renderer[0]);
    player[1].addVideoRenderer(&renderer[1]);

    QByteArray data = vidfile.read(1024*1024);
    vidfile.seek(0);
    QBuffer buf(&data);
    if (buf.open(QIODevice::ReadOnly)) {
        player[1].setIODevice(&buf);
    }
    player[0].setIODevice(&vidfile);
    player[0].play();
    player[1].play();

    return a.exec();
}
