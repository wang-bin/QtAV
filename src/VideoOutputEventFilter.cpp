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

#include "QtAV/VideoOutputEventFilter.h"
#include "QtAV/VideoRenderer.h"
#include <QtCore/QEvent>
#include <QKeyEvent>
#include <QWidget>

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
    if (watched != mpRenderer->widget()
            /* && watched != mpRenderer->graphicsWidget()*/ //no showFullScreen() etc.
            )
        return false;
    switch (event->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *key_event = static_cast<QKeyEvent*>(event);
        int key = key_event->key();
        Qt::KeyboardModifiers modifiers = key_event->modifiers();
        switch (key) {
        case Qt::Key_F: { //TODO: move to gui
            //TODO: active window
            QWidget *w = mpRenderer->widget();
            if (!w)
                return false;
            if (w->isFullScreen())
                w->showNormal();
            else
                w->showFullScreen();
        }
            break;
        }
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

} //namespace QtAV
