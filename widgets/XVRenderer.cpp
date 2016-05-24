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

/*
 * X11 headers define 'Bool' type which is used in qmetatype.h. we must include X11 files at last, i.e. XVRenderer_p.h. otherwise compile error
*/
#include "QtAV/VideoRenderer.h"
#include "QtAV/private/VideoRenderer_p.h"
#include "QtAV/FilterContext.h"
#include <QWidget>
#include <QResizeEvent>
#include <QtCore/qmath.h>
//#error qtextstream.h must be included before any header file that defines Status. Xlib.h defines Status
#include <QtCore/QTextStream> //build error
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>
#include "QtAV/private/factory.h"
//http://huangbster.i.sohu.com/blog/view/256490057.htm

namespace QtAV {

inline int scaleEQValue(int val, int min, int max)
{
    // max-min?
    return (val + 100)*((qAbs(min) + qAbs(max)))/200 - qAbs(min);
}

class XVRendererPrivate;
class XVRenderer: public QWidget, public VideoRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(XVRenderer)
public:
    XVRenderer(QWidget* parent = 0, Qt::WindowFlags f = 0);
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
    virtual void drawBackground() Q_DECL_OVERRIDE;
    virtual void drawFrame() Q_DECL_OVERRIDE;
    virtual void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    //stay on top will change parent, hide then show(windows)
    virtual void showEvent(QShowEvent *) Q_DECL_OVERRIDE;
private:
    virtual bool onSetBrightness(qreal b) Q_DECL_OVERRIDE;
    virtual bool onSetContrast(qreal c) Q_DECL_OVERRIDE;
    virtual bool onSetHue(qreal h) Q_DECL_OVERRIDE;
    virtual bool onSetSaturation(qreal s) Q_DECL_OVERRIDE;
};
typedef XVRenderer VideoRendererXV;
extern VideoRendererId VideoRendererId_XV;
#if 0
FACTORY_REGISTER_ID_AUTO(VideoRenderer, XV, "XVideo")
#else
void RegisterVideoRendererXV_Man()
{
    VideoRenderer::Register<XVRenderer>(VideoRendererId_XV, "XVideo");
}
#endif

VideoRendererId XVRenderer::id() const
{
    return VideoRendererId_XV;
}
#define FOURCC(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((unsigned)(d)<<24))
static const struct xv_format_entry_t {
    VideoFormat::PixelFormat format;
    int fourcc;
} xv_fmt[] = {
{ VideoFormat::Format_YUV420P, FOURCC('Y', 'V', '1', '2')},
{ VideoFormat::Format_YUV420P, FOURCC('I', '4', '2', '0')},
{ VideoFormat::Format_UYVY, FOURCC('U', 'Y', 'V', 'Y')},
{ VideoFormat::Format_YUYV, FOURCC('Y', 'U', 'Y', '2')},
// split nv12 in xv to reduce copy times(no need to convert in sws)
{ VideoFormat::Format_NV12, FOURCC('Y', 'V', '1', '2')},
{ VideoFormat::Format_NV21, FOURCC('Y', 'V', '1', '2')},
};
#undef FOURCC
int pixelFormatToXv(VideoFormat::PixelFormat fmt)
{
    for (size_t i = 0; i < sizeof(xv_fmt)/sizeof(xv_fmt[0]); ++i) {
        if (xv_fmt[i].format == fmt)
            return xv_fmt[i].fourcc;
    }
    return 0;
}

int xvFormatInPort(Display* disp, XvPortID port, VideoFormat::PixelFormat fmt) {
    const int xv_id = pixelFormatToXv(fmt);
    if (!xv_id)
        return 0;
    const int xv_type = VideoFormat::isRGB(fmt) ? XvRGB : XvYUV;
    const int xv_plane = VideoFormat::isPlanar(fmt) ? XvPlanar : XvPacked;
    int count = 0;
    XvImageFormatValues *xvifmts = XvListImageFormats(disp, port, &count);
    for (const XvImageFormatValues *xvifmt = xvifmts; xvifmt <  xvifmts+count; ++xvifmt) {
        qDebug("XvImageFormatValues: %s", xvifmt->guid);
        if (xvifmt->type == xv_type
                && xvifmt->format == xv_plane
                && xvifmt->id == xv_id
                ) {
            if (XvGrabPort(disp, port, 0) == Success) {
                XFree(xvifmts);
                return xv_id;
            }
        }
    }
    XFree(xvifmts);
    return 0;
}

