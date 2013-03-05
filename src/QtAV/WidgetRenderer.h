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

#ifndef QTAV_WIDGETRENDERER_H
#define QTAV_WIDGETRENDERER_H

#include <QtAV/ImageRenderer.h>
#include <qwidget.h>

namespace QtAV {

class WidgetRendererPrivate;
class Q_EXPORT WidgetRenderer : public QWidget, public ImageRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(WidgetRenderer)
public:
    //GestureAction is useful for small screen windows that are hard to select frame
    enum GestureAction { GestureMove, GestureResize};

    explicit WidgetRenderer(QWidget *parent = 0, Qt::WindowFlags f = 0);
    virtual ~WidgetRenderer();

protected:
    virtual void resizeEvent(QResizeEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void paintEvent(QPaintEvent *);
    virtual bool write();
protected:
    WidgetRenderer(WidgetRendererPrivate& d, QWidget *parent, Qt::WindowFlags f);
};

} //namespace QtAV
#endif // QTAV_WIDGETRENDERER_H
