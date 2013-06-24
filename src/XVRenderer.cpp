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
    d_func().widget_holder = this;
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_PaintOnScreen, true);
}

XVRenderer::~XVRenderer()
{
}


QPaintEngine* XVRenderer::paintEngine() const
{
    return QWidget::paintEngine();
    return 0; //use native engine
}

void XVRenderer::convertData(const QByteArray &data)
{
    DPTR_D(XVRenderer);
    if (!d.prepareImage(d.src_width, d.src_height))
        return;
    //TODO: if date is deep copied, mutex can be avoided
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);

    const int Cr = d.src_width * d.src_height;
    const int Cb = Cr + d.src_width/2 * d.src_height/2;
    const uint *subs_data = (const uint *)data.constData();
    for (int h = 0 ; h < d.src_height; h++) {
        for (int w = 0 ; w < d.src_width; w++) {
            const uint &pixel = subs_data[h*d.src_width + w];
            uchar A = (pixel >> 24) & 0xFF;
            if (!A) {
                continue;
            }
            uchar R = (pixel >> 16) & 0xFF;
            uchar G = (pixel >> 8) & 0xFF;
            uchar B = pixel & 0xFF;
//TODO Alpha channel
            uchar Y = 0.257*R + 0.504*G + 0.098*B + 16;
// 				double dA = ( 255 - A ) / 255.;
            d.xv_image->data[h*d.src_width + w] = Y;
            if (!(w%2) && !(h%2)) {// (!(w&0x1) && !(h&0x1)) {
                d.xv_image->data[Cb + (h/2*d.src_width/2 + w/2)] = -0.148*R - 0.291*G + 0.439*B + 128;
                d.xv_image->data[Cr + (h/2*d.src_width/2 + w/2)] =  0.439*R - 0.368*G - 0.071*B + 128;
            }
        }
    }
    //memcpy(d.xv_image->data, d.data.data(), d.xv_image->data_size);
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
    return  d.xv_image || !d.data.isEmpty();
}

void XVRenderer::drawFrame()
{
    DPTR_D(XVRenderer);
    if (!d.use_shm)
        XvPutImage(d.display, d.xv_port, winId(), d.gc, d.xv_image
                   , 0, 0, d.src_width, d.src_height
                   , d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height());
    else
        XvShmPutImage(d.display, d.xv_port, winId(), d.gc, d.xv_image
                      , 0, 0, d.src_width, d.src_height
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

bool XVRenderer::write()
{
    update();
    return true;
}

} //namespace QtAV
