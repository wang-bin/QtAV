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

#include "GDIRenderer.h"
#include "private/GDIRenderer_p.h"
#include <QResizeEvent>

//http://msdn.microsoft.com/en-us/library/ms927613.aspx
//#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
namespace QtAV {

GDIRenderer::GDIRenderer(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f),VideoRenderer(*new GDIRendererPrivate())
{
    DPTR_INIT_PRIVATE(GDIRenderer);
    d_func().widget_holder = this;
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    //setDCFromPainter(false); //changeEvent will be called on show()
    setAttribute(Qt::WA_PaintOnScreen, true);
}

GDIRenderer::~GDIRenderer()
{
}

QPaintEngine* GDIRenderer::paintEngine() const
{
    return 0;
}

void GDIRenderer::convertData(const QByteArray &data)
{
    DPTR_D(GDIRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.data = data;
}

void GDIRenderer::drawBackground()
{
    DPTR_D(GDIRenderer);
    //HDC hdc = d.device_context;
    Graphics g(d.device_context);
    SolidBrush brush(Color(255, 0, 0, 0)); //argb
    g.FillRectangle(&brush, 0, 0, width(), height());
}

void GDIRenderer::drawFrame()
{
    DPTR_D(GDIRenderer);
    /* http://msdn.microsoft.com/en-us/library/windows/desktop/ms533829%28v=vs.85%29.aspx
     * Improving Performance by Avoiding Automatic Scaling
     * TODO: How about QPainter?
     */
    //steps to use BitBlt: http://bbs.csdn.net/topics/60183502
    Bitmap bitmap(d.src_width, d.src_height, d.src_width*4*sizeof(char)
                  , PixelFormat32bppRGB, (BYTE*)d.data.data());
    if (FAILED(bitmap.GetHBITMAP(Color(), &d.off_bitmap))) {
        qWarning("Failed GetHBITMAP");
        return;
    }

    HDC hdc = d.device_context;
    HBITMAP hbmp_old = (HBITMAP)SelectObject(d.off_dc, d.off_bitmap);
    // && image.size() != size()
    //assume that the image data is already scaled to out_size(NOT renderer size!)
    if (!d.scale_in_renderer || (d.src_width == d.out_rect.width() && d.src_height == d.out_rect.height())) {
        BitBlt(hdc
               , d.out_rect.left(), d.out_rect.top()
               , d.out_rect.width(), d.out_rect.height()
               , d.off_dc
               , 0, 0
               , SRCCOPY);
    } else {
        StretchBlt(hdc
                   , d.out_rect.left(), d.out_rect.top()
                   , d.out_rect.width(), d.out_rect.height()
                   , d.off_dc
                   , 0, 0
                   , d.src_width, d.src_height
                   , SRCCOPY);
    }
    SelectObject(d.off_dc, hbmp_old);
    DeleteObject(d.off_bitmap); //avoid mem leak
    //end paint
}

void GDIRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(GDIRenderer);
    {
        //lock is required only when drawing the frame
        QMutexLocker locker(&d.img_mutex);
        Q_UNUSED(locker);
        //begin paint. how about QPainter::beginNativePainting()?
        //fill background color when necessary, e.g. renderer is resized, image is null
        //we access d.data which will be modified in AVThread, so must be protected
        if ((d.update_background && d.out_rect != rect()) || d.data.isEmpty()) {
            d.update_background = false;
            //fill background color. DO NOT return, you must continue drawing
            drawBackground();
        }
        //DO NOT return if no data. we should draw other things
        //NOTE: if data is not copyed in convertData, you should always call drawFrame()
        if (!d.data.isEmpty()) {
            drawFrame();
        }
    }
    //end paint. how about QPainter::endNativePainting()?
}

void GDIRenderer::resizeEvent(QResizeEvent *e)
{
    d_func().update_background = true;
    resizeRenderer(e->size());
    update();
}

void GDIRenderer::showEvent(QShowEvent *)
{
    DPTR_D(GDIRenderer);
    d.update_background = true;
    d_func().prepare();
}

bool GDIRenderer::write()
{
    update();
    return true;
}

} //namespace QtAV
