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
        dc_painter(false)
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
    }

    bool dc_painter;
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
    if (d_func().dc_painter) {
        return QWidget::paintEngine();
    } else {
        return 0;
    }
}

void GDIRenderer::setDCFromPainter(bool dc)
{
    DPTR_D(GDIRenderer);
    d.dc_painter = dc;
    if (dc) {
        setAttribute(Qt::WA_PaintOnScreen, false);
    } else {
        d_func().getDeviceContext();
        if (d_func().device_context) {
            setAttribute(Qt::WA_PaintOnScreen, true); //use native engine
        }
    }
}

void GDIRenderer::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) { //auto called when show
        setDCFromPainter(d_func().dc_painter);
        event->accept();
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
    HDC hdc = d.device_context;
    if (d.dc_painter) {
        QPainter p(this);
        hdc = p.paintEngine()->getDC();
    }
    Graphics g(hdc);
    if (!d.image.isNull()) {
        Bitmap bitmap(d.image.width(), d.image.height(), d.image.bytesPerLine()
                      , PixelFormat32bppRGB, d.image.bits());
        /* http://msdn.microsoft.com/en-us/library/windows/desktop/ms533829%28v=vs.85%29.aspx
         * Improving Performance by Avoiding Automatic Scaling
         * TODO: How about QPainter?
         */
        g.DrawImage(&bitmap, 0, 0, d.width, d.height);
    } else if (!d.preview.isNull()){
        Bitmap bitmap(d.preview.width(), d.preview.height(), d.preview.bytesPerLine()
                      , PixelFormat32bppRGB, d.preview.bits());
        g.DrawImage(&bitmap, 0, 0, d.width, d.height);
    } else {
        d.preview = QImage(videoSize(), QImage::Format_RGB32);
        d.preview.fill(Qt::black); //maemo 4.7.0: QImage.fill(uint)
        Bitmap bitmap(d.preview.width(), d.preview.height(), d.preview.bytesPerLine()
                      , PixelFormat32bppRGB, d.preview.bits());
        g.DrawImage(&bitmap, 0, 0, d.width, d.height);
    }
    if (!d.scale_in_qt) {
        d.img_mutex.unlock();
    }

}

} //namespace QtAV
