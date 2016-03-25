/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/VideoRenderer.h"
#include "QtAV/private/VideoRenderer_p.h"
#include <QWidget>
#include <windows.h> //GetDC()
#include <gdiplus.h>
#include <QResizeEvent>
#include "QtAV/private/factory.h"

#define USE_GRAPHICS 0

//http://msdn.microsoft.com/en-us/library/ms927613.aspx
//#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
namespace QtAV {

class GDIRendererPrivate;
class GDIRenderer : public QWidget, public VideoRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GDIRenderer)
public:
    GDIRenderer(QWidget* parent = 0, Qt::WindowFlags f = 0); //offscreen?
    VideoRendererId id() const Q_DECL_OVERRIDE;
    bool isSupported(VideoFormat::PixelFormat pixfmt) const Q_DECL_OVERRIDE;
    /* WA_PaintOnScreen: To render outside of Qt's paint system, e.g. If you require
     * native painting primitives, you need to reimplement QWidget::paintEngine() to
     * return 0 and set this flag
     */
    QPaintEngine* paintEngine() const Q_DECL_OVERRIDE;

    /*http://lists.trolltech.com/qt4-preview-feedback/2005-04/thread00609-0.html
     * true: paintEngine.getDC(), double buffer is enabled by defalut.
     * false: GetDC(winId()), no double buffer, should reimplement paintEngine()
     */
    QWidget* widget() Q_DECL_OVERRIDE { return this; }
protected:
    bool receiveFrame(const VideoFrame& frame) Q_DECL_OVERRIDE;
    void drawBackground() Q_DECL_OVERRIDE;
    void drawFrame() Q_DECL_OVERRIDE;
    /*usually you don't need to reimplement paintEvent, just drawXXX() is ok. unless you want do all
     *things yourself totally*/
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    //stay on top will change parent, hide then show(windows). we need GetDC() again
    void showEvent(QShowEvent *) Q_DECL_OVERRIDE;
};
typedef GDIRenderer VideoRendererGDI;
extern VideoRendererId VideoRendererId_GDI;
#if 0
FACTORY_REGISTER_ID_AUTO(VideoRenderer, GDI, "GDI")
#else
void RegisterVideoRendererGDI_Man()
{
    VideoRenderer::Register<GDIRenderer>(VideoRendererId_GDI, "GDI");
}
#endif

VideoRendererId GDIRenderer::id() const
{
    return VideoRendererId_GDI;
}

class GDIRendererPrivate : public VideoRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(GDIRenderer)

    GDIRendererPrivate():
        VideoRendererPrivate()
      , support_bitblt(true)
      , gdiplus_token(0)
      , device_context(0)
  #if USE_GRAPHICS
      , graphics(0)
  #endif //USE_GRAPHICS
    {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplus_token, &gdiplusStartupInput, NULL);
    }
    ~GDIRendererPrivate() {
        if (device_context) {
            DPTR_P(GDIRenderer);
            ReleaseDC((HWND)p.winId(), device_context); /*Q5: must cast WID to HWND*/
#if !USE_GRAPHICS
            DeleteDC(off_dc);
#endif //USE_GRAPHICS
            device_context = 0;
        }
        GdiplusShutdown(gdiplus_token);
    }
    void prepare() {
        update_background = true;
        DPTR_P(GDIRenderer);
        device_context = GetDC((HWND)p.winId()); /*Q5: must cast WID to HWND*/
#if USE_GRAPHICS
        if (graphics) {
            delete graphics;
        }
        graphics = new Graphics(device_context);
#endif //USE_GRAPHICS
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
#if !USE_GRAPHICS
        off_dc = CreateCompatibleDC(device_context);
#endif //USE_GRAPHICS
    }

    void setupQuality() {
    //http://www.codeproject.com/Articles/9184/Custom-AntiAliasing-with-GDI
    //http://msdn.microsoft.com/en-us/library/windows/desktop/ms533836%28v=vs.85%29.aspx
    /*
     *Graphics.DrawImage, Graphics.InterpolationMode
     * bitblit?
     */
#if USE_GRAPHICS
        if (!graphics)
            return;
        switch (quality) {
        case VideoRenderer::QualityBest:
            graphics->SetInterpolationMode(InterpolationModeHighQualityBicubic);
            break;
        case VideoRenderer::QualityFastest:
            graphics->SetInterpolationMode(InterpolationModeNearestNeighbor);
            break;
        default:
            graphics->SetInterpolationMode(InterpolationModeDefault);
            break;
        }
#endif //USE_GRAPHICS
    }

    bool support_bitblt;
    ULONG_PTR gdiplus_token;
    /*
     * GetDC(winID()): will flick
     * QPainter.paintEngine()->getDC() in paintEvent: doc says it's for internal use
     */
    HDC device_context;
    /* Our offscreen bitmap and its framebuffer */

