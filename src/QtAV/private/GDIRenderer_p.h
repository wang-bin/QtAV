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

#ifndef QTAV_GDIRENDERER_P_H
#define QTAV_GDIRENDERER_P_H

#include "private/VideoRenderer_p.h"
#include <windows.h> //GetDC()
#include <gdiplus.h>

using namespace Gdiplus;
namespace QtAV {

class GDIRendererPrivate : public VideoRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(GDIRenderer)

    GDIRendererPrivate():
        support_bitblt(true)
      , gdiplus_token(0)
      , device_context(0)
    {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplus_token, &gdiplusStartupInput, NULL);
    }
    ~GDIRendererPrivate() {
        if (device_context) {
            DPTR_P(GDIRenderer);
            ReleaseDC((HWND)p.winId(), device_context); /*Q5: must cast WID to HWND*/
            DeleteDC(off_dc);
            device_context = 0;
        }
        GdiplusShutdown(gdiplus_token);
    }
    void prepare() {
        DPTR_P(GDIRenderer);
        update_background = true;
        device_context = GetDC((HWND)p.winId()); /*Q5: must cast WID to HWND*/
        //TODO: check bitblt support
        int ret = GetDeviceCaps(device_context, RC_BITBLT);
        qDebug("bitblt=%d", ret);
        //TODO: wingapi? vlc
#if 0
        BITMAPINFOHEADER bih;
        bih.biSize          = sizeof(BITMAPINFOHEADER);
        bih.biSizeImage     = 0;
        bih.biPlanes        = 1;
        bih.biCompression   = BI_RGB; //vlc: 16bpp=>BI_RGB, 15bpp=>BI_BITFIELDS
        bih.biBitCount      = 32;
        bih.biWidth         = src_width;
        bih.biHeight        = src_height;
        bih.biClrImportant  = 0;
        bih.biClrUsed       = 0;
        bih.biXPelsPerMeter = 0;
        bih.biYPelsPerMeter = 0;

        off_bitmap = CreateDIBSection(device_context,
                                      , (BITMAPINFO*)&bih
                                      , DIB_RGB_COLORS
                                      , &p_pic_buffer, NULL, 0);
#endif //0
        off_dc = CreateCompatibleDC(device_context);
    }

    bool support_bitblt;
    ULONG_PTR gdiplus_token;
    /*
     * GetDC(winID()): will flick
     * QPainter.paintEngine()->getDC() in paintEvent: doc says it's for internal use
     */
    HDC device_context;
    /* Our offscreen bitmap and its framebuffer */
    HDC        off_dc;
    HBITMAP    off_bitmap;

};

} //namespace QtAV
#endif // QTAV_GDIRENDERER_P_H
