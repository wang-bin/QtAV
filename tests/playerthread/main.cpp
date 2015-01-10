/******************************************************************************
    VideoGroup:  this file is part of QtAV examples
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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

using namespace QtAV;
class Thread : public QThread
{
public:
    Thread(AVPlayer *player):
        QThread(0)
      , mpPlayer(player)
    {}
protected:
    virtual void run() {
        //mpPlayer->play();
        exec();
    }
    AVPlayer *mpPlayer;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    AVPlayer player;
    WidgetRenderer renderer;
    renderer.show();
    player.addVideoRenderer(&renderer);
    player.setFile(a.arguments().last());
    Thread thread(&player);
    player.moveToThread(&thread);
    thread.start();
    player.play();

    return a.exec();
}
