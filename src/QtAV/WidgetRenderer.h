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

#include <QtAV/QPainterRenderer.h>
#include <qwidget.h>

namespace QtAV {

class WidgetRendererPrivate;
class Q_EXPORT WidgetRenderer : public QWidget, public QPainterRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(WidgetRenderer)
public:
    explicit WidgetRenderer(QWidget *parent = 0, Qt::WindowFlags f = 0);
    virtual ~WidgetRenderer();
    virtual VideoRendererId id() const;
signals:
    void imageReady();
protected:
    virtual bool needUpdateBackground() const;
    //called in paintEvent before drawFrame() when required
    virtual void drawBackground();
    //draw the current frame using the current paint engine. called by paintEvent()
    virtual void drawFrame();
    virtual void resizeEvent(QResizeEvent *);
    /*usually you don't need to reimplement paintEvent, just drawXXX() is ok. unless you want do all
     *things yourself totally*/
    virtual void paintEvent(QPaintEvent *);
    virtual bool write();
protected:
    WidgetRenderer(WidgetRendererPrivate& d, QWidget *parent, Qt::WindowFlags f);
};

typedef WidgetRenderer VideoRendererWidget;
} //namespace QtAV
#endif // QTAV_WIDGETRENDERER_H
