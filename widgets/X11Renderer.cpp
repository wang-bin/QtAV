/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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
 * X11 headers define 'Bool' type which is used in qmetatype.h. we must include X11 files at last, i.e. X11Renderer_p.h. otherwise compile error
*/
#include "QtAV/VideoRenderer.h"
#include "QtAV/private/VideoRenderer_p.h"
#include <QWidget>
#include <QResizeEvent>
#include <QtCore/qmath.h>
#include <QtDebug>
//#error qtextstream.h must be included before any header file that defines Status. Xlib.h defines Status
#include <QtCore/QTextStream> //build error
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
//#include "QtAV/private/factory.h"
//scale: http://www.opensource.apple.com/source/X11apps/X11apps-14/xmag/xmag-X11R7.0-1.0.1/Scale.c
namespace QtAV {
class X11RendererPrivate;
class X11Renderer: public QWidget, public VideoRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(X11Renderer)
public:
    X11Renderer(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual VideoRendererId id() const Q_DECL_OVERRIDE;
    virtual bool isSupported(VideoFormat::PixelFormat pixfmt) const Q_DECL_OVERRIDE;

    /* WA_PaintOnScreen: To render outside of Qt's paint system, e.g. If you require
     * native painting primitives, you need to reimplement QWidget::paintEngine() to
     * return 0 and set this flag
     * If paintEngine != 0, the window will flicker when drawing without QPainter.
     * Make sure that paintEngine returns 0 to avoid flicking.
     */
    virtual QPaintEngine* paintEngine() const Q_DECL_OVERRIDE;
    /*http://lists.trolltech.com/qt4-preview-feedback/2005-04/thread00609-0.html
     * true: paintEngine is QPainter. Painting with QPainter support double buffer
     * false: no double buffer, should reimplement paintEngine() to return 0 to avoid flicker
     */
    virtual QWidget* widget() Q_DECL_OVERRIDE { return this; }
protected:
    virtual bool receiveFrame(const VideoFrame& frame) Q_DECL_OVERRIDE;
    virtual bool needUpdateBackground() const Q_DECL_OVERRIDE;
    //called in paintEvent before drawFrame() when required
    virtual void drawBackground() Q_DECL_OVERRIDE;
    virtual bool needDrawFrame() const Q_DECL_OVERRIDE;
    //draw the current frame using the current paint engine. called by paintEvent()
    virtual void drawFrame() Q_DECL_OVERRIDE;
    virtual void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    //stay on top will change parent, hide then show(windows). we need GetDC() again
    virtual void showEvent(QShowEvent *) Q_DECL_OVERRIDE;
};
typedef X11Renderer VideoRendererX11;
extern VideoRendererId VideoRendererId_X11;
#if 0
FACTORY_REGISTER_ID_AUTO(VideoRenderer, X11, "X11")
#else
void RegisterVideoRendererX11_Man()
{
    VideoRenderer::Register<X11Renderer>(VideoRendererId_X11, "X11");
}
#endif

VideoRendererId X11Renderer::id() const
{
    return VideoRendererId_X11;
}

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define BO_NATIVE    LSBFirst
#define BO_NONNATIVE MSBFirst
#else
#define BO_NATIVE    MSBFirst
#define BO_NONNATIVE LSBFirst
#endif
static const struct fmt2Xfmtentry {
    VideoFormat::PixelFormat fmt;
    int byte_order;
    unsigned red_mask;
    unsigned green_mask;
    unsigned blue_mask;
} fmt2Xfmt[] = {
    {VideoFormat::Format_BGR555, BO_NATIVE,    0x0000001F, 0x000003E0, 0x00007C00},
    {VideoFormat::Format_BGR555, BO_NATIVE,    0x00007C00, 0x000003E0, 0x0000001F},
    {VideoFormat::Format_BGR565, BO_NATIVE,    0x0000001F, 0x000007E0, 0x0000F800},
    {VideoFormat::Format_RGB565, BO_NATIVE,    0x0000F800, 0x000007E0, 0x0000001F},
    {VideoFormat::Format_RGB24,  MSBFirst,     0x00FF0000, 0x0000FF00, 0x000000FF},
    {VideoFormat::Format_RGB24,  LSBFirst,     0x000000FF, 0x0000FF00, 0x00FF0000},
    {VideoFormat::Format_BGR24,  MSBFirst,     0x000000FF, 0x0000FF00, 0x00FF0000},
    {VideoFormat::Format_BGR24,  LSBFirst,     0x00FF0000, 0x0000FF00, 0x000000FF},
    {VideoFormat::Format_BGR32,  BO_NATIVE,    0x000000FF, 0x0000FF00, 0x00FF0000},
    {VideoFormat::Format_BGR32,  BO_NONNATIVE, 0xFF000000, 0x00FF0000, 0x0000FF00}, //abgr rgba
    {VideoFormat::Format_RGB32,  BO_NATIVE,    0x00FF0000, 0x0000FF00, 0x000000FF}, //argb bgra
    {VideoFormat::Format_RGB32,  BO_NONNATIVE, 0x0000FF00, 0x00FF0000, 0xFF000000},
    {VideoFormat::Format_ARGB32,   MSBFirst,     0x00FF0000, 0x0000FF00, 0x000000FF},
    {VideoFormat::Format_ARGB32,   LSBFirst,     0x0000FF00, 0x00FF0000, 0xFF000000},
    {VideoFormat::Format_ABGR32,   MSBFirst,     0x000000FF, 0x0000FF00, 0x00FF0000},
    {VideoFormat::Format_ABGR32,   LSBFirst,     0xFF000000, 0x00FF0000, 0x0000FF00},
    {VideoFormat::Format_RGBA32,   MSBFirst,     0xFF000000, 0x00FF0000, 0x0000FF00},
    {VideoFormat::Format_RGBA32,   LSBFirst,     0x000000FF, 0x0000FF00, 0x00FF0000},
    {VideoFormat::Format_BGRA32,   MSBFirst,     0x0000FF00, 0x00FF0000, 0xFF000000},
    {VideoFormat::Format_BGRA32,   LSBFirst,     0x00FF0000, 0x0000FF00, 0x000000FF},
    {VideoFormat::Format_Invalid, BO_NATIVE, 0, 0, 0}
};

VideoFormat::PixelFormat pixelFormat(XImage* xi) {
    const struct fmt2Xfmtentry *fmte = fmt2Xfmt;
    while (fmte->fmt != VideoFormat::Format_Invalid) {
        int depth = VideoFormat(fmte->fmt).bitsPerPixel();
        // 15->16? mpv
        if (depth == xi->bits_per_pixel && fmte->byte_order == xi->byte_order
                && fmte->red_mask == xi->red_mask
                && fmte->green_mask == xi->green_mask
                && fmte->blue_mask == xi->blue_mask)
            break;
        //qDebug() << fmte->fmt;
        fmte++;
    }
    qDebug("XImage format: bpp %d, endian: %d, R %X, G %X, B %X", xi->bits_per_pixel, xi->byte_order, xi->red_mask, xi->green_mask, xi->blue_mask);
    qDebug() << "PixelFormat: " << fmte->fmt;
    return fmte->fmt;
}

class X11RendererPrivate : public VideoRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(X11Renderer)
    X11RendererPrivate():
        use_shm(true)
      , num_adaptors(0)
      , ximage(NULL)
      , ximage_width(0)
      , ximage_height(0)
      , gc(NULL)
      , pixfmt(VideoFormat::Format_Invalid)
    {
        XInitThreads();
#ifndef _XSHM_H_
        use_shm = false;
#endif //_XSHM_H_
       //XvQueryExtension()
        char* dispName = XDisplayName(NULL);
        qDebug("X11 open display: %s", dispName);
        display = XOpenDisplay(dispName);
        if (!display) {
            available = false;
            qWarning("Open X11 display error");
            return;
        }
        XWindowAttributes attribs;
        XGetWindowAttributes(display, DefaultRootWindow(display), &attribs);
        depth = attribs.depth;
        if (!XMatchVisualInfo(display, DefaultScreen(display), depth, TrueColor, &vinfo)) {
            qWarning("XMatchVisualInfo error");
            available = false;
            return;
        }
        XImage *ximg = NULL;
        if (depth != 15 && depth != 16 && depth != 24 && depth != 32) { //??
            Visual *vs;
            //depth = //find_depth_from_visuals
        } else {
            ximg = XGetImage(display, DefaultRootWindow(display), 0, 0, 1, 1, AllPlanes, ZPixmap);
        }
        ximage_depth = depth;
        unsigned int mask = 0;
        if (ximg) {
            bpp = ximg->bits_per_pixel;
            if ((ximage_depth+7)/8 != (bpp+7)/8) //24bpp use 32 depth
                ximage_depth = bpp;
            mask = ximg->red_mask | ximg->green_mask | ximg->blue_mask;
            qDebug("color mask: %X R:%1X G:%1X B:%1X", mask, ximg->red_mask, ximg->green_mask, ximg->blue_mask);
            XDestroyImage(ximg);
        }
        if (((ximage_depth + 7) / 8) == 2) {
            if (mask == 0x7FFF)
                ximage_depth = 15;
            else if (mask == 0xFFFF)
                ximage_depth = 16;
        }
    }
    ~X11RendererPrivate() {
        destroyX11Image();
        XCloseDisplay(display);
    }
    void destroyX11Image() {
        if (use_shm) {
            if (shm.shmaddr) {
                XShmDetach(display, &shm);
                shmctl(shm.shmid, IPC_RMID, 0);
                shmdt(shm.shmaddr);
            }
        }
        if (ximage)
            XDestroyImage(ximage);
        ximage = NULL;
        ximage_width = 0;
        ximage_height = 0;
    }
    bool prepareDeviceResource() {
        if (gc) {
            XFreeGC(display, gc);
            gc = 0;
        }
        gc = XCreateGC(display, q_func().winId(), 0, 0); //DefaultRootWindow
        if (!gc) {
            available = false;
            qCritical("Create GC failed!");
            return false;
        }
        XSetBackground(display, gc, BlackPixel(display, DefaultScreen(display)));
        return true;
    }

