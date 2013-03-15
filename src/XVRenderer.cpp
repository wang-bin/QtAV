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

#include "QtAV/XVRenderer.h"
#include "private/VideoRenderer_p.h"
#include <QResizeEvent>
#include <QX11Info>

#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>

//http://huangbster.i.sohu.com/blog/view/256490057.htm

namespace QtAV {

class XVRendererPrivate : public VideoRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(XVRenderer)

    XVRendererPrivate():
        use_shm(true)
      , num_adaptors(0)
      , format_id(0x32315659) /*YV12*/
      , xv_image_width(0)
      , xv_image_height(0)
      , xv_port(0)
    {
        //XvQueryExtension()
        display = QX11Info::display();
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
            if (use_shm) {
                if (shm.shmaddr) {
                    XShmDetach(display, &shm);
                    shmctl(shm.shmid, IPC_RMID, 0);
                    shmdt(shm.shmaddr);
                }
            } else {
                delete [] xv_image->data;
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
    bool prepairDeviceResource() {
        gc = XCreateGC(display, q_func().winId(), 0, 0);
        if (!gc) {
            available = false;
            qCritical("Create GC failed!");
            return false;
        }
    }

    bool prepairImage(int w, int h) {
        //TODO: check shm
        if (xv_image_width == w && xv_image_height == h && xv_image)
            return true;
        xv_image_width = w;
        xv_image_height = h;
        if (!use_shm) {
            xv_image = XvCreateImage(display, xv_port, format_id, 0, xv_image_width, xv_image_height);
            xv_image->data = new char[xv_image->data_size];
        } else {
            xv_image = XvShmCreateImage(display, xv_port, format_id, 0, xv_image_width, xv_image_height, &shm);
            shm.shmid = shmget(IPC_PRIVATE, xv_image->data_size, IPC_CREAT | 0777);
            if (shm.shmid < 0) {
                qCritical("get shm failed");
                return false;
            }
            shm.shmaddr = (char *)shmat(shm.shmid, 0, 0);
            xv_image->data = shm.shmaddr;
            shm.readOnly = 0;
            if (!XShmAttach(display, &shm)) {
                qCritical("Attach to shm failed!");
                return false;
            }
            XSync(display, false);
            shmctl(shm.shmid, IPC_RMID, 0);
        }
        return true;
    }

    bool use_shm;
    unsigned int num_adaptors;
    XvAdaptorInfo *xv_adaptor_info;
    Display *display;
    XvImage *xv_image;
    int format_id;
    int xv_image_width, xv_image_height;
    XvPortID xv_port;
    GC gc;
    XShmSegmentInfo shm;
};

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
    return 0; //use native engine
}

void XVRenderer::convertData(const QByteArray &data)
{
    DPTR_D(XVRenderer);
    if (!d.prepairImage(d.src_width, d.src_height))
        return;
    //TODO: if date is deep copied, mutex can be avoided
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    memcpy(d.xv_image->data, d.data.data(), d.xv_image->data_size);
}

void XVRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(XVRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    //begin paint. how about QPainter::beginNativePainting()?

    //fill background color when necessary, e.g. renderer is resized, image is null
    if ((d.update_background && d.out_rect != rect()) || d.data.isEmpty()) {
        d.update_background = false;
        //fill background color. DO NOT return, you must continue drawing
    }
    if (d.data.isEmpty()) {
        return;
    }
    if (!d.use_shm)
        XvPutImage(d.display, d.xv_port, winId(), d.gc, d.xv_image
                   , 0, 0, d.src_width, d.src_height
                   , d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height());
    else
        XvShmPutImage(d.display, d.xv_port, winId(), d.gc, d.xv_image
                      , 0, 0, d.src_width, d.src_height
                      , d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height()
                      , false /*true: send event*/);
    //end paint. how about QPainter::endNativePainting()?
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
    d.prepairDeviceResource();
}

bool XVRenderer::write()
{
    update();
    return true;
}

} //namespace QtAV
