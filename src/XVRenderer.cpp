/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

/*
 * X11 headers define 'Bool' type which is used in qmetatype.h. we must include X11 files at last, i.e. XVRenderer_p.h. otherwise compile error
*/
#include "QtAV/XVRenderer.h"
#include <QResizeEvent>
#include "private/XVRenderer_p.h"
namespace QtAV {

XVRenderer::XVRenderer(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f)
  , VideoRenderer(*new XVRendererPrivate())
{
    DPTR_INIT_PRIVATE(XVRenderer);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    /* To rapidly update custom widgets that constantly paint over their entire areas with
     * opaque content, e.g., video streaming widgets, it is better to set the widget's
     * Qt::WA_OpaquePaintEvent, avoiding any unnecessary overhead associated with repainting the
     * widget's background
     */
    setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_PaintOnScreen, true);
}

bool XVRenderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(XVRenderer);
    if (!d.prepareImage(d.src_width, d.src_height))
        return false;
    //TODO: if date is deep copied, mutex can be avoided
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.video_frame = frame;
    d.video_frame.convertTo(VideoFormat::Format_YUV420P);
    d.xv_image->data = (char*)d.video_frame.bits();

    update();
    return true;
}

QPaintEngine* XVRenderer::paintEngine() const
{
    return 0; //use native engine
}

bool XVRenderer::needUpdateBackground() const
{
    DPTR_D(const XVRenderer);
    return d.update_background && d.out_rect != rect();/* || d.data.isEmpty()*/ //data is always empty because we never copy it now.
}

void XVRenderer::drawBackground()
{
    DPTR_D(XVRenderer);
    XFillRectangle(d.display, winId(), d.gc, 0, 0, width(), height());
    //FIXME: xv should always draw the background. so shall we only paint the border rectangles, but not the whole widget
    d.update_background = true;
}

bool XVRenderer::needDrawFrame() const
{
    DPTR_D(const XVRenderer);
    return  d.xv_image || d.video_frame.isValid();
}

void XVRenderer::drawFrame()
{
    DPTR_D(XVRenderer);
    QRect roi = realROI();
    if (!d.use_shm)
        XvPutImage(d.display, d.xv_port, winId(), d.gc, d.xv_image
                   , roi.x(), roi.y(), roi.width(), roi.height()
                   , d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height());
    else
        XvShmPutImage(d.display, d.xv_port, winId(), d.gc, d.xv_image
                      , roi.x(), roi.y(), roi.width(), roi.height()
                      , d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height()
                      , false /*true: send event*/);
}

void XVRenderer::paintEvent(QPaintEvent *)
{
    handlePaintEvent();
}

void XVRenderer::resizeEvent(QResizeEvent *e)
{
    DPTR_D(XVRenderer);
    d.update_background = true;
    resizeRenderer(e->size());
    update();
}

void XVRenderer::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    DPTR_D(XVRenderer);
    d.update_background = true;
    /*
     * Do something that depends on widget below! e.g. recreate render target for direct2d.
     * When Qt::WindowStaysOnTopHint changed, window will hide first then show. If you
     * don't do anything here, the widget content will never be updated.
     */
    d.prepareDeviceResource();
}

} //namespace QtAV
