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
#include "QtAV/VideoRenderer.h"
#include "QtAV/private/VideoRenderer_p.h"
#include <QWidget>
#include <QResizeEvent>
#include <QtCore/qmath.h>
//#error qtextstream.h must be included before any header file that defines Status. Xlib.h defines Status
#include <QtCore/QTextStream> //build error
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>
#include "QtAV/private/prepost.h"

#include "utils/Logger.h"

//http://huangbster.i.sohu.com/blog/view/256490057.htm

namespace QtAV {

inline int scaleEQValue(int val, int min, int max)
{
    // max-min?
    return (val + 100)*((qAbs(min) + qAbs(max)))/200 - qAbs(min);
}

class XVRendererPrivate;
class XVRenderer : public QWidget, public VideoRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(XVRenderer)
public:
    XVRenderer(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual VideoRendererId id() const;
    virtual bool isSupported(VideoFormat::PixelFormat pixfmt) const;

    /* WA_PaintOnScreen: To render outside of Qt's paint system, e.g. If you require
     * native painting primitives, you need to reimplement QWidget::paintEngine() to
     * return 0 and set this flag
     * If paintEngine != 0, the window will flicker when drawing without QPainter.
     * Make sure that paintEngine returns 0 to avoid flicking.
     */
    virtual QPaintEngine* paintEngine() const;
    /*http://lists.trolltech.com/qt4-preview-feedback/2005-04/thread00609-0.html
     * true: paintEngine is QPainter. Painting with QPainter support double buffer
     * false: no double buffer, should reimplement paintEngine() to return 0 to avoid flicker
     */
    virtual QWidget* widget() { return this; }
protected:
    virtual bool receiveFrame(const VideoFrame& frame);
    virtual bool needUpdateBackground() const;
    //called in paintEvent before drawFrame() when required
    virtual void drawBackground();
    virtual bool needDrawFrame() const;
    //draw the current frame using the current paint engine. called by paintEvent()
    virtual void drawFrame();
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    //stay on top will change parent, hide then show(windows). we need GetDC() again
    virtual void showEvent(QShowEvent *);

private:
    virtual bool onSetBrightness(qreal b);
    virtual bool onSetContrast(qreal c);
    virtual bool onSetHue(qreal h);
    virtual bool onSetSaturation(qreal s);
};
typedef XVRenderer VideoRendererXV;
extern VideoRendererId VideoRendererId_XV;
FACTORY_REGISTER_ID_AUTO(VideoRenderer, XV, "XVideo")

void RegisterVideoRendererXV_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, XV, "XVideo")
}

VideoRendererId XVRenderer::id() const
{
    return VideoRendererId_XV;
}