class XVRendererPrivate : public VideoRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(XVRenderer)
    XVRendererPrivate():
        use_shm(true)
      , num_adaptors(0)
      , xv_image(0)
      , format_id(0x32315659) /*YV12*/
      , xv_image_width(0)
      , xv_image_height(0)
      , xv_port(0)
      , gc(NULL)
      , format(VideoFormat::Format_Invalid)
    {
        XInitThreads();
#ifndef _XSHM_H_
        use_shm = false;
#endif //_XSHM_H_
       //XvQueryExtension()
        display = XOpenDisplay(NULL);
        if (XvQueryAdaptors(display, DefaultRootWindow(display), &num_adaptors, &xv_adaptor_info) != Success) {
            available = false;
            qCritical("Query adaptors failed!");
            return;
        }
        if (num_adaptors < 1) {
            available = false;
            qCritical("No adaptor found!");
            return;
        }
    }
    ~XVRendererPrivate() {
        if (xv_adaptor_info) {
            XvFreeAdaptorInfo(xv_adaptor_info);
            xv_adaptor_info = 0;
        }
        destroyXVImage();
        if (gc) {
            XFreeGC(display, gc);
            gc = 0;
        }
        if (xv_port) {
            XvUngrabPort(display, xv_port, 0);
            xv_port = 0;
        }
        XCloseDisplay(display);
    }
    void destroyXVImage() {
        if (!xv_image)
            return;
#ifdef _XSHM_H_
        if (use_shm) {
            if (shm.shmaddr) {
                XShmDetach(display, &shm);
                shmctl(shm.shmid, IPC_RMID, 0);
                shmdt(shm.shmaddr);
            }
        } else
#endif //_XSHM_H_
        {
            // free if use copy (e.g. shm)
            free(xv_image->data);
        }
        XFree(xv_image);
        xv_image_width = 0;
        xv_image_height = 0;
    }
    bool prepareDeviceResource() {
        if (gc) {
            XFreeGC(display, gc);
            gc = 0;
        }
        gc = XCreateGC(display, q_func().winId(), 0, 0);
        if (!gc) {
            available = false;
            qCritical("Create GC failed!");
            return false;
        }
        XSetBackground(display, gc, BlackPixel(display, DefaultScreen(display)));
        if (filter_context)
            ((X11FilterContext*)filter_context)->resetX11((X11FilterContext::Display*)display, (X11FilterContext::GC)gc, (X11FilterContext::Drawable)q_func().winId());
        return true;
    }

    bool ensureImage(int w, int h, VideoFormat::PixelFormat pixfmt) {
        if (xv_image_width == w && xv_image_height == h && xv_image && format == pixfmt)
            return true;
        destroyXVImage();
        qDebug("port count: %d", num_adaptors);
        for (uint i = 0; i < num_adaptors; ++i) {
            //??
            if ((xv_adaptor_info[i].type & (XvInputMask | XvImageMask)) == (XvInputMask | XvImageMask)) {
                for (XvPortID p = xv_adaptor_info[i].base_id; p < xv_adaptor_info[i].base_id + xv_adaptor_info[i].num_ports; ++p) {
                    qDebug("XvAdaptorInfo: %s", xv_adaptor_info[i].name);
                    format_id = xvFormatInPort(display, p, pixfmt);
                    if (format_id) {
                        xv_port = p;
                        break;
                    }
                }
            }
            if (xv_port)
                break;
        }
        if (!xv_port) {
            qWarning("xv port not found!");
        }
        format = pixfmt;
        xv_image_width = w;
        xv_image_height = h;
#ifdef _XSHM_H_
        use_shm = XShmQueryExtension(display);
        qDebug("use xv shm: %d", use_shm);
        if (!use_shm)
            goto no_shm;
        xv_image = XvShmCreateImage(display, xv_port, format_id, 0, xv_image_width, xv_image_height, &shm);
        if (!xv_image)
            goto no_shm;
        shm.shmid = shmget(IPC_PRIVATE, xv_image->data_size, IPC_CREAT | 0777);
        qDebug("shmid: %d xv_image->data_size: %d, %dx%d", shm.shmid, xv_image->data_size, xv_image_width, xv_image_height);
        if (shm.shmid < 0) {
            qWarning("get shm failed. try to use none shm");
            goto no_shm;
        }
        shm.shmaddr = (char *)shmat(shm.shmid, 0, 0);
        if (shm.shmaddr == (char*)-1) {
            XFree(xv_image);
            qWarning("Shared memory error,disabling ( seg id error )");
            goto no_shm;
        }
        xv_image->data = shm.shmaddr;
        shm.readOnly = False;
        if (!XShmAttach(display, &shm)) {
            qWarning("Attach to shm failed! try to use none shm");
            goto no_shm;
        }
        XSync(display, False);
        shmctl(shm.shmid, IPC_RMID, 0);
        return true;
#endif //_XSHM_H_
no_shm:
        xv_image = XvCreateImage(display, xv_port, format_id, 0, xv_image_width, xv_image_height);
        if (!xv_image)
            return false;
        // malloc if use copy (e.g. shm)
        xv_image->data = (char*)malloc(xv_image->data_size);
        if (!xv_image->data)
            return false;
        XSync(display, False);
        return true;
    }

    bool XvSetPortAttributeIfExists(const char *key, int value);

    bool use_shm; //TODO: set by user
    unsigned int num_adaptors;
    XvAdaptorInfo *xv_adaptor_info;
    Display *display;
    XvImage *xv_image;
    int format_id;
    int xv_image_width, xv_image_height;
    XvPortID xv_port;
    GC gc;