    bool ensureImage(int w, int h) {
        if (ximage_width == w && ximage_height == h && ximage)
            return true;
        destroyX11Image();
        ximage_width = w;
        ximage_height = h;
        use_shm = XShmQueryExtension(display);
        qDebug("use xv shm: %d", use_shm);
        if (!use_shm)
            goto no_shm;
        ximage = XShmCreateImage(display, vinfo.visual, depth, ZPixmap, NULL, &shm, ximage_width, ximage_height);
        if (!ximage) {
            qWarning("XShmCreateImage error");
            goto no_shm;
        }
        shm.shmid = shmget(IPC_PRIVATE, ximage->bytes_per_line*ximage->height, IPC_CREAT | 0777);
        if (shm.shmid < 0) {
            qWarning("shmget error");
            goto no_shm;
        }
        shm.shmaddr = (char *)shmat(shm.shmid, 0, 0);
        if (shm.shmaddr == (char*)-1) {
            XDestroyImage(ximage);
            ximage = NULL;
            qWarning("Shared memory error,disabling ( seg id error )");
            goto no_shm;
        }
        ximage->data = shm.shmaddr;
        shm.readOnly = False;
        if (!XShmAttach(display, &shm)) {
            qWarning("Attach to shm failed! try to use none shm");
            goto no_shm;
        }
        XSync(display, False);
        shmctl(shm.shmid, IPC_RMID, 0);
        pixfmt = pixelFormat(ximage);
        return true;
no_shm:
        ximage = XCreateImage(display, vinfo.visual, depth, ZPixmap, 0, NULL, ximage_width, ximage_height, 8, 0);
        if (!ximage)
            return false;
        // malloc if use copy (e.g. shm)
        // TODO: align 16?
        ximage->data = NULL;// (char*)calloc(1, ximage->bytes_per_line*ximage->height);
        if (!ximage->data)
            return false;
        XSync(display, False);
        pixfmt = pixelFormat(ximage);
        return true;
    }