#if USE_GRAPHICS
    Graphics *graphics;
#else
    HDC        off_dc;
    HBITMAP    off_bitmap;
#endif //USE_GRAPHICS
};

GDIRenderer::GDIRenderer(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f),VideoRenderer(*new GDIRendererPrivate())
{
    DPTR_INIT_PRIVATE(GDIRenderer);
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
    //setDCFromPainter(false); //changeEvent will be called on show()
    setAttribute(Qt::WA_PaintOnScreen, true);
}

bool GDIRenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    return VideoFormat::isRGB(pixfmt);
}

QPaintEngine* GDIRenderer::paintEngine() const
{
    return 0;
}

bool GDIRenderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(GDIRenderer);
    if (frame.constBits(0))
        d.video_frame = frame;
    else
        d.video_frame = frame.to(frame.pixelFormat());
    updateUi();
    return true;
}

void GDIRenderer::drawBackground()
{
    const QRegion bgRegion(backgroundRegion());
    if (bgRegion.isEmpty())
        return;
    const QColor bc(backgroundColor());
    DPTR_D(GDIRenderer);
    //HDC hdc = d.device_context;
    Graphics g(d.device_context);
    SolidBrush brush(Color(bc.alpha(), bc.red(), bc.green(), bc.blue())); //argb
    const QVector<QRect> bg(bgRegion.rects());
    foreach (const QRect& r, bg) {
        g.FillRectangle(&brush, r.x(), r.y(), r.width(), r.height());
    }
}

void GDIRenderer::drawFrame()
{
    DPTR_D(GDIRenderer);
    /* http://msdn.microsoft.com/en-us/library/windows/desktop/ms533829%28v=vs.85%29.aspx
     * Improving Performance by Avoiding Automatic Scaling
     * TODO: How about QPainter?
     */
    //steps to use BitBlt: http://bbs.csdn.net/topics/60183502
    Bitmap bitmap(d.video_frame.width(), d.video_frame.height(), d.video_frame.bytesPerLine(0)
                  , PixelFormat32bppRGB, (BYTE*)d.video_frame.constBits(0));
#if USE_GRAPHICS
    if (d.graphics)
        d.graphics->DrawImage(&bitmap, d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height());
#else
    if (FAILED(bitmap.GetHBITMAP(Color(), &d.off_bitmap))) {
        qWarning("Failed GetHBITMAP");
        return;
    }
    HDC hdc = d.device_context;
    HBITMAP hbmp_old = (HBITMAP)SelectObject(d.off_dc, d.off_bitmap);
    QRect roi = realROI();
    // && image.size() != size()
    //assume that the image data is already scaled to out_size(NOT renderer size!)
        StretchBlt(hdc
                   , d.out_rect.left(), d.out_rect.top()
                   , d.out_rect.width(), d.out_rect.height()
                   , d.off_dc
                   , roi.x(), roi.y()
                   , roi.width(), roi.height()
                   , SRCCOPY);
    SelectObject(d.off_dc, hbmp_old);
    DeleteObject(d.off_bitmap); //avoid mem leak
#endif
    //end paint
}

void GDIRenderer::paintEvent(QPaintEvent *)
{
    handlePaintEvent();
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

} //namespace QtAV

#include "GDIRenderer.moc"
