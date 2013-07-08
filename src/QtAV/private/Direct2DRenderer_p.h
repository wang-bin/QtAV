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

#ifndef QTAV_DIRECT2DRENDERER_P_H
#define QTAV_DIRECT2DRENDERER_P_H

#include "private/VideoRenderer_p.h"
#include <QtCore/QLibrary>

//#define CINTERFACE //http://rxlib.ru/faqs/faqc_en/15596.html
//#include <windows.h>
#include <d2d1.h>

//TODO: why we can link d2d app without it's lib?
//TODO: only for mingw. why undef?
//#include <initguid.h>
//#undef GUID_EXT
//#define GUID_EXT
//vlc redefine this
//DEFINE_GUID(IID_ID2D1Factory, 0x6152247, 0x6f50, 0x465a, 0x92, 0x45, 0x11, 0x8b, 0xfd, 0x3b, 0x60, 0x7);


//steps: http://msdn.microsoft.com/zh-cn/library/dd317121(v=vs.85).aspx
//performance: http://msdn.microsoft.com/en-us/library/windows/desktop/dd372260(v=vs.85).aspx
//vlc is helpful
//layer(opacity): http://www.cnblogs.com/graphics/archive/2013/04/15/2781969.html
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
      , interpolation(D2D1_BITMAP_INTERPOLATION_MODE_LINEAR)
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
        /*
         * d2d is accessed by AVThread and GUI thread, so we use D2D1_FACTORY_TYPE_MULTI_THREADED
         * and let d2d to deal with the thread safe problems. otherwise, if we use
         * D2D1_FACTORY_TYPE_SINGLE_THREADED, we must use lock when copying ID2D1Bitmap and calling EndDraw.
         */
        /// http://msdn.microsoft.com/en-us/library/windows/desktop/dd368104%28v=vs.85%29.aspx
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED
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
        pixel_format.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;//D2D1_ALPHA_MODE_IGNORE;
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
            //TODO: what do these mean?
            D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS //D2D1_PRESENT_OPTIONS_IMMEDIATELY /* this might need fiddling */
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
    void recreateDeviceResource() {
        qDebug("D2DERR_RECREATE_TARGET");
        QMutexLocker locker(&img_mutex);
        Q_UNUSED(locker);
        update_background = true;
        destroyDeviceResource();
        createDeviceResource();
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
    //it seems that only D2D1_BITMAP_INTERPOLATION_MODE(used in DrawBitmap) matters when drawing an image
    void setupQuality() {
        switch (quality) {
        case VideoRenderer::QualityFastest:
            interpolation = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
            render_target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
            render_target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
            break;
        case VideoRenderer::QualityBest:
            interpolation = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
            render_target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            render_target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            break;
        default:
            interpolation = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
            render_target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            render_target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
            break;
        }
    }

    ID2D1Factory *d2d_factory;
    ID2D1HwndRenderTarget *render_target;
    D2D1_PIXEL_FORMAT pixel_format;
    D2D1_BITMAP_PROPERTIES bitmap_properties;
    ID2D1Bitmap *bitmap;
    int bitmap_width, bitmap_height; //can not use src_width, src height because bitmap not update when they changes
    D2D1_BITMAP_INTERPOLATION_MODE interpolation;
    QLibrary dll;
};

} //namespace QtAV
#endif // QTAV_DIRECT2DRENDERER_P_H
