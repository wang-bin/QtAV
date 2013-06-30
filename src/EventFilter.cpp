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
#include <QGraphicsObject>
#include <QGraphicsSceneContextMenuEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QtAV/AVPlayer.h>
#include <QtAV/AudioOutput.h>
#include <QtAV/VideoRenderer.h>
#include <QtAV/OSDFilter.h>

namespace QtAV {

EventFilter::EventFilter(QObject *parent) :
    QObject(parent),menu(0)
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
    static QString help = "<h4>" +tr("Drag and drop a file to player\n") + "</h4>"
                       "<p>" + tr("Double click to switch fullscreen") + "</p>"
                       "<p>" + tr("Shortcut:\n") + "</p>"
                       "<p>" + tr("Space: pause/continue\n") + "</p>"
                       "<p>" + tr("F: fullscreen on/off\n") + "</p>"
                       "<p>" + tr("I: switch video display quality\n") + "</p>"
                       "<p>" + tr("T: stays on top on/off\n") + "</p>"
                       "<p>" + tr("N: show next frame. Continue the playing by pressing 'Space'\n") + "</p>"
                       "<p>" + tr("Ctrl+O: open a file\n") + "</p>"
                       "<p>" + tr("O: OSD\n") + "</p>"
                       "<p>" + tr("P: replay\n") + "</p>"
                       "<p>" + tr("Q/ESC: quit\n") + "</p>"
                       "<p>" + tr("S: stop\n") + "</p>"
                       "<p>" + tr("R: switch aspect ratio") + "</p>"
                       "<p>" + tr("M: mute on/off\n") + "</p>"
                       "<p>" + tr("C: capture video") + "</p>"
                       "<p>" + tr("Up/Down: volume +/-\n") + "</p>"
                       "<p>" + tr("Ctrl+Up/Down: speed +/-\n") + "</p>"
                       "<p>" + tr("-&gt;/&lt;-: seek forward/backward\n");
    QMessageBox::about(0, tr("Help"), help);
}

bool EventFilter::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    AVPlayer *player = static_cast<AVPlayer*>(parent());
    if (!player)
        return false;
#if 0 //TODO: why crash on close?
    if (player->renderer()->widget() != watched
            && (QGraphicsObject*)player->renderer()->graphicsItem() != watched) {
        return false;
    }
#else
#ifndef QT_NO_DYNAMIC_CAST //dynamic_cast is defined as a macro to force a compile error
    if (player->renderer() != dynamic_cast<VideoRenderer*>(watched)) {
        return false;
    }
#endif
#endif //0
    QEvent::Type type = event->type();
    switch (type) {
    case QEvent::MouseButtonPress: {
        qDebug("EventFilter: Mouse press");
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        Qt::MouseButton mbt = me->button();
        if (mbt == Qt::LeftButton) {
            gMousePos = me->globalPos();
            iMousePos = me->pos();
        }
        //TODO: wheel to control volume etc.
    }
        break;
    case QEvent::MouseButtonRelease: {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        Qt::MouseButton mbt = me->button();
        qDebug("release btn = %d", mbt);
        if (mbt != Qt::LeftButton)
            return false;
        iMousePos = QPoint();
        gMousePos = QPoint();
    }
        break;
    case QEvent::MouseMove: {
        if (iMousePos.isNull() || gMousePos.isNull() || !player->renderer()->widget())
            return false;
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        QWidget *window = player->renderer()->widget()->window();
        int x = window->pos().x();
        int y = window->pos().y();
        int dx = me->globalPos().x() - gMousePos.x();
        int dy = me->globalPos().y() - gMousePos.y();
        gMousePos = me->globalPos();
        window->move(x + dx, y + dy);
    }
        break;
    case QEvent::MouseButtonDblClick: { //TODO: move to gui
        QWidget *w = qApp->activeWindow();
        if (!w)
            return false;
        if (w->isFullScreen())
            w->showNormal();
        else
            w->showFullScreen();
    }
        break;
    case QEvent::KeyPress: {
        QKeyEvent *key_event = static_cast<QKeyEvent*>(event);
        int key = key_event->key();
        Qt::KeyboardModifiers modifiers = key_event->modifiers();
        switch (key) {
        case Qt::Key_C: //capture
            player->captureVideo();
            break;
        case Qt::Key_I: { //Interpolation
            VideoRenderer *renderer = player->renderer();
            renderer->setQuality(VideoRenderer::Quality(((int)renderer->quality()+1)%3));
        }
            break;
        case Qt::Key_N: //check playing?
            player->playNextFrame();
            break;
        case Qt::Key_P:
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
            if (w->isFullScreen())
                w->showNormal();
            else
                w->showFullScreen();
        }
            break;
        case Qt::Key_Up: {
            AudioOutput *ao = player->audio();
            if (modifiers == Qt::ControlModifier) {
                if (ao && ao->isAvailable()) {
                    qreal s = ao->speed();
                    if (s < 1.4)
                        s += 0.02;
                    else
                        s += 0.05;
                    if (qAbs<qreal>(s-1.0) <= 0.01)
                        s = 1.0;
                    qDebug("set speed %.2f =>> %.2f", ao->speed(), s);
                    ao->setSpeed(s);
                } else {
                    //clock speed
                }
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
                qreal s = ao->speed();
                if (s < 1.4)
                    s -= 0.02;
                else
                    s -= 0.05;
                if (qAbs<qreal>(s-1.0) <= 0.01)
                    s = 1.0;
                s = qMax(s, 0.0);
                if (ao && ao->isAvailable()) {
                    qDebug("set speed %.2f =>> %.2f", ao->speed(), s);
                    ao->setSpeed(s);
                } else {
                    //clock speed
                }
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
                //foreach renderer, or just current widget? add shortcuts for all vo?
                OSDFilter *osd = player->renderer()->osdFilter();
                if (osd)
                    osd->useNextShowType();
            }
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
        case Qt::Key_R: {
            VideoRenderer* renderer = player->renderer();
            VideoRenderer::OutAspectRatioMode r = renderer->outAspectRatioMode();
            renderer->setOutAspectRatioMode(VideoRenderer::OutAspectRatioMode(((int)r+1)%2));
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
        menu->addAction(tr("About Qt"), qApp, SLOT(aboutQt()));
    }
    menu->exec(p);
}

} //namespace QtAV