    bool use_shm; //TODO: set by user
    unsigned int num_adaptors;
    int bpp;
    int depth;
    XVisualInfo vinfo;
    Display *display;
    XImage *ximage;
    int current_buf;
    int ximage_depth;
    int ximage_width, ximage_height;
    GC gc;
    XShmSegmentInfo shm;
    VideoFormat::PixelFormat pixfmt;
};

X11Renderer::X11Renderer(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f)
  , VideoRenderer(*new X11RendererPrivate())
{
    DPTR_INIT_PRIVATE(X11Renderer);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    /* To rapidly update custom widgets that constantly paint over their entire areas with
     * opaque content, e.g., video streaming widgets, it is better to set the widget's
     * Qt::WA_OpaquePaintEvent, avoiding any unnecessary overhead associated with repainting the
     * widget's background
     */
    setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    //setAutoFillBackground(false);
    setAttribute(Qt::WA_PaintOnScreen, true);
}

bool X11Renderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{    
    Q_UNUSED(pixfmt);
    // if always return true, then convert to x11 format and scale in receiveFrame() only once. no need to convert format first then scale
    return true;//pixfmt == d_func().pixfmt && d_func().pixfmt != VideoFormat::Format_Invalid;
}

bool X11Renderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(X11Renderer);
    if (frame.isValid()) {
        if (!d.ensureImage(frame.width(), frame.height()))
            return false;
        if (preferredPixelFormat() != d.pixfmt) {
            qDebug() << "x11 preferred pixel format: " << d.pixfmt;
            setPreferredPixelFormat(d.pixfmt);
        }
    }
    if (frame.constBits(0)) {
        if (frame.pixelFormat() != d.pixfmt)
            d.video_frame = frame.to(d.pixfmt);
        else
            d.video_frame = frame;
        if (d.use_shm) {
            memcpy(d.ximage->data, d.video_frame.constBits(0), d.ximage->bytes_per_line*d.ximage->height);
        } else {
            d.ximage->data = (char*)d.video_frame.constBits(0);
        }
    } else {
        if (!d.video_frame.map(HostMemorySurface, d.ximage)) { //check pixel format and scale to ximage size&line_size
            d.video_frame = frame.to(d.pixfmt);
            d.ximage->data = (char*)d.video_frame.constBits(0);
        }
    }
    update();
    return true;
}

