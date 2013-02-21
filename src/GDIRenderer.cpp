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
        gdiplus_token(0)
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
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    d_func().getDeviceContext();
    if (d_func().device_context) {
        setAttribute(Qt::WA_PaintOnScreen); //use native engine
    }
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
    if (d_func().device_context) {
        return 0;
    } else {
        return QWidget::paintEngine();
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
    QPainter p(this);
    if (!d.image.isNull()) {
        Bitmap bitmap(d.image.width(), d.image.height(), d.image.bytesPerLine()
                      , PixelFormat32bppRGB, d.image.bits());

        HDC hdc = d.device_context;
        if (!hdc)
            hdc = p.paintEngine()->getDC();
        Graphics g(hdc);
        if (d.image.size() == QSize(d.width, d.height)) {
            //d.preview = d.image;
            g.DrawImage(&bitmap, 0, 0);
        } else {
            //qDebug("size not fit. may slow. %dx%d ==> %dx%d"
            //       , d.image.size().width(), d.image.size().height(), d.width, d.height);
            g.DrawImage(&bitmap, 0, 0, d.width, d.height);
        }
    } else if (!d.preview.isNull()){
        if (d.preview.size() == QSize(d.width, d.height)) {
            p.drawImage(QPoint(), d.preview);
        } else {
            p.drawImage(rect(), d.preview);
        }
    } else {
        d.preview = QImage(videoSize(), QImage::Format_RGB32);
        d.preview.fill(Qt::black); //maemo 4.7.0: QImage.fill(uint)
        p.drawImage(QPoint(), d.preview);
    }
    if (!d.scale_in_qt) {
        d.img_mutex.unlock();
    }

}

} //namespace QtAV
