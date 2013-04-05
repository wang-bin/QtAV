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
#include "private/VideoRenderer_p.h"
#include <QtCore/QLibrary>
#include <QResizeEvent>

//#define CINTERFACE //http://rxlib.ru/faqs/faqc_en/15596.html
//#include <windows.h>
#include <d2d1.h>

//TODO: only for mingw. why undef?
//#include <initguid.h>
//#undef GUID_EXT
//#define GUID_EXT
//vlc redefine this
//DEFINE_GUID(IID_ID2D1Factory, 0x6152247, 0x6f50, 0x465a, 0x92, 0x45, 0x11, 0x8b, 0xfd, 0x3b, 0x60, 0x7);


//steps: http://msdn.microsoft.com/zh-cn/library/dd317121(v=vs.85).aspx
//performance: http://msdn.microsoft.com/en-us/library/windows/desktop/dd372260(v=vs.85).aspx
//vlc is helpful

namespace QtAV {

template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease)
{
    if (*ppInterfaceToRelease != NULL){
        (*ppInterfaceToRelease)->Release();
        (*ppInterfaceToRelease) = NULL;
    }
}

class Direct2DRendererPrivate : public VideoRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(Direct2DRenderer)

    Direct2DRendererPrivate():
        d2d_factory(0)
      , render_target(0)
      , bitmap(0)
      , bitmap_width(0)
      , bitmap_height(0)
    {
        dll.setFileName("d2d1");
        if (!dll.load()) {
            available = false;
            qWarning("Direct2D is disabled. Failed to load 'd2d1.dll': %s", dll.errorString().toUtf8().constData());
            return;
        }
        typedef HRESULT (WINAPI *D2D1CreateFactory_t)(D2D1_FACTORY_TYPE, REFIID, const D2D1_FACTORY_OPTIONS *, void **ppIFactory);
        D2D1CreateFactory_t D2D1CreateFactory;
        D2D1CreateFactory = (D2D1CreateFactory_t)dll.resolve("D2D1CreateFactory");
        if (!D2D1CreateFactory) {
            available = false;
            qWarning("Direct2D is disabled. Failed to resolve symble 'D2D1CreateFactory': %s", dll.errorString().toUtf8().constData());
            return;
        }

        D2D1_FACTORY_OPTIONS factory_opt = { D2D1_DEBUG_LEVEL_NONE };
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED
                                       , (REFIID)IID_ID2D1Factory
                                       , &factory_opt
                                       , (void**)&d2d_factory);
        if (FAILED(hr)) {
            available = false;
            qWarning("Direct2D is disabled. Create d2d factory failed");
            return;
        }
        FLOAT dpiX, dpiY;
        d2d_factory->GetDesktopDpi(&dpiX, &dpiY);
        //gcc: extended initializer lists only available with -std=c++11 or -std=gnu++11
        //vc: http://msdn.microsoft.com/zh-cn/library/t8xe60cf(v=vs.80).aspx
        /*pixel_format = {
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_IGNORE
        };*/
        pixel_format.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        pixel_format.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        /*bitmap_properties = {
            pixel_format,
            dpiX,
            dpiY
        };*/
        bitmap_properties.pixelFormat = pixel_format;
        bitmap_properties.dpiX = dpiX;
        bitmap_properties.dpiY = dpiY;
    }
    ~Direct2DRendererPrivate() {
        destroyDeviceResource();
        SafeRelease(&d2d_factory);//vlc does not call this. why? bug?
        dll.unload();
    }
    bool createDeviceResource() {
        DPTR_P(Direct2DRenderer);
        update_background = true;
        destroyDeviceResource();
        //
        //  This method creates resources which are bound to a particular
        //  Direct3D device. It's all centralized here, in case the resources
        //  need to be recreated in case of Direct3D device loss (eg. display
        //  change, remoting, removal of video card, etc).
        //
        //TODO: move to prepare(), or private. how to call less times
        D2D1_RENDER_TARGET_PROPERTIES rtp = {
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            pixel_format,
            0,
            0,
            D2D1_RENDER_TARGET_USAGE_NONE,
            D2D1_FEATURE_LEVEL_DEFAULT
        };
        D2D1_SIZE_U size = {
            (UINT32)p.width(),
            (UINT32)p.height()
        };//d.renderer_width, d.renderer_height?
        // Create a Direct2D render target.
        D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rtp = {
            (HWND)p.winId(),
            size,
            D2D1_PRESENT_OPTIONS_IMMEDIATELY /* this might need fiddling */
        };
        HRESULT hr = d2d_factory->CreateHwndRenderTarget(&rtp//D2D1::RenderTargetProperties() //TODO: vlc set properties
                                                         , &hwnd_rtp//D2D1::HwndRenderTargetProperties(, size)
                                                         , &render_target);
        if (FAILED(hr)) {
            qWarning("Direct2D is disabled. CreateHwndRenderTarget() failed: %d", (int)GetLastError());
            render_target = 0;
            return false;
        }
        SafeRelease(&bitmap);
        prepareBitmap(src_width, src_height); //bitmap depends on render target
        return hr == S_OK;
    }
    void destroyDeviceResource() {
        SafeRelease(&render_target);
        SafeRelease(&bitmap);
    }

    //create an empty bitmap with given size. if size is equal as current and bitmap already exists, do nothing
    bool prepareBitmap(int w, int h) {
        if (w == bitmap_width && h == bitmap_height && bitmap)
            return true;
        if (!render_target) {
            qWarning("No render target, bitmap will not be created!!!");
            return false;
        }
        bitmap_width = w;
        bitmap_height = h;
        qDebug("Resize bitmap to %d x %d", w, h);
        D2D1_SIZE_U s = {(UINT32)w, (UINT32)h};
        SafeRelease(&bitmap);
        HRESULT hr = render_target->CreateBitmap(s
                                                   , NULL
                                                   , 0
                                                   , &bitmap_properties
                                                   , &bitmap);
        if (FAILED(hr)) {
            qWarning("Failed to create ID2D1Bitmap (%ld)", hr);
            SafeRelease(&bitmap);
            SafeRelease(&render_target);
            return false;
        }
        return true;
    }

    ID2D1Factory *d2d_factory;
    ID2D1HwndRenderTarget *render_target;
    D2D1_PIXEL_FORMAT pixel_format;
    D2D1_BITMAP_PROPERTIES bitmap_properties;
    ID2D1Bitmap *bitmap;
    int bitmap_width, bitmap_height; //can not use src_width, src height because bitmap not update when they changes
    QLibrary dll;
};

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

QPaintEngine* Direct2DRenderer::paintEngine() const
{
    return 0; //use native engine
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

    if ((d.update_background && d.out_rect != rect())|| d.data.isEmpty()) {
        d.update_background = false;
        D2D1_COLOR_F c = {0, 0, 0, 255};
        d.render_target->Clear(&c); //const D2D1_COlOR_F&?
//http://msdn.microsoft.com/en-us/library/windows/desktop/dd535473(v=vs.85).aspx
        //ID2D1SolidColorBrush *brush;
        //d.render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);
        //d.render_target->FillRectangle(D2D1::RectF(0, 0, width(), height()), brush);
    }
    //d.data is always empty because we did not assign a vaule in convertData?
    if (d.data.isEmpty()) {
        //return; //why the background is white if return? the below code draw an empty bitmap?
    }

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
