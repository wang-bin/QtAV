/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/


#include <QtAV/EventFilter.h>
#include <QApplication>
#include <QtCore/QUrl>
#include <QEvent>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QtAV/AVPlayer.h>
#include <QtAV/AudioOutput.h>

namespace QtAV {

EventFilter::EventFilter(QObject *parent) :
    QObject(parent)
{
}

EventFilter::~EventFilter()
{
}

//TODO: single player event filter
bool EventFilter::eventFilter(QObject *watched, QEvent *event)
{
    //qDebug("EventFilter::eventFilter to %p", watched);
    Q_UNUSED(watched);
    AVPlayer *player = static_cast<AVPlayer*>(parent());
    if (!player)
        return false;
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
            player->captureVideo();
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
            if (player->audio()) {
                qreal v = player->audio()->volume();
                if (v > 0.5)
                    v += 0.1;
                else if (v > 0.1)
                    v += 0.05;
                else
                    v += 0.025;
                player->audio()->setVolume(v);
                qDebug("vol = %.3f", player->audio()->volume());
            }
            break;
        case Qt::Key_Down:
            if (player->audio()) {
                qreal v = player->audio()->volume();
                if (v > 0.5)
                    v -= 0.1;
                else if (v > 0.1)
                    v -= 0.05;
                else
                    v -= 0.025;
                player->audio()->setVolume(v);
                qDebug("vol = %.3f", player->audio()->volume());
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
            if (player->audio()) {
                player->audio()->setMute(!player->audio()->isMute());
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
    case QEvent::DragEnter:
    case QEvent::DragMove: {
        QDropEvent *e = static_cast<QDropEvent*>(event);
        e->acceptProposedAction();
    }
        break;
    case QEvent::Drop: {
        QDropEvent *e = static_cast<QDropEvent*>(event);
        QString path = e->mimeData()->urls().first().toLocalFile();
        player->stop();
        player->load(path);
        player->play();
        e->acceptProposedAction();
    }
        break;
    default:
        return false;
    }
    return true; //false: for text input
}

} //namespace QtAV
