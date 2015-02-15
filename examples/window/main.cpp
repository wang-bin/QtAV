/******************************************************************************
    Simple Player:  this file is part of QtAV examples
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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
#include <QtGui>
#include <QtAV>

using namespace QtAV;
class PlayerWindow : public OpenGLWindowRenderer
{
public:
    PlayerWindow(QWindow* parent = 0)
        : OpenGLWindowRenderer(NoPartialUpdate, parent)
    {
        player.setRenderer(this);
    }
    void play(const QString& file) {
        setTitle(file);
        player.setFile(file);
        player.play();
    }
protected:
    virtual void keyPressEvent(QKeyEvent *e) {
        int key = e->key();
        if (key == Qt::Key_Space) {
            player.togglePause();
        } else if (key == Qt::Key_Left) {
            player.seekBackward();
        } else if (key == Qt::Key_Right) {
            player.seekForward();
        }
    }
private:
    AVPlayer player;
};

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    if (a.arguments().size() < 2) {
        qDebug("./window file");
        return 0;
    }
    PlayerWindow win;
    win.resize(600, 400);
    win.show();
    win.play(a.arguments().at(1));
    return a.exec();
}
