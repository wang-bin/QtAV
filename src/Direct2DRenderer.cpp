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
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_PaintOnScreen, true);
}

Direct2DRenderer::~Direct2DRenderer()
{
}

QPaintEngine* Direct2DRenderer::paintEngine() const
{
    return 0; //use native engine
}

void Direct2DRenderer::convertData(const QByteArray &data)
{
    DPTR_D(Direct2DRenderer);
    if (!d.prepareBitmap(d.src_width, d.src_height))
        return;
    HRESULT hr = S_OK;
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    //TODO: if CopyFromMemory() is deep copy, mutex can be avoided
        /*if lock is required, do not use locker in if() scope, it will unlock outside the scope*/
        //d.img_mutex.lock();//TODO: d2d often crash, should we always lock? How about other renderer?
    hr = d.bitmap->CopyFromMemory(NULL //&D2D1::RectU(0, 0, image.width(), image.height()) /*&dstRect, NULL?*/,
                                  , data.constData() //data.constData() //msdn: const void*
                                  , d.src_width*4*sizeof(char));
    if (hr != S_OK) {
        qWarning("Failed to copy from memory to bitmap (%ld)", hr);
        //forgot unlock before, so use locker for easy
        return;
    }
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

void Direct2DRenderer::drawFrame()
{
    DPTR_D(Direct2DRenderer);
    D2D1_RECT_F out_rect = {
        (FLOAT)d.out_rect.left(),
        (FLOAT)d.out_rect.top(),
        (FLOAT)d.out_rect.right(),
        (FLOAT)d.out_rect.bottom()
    };
    //d.render_target->SetTransform
    d.render_target->DrawBitmap(d.bitmap
                                , &out_rect
                                , 1 //opacity
                                , D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
                                , NULL);//&D2D1::RectF(0, 0, d.src_width, d.src_height));
}

void Direct2DRenderer::drawSubtitle()
{
}

void Direct2DRenderer::drawOSD()
{
}

void Direct2DRenderer::drawCustom()
{
}

void Direct2DRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(Direct2DRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    if (!d.render_target) {
        qWarning("No render target!!!");
        return;
    }
    HRESULT hr = S_OK;
    //begin paint
    //http://www.daimakuai.net/?page_id=1574
    d.render_target->BeginDraw();
    //The first bitmap size is 0x0, we should only draw the background
    //we access d.data which will be modified in AVThread, so must be protected
    if ((d.update_background && d.out_rect != rect())|| d.data.isEmpty()) {
        d.update_background = false;
        drawBackground();
    }
    //d.data is always empty because we did not assign a vaule in convertData?
    if (!d.data.isEmpty()) {
        //return; //why the background is white if return? the below code draw an empty bitmap?
    }
    drawFrame();
    //drawXXX only implement the painting, no other logic
    if (d.draw_osd)
        drawOSD();
    if (d.draw_subtitle)
        drawSubtitle();
    if (d.draw_custom)
        drawCustom();
    hr = d.render_target->EndDraw(NULL, NULL);
    if (hr == D2DERR_RECREATE_TARGET) {
        qDebug("D2DERR_RECREATE_TARGET");
        hr = S_OK;
        d.destroyDeviceResource();
        d.createDeviceResource(); //?
    }
    //end paint
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
    d.update_background = true;
    d.destroyDeviceResource();
    d.createDeviceResource();
}

bool Direct2DRenderer::write()
{
    update();
    return true;
}

} //namespace QtAV
