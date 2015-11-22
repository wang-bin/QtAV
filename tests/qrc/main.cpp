/******************************************************************************
    Simple Player:  this file is part of QtAV examples
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

#include <QtAV>
#include <QtAVWidgets>

int main(int argc, char *argv[])
{
    QtAV::Widgets::registerRenderers();
    QApplication a(argc, argv);
    QtAV::VideoOutput renderer;
    renderer.widget()->show();
    renderer.widget()->setWindowTitle(QString::fromLatin1("Play video from qrc--QtAV %1 wbsecg1@gmail.com").arg(QtAV_Version_String_Long()));
    QtAV::AVPlayer player;
    player.setRenderer(&renderer);

    player.play(QString::fromLatin1("qrc:/test.mp4"));

    return a.exec();
}
