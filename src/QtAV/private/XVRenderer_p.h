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

#ifndef QTAV_XVRENDERER_P_H
#define QTAV_XVRENDERER_P_H

#include "private/VideoRenderer_p.h"
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
#ifdef _XSHM_H_
            if (use_shm) {
                if (shm.shmaddr) {
                    XShmDetach(display, &shm);
                    shmctl(shm.shmid, IPC_RMID, 0);
                    shmdt(shm.shmaddr);
                }
            } else {
#else //_XSHM_H_
                delete [] xv_image->data;
#endif //_XSHM_H_
#ifdef _XSHM_H_
            }
#endif //_XSHM_H_
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
            xv_image->data = new char[xv_image->data_size];
        }
        return true;
    }

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

} //namespace QtAV
#endif // QTAV_XVRENDERER_P_H
