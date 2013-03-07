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
#include "private/VideoRenderer_p.h"
#include <windows.h> //GetDC()
#include <gdiplus.h>
#include <QResizeEvent>

//http://msdn.microsoft.com/en-us/library/ms927613.aspx
//#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
namespace QtAV {

class GDIRendererPrivate : public VideoRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(GDIRenderer)

    GDIRendererPrivate():
        support_bitblt(true)
      , gdiplus_token(0)
      , device_context(0)
    {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplus_token, &gdiplusStartupInput, NULL);
    }
    ~GDIRendererPrivate() {
        if (device_context) {
            DPTR_P(GDIRenderer);
            ReleaseDC((HWND)p.winId(), device_context); /*Q5: must cast WID to HWND*/
            DeleteDC(off_dc);
            device_context = 0;
        }
        GdiplusShutdown(gdiplus_token);
    }
    void prepare() {
        DPTR_P(GDIRenderer);
        update_background = true;
        device_context = GetDC((HWND)p.winId()); /*Q5: must cast WID to HWND*/
        //TODO: check bitblt support
        int ret = GetDeviceCaps(device_context, RC_BITBLT);
        qDebug("bitblt=%d", ret);
        //TODO: wingapi? vlc
#if 0
        BITMAPINFOHEADER bih;
        bih.biSize          = sizeof(BITMAPINFOHEADER);
        bih.biSizeImage     = 0;
        bih.biPlanes        = 1;
        bih.biCompression   = BI_RGB; //vlc: 16bpp=>BI_RGB, 15bpp=>BI_BITFIELDS
        bih.biBitCount      = 32;
        bih.biWidth         = src_width;
        bih.biHeight        = src_height;
        bih.biClrImportant  = 0;
        bih.biClrUsed       = 0;
        bih.biXPelsPerMeter = 0;
        bih.biYPelsPerMeter = 0;

        off_bitmap = CreateDIBSection(device_context,
                                      , (BITMAPINFO*)&bih
                                      , DIB_RGB_COLORS
                                      , &p_pic_buffer, NULL, 0);
#endif //0
        off_dc = CreateCompatibleDC(device_context);
    }

    bool support_bitblt;
    ULONG_PTR gdiplus_token;
    /*
     * GetDC(winID()): will flick
     * QPainter.paintEngine()->getDC() in paintEvent: doc says it's for internal use
     */
    HDC device_context;
    /* Our offscreen bitmap and its framebuffer */
    HDC        off_dc;
    HBITMAP    off_bitmap;

};

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

void GDIRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(GDIRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    //begin paint
    HDC hdc = d.device_context;
    if ((d.update_background && d.out_rect != rect())|| d.data.isEmpty()) {
        d.update_background = false;
        Graphics g(hdc);
        SolidBrush brush(Color(255, 0, 0, 0)); //argb
        g.FillRectangle(&brush, 0, 0, width(), height());
        //Rectangle(hdc, 0, 0, width(), height());
    }
    if (d.data.isEmpty()) {
        return;
    }
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
