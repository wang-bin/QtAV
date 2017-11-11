/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "EventFilter.h"
#include <QtAVWidgets>
#include <QApplication>
#include <QtCore/QUrl>
#include <QEvent>
#include <QFileDialog>
#include <QGraphicsObject>
#include <QGraphicsSceneContextMenuEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QtGui/QWindowStateChangeEvent>
#include <QtAV/AVPlayer.h>
#include <QtAV/AudioOutput.h>
#include <QtAV/VideoCapture.h>
#include <QtAV/VideoRenderer.h>

using namespace QtAV;

// TODO: watch main window
EventFilter::EventFilter(AVPlayer *player) :
    QObject(player),
    menu(0)
{
}

EventFilter::~EventFilter()
{
    if (menu) {
        delete menu;
        menu = 0;
    }
}

void EventFilter::openLocalFile()
{
    QString file = QFileDialog::getOpenFileName(0, tr("Open a video"));
    if (file.isEmpty())
        return;
    AVPlayer *player = static_cast<AVPlayer*>(parent());
    player->play(file);
}

void EventFilter::openUrl()
{
    QString url = QInputDialog::getText(0, tr("Open an url"), tr("Url"));
    if (url.isEmpty())
        return;
    AVPlayer *player = static_cast<AVPlayer*>(parent());
    player->play(url);
}

void EventFilter::about()
{
    QtAV::about();
}

void EventFilter::aboutFFmpeg()
{
    QtAV::aboutFFmpeg();
}

