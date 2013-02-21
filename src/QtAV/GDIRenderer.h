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

#ifndef QTAV_GDIRENDERER_H
#define QTAV_GDIRENDERER_H

#include <QtAV/ImageRenderer.h>
#include <QWidget>

/*TODO:
 *  how to choose paintEngine.getDC() or GetDC(winId())?
 */
namespace QtAV {

class GDIRendererPrivate;
class Q_EXPORT GDIRenderer : public QWidget, public ImageRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GDIRenderer)
public:
    GDIRenderer(QWidget* parent = 0, Qt::WindowFlags f = 0); //offscreen?
    virtual ~GDIRenderer();
    virtual bool write();

    /* WA_PaintOnScreen: To render outside of Qt's paint system, e.g. If you require
     * native painting primitives, you need to reimplement QWidget::paintEngine() to
     * return 0 and set this flag
     */
    virtual QPaintEngine* paintEngine() const;
protected:
    virtual void resizeEvent(QResizeEvent *);
    virtual void paintEvent(QPaintEvent *);
};

} //namespace QtAV

#endif // QTAV_GDIRENDERER_H
