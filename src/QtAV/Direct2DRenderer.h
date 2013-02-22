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

#ifndef QTAV_DIRECT2DRENDERER_H
#define QTAV_DIRECT2DRENDERER_H

#include <QtAV/ImageRenderer.h>
#include <QWidget>

/*TODO:
 *  draw yuv directly
 */

namespace QtAV {

class Direct2DRendererPrivate;
class Q_EXPORT Direct2DRenderer : public QWidget, public ImageRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(Direct2DRenderer)
public:
    Direct2DRenderer(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~Direct2DRenderer();
    virtual bool write();

    /* WA_PaintOnScreen: To render outside of Qt's paint system, e.g. If you require
     * native painting primitives, you need to reimplement QWidget::paintEngine() to
     * return 0 and set this flag
     */
    virtual QPaintEngine* paintEngine() const;
	bool useQPainter() const;
    void useQPainter(bool qp);
protected:
    virtual void changeEvent(QEvent *event); //stay on top will change parent, we need GetDC() again
    virtual void resizeEvent(QResizeEvent *);
    virtual void paintEvent(QPaintEvent *);
};

} //namespace QtAV

#endif // QTAV_Direct2DRenderer_H