QPaintEngine* X11Renderer::paintEngine() const
{
    return 0; //use native engine
}

bool X11Renderer::needUpdateBackground() const
{
    DPTR_D(const X11Renderer);
    return d.update_background && d.out_rect != rect();/* || d.data.isEmpty()*/ //data is always empty because we never copy it now.
}

void X11Renderer::drawBackground()
{
    if (autoFillBackground())
        return;
    DPTR_D(X11Renderer);
    // TODO: fill once each resize? mpv
    if (d.video_frame.isValid()) {
        if (d.out_rect.width() < width()) {
            XFillRectangle(d.display, winId(), d.gc, 0, 0, (width() - d.out_rect.width())/2, height());
            XFillRectangle(d.display, winId(), d.gc, d.out_rect.right(), 0, (width() - d.out_rect.width())/2, height());
        }
        if (d.out_rect.height() < height()) {
            XFillRectangle(d.display, winId(), d.gc, 0, 0,  width(), (height() - d.out_rect.height())/2);
            XFillRectangle(d.display, winId(), d.gc, 0, d.out_rect.bottom(), width(), (height() - d.out_rect.height())/2);
        }
    } else {
        XFillRectangle(d.display, winId(), d.gc, 0, 0, width(), height());
    }
    XFlush(d.display); // apply the color
}

bool X11Renderer::needDrawFrame() const
{
    DPTR_D(const X11Renderer);
    return  d.ximage || d.video_frame.isValid();
}

void X11Renderer::drawFrame()
{
    // TODO: interop
    DPTR_D(X11Renderer);
    QRect roi = realROI();
    if (d.use_shm) {
        XShmPutImage(d.display, winId(), d.gc, d.ximage
                      , roi.x(), roi.y()//, roi.width(), roi.height()
                      , d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height()
                      , false /*true: send event*/);
    } else {
        XPutImage(d.display, winId(), d.gc, d.ximage
                   , roi.x(), roi.y()//, roi.width(), roi.height()
                   , d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height());
    }
    XSync(d.display, False); // update immediately
}

void X11Renderer::paintEvent(QPaintEvent *)
{
    handlePaintEvent();
}

void X11Renderer::resizeEvent(QResizeEvent *e)
{
    DPTR_D(X11Renderer);
    d.update_background = true;
    resizeRenderer(e->size());
    update(); //update background
}

void X11Renderer::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    DPTR_D(X11Renderer);
    d.update_background = true;
    /*
     * Do something that depends on widget below! e.g. recreate render target for direct2d.
     * When Qt::WindowStaysOnTopHint changed, window will hide first then show. If you
     * don't do anything here, the widget content will never be updated.
     */
    d.prepareDeviceResource();
}
} //namespace QtAV

#include "X11Renderer.moc"
