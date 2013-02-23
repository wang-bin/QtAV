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
#include "private/ImageRenderer_p.h"
#include <windows.h> //GetDC()
#include <gdiplus.h>
#include <QtGui/QPainter>
#include <QtGui/QPaintEngine>
#include <QResizeEvent>

//#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
namespace QtAV {

class GDIRendererPrivate : public ImageRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(GDIRenderer)

    GDIRendererPrivate():
        use_qpainter(false)
      , support_bitblt(true)
      , gdiplus_token(0)
      , device_context(0)
    {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplus_token, &gdiplusStartupInput, NULL);
    }
    ~GDIRendererPrivate() {
        if (device_context) {
            DPTR_P(GDIRenderer);
            ReleaseDC(p.winId(), device_context);
            device_context = 0;
        }
        GdiplusShutdown(gdiplus_token);
    }
    void getDeviceContext() {
        DPTR_P(GDIRenderer);
        device_context = GetDC(p.winId());
        //TODO: check bitblt support
        int ret = GetDeviceCaps(device_context, RC_BITBLT);
        qDebug("bitblt=%d", ret);
    }

    bool use_qpainter;
    bool support_bitblt;
    ULONG_PTR gdiplus_token;
    /*
     * GetDC(winID()): will flick
     * QPainter.paintEngine()->getDC() in paintEvent: doc says it's for internal use
     */
    HDC device_context;
};

GDIRenderer::GDIRenderer(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f),ImageRenderer(*new GDIRendererPrivate())
{
    DPTR_INIT_PRIVATE(GDIRenderer);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    //setDCFromPainter(false); //changeEvent will be called on show()
}

GDIRenderer::~GDIRenderer()
{
}

bool GDIRenderer::write()
{
    update();
    return true;
}

QPaintEngine* GDIRenderer::paintEngine() const
{
    if (d_func().use_qpainter) {
        return QWidget::paintEngine();
    } else {
        return 0;
    }
}

void GDIRenderer::useQPainter(bool qp)
{
    DPTR_D(GDIRenderer);
    d.use_qpainter = qp;
    setAttribute(Qt::WA_PaintOnScreen, !d.use_qpainter);
}

bool GDIRenderer::useQPainter() const
{
    DPTR_D(const GDIRenderer);
    return d.use_qpainter;
}

void GDIRenderer::showEvent(QShowEvent *)
{
    DPTR_D(const GDIRenderer);
    useQPainter(d.use_qpainter);
    if (!d.use_qpainter) {
        d_func().getDeviceContext();
    }
}

void GDIRenderer::resizeEvent(QResizeEvent *e)
{
    resizeVideo(e->size());
    update();
}

void GDIRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(GDIRenderer);
    if (!d.scale_in_qt) {
        d.img_mutex.lock();
    }
    QImage image = d.image; //TODO: other renderer use this style
    if (image.isNull()) {
        if (d.preview.isNull()) {
            d.preview = QImage(videoSize(), QImage::Format_RGB32);
            d.preview.fill(Qt::black); //maemo 4.7.0: QImage.fill(uint)
        }
        image = d.preview;
    }
    Bitmap bitmap(image.width(), image.height(), image.bytesPerLine()
                  , PixelFormat32bppRGB, image.bits());
    HDC hdc = d.device_context;
    if (d.use_qpainter) {
        QPainter p(this);
        hdc = p.paintEngine()->getDC();
    }
    //begin paint
    // && image.size() != size()
    if (d.scale_in_qt) { //TODO:rename scale_on_paint
        //qDebug("image size and target size not match. SLOW!!!");
        /* http://msdn.microsoft.com/en-us/library/windows/desktop/ms533829%28v=vs.85%29.aspx
         * Improving Performance by Avoiding Automatic Scaling
         * TODO: How about QPainter?
         */
        Graphics g(hdc);
        g.SetSmoothingMode(SmoothingModeHighSpeed);
        g.DrawImage(&bitmap, 0, 0, d.width, d.height);
    } else {
        //steps to use BitBlt: http://bbs.csdn.net/topics/60183502
        HBITMAP hbmp = 0;// CreateCompatibleBitmap(hdc, d.image.width(), d.image.height());
        if (SUCCEEDED(bitmap.GetHBITMAP(Color(), &hbmp))) {
            //PAINTSTRUCT ps;
            //BeginPaint(winId(), &ps); //why it's not necessary?
            HDC hdc_mem = CreateCompatibleDC(hdc);
            HBITMAP hbmp_old = (HBITMAP)SelectObject(hdc_mem, hbmp);
            BitBlt(hdc, 0, 0, image.width(), image.height(), hdc_mem, 0, 0, SRCCOPY);
            SelectObject(hdc_mem, hbmp_old);
            DeleteDC(hdc_mem);
            //EndPaint(winId(), &ps);
        }
    }
    //end paint
    if (!d.scale_in_qt) {
        d.img_mutex.unlock();
    }
}

} //namespace QtAV