class XVRendererPrivate : public VideoRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(XVRenderer)

    XVRendererPrivate():
        use_shm(false)
      , num_adaptors(0)
      , xv_image(0)
      , format_id(0x32315659) /*YV12*/
      , xv_image_width(0)
      , xv_image_height(0)
      , xv_port(0)
    {
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
        for (uint i = 0; i < num_adaptors; ++i) {
            //??
            if ((xv_adaptor_info[i].type & (XvInputMask | XvImageMask)) == (XvInputMask | XvImageMask)) {
                //if ( !adaptorName.isEmpty() && adaptorName != ai[ i ].name )
                //    continue;
                for (XvPortID p = xv_adaptor_info[i].base_id; p < xv_adaptor_info[i].base_id + xv_adaptor_info[i].num_ports; ++p) {
                    if (findYV12Port(p))
                        break;
                }
            }
            if (xv_port)
                break;
        }
        if (!xv_port) {
            available = false;
            qCritical("YV12 port not found!");
            return;
        }
    }
    ~XVRendererPrivate() {
        if (xv_adaptor_info) {
            XvFreeAdaptorInfo(xv_adaptor_info);
            xv_adaptor_info = 0;
        }
        if (xv_image) {
#ifdef _XSHM_H_
            if (use_shm) {
                if (shm.shmaddr) {
                    XShmDetach(display, &shm);
                    shmctl(shm.shmid, IPC_RMID, 0);
                    shmdt(shm.shmaddr);
                }
            }
#endif //_XSHM_H_
            if (!use_shm) {
                // free if use copy (e.g. shm)
                free(xv_image->data);
            }
            XFree(xv_image);
        }
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
    bool findYV12Port(XvPortID port) {
        int count = 0;
        XvImageFormatValues *xv_image_formats = XvListImageFormats(display, port, &count);
        for (int j = 0; j < count; ++j) {
            if (xv_image_formats[j].type == XvYUV && QLatin1String(xv_image_formats[j].guid) == QLatin1String("YV12")) {
                if (XvGrabPort(display, port, 0) == Success) {
                    xv_port = port;
                    format_id = xv_image_formats[j].id;
                    XFree(xv_image_formats);
                    return true;
                }
            }
        }
        XFree(xv_image_formats);
        return false;
    }
    bool prepareDeviceResource() {
        gc = XCreateGC(display, q_func().winId(), 0, 0);
        if (!gc) {
            available = false;
            qCritical("Create GC failed!");
            return false;
        }
        XSetBackground(display, gc, BlackPixel(display, DefaultScreen(display)));
        return true;
    }

    bool prepareImage(int w, int h) {
        if (xv_image_width == w && xv_image_height == h && xv_image)
            return true;
        xv_image_width = w;
        xv_image_height = h;
#ifdef _XSHM_H_
        if (use_shm) {
            xv_image = XvShmCreateImage(display, xv_port, format_id, 0, xv_image_width, xv_image_height, &shm);
            shm.shmid = shmget(IPC_PRIVATE, xv_image->data_size, IPC_CREAT | 0777);
            if (shm.shmid < 0) {
                qCritical("get shm failed. try to use none shm");
                use_shm = false;
            } else {
                shm.shmaddr = (char *)shmat(shm.shmid, 0, 0);
                xv_image->data = shm.shmaddr;
                shm.readOnly = 0;
                if (XShmAttach(display, &shm)) {
                    XSync(display, false);
                    shmctl(shm.shmid, IPC_RMID, 0);
                } else {
                    qCritical("Attach to shm failed! try to use none shm");
                    use_shm = false;
                }
            }
        }
#endif //_XSHM_H_
        if (!use_shm) {
            xv_image = XvCreateImage(display, xv_port, format_id, 0, xv_image_width, xv_image_height);
            // malloc if use copy (e.g. shm)
            xv_image->data = (char*)malloc(xv_image->data_size);
        }
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

// TODO: add yuv support
bool XVRenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    // TODO: NV12
    return pixfmt == VideoFormat::Format_YUV420P || pixfmt == VideoFormat::Format_YV12
            || pixfmt == VideoFormat::Format_NV12|| pixfmt == VideoFormat::Format_NV21;
}

static void CopyPlane(void *dest, const uchar* src, unsigned dst_linesize, unsigned src_linesize, unsigned height )
{
    unsigned offset = 0;
    unsigned char *dest_data = (unsigned char*)dest;
    for (unsigned i = 0; i < height; ++i) {
        memcpy(dest_data, src + offset, dst_linesize);
        offset += src_linesize;
        dest_data += dst_linesize;
    }
}

static void CopyYV12(void *dest, const uchar* src[], unsigned luma_width, unsigned chroma_width, unsigned src_linesize[], unsigned height )
{
    unsigned offset = 0;
    unsigned char *dest_data = (unsigned char*)dest;
    for (unsigned i = 0; i < height; ++i) {
        memcpy(dest_data, src[0] + offset, luma_width);
        offset += src_linesize[0];
        dest_data += luma_width;
    }
    offset = 0;
    height >>= 1;
    const unsigned wh = chroma_width * height;
    for (unsigned i = 0; i < height; ++i) {
        memcpy(dest_data, src[2] + offset, chroma_width);
        memcpy(dest_data + wh, src[1] + offset, chroma_width);
        offset += src_linesize[1];
        dest_data += chroma_width;
    }
}

static void CopyPlane(quint8 *dst, size_t dst_pitch,
                      const quint8 *src, size_t src_pitch,
                      unsigned width, unsigned height)
{
    for (unsigned y = 0; y < height; y++) {
        memcpy(dst, src, width);
        src += src_pitch;
        dst += dst_pitch;
    }
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
    CopyPlane(dst[0], dst_pitch[0], src[0], src_pitch[0], width, height);
    SplitPlanes(dst[2], dst_pitch[2], dst[1], dst_pitch[1], src[1], src_pitch[1], width/2, height/2);
}

void CopyFromYv12(quint8 *dst[], size_t dst_pitch[],
                  const quint8 *src[3], size_t src_pitch[3],
                  unsigned width, unsigned height)
{
     CopyPlane(dst[0], dst_pitch[0], src[0], src_pitch[0], width, height);
     CopyPlane(dst[1], dst_pitch[1], src[1], src_pitch[1], width/2, height/2);
     CopyPlane(dst[2], dst_pitch[2], src[2], src_pitch[2], width/2, height/2);
}

bool XVRenderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(XVRenderer);
    if (!d.prepareImage(d.src_width, d.src_height))
        return false;
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.video_frame = frame;//.clone();
#if 1
    int nb_planes = d.video_frame.planeCount();
    QVector<size_t> src_linesize(nb_planes);
    QVector<const quint8*> src(nb_planes);
    for (int i = 0; i < nb_planes; ++i) {
        src[i] = d.video_frame.bits(i);
        src_linesize[i] = d.video_frame.bytesPerLine(i);
    }
#endif
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
    switch (d.video_frame.pixelFormat()) {
    case VideoFormat::Format_YUV420P:
    case VideoFormat::Format_YV12: {
#if 1
        CopyFromYv12(dst, dst_linesize, src.data(), src_linesize.data(), dst_linesize[0], d.xv_image->height);
#else
        const uchar *src[] = {
            d.video_frame.bits(0),
            d.video_frame.bits(1),
            d.video_frame.bits(2)
        };
        unsigned src_linesize[] = {
            (unsigned)d.video_frame.bytesPerLine(0),
            (unsigned)d.video_frame.bytesPerLine(1),
            (unsigned)d.video_frame.bytesPerLine(2)
        };
#define COPY_YV12 1
#if COPY_YV12
        CopyYV12(d.xv_image->data, src, d.xv_image->pitches[0],d.xv_image->pitches[1], src_linesize, d.xv_image->height);
#else
        CopyPlane(d.xv_image->data + d.xv_image->offsets[0], src[0], d.xv_image->pitches[0], src_linesize[0], d.xv_image->height);
        CopyPlane(d.xv_image->data + d.xv_image->offsets[2], src[1], d.xv_image->pitches[2], src_linesize[1], d.xv_image->height/2);
        CopyPlane(d.xv_image->data + d.xv_image->offsets[1], src[2], d.xv_image->pitches[1], src_linesize[2], d.xv_image->height/2);
#endif
#endif
    }
        break;
    case VideoFormat::Format_NV12:
        std::swap(dst[1], dst[2]);
        std::swap(dst_linesize[1], dst_linesize[2]);
        CopyFromNv12(dst, dst_linesize, src.data(), src_linesize.data(), dst_linesize[0], d.xv_image->height);
        break;
    case VideoFormat::Format_NV21:
        CopyFromNv12(dst, dst_linesize, src.data(), src_linesize.data(), dst_linesize[0], d.xv_image->height);
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

bool XVRenderer::needUpdateBackground() const
{
    DPTR_D(const XVRenderer);
    return d.update_background && d.out_rect != rect();/* || d.data.isEmpty()*/ //data is always empty because we never copy it now.
}

void XVRenderer::drawBackground()
{
    DPTR_D(XVRenderer);
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
    //FIXME: xv should always draw the background.
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
