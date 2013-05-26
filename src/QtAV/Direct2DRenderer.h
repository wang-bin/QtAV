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

#include <QtAV/VideoRenderer.h>
#include <QWidget>

/*TODO:
 *  draw yuv directly
 */

namespace QtAV {

class Direct2DRendererPrivate;
class Q_EXPORT Direct2DRenderer : public QWidget, public VideoRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(Direct2DRenderer)
public:
    Direct2DRenderer(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~Direct2DRenderer();
    virtual VideoRendererId id() const;
    /* WA_PaintOnScreen: To render outside of Qt's paint system, e.g. If you require
     * native painting primitives, you need to reimplement QWidget::paintEngine() to
     * return 0 and set this flag
     */
    virtual QPaintEngine* paintEngine() const;
protected:
    virtual void convertData(const QByteArray &data);
    virtual bool needUpdateBackground() const;
    //called in paintEvent before drawFrame() when required
    virtual void drawBackground();
    virtual bool needDrawFrame() const;
    //draw the current frame using the current paint engine. called by paintEvent()
    virtual void drawFrame();
    /*usually you don't need to reimplement paintEvent, just drawXXX() is ok. unless you want do all
     *things yourself totally*/
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    //stay on top will change parent, hide then show(windows). we need GetDC() again
    virtual void showEvent(QShowEvent *);
    virtual bool write();
};
typedef Direct2DRenderer VideoRendererDirect2D;

} //namespace QtAV

#endif // QTAV_Direct2DRenderer_H
