/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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

#include "DirectXHelper.h"
#include "utils/Logger.h"

namespace QtAV {
namespace DXHelper {

const char* vendorName(unsigned id)
{
    static const struct {
        unsigned id;
        char     name[32];
    } vendors [] = {
        { 0x1002, "ATI" },
        { 0x10DE, "NVIDIA" },
        { 0x1106, "VIA" },
        { 0x8086, "Intel" },
        { 0x5333, "S3 Graphics" },
        { 0x4D4F4351, "Qualcomm" },
        { 0, "" }
    };
    const char *vendor = "Unknown";
    for (int i = 0; vendors[i].id != 0; i++) {
        if (vendors[i].id == id) {
            vendor = vendors[i].name;
            break;
        }
    }
    return vendor;
}

#ifndef Q_OS_WINRT
static void InitParameters(D3DPRESENT_PARAMETERS* d3dpp)
{
    ZeroMemory(d3dpp, sizeof(*d3dpp));
    // use mozilla's parameters
    d3dpp->Flags                  = D3DPRESENTFLAG_VIDEO;
    d3dpp->Windowed               = TRUE;
    d3dpp->hDeviceWindow          = ::GetShellWindow(); //NULL;
    d3dpp->SwapEffect             = D3DSWAPEFFECT_DISCARD;
    //d3dpp->MultiSampleType        = D3DMULTISAMPLE_NONE;
    //d3dpp->PresentationInterval   = D3DPRESENT_INTERVAL_DEFAULT;
    d3dpp->BackBufferCount        = 1; //0;                  /* FIXME what to put here */
    d3dpp->BackBufferFormat       = D3DFMT_UNKNOWN; //D3DFMT_X8R8G8B8;    /* FIXME what to put here */
    d3dpp->BackBufferWidth        = 1; //0;
    d3dpp->BackBufferHeight       = 1; //0;
    //d3dpp->EnableAutoDepthStencil = FALSE;
}

IDirect3DDevice9* CreateDevice9Ex(HINSTANCE dll, IDirect3D9Ex** d3d9ex, D3DADAPTER_IDENTIFIER9 *d3dai)
{
    qDebug("creating d3d9 device ex... dll: %p", dll);
    //http://msdn.microsoft.com/en-us/library/windows/desktop/bb219676(v=vs.85).aspx
    typedef HRESULT (WINAPI *Create9ExFunc)(UINT SDKVersion, IDirect3D9Ex **ppD3D); //IDirect3D9Ex: void is ok
    Create9ExFunc Create9Ex = (Create9ExFunc)GetProcAddress(dll, "Direct3DCreate9Ex");
    if (!Create9Ex) {
        qWarning("Symbol not found: Direct3DCreate9Ex");
        return NULL;
    }
    DX_ENSURE(Create9Ex(D3D_SDK_VERSION, d3d9ex), NULL); //TODO: will D3D_SDK_VERSION be override by other headers?
    if (d3dai)
        DX_WARN((*d3d9ex)->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, d3dai));

    D3DPRESENT_PARAMETERS d3dpp;
    InitParameters(&d3dpp);
    // D3DCREATE_MULTITHREADED is required by gl interop. https://www.opengl.org/registry/specs/NV/DX_interop.txt
    // D3DCREATE_SOFTWARE_VERTEXPROCESSING in other dxva decoders. D3DCREATE_HARDWARE_VERTEXPROCESSING is required by cuda in cuD3D9CtxCreate()
    DWORD flags = D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING;
    IDirect3DDevice9Ex *d3d9dev = NULL;
    // mpv:
    /* Direct3D needs a HWND to create a device, even without using ::Present
    this HWND is used to alert Direct3D when there's a change of focus window.
    For now, use GetDesktopWindow, as it looks harmless */
    DX_ENSURE((*d3d9ex)->CreateDeviceEx(D3DADAPTER_DEFAULT,
                                         D3DDEVTYPE_HAL, GetShellWindow(),// GetDesktopWindow(), //GetShellWindow()?
                                         flags,
                                         &d3dpp,
                                         NULL,
                                         (IDirect3DDevice9Ex**)(&d3d9dev))
            , NULL);
    qDebug("IDirect3DDevice9Ex created");
    return d3d9dev;
}

IDirect3DDevice9* CreateDevice9(HINSTANCE dll, IDirect3D9** d3d9, D3DADAPTER_IDENTIFIER9 *d3dai)
{
    qDebug("creating d3d9 device...");
    typedef IDirect3D9* (WINAPI *Create9Func)(UINT SDKVersion);
    Create9Func Create9 = (Create9Func)GetProcAddress(dll, "Direct3DCreate9");
    if (!Create9) {
        qWarning("Symbol not found: Direct3DCreate9");
        return NULL;
    }
    *d3d9 = Create9(D3D_SDK_VERSION);
    if (!(*d3d9)) {
        qWarning("Direct3DCreate9 failed");
        return NULL;
    }
    if (d3dai)
        DX_WARN((*d3d9)->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, d3dai));

    D3DPRESENT_PARAMETERS d3dpp;
    InitParameters(&d3dpp);
    DWORD flags = D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_MIXED_VERTEXPROCESSING;
    IDirect3DDevice9 *d3d9dev = NULL;
    DX_ENSURE(((*d3d9)->CreateDevice(D3DADAPTER_DEFAULT,
                                   D3DDEVTYPE_HAL, GetShellWindow(),// GetDesktopWindow(), //GetShellWindow()?
                                   flags,
                                   &d3dpp, &d3d9dev))
            , NULL);
    qDebug("IDirect3DDevice9 created");
    return d3d9dev;
}
#endif //Q_OS_WINRT
} // namespace DXHelper
} //namespace QtAV
