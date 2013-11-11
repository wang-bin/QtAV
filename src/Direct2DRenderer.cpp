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

#include "QtAV/Direct2DRenderer.h"
#include "private/Direct2DRenderer_p.h"
#include <QResizeEvent>

namespace QtAV {

Direct2DRenderer::Direct2DRenderer(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f),VideoRenderer(*new Direct2DRendererPrivate())
{
    DPTR_INIT_PRIVATE(Direct2DRenderer);
    d_func().widget_holder = this;
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

Direct2DRenderer::~Direct2DRenderer()
{
}

bool Direct2DRenderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(Direct2DRenderer);
    if (!d.prepareBitmap(frame.width(), frame.height()))
        return false;
    HRESULT hr = S_OK;
    //if d2d factory is D2D1_FACTORY_TYPE_SINGLE_THREADED, we need to lock
    //QMutexLocker locker(&d.img_mutex);
    //Q_UNUSED(locker);
    d.video_frame = frame;
    //TODO: if CopyFromMemory() is deep copy, mutex can be avoided
    /*if lock is required, do not use locker in if() scope, it will unlock outside the scope*/
    //TODO: d2d often crash, should we always lock? How about other renderer?
    hr = d.bitmap->CopyFromMemory(NULL //&D2D1::RectU(0, 0, image.width(), image.height()) /*&dstRect, NULL?*/,
                                  , frame.bits(0) //data.constData() //msdn: const void*
                                  , frame.bytesPerLine(0));
    if (hr != S_OK) {
        qWarning("Failed to copy from memory to bitmap (%ld)", hr);
    }
    update();
    return true;
}

QPaintEngine* Direct2DRenderer::paintEngine() const
{
    return 0; //use native engine
}

bool Direct2DRenderer::needUpdateBackground() const
{
    DPTR_D(const Direct2DRenderer);
    return (d.update_background && d.out_rect != rect()) || !d.video_frame.isValid();
}

void Direct2DRenderer::drawBackground()
{
    DPTR_D(Direct2DRenderer);
    D2D1_COLOR_F c = {0, 0, 0, 255};
    d.render_target->Clear(&c); //const D2D1_COlOR_F&?
//http://msdn.microsoft.com/en-us/library/windows/desktop/dd535473(v=vs.85).aspx
    //ID2D1SolidColorBrush *brush;
    //d.render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);
    //d.render_target->FillRectangle(D2D1::RectF(0, 0, width(), height()), brush);
}

bool Direct2DRenderer::needDrawFrame() const
{
    return true;
}

void Direct2DRenderer::drawFrame()
{
    DPTR_D(Direct2DRenderer);
    D2D1_RECT_F out_rect = {
        (FLOAT)d.out_rect.left(),
        (FLOAT)d.out_rect.top(),
        (FLOAT)d.out_rect.right(),
        (FLOAT)d.out_rect.bottom()
    };
    QRect roi = realROI();
    D2D1_RECT_F roi_d2d = {
        (FLOAT)roi.left(),
        (FLOAT)roi.top(),
        (FLOAT)roi.right(),
        (FLOAT)roi.bottom()
    };
    //d.render_target->SetTransform
    d.render_target->DrawBitmap(d.bitmap
                                , &out_rect
                                , 1 //opacity
                                , d.interpolation
                                , &roi_d2d);
}

void Direct2DRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(Direct2DRenderer);
    if (!d.render_target) {
        qWarning("No render target!!!");
        return;
    }
    //http://www.daimakuai.net/?page_id=1574
    d.render_target->BeginDraw();
    handlePaintEvent();
    HRESULT hr = S_OK;
    {
        //if d2d factory is D2D1_FACTORY_TYPE_SINGLE_THREADED, we need to lock
        //QMutexLocker locker(&d.img_mutex);
        //Q_UNUSED(locker);
        hr = d.render_target->EndDraw(NULL, NULL); //TODO: why it need lock? otherwise crash
    }
    if (hr == D2DERR_RECREATE_TARGET) {
        d.recreateDeviceResource();
    }
}

void Direct2DRenderer::resizeEvent(QResizeEvent *e)
{
    resizeRenderer(e->size());
    DPTR_D(Direct2DRenderer);
    d.update_background = true;
    if (d.render_target) {
        D2D1_SIZE_U size = {
            (UINT32)e->size().width(),
            (UINT32)e->size().height()
        };
        // Note: This method can fail, but it's okay to ignore the
        // error here -- it will be repeated on the next call to
        // EndDraw.
        d.render_target->Resize(&size); //D2D1_SIZE_U&?
    }
    update();
}

void Direct2DRenderer::showEvent(QShowEvent *)
{
    DPTR_D(Direct2DRenderer);
    d.recreateDeviceResource();
}

} //namespace QtAV