#ifdef _XSHM_H_
    XShmSegmentInfo shm;
#endif //_XSHM_H_
    VideoFormat::PixelFormat format;
};

bool XVRendererPrivate::XvSetPortAttributeIfExists(const char *key, int value)
{
    int nb_attributes;
    XvAttribute *attributes = XvQueryPortAttributes(display, xv_port, &nb_attributes);
    if (!attributes) {
        qWarning("XvQueryPortAttributes error");
        return false;
    }
    for (int i = 0; i < nb_attributes; ++i) {
        const XvAttribute &attribute = ((XvAttribute*)attributes)[i];
        if (!qstrcmp(attribute.name, key) && (attribute.flags & XvSettable)) {
            XvSetPortAttribute(display, xv_port, XInternAtom(display, key, false), scaleEQValue(value, attribute.min_value, attribute.max_value));
            return true;
        }
    }
    qWarning("Can not set Xv attribute at key '%s'", key);
    return false;
}

XVRenderer::XVRenderer(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f)
  , VideoRenderer(*new XVRendererPrivate())
{
    setPreferredPixelFormat(VideoFormat::Format_YUV420P);
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
    //setAutoFillBackground(false);
    setAttribute(Qt::WA_PaintOnScreen, true);
    d_func().filter_context = VideoFilterContext::create(VideoFilterContext::X11);
    if (!d_func().filter_context) {
        qWarning("No filter context for X11");
    } else {
        d_func().filter_context->paint_device = this;
    }
}

bool XVRenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    // TODO: rgb use copyplane
    return pixfmt == VideoFormat::Format_YUV420P || pixfmt == VideoFormat::Format_YV12
            || pixfmt == VideoFormat::Format_NV12|| pixfmt == VideoFormat::Format_NV21
            || pixfmt == VideoFormat::Format_UYVY || pixfmt == VideoFormat::Format_YUYV
            ;
}

static void SplitPlanes(quint8 *dstu, size_t dstu_pitch,
                        quint8 *dstv, size_t dstv_pitch,
                        const quint8 *src, size_t src_pitch,
                        unsigned width, unsigned height)
{
    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            dstu[x] = src[2*x+0];
            dstv[x] = src[2*x+1];
        }
        src  += src_pitch;
        dstu += dstu_pitch;
        dstv += dstv_pitch;
    }
}

void CopyFromNv12(quint8 *dst[], size_t dst_pitch[],
                  const quint8 *src[2], size_t src_pitch[2],
                  unsigned width, unsigned height)
{
    VideoFrame::copyPlane(dst[0], dst_pitch[0], src[0], src_pitch[0], width, height);
    SplitPlanes(dst[2], dst_pitch[2], dst[1], dst_pitch[1], src[1], src_pitch[1], width/2, height/2);
}

void CopyFromYv12(quint8 *dst[], size_t dst_pitch[],
                  const quint8 *src[3], size_t src_pitch[3],
                  unsigned width, unsigned height)
{
     VideoFrame::copyPlane(dst[0], dst_pitch[0], src[0], src_pitch[0], width, height);
     VideoFrame::copyPlane(dst[1], dst_pitch[1], src[1], src_pitch[1], width/2, height/2);
     VideoFrame::copyPlane(dst[2], dst_pitch[2], src[2], src_pitch[2], width/2, height/2);
}

void CopyFromYv12_2(quint8 *dst[], size_t dst_pitch[],
                  const quint8 *src[3], size_t src_pitch[3],
                  unsigned width, unsigned height)
{
     VideoFrame::copyPlane(dst[0], dst_pitch[0], src[0], src_pitch[0], width, height);
     width /= 2;
     height /= 2;
     if (width == dst_pitch[1] && dst_pitch[1] == src_pitch[1]) {
         VideoFrame::copyPlane(dst[1], dst_pitch[1], src[1], src_pitch[1], width, height);
         VideoFrame::copyPlane(dst[2], dst_pitch[2], src[2], src_pitch[2], width, height);
     } else {
         for (unsigned i = 0; i < height; ++i) {
             memcpy(dst[2], src[2], width);
             memcpy(dst[1], src[1], width);
             src[1] += src_pitch[1];
             src[2] += src_pitch[2];
             dst[1] += dst_pitch[1];
             dst[2] += dst_pitch[2];
         }
     }
}