void EventFilter::help()
{
    emit helpRequested();
    return;
    static QString help = QString::fromLatin1("<h4>") +tr("Drag and drop a file to player\n") + QString::fromLatin1("</h4>"
                       "<p>") + tr("A: switch aspect ratio") + QString::fromLatin1("</p>"
                       "<p>") + tr("Double click to switch fullscreen") + QString::fromLatin1("</p>"
                       "<p>") + tr("Shortcut:\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("Space: pause/continue\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("F: fullscreen on/off\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("T: stays on top on/off\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("N: show next frame. Continue the playing by pressing 'Space'\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("Ctrl+O: open a file\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("O: OSD\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("P: replay\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("Q/ESC: quit\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("S: stop\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("R: rotate 90") + QString::fromLatin1("</p>"
                       "<p>") + tr("M: mute on/off\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("C: capture video") + QString::fromLatin1("</p>"
                       "<p>") + tr("Up/Down: volume +/-\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("Ctrl+Up/Down: speed +/-\n") + QString::fromLatin1("</p>"
                       "<p>") + tr("-&gt;/&lt;-: seek forward/backward\n");
    QMessageBox::about(0, tr("Help"), help);
}

bool EventFilter::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    AVPlayer *player = static_cast<AVPlayer*>(parent());
    if (!player || !player->renderer() || !player->renderer()->widget())
        return false;
    if (qobject_cast<QWidget*>(watched) != player->renderer()->widget()) {
        return false;
    }
#ifndef QT_NO_DYNAMIC_CAST //dynamic_cast is defined as a macro to force a compile error
    if (player->renderer() != dynamic_cast<VideoRenderer*>(watched)) {
       // return false;
    }
#endif
    QEvent::Type type = event->type();
    switch (type) {
    case QEvent::KeyPress: {
        QKeyEvent *key_event = static_cast<QKeyEvent*>(event);
        int key = key_event->key();
        Qt::KeyboardModifiers modifiers = key_event->modifiers();
        switch (key) {
        case Qt::Key_0:
            player->seek(0LL);
            break;
        case Qt::Key_C: //capture
            player->videoCapture()->capture();
            break;
        case Qt::Key_N: //check playing?
            player->stepForward();
            break;
        case Qt::Key_B:
            player->stepBackward();
            break;
        case Qt::Key_P:
            player->stop();
            player->play();
            break;
        case Qt::Key_Q:
        case Qt::Key_Escape:
            qApp->quit();
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
            w->setWindowState(w->windowState() ^ Qt::WindowFullScreen);
        }
            break;
        case Qt::Key_U:
            player->setNotifyInterval(player->notifyInterval() + 100);
            break;
        case Qt::Key_D:
            player->setNotifyInterval(player->notifyInterval() - 100);
            break;
        case Qt::Key_Up: {
            AudioOutput *ao = player->audio();
            if (modifiers == Qt::ControlModifier) {
                qreal s = player->speed();
                if (s < 1.4)
                    s += 0.02;
                else
                    s += 0.05;
                if (qAbs<qreal>(s-1.0) <= 0.01)
                    s = 1.0;
                player->setSpeed(s);
                return true;
            }
            if (ao && ao->isAvailable()) {
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
        }
            break;
        case Qt::Key_Down: {
            AudioOutput *ao = player->audio();
            if (modifiers == Qt::ControlModifier) {
                qreal s = player->speed();
                if (s < 1.4)
                    s -= 0.02;
                else
                    s -= 0.05;
                if (qAbs<qreal>(s-1.0) <= 0.01)
                    s = 1.0;
                s = qMax<qreal>(s, 0.0);
                player->setSpeed(s);
                return true;
            }
            if (ao && ao->isAvailable()) {
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
        }
            break;
        case Qt::Key_O: {
            if (modifiers == Qt::ControlModifier) {
                //TODO: emit a signal so we can use custome dialogs?
                openLocalFile();
            } else/* if (m == Qt::NoModifier) */{
                emit showNextOSD();
            }
        }
            break;
        case Qt::Key_Left:
            qDebug("<-");
            player->setSeekType(key_event->isAutoRepeat() ? KeyFrameSeek : AccurateSeek);
            player->seekBackward();
            break;
        case Qt::Key_Right:
            qDebug("->");
            player->setSeekType(key_event->isAutoRepeat() ? KeyFrameSeek : AccurateSeek);
            player->seekForward();
            break;
        case Qt::Key_M:
            if (player->audio()) {
                player->audio()->setMute(!player->audio()->isMute());
            }
            break;
        case Qt::Key_A: {
            VideoRenderer* renderer = player->renderer();
            VideoRenderer::OutAspectRatioMode r = renderer->outAspectRatioMode();
            renderer->setOutAspectRatioMode(VideoRenderer::OutAspectRatioMode(((int)r+1)%2));
        }
            break;
        case Qt::Key_R: {
            VideoRenderer* renderer = player->renderer();
            renderer->setOrientation(renderer->orientation() + 90);
            qDebug("orientation: %d", renderer->orientation());
        }
            break;
        case Qt::Key_T: {
            QWidget *w = qApp->activeWindow();
            if (!w)
                return false;
            w->setWindowFlags(w->window()->windowFlags() ^ Qt::WindowStaysOnTopHint);
            //call setParent() when changing the flags, causing the widget to be hidden
            w->show();
        }
            break;
        case Qt::Key_F1:
            help();
            break;
        default:
            return false;
        }
        break;
    }
    case QEvent::DragEnter:
    case QEvent::DragMove: {
        QDropEvent *e = static_cast<QDropEvent*>(event);
        if (e->mimeData()->hasUrls())
            e->acceptProposedAction();
        else
            e->ignore();
    }
        break;
    case QEvent::Drop: {
        QDropEvent *e = static_cast<QDropEvent*>(event);
        if (e->mimeData()->hasUrls()) {
            QString path = e->mimeData()->urls().first().toLocalFile();
            player->stop();
            player->play(path);
            e->acceptProposedAction();
        } else {
            e->ignore();
        }
    }
        break;

    case QEvent::MouseButtonDblClick: {
          QMouseEvent *me = static_cast<QMouseEvent*>(event);
          Qt::MouseButton mbt = me->button();
          QWidget *mpWindow =  static_cast<QWidget*>(player->parent());
        if (mbt == Qt::LeftButton) {
            if (Qt::WindowFullScreen ==mpWindow->windowState()){
               mpWindow->setWindowState(mpWindow->windowState() ^ Qt::WindowFullScreen);
            }else{
               mpWindow->showFullScreen();
            }
        }
        break;
    }

    case QEvent::GraphicsSceneContextMenu: {
        QGraphicsSceneContextMenuEvent *e = static_cast<QGraphicsSceneContextMenuEvent*>(event);
        showMenu(e->screenPos());
    }
        break;
    case QEvent::ContextMenu: {
        QContextMenuEvent *e = static_cast<QContextMenuEvent*>(event);
        showMenu(e->globalPos());
    }
        break;
    default:
        return false;
    }
    return true; //false: for text input
}

void EventFilter::showMenu(const QPoint &p)
{
    if (!menu) {
        menu = new QMenu();
        menu->addAction(tr("Open"), this, SLOT(openLocalFile()));
        menu->addAction(tr("Open Url"), this, SLOT(openUrl()));
        menu->addSeparator();
        menu->addAction(tr("About"), this, SLOT(about()));
        menu->addAction(tr("Help"), this, SLOT(help()));
        menu->addSeparator();
    }
    menu->exec(p);
}

WindowEventFilter::WindowEventFilter(QWidget *window)
    : QObject(window)
    , mpWindow(window)
{

}

bool WindowEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != mpWindow)
        return false;
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent *e = static_cast<QWindowStateChangeEvent*>(event);
        mpWindow->updateGeometry();
        if (mpWindow->windowState().testFlag(Qt::WindowFullScreen) || e->oldState().testFlag(Qt::WindowFullScreen)) {
            emit fullscreenChanged();
        }
        return false;
    }

    if (event->type() ==  QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        Qt::MouseButton mbt = me->button();
        if (mbt == Qt::LeftButton) {
            gMousePos = me->globalPos();
            iMousePos = me->pos();
        }
        return false;
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        Qt::MouseButton mbt = me->button();
        if (mbt != Qt::LeftButton)
            return false;
        iMousePos = QPoint();
        gMousePos = QPoint();
        return false;
    }
    if (event->type() == QEvent::MouseMove) {
        if (iMousePos.isNull() || gMousePos.isNull())
            return false;
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        int x = mpWindow->pos().x();
        int y = mpWindow->pos().y();
        int dx = me->globalPos().x() - gMousePos.x();
        int dy = me->globalPos().y() - gMousePos.y();
        gMousePos = me->globalPos();
        mpWindow->move(x + dx, y + dy);
        return false;
    }
    return false;
}
