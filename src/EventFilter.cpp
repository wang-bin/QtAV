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


#include <QtAV/EventFilter.h>
#include <QApplication>
#include <QEvent>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QtAV/AVPlayer.h>
#include <QtAV/AudioOutput.h>

namespace QtAV {

EventFilter::EventFilter(AVPlayer *parent) :
    QObject(parent),player(parent)
{
}

bool EventFilter::eventFilter(QObject *watched, QEvent *event)
{
    //qDebug("EventFilter::eventFilter to %p", watched);
    Q_UNUSED(watched);
    /*if (watched == reinterpret_cast<QObject*>(player->renderer)) {
        qDebug("Event target is renderer: %s", watched->objectName().toAscii().constData());
    }*/
    //TODO: if not send to QWidget based class, return false; instanceOf()?
    QEvent::Type type = event->type();
    switch (type) {
    case QEvent::MouseButtonPress:
        qDebug("EventFilter: Mouse press");
        static_cast<QMouseEvent*>(event)->button();
        //TODO: wheel to control volume etc.
        return false;
        break;
    case QEvent::KeyPress: {
        //qDebug("Event target = %p %p", watched, player->renderer);
        //avoid receive an event multiple times
        int key = static_cast<QKeyEvent*>(event)->key();
        switch (key) {
        case Qt::Key_C: //capture
            player->capture();
            break;
        case Qt::Key_N: //check playing?
            player->playNextFrame();
            break;
        case Qt::Key_P:
            player->play();
            break;
        case Qt::Key_S:
            player->stop(); //check playing?
            break;
        case Qt::Key_Space: //check playing?
            qDebug("isPaused = %d", player->isPaused());
            player->pause(!player->isPaused());
            break;
        case Qt::Key_F: { //TODO: move to gui
            QWidget *w = qApp->activeWindow();
            if (!w)
                return false;
            if (w->isFullScreen())
                w->showNormal();
            else
                w->showFullScreen();
        }
            break;
        case Qt::Key_Up:
            if (player->audio) {
                player->audio->setVolume(player->audio->volume() + 0.1);
                qDebug("vol = %.1f", player->audio->volume());
            }
            break;
        case Qt::Key_Down:
            if (player->audio) {
                player->audio->setVolume(player->audio->volume() - 0.1);
                qDebug("vol = %.1f", player->audio->volume());
            }
            break;
        case Qt::Key_O: {
            //TODO: emit a signal so we can use custome dialogs
            QString file = QFileDialog::getOpenFileName(0, tr("Open a video"));
            if (!file.isEmpty())
                player->play(file);
        }
            break;
        case Qt::Key_Left:
            qDebug("<-");
            player->seekBackward();
            break;
        case Qt::Key_Right:
            qDebug("->");
            player->seekForward();
            break;
        case Qt::Key_M:
            if (player->audio) {
                player->audio->setMute(!player->audio->isMute());
            }
            break;
        case Qt::Key_T: {
            QWidget *w = qApp->activeWindow();
            if (!w)
                return false;
            Qt::WindowFlags wf = w->windowFlags();
            if (wf & Qt::WindowStaysOnTopHint) {
                qDebug("Window not stays on top");
                w->setWindowFlags(wf & ~Qt::WindowStaysOnTopHint);
            } else {
                qDebug("Window stays on top");
                w->setWindowFlags(wf | Qt::WindowStaysOnTopHint);
            }
            //call setParent() when changing the flags, causing the widget to be hidden
            w->show();
        }
            break;
        default:
            return false;
        }
        break;
    }
    default:
        return false;
    }
    return true; //false: for text input
}

} //namespace QtAV
