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
#include "QtAV/XVRenderer.h"
#include <QResizeEvent>
#include <QtCore/qmath.h>
#include "private/XVRenderer_p.h"
namespace QtAV {

inline int scaleEQValue(int val, int min, int max)
{
    // max-min?
    return (val + 100)*((qAbs(min) + qAbs(max)))/200 - qAbs(min);
}

bool XVRendererPrivate::XvSetPortAttributeIfExists(void *attributes, int attrib_count, const char *k, int v)
{
    for (int i = 0; i < attrib_count; ++i) {
        const XvAttribute &attribute = ((XvAttribute*)attributes)[i];
        if (!qstrcmp(attribute.name, k) && (attribute.flags & XvSettable)) {
            XvSetPortAttribute(display, xv_port, XInternAtom(display, k, false), scaleEQValue(v, attribute.min_value, attribute.max_value));
            return true;
        }
    }
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
    return pixfmt == VideoFormat::Format_YUV420P || pixfmt == VideoFormat::Format_YV12;
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

bool XVRenderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(XVRenderer);
    if (!d.prepareImage(d.src_width, d.src_height))
        return false;
    //TODO: if date is deep copied, mutex can be avoided
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.video_frame = frame.clone();
    //d.video_frame.convertTo(VideoFormat::Format_YUV420P); //for rgb
    //d.xv_image->data = (char*)d.video_frame.bits();
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
#define COPY_YV12 0
#if COPY_YV12
    CopyYV12(d.xv_image->data, src, d.xv_image->pitches[0],d.xv_image->pitches[1], src_linesize, d.xv_image->height);
#else
    CopyPlane(d.xv_image->data + d.xv_image->offsets[0], src[0], d.xv_image->pitches[0], src_linesize[0], d.xv_image->height);
    CopyPlane(d.xv_image->data + d.xv_image->offsets[2], src[1], d.xv_image->pitches[2], src_linesize[1], d.xv_image->height/2);
    CopyPlane(d.xv_image->data + d.xv_image->offsets[1], src[2], d.xv_image->pitches[1], src_linesize[2], d.xv_image->height/2);
#endif
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
    XFillRectangle(d.display, winId(), d.gc, 0, 0, width(), height());
    //FIXME: xv should always draw the background. so shall we only paint the border rectangles, but not the whole widget
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

bool XVRenderer::onChangingBrightness(qreal b)
{
    DPTR_D(XVRenderer);
    int nb_attributes;
    XvAttribute *attributes = XvQueryPortAttributes(d.display, d.xv_port, &nb_attributes);
    if (!attributes) {
        qWarning("XvQueryPortAttributes error");
        return false;
    }
    return d.XvSetPortAttributeIfExists(attributes, nb_attributes, "XV_BRIGHTNESS", b*100);
}

bool XVRenderer::onChangingContrast(qreal c)
{
    DPTR_D(XVRenderer);
    int nb_attributes;
    XvAttribute *attributes = XvQueryPortAttributes(d.display, d.xv_port, &nb_attributes);
    if (!attributes) {
        qWarning("XvQueryPortAttributes error");
        return false;
    }
    return d.XvSetPortAttributeIfExists(attributes, nb_attributes, "XV_CONTRAST",c*100);
}

bool XVRenderer::onChangingHue(qreal h)
{
    DPTR_D(XVRenderer);
    int nb_attributes;
    XvAttribute *attributes = XvQueryPortAttributes(d.display, d.xv_port, &nb_attributes);
    if (!attributes) {
        qWarning("XvQueryPortAttributes error");
        return false;
    }
    return d.XvSetPortAttributeIfExists(attributes, nb_attributes, "XV_HUE", h*100);
}

bool XVRenderer::onChangingSaturation(qreal s)
{
    DPTR_D(XVRenderer);
    int nb_attributes;
    XvAttribute *attributes = XvQueryPortAttributes(d.display, d.xv_port, &nb_attributes);
    if (!attributes) {
        qWarning("XvQueryPortAttributes error");
        return false;
    }
    return d.XvSetPortAttributeIfExists(attributes, nb_attributes, "XV_SATURATION", s*100);
}

} //namespace QtAV