bool XVRenderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(XVRenderer);
    if (!frame.isValid()) {
        d.update_background = true;
        d.video_frame = VideoFrame(); // fill background
        updateUi();
        return true;
    }
    if (!d.ensureImage(frame.width(), frame.height(), frame.format().pixelFormat()))
        return false;
    if (frame.constBits(0))
        d.video_frame = frame;
    else // FIXME: not work
        d.video_frame = frame.to(frame.pixelFormat()); // assume frame format is supported
    int nb_planes = d.video_frame.planeCount();
    QVector<size_t> src_linesize(nb_planes);
    QVector<const quint8*> src(nb_planes);
    for (int i = 0; i < nb_planes; ++i) {
        src[i] = d.video_frame.constBits(i);
        src_linesize[i] = d.video_frame.bytesPerLine(i);
    }
    //swap UV
    quint8* dst[] = {
        (quint8*)(d.xv_image->data + d.xv_image->offsets[0]),
        (quint8*)(d.xv_image->data + d.xv_image->offsets[2]),
        (quint8*)(d.xv_image->data + d.xv_image->offsets[1])
    };
    size_t dst_linesize[] = {
        (size_t)d.xv_image->pitches[0],
        (size_t)d.xv_image->pitches[2],
        (size_t)d.xv_image->pitches[1]
    };
    // TODO: if not using shm and linesizes match, no copy is required. but seems no benefit
    switch (d.video_frame.pixelFormat()) {
    case VideoFormat::Format_YUV420P:
    case VideoFormat::Format_YV12:
        CopyFromYv12_2(dst, dst_linesize, src.data(), src_linesize.data(), dst_linesize[0], d.xv_image->height);
        break;
    case VideoFormat::Format_NV12:
        std::swap(dst[1], dst[2]);
        std::swap(dst_linesize[1], dst_linesize[2]);
        CopyFromNv12(dst, dst_linesize, src.data(), src_linesize.data(), dst_linesize[0], d.xv_image->height);
        break;
    case VideoFormat::Format_NV21:
        CopyFromNv12(dst, dst_linesize, src.data(), src_linesize.data(), dst_linesize[0], d.xv_image->height);
        break;
    case VideoFormat::Format_UYVY:
    case VideoFormat::Format_YUYV:
        VideoFrame::copyPlane(dst[0], dst_linesize[0], src[0], src_linesize[0], dst_linesize[0], d.xv_image->height);
        break;
    case VideoFormat::Format_BGR24:
        break;
    default:
        break;
    }
    update();
    return true;
}

QPaintEngine* XVRenderer::paintEngine() const
{
    return 0; //use native engine
}

void XVRenderer::drawBackground()
{
    const QRegion bgRegion(backgroundRegion());
    if (bgRegion.isEmpty())
        return;
    DPTR_D(XVRenderer);
    // TODO: set color
    const QVector<QRect> bg(bgRegion.rects());
    foreach (const QRect& r, bg) {
        XFillRectangle(d.display, winId(), d.gc, r.x(), r.y(), r.width(), r.height());
    }
    XFlush(d.display);
}

void XVRenderer::drawFrame()
{
    DPTR_D(XVRenderer);
    QRect roi = realROI();
#ifdef _XSHM_H_
        if (d.use_shm) {
            XvShmPutImage(d.display, d.xv_port, winId(), d.gc, d.xv_image
                          , roi.x(), roi.y(), roi.width(), roi.height()
                          , d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height()
                          , false /*true: send event*/);
            XSync(d.display, False); // update immediately
            return;
        }
#endif
        XvPutImage(d.display, d.xv_port, winId(), d.gc, d.xv_image
                   , roi.x(), roi.y(), roi.width(), roi.height()
                   , d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height());
        XSync(d.display, False);
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
    update(); //update background
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

bool XVRenderer::onSetBrightness(qreal b)
{
    DPTR_D(XVRenderer);
    return d.XvSetPortAttributeIfExists("XV_BRIGHTNESS", b*100);
}

bool XVRenderer::onSetContrast(qreal c)
{
    DPTR_D(XVRenderer);
    return d.XvSetPortAttributeIfExists("XV_CONTRAST",c*100);
}

bool XVRenderer::onSetHue(qreal h)
{
    DPTR_D(XVRenderer);
    return d.XvSetPortAttributeIfExists("XV_HUE", h*100);
}

bool XVRenderer::onSetSaturation(qreal s)
{
    DPTR_D(XVRenderer);
    return d.XvSetPortAttributeIfExists("XV_SATURATION", s*100);
}

} //namespace QtAV

#include "XVRenderer.moc"
