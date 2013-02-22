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
#include "private/ImageRenderer_p.h"
#include <QtGui/QPainter>
#include <QtGui/QPaintEngine>
#include <QResizeEvent>

#include <d2d1.h>

//http://msdn.microsoft.com/zh-cn/library/dd317121(v=vs.85).aspx

namespace QtAV {

template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease)
{
    if (*ppInterfaceToRelease != NULL){
        (*ppInterfaceToRelease)->Release();
        (*ppInterfaceToRelease) = NULL;
    }
}

class Direct2DRendererPrivate : public ImageRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(Direct2DRenderer)

    Direct2DRendererPrivate():
        use_qpainter(false)
      , m_pD2DFactory(0)
      , m_pRenderTarget(0)
    {
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
        if (FAILED(hr)) {
            qWarning("Create d2d factory failed");
        }
        pixel_format = D2D1::PixelFormat(
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    D2D1_ALPHA_MODE_IGNORE
                    );
        bitmap_properties = D2D1::BitmapProperties(pixel_format);
    }
    ~Direct2DRendererPrivate() {
        SafeRelease(&m_pD2DFactory);
        SafeRelease(&m_pRenderTarget);
    }
    bool createDeviceResource() {
        DPTR_P(Direct2DRenderer);
        SafeRelease(&m_pRenderTarget); //force create a new one
        //
        //  This method creates resources which are bound to a particular
        //  Direct3D device. It's all centralized here, in case the resources
        //  need to be recreated in case of Direct3D device loss (eg. display
        //  change, remoting, removal of video card, etc).
        //
        //TODO: move to prepare(), or private. how to call less times
        HRESULT hr = S_OK;
        if (!m_pRenderTarget) {
            D2D1_SIZE_U size = D2D1::SizeU(p.width(), p.height());//d.width, d.height?
            // Create a Direct2D render target.
            hr = m_pD2DFactory->CreateHwndRenderTarget(
                        D2D1::RenderTargetProperties(),
                        D2D1::HwndRenderTargetProperties(p.winId(), size),
                        &m_pRenderTarget
                        );
            if (FAILED(hr)) {
                qWarning("CreateHwndRenderTarget() failed: %d", GetLastError());
            }
        }
        return hr == S_OK;
    }

    bool use_qpainter; //TODO: move to base class
    ID2D1Factory *m_pD2DFactory;
    ID2D1HwndRenderTarget *m_pRenderTarget;
    D2D1_PIXEL_FORMAT pixel_format;
    D2D1_BITMAP_PROPERTIES bitmap_properties;
};

Direct2DRenderer::Direct2DRenderer(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f),ImageRenderer(*new Direct2DRendererPrivate())
{
    DPTR_INIT_PRIVATE(Direct2DRenderer);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
}

Direct2DRenderer::~Direct2DRenderer()
{
}

bool Direct2DRenderer::write()
{
    update();
    return true;
}

QPaintEngine* Direct2DRenderer::paintEngine() const
{
    if (d_func().use_qpainter) {
        return QWidget::paintEngine();
    } else {
        return 0; //use native engine
    }
}

void Direct2DRenderer::useQPainter(bool qp)
{
    DPTR_D(Direct2DRenderer);
    d.use_qpainter = qp;
    setAttribute(Qt::WA_PaintOnScreen, !d.use_qpainter);
}

bool Direct2DRenderer::useQPainter() const
{
    DPTR_D(const Direct2DRenderer);
	return d.use_qpainter;
}

void Direct2DRenderer::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) { //auto called when show
        DPTR_D(Direct2DRenderer);
        useQPainter(d.use_qpainter);
        d.createDeviceResource();
        event->accept();
    }
}

void Direct2DRenderer::resizeEvent(QResizeEvent *e)
{
    resizeVideo(e->size());

    DPTR_D(Direct2DRenderer);
    if (d.m_pRenderTarget) {
        D2D1_SIZE_U size;
        size.width = e->size().width();
        size.height = e->size().height();
        // Note: This method can fail, but it's okay to ignore the
        // error here -- it will be repeated on the next call to
        // EndDraw.
        d.m_pRenderTarget->Resize(size);
    }
    update();
}

void Direct2DRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(Direct2DRenderer);
    if (!d.scale_in_qt) {
        d.img_mutex.lock();
    }
    QImage image = d.image; //TODO: other renderer use this style
    if (image.isNull()) {
        if (d.preview.isNull()) {
            d.preview = QImage(videoSize(), QImage::Format_RGB32);
            d.preview.fill(Qt::black); //maemo 4.7.0: QImage.fill(uint)
        }
        image = d.preview;
    }
    //begin paint
    //http://www.daimakuai.net/?page_id=1574
    ID2D1Bitmap *pBitmap = 0;
    HRESULT hr = d.m_pRenderTarget->CreateBitmap(D2D1::SizeU(image.width(), image.height()), image.bits()
                                         , image.bytesPerLine()
                                         , &d.bitmap_properties
                                         , &pBitmap);
    if (SUCCEEDED(hr)) {
        d.m_pRenderTarget->BeginDraw();
        d.m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        //d.m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
        d.m_pRenderTarget->DrawBitmap(pBitmap
                                      , &D2D1::RectF(0, 0, width(), height())
                                      , 1 //opacity
                                      , D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
                                      , &D2D1::RectF(0, 0, image.width(), image.height()));
        hr = d.m_pRenderTarget->EndDraw();
        SafeRelease(&pBitmap);
        if (hr == D2DERR_RECREATE_TARGET) {
            qDebug("D2DERR_RECREATE_TARGET");
            hr = S_OK;
            SafeRelease(&d.m_pRenderTarget);
            //d.createDeviceResource(); //?
        }
    } else {
        qWarning("create bitmap failed: %d", GetLastError());
    }
    //end paint
    if (!d.scale_in_qt) {
        d.img_mutex.unlock();
    }
}

} //namespace QtAV
