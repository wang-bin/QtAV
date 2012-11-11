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

#ifndef QAV_WIDGETRENDERER_H
#define QAV_WIDGETRENDERER_H

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

    explicit WidgetRenderer(QWidget *parent = 0);
    virtual ~WidgetRenderer();
    virtual int write(const QByteArray &data);
    void setPreview(const QImage& preivew);

protected:
    virtual void resizeEvent(QResizeEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
#if !CONFIG_EZX
    virtual void paintEvent(QPaintEvent *);
    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dropEvent(QDropEvent *);
#endif
    
protected:
    WidgetRenderer(WidgetRendererPrivate& d, QWidget *parent);
};

} //namespace QtAV
#endif // QAV_WIDGETRENDERER_H
