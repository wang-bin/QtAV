/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#include "output/video/VideoOutputEventFilter.h"
#include "QtAV/VideoRenderer.h"
#include <QtCore/QEvent>
#include <QKeyEvent>
#include <QWidget>
#include "utils/Logger.h"

namespace QtAV {

VideoOutputEventFilter::VideoOutputEventFilter(VideoRenderer *renderer):
    QObject(renderer->widget())
  , mRendererIsQObj(!!renderer->widget())
  , mpRenderer(renderer)
{
    if (renderer->widget()) {
        connect(renderer->widget(), SIGNAL(destroyed()), SLOT(stopFiltering()), Qt::DirectConnection);
    }
}

bool VideoOutputEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    // a widget shown means it's available and not deleted
    if (event->type() == QEvent::Show || event->type() == QEvent::ShowToParent) {
        mRendererIsQObj = !!mpRenderer->widget();
        return false;
    }
    if (!mRendererIsQObj)
        return false;
    /*
     * deleted renderer (after destroyed()) can not access it's member, so we must mark it as invalid.
     * hide event is sent when close. what about QEvent::Close?
     */
    if (event->type() == QEvent::Close || event->type() == QEvent::Hide) {
        mRendererIsQObj = false;
        return true;
    }
    if (!mpRenderer)
        return false;
    if (!mpRenderer->isDefaultEventFilterEnabled())
        return false;
    if (watched != mpRenderer->widget()
            /* && watched != mpRenderer->graphicsWidget()*/ //no showFullScreen() etc.
            )
        return false;
    switch (event->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *key_event = static_cast<QKeyEvent*>(event);
        int key = key_event->key();
        //Qt::KeyboardModifiers modifiers = key_event->modifiers();
        switch (key) {
        case Qt::Key_F:
            switchFullScreen();
            break;
        case Qt::Key_I:
            mpRenderer->setQuality(VideoRenderer::Quality(((int)mpRenderer->quality()+1)%3));
            break;
        case Qt::Key_T: {
            QWidget *w = mpRenderer->widget()->window();
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
        }
    }
        break;
    case QEvent::MouseButtonDblClick:
        switchFullScreen();
        break;
    case QEvent::MouseButtonPress: {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        Qt::MouseButton mbt = me->button();
        if (mbt == Qt::LeftButton) {
            gMousePos = me->globalPos();
            iMousePos = me->pos();
        }
    }
        break;
    case QEvent::MouseButtonRelease: {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        Qt::MouseButton mbt = me->button();
        if (mbt != Qt::LeftButton)
            return false;
        iMousePos = QPoint();
        gMousePos = QPoint();
    }
        break;
    case QEvent::MouseMove: {
        if (iMousePos.isNull() || gMousePos.isNull())
            return false;
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        QWidget *window = mpRenderer->widget()->window();
        int x = window->pos().x();
        int y = window->pos().y();
        int dx = me->globalPos().x() - gMousePos.x();
        int dy = me->globalPos().y() - gMousePos.y();
        gMousePos = me->globalPos();
        window->move(x + dx, y + dy);
    }
        break;
    default:
        break;
    }
    return false;
}

void VideoOutputEventFilter::stopFiltering()
{
    mpRenderer = 0;
}

void VideoOutputEventFilter::switchFullScreen()
{
    if (!mpRenderer || !mpRenderer->widget())
        return;
    QWidget *window = mpRenderer->widget()->window();
    if (window->isFullScreen())
        window->showNormal();
    else
        window->showFullScreen();
}

} //namespace QtAV
