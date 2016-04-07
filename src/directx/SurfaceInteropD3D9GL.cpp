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

#include "SurfaceInteropD3D9.h"
#define DX_LOG_COMPONENT "D3D9GL Interop"
#include "utils/DirectXHelper.h"
#include "opengl/OpenGLHelper.h"

//dynamic gl or desktop gl
#define WGL_ENSURE(x, ...) \
    do { \
        if (!(x)) { \
            qWarning() << "WGL error " << __FILE__ << "@" << __LINE__ << " " << #x << ": " << qt_error_string(GetLastError()); \
            return __VA_ARGS__; \
        } \
    } while(0)
#define WGL_WARN(x, ...) \
    do { \
        if (!(x)) { \
    qWarning() << "WGL error " << __FILE__ << "@" << __LINE__ << " " << #x << ": " << qt_error_string(GetLastError()); \
        } \
    } while(0)

//#include <GL/wglext.h> //not found in vs2013

namespace QtAV {
namespace d3d9 {
struct WGL;
class GLInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    GLInteropResource(IDirect3DDevice9 * d3device);
    ~GLInteropResource();
    bool map(IDirect3DSurface9 *surface, GLuint tex, int frame_w, int frame_h, int) Q_DECL_OVERRIDE;
    bool unmap(GLuint tex) Q_DECL_OVERRIDE;
private:
    bool ensureWGL();
    bool ensureResource(int w, int h, GLuint tex);

    HANDLE interop_dev;
    HANDLE interop_obj;
    WGL *wgl;
};

InteropResource* CreateInteropGL(IDirect3DDevice9 *dev)
{
    return new GLInteropResource(dev);
}

#define WGL_ACCESS_READ_ONLY_NV           0x00000000
#define WGL_ACCESS_READ_WRITE_NV          0x00000001
#define WGL_ACCESS_WRITE_DISCARD_NV       0x00000002
typedef BOOL (WINAPI * PFNWGLDXSETRESOURCESHAREHANDLENVPROC) (void *dxObject, HANDLE shareHandle);
typedef HANDLE (WINAPI * PFNWGLDXOPENDEVICENVPROC) (void *dxDevice);
typedef BOOL (WINAPI * PFNWGLDXCLOSEDEVICENVPROC) (HANDLE hDevice);
typedef HANDLE (WINAPI * PFNWGLDXREGISTEROBJECTNVPROC) (HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access);
typedef BOOL (WINAPI * PFNWGLDXUNREGISTEROBJECTNVPROC) (HANDLE hDevice, HANDLE hObject);
typedef BOOL (WINAPI * PFNWGLDXOBJECTACCESSNVPROC) (HANDLE hObject, GLenum access);
typedef BOOL (WINAPI * PFNWGLDXLOCKOBJECTSNVPROC) (HANDLE hDevice, GLint count, HANDLE *hObjects);
typedef BOOL (WINAPI * PFNWGLDXUNLOCKOBJECTSNVPROC) (HANDLE hDevice, GLint count, HANDLE *hObjects);

//https://www.opengl.org/registry/specs/NV/DX_interop.txt
struct WGL {
    PFNWGLDXSETRESOURCESHAREHANDLENVPROC DXSetResourceShareHandleNV;
    PFNWGLDXOPENDEVICENVPROC DXOpenDeviceNV;
    PFNWGLDXCLOSEDEVICENVPROC DXCloseDeviceNV;
    PFNWGLDXREGISTEROBJECTNVPROC DXRegisterObjectNV;
    PFNWGLDXUNREGISTEROBJECTNVPROC DXUnregisterObjectNV;
    PFNWGLDXOBJECTACCESSNVPROC DXObjectAccessNV;
    PFNWGLDXLOCKOBJECTSNVPROC DXLockObjectsNV;
    PFNWGLDXUNLOCKOBJECTSNVPROC DXUnlockObjectsNV;
};

GLInteropResource::GLInteropResource(IDirect3DDevice9 *d3device)
    : InteropResource(d3device)
    , interop_dev(NULL)
    , interop_obj(NULL)
    , wgl(0)
{
}

GLInteropResource::~GLInteropResource()
{
    // FIXME: why unregister/close interop obj/dev here will crash(tested on intel driver)? must be in current opengl context?
    if (wgl) {
        delete wgl;
        wgl = NULL;
    }
}

bool GLInteropResource::map(IDirect3DSurface9 *surface, GLuint tex, int w, int h, int)
{
    if (!ensureResource(w, h, tex)) {
        releaseDX();
        return false;
    }
    // open/close and register/unregster in every map/unmap to ensure called in current context and avoid crash (tested on intel driver)
    // interop operations begin
    WGL_ENSURE((interop_dev = wgl->DXOpenDeviceNV(d3ddev)) != NULL, false);
    // call in ensureResource or in map?
    WGL_ENSURE((interop_obj = wgl->DXRegisterObjectNV(interop_dev, dx_surface, tex, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV)) != NULL, false);
    // prepare dx resources for gl
    const RECT src = { 0, 0, w, h};
    DX_ENSURE_OK(d3ddev->StretchRect(surface, &src, dx_surface, NULL, D3DTEXF_NONE), false);
    // lock dx resources
    WGL_ENSURE(wgl->DXLockObjectsNV(interop_dev, 1, &interop_obj), false);
    WGL_ENSURE(wgl->DXObjectAccessNV(interop_obj, WGL_ACCESS_READ_ONLY_NV), false);
    DYGL(glBindTexture(GL_TEXTURE_2D, tex));
    return true;
}

bool GLInteropResource::unmap(GLuint tex)
{
    Q_UNUSED(tex);
    if (!interop_obj || !interop_dev)
        return false;
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
    WGL_ENSURE(wgl->DXUnlockObjectsNV(interop_dev, 1, &interop_obj), false);
    WGL_WARN(wgl->DXUnregisterObjectNV(interop_dev, interop_obj));
    // interop operations end
    WGL_WARN(wgl->DXCloseDeviceNV(interop_dev));
    interop_obj = NULL;
    interop_dev = NULL;
    return true;
}

// IDirect3DDevice9 can not be used on WDDM OSes(>=vista)
bool GLInteropResource::ensureWGL()
{
    if (wgl)
        return true;
    static const char* ext[] = {
        "WGL_NV_DX_interop2",
        "WGL_NV_DX_interop",
        NULL,
    };
    if (!OpenGLHelper::hasExtension(ext)) { // TODO: use wgl getprocaddress function (for qt4)
        qWarning("WGL_NV_DX_interop is required");
    }
    wgl = new WGL();
    memset(wgl, 0, sizeof(*wgl));
    const QOpenGLContext *ctx = QOpenGLContext::currentContext(); //const for qt4
    // QGLContext::getProcAddress(const QByteArray&), QOpenGLContext::getProcAddress(const QString&). So to work with QT_NO_CAST_FROM_ASCII we need a wrapper
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#define QB(x) x
#else
#define QB(x) QString::fromLatin1(x)
#endif
    wgl->DXSetResourceShareHandleNV = (PFNWGLDXSETRESOURCESHAREHANDLENVPROC)ctx->getProcAddress(QB("wglDXSetResourceShareHandleNV"));
    wgl->DXOpenDeviceNV = (PFNWGLDXOPENDEVICENVPROC)ctx->getProcAddress(QB("wglDXOpenDeviceNV"));
    wgl->DXCloseDeviceNV = (PFNWGLDXCLOSEDEVICENVPROC)ctx->getProcAddress(QB("wglDXCloseDeviceNV"));
    wgl->DXRegisterObjectNV = (PFNWGLDXREGISTEROBJECTNVPROC)ctx->getProcAddress(QB("wglDXRegisterObjectNV"));
    wgl->DXUnregisterObjectNV = (PFNWGLDXUNREGISTEROBJECTNVPROC)ctx->getProcAddress(QB("wglDXUnregisterObjectNV"));
    wgl->DXObjectAccessNV = (PFNWGLDXOBJECTACCESSNVPROC)ctx->getProcAddress(QB("wglDXObjectAccessNV"));
    wgl->DXLockObjectsNV = (PFNWGLDXLOCKOBJECTSNVPROC)ctx->getProcAddress(QB("wglDXLockObjectsNV"));
    wgl->DXUnlockObjectsNV = (PFNWGLDXUNLOCKOBJECTSNVPROC)ctx->getProcAddress(QB("wglDXUnlockObjectsNV"));
#undef QB
    Q_ASSERT(wgl->DXRegisterObjectNV);
    return true;
}

bool GLInteropResource::ensureResource(int w, int h, GLuint tex)
{
    Q_UNUSED(tex);
    if (!ensureWGL())
        return false;
    if (dx_surface && width == w && height == h)
        return true;
    releaseDX();
    HANDLE share_handle = NULL;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    const bool has_alpha = QOpenGLContext::currentContext()->format().hasAlpha();
#else
    const bool has_alpha = QOpenGLContext::currentContext()->format().alpha();
#endif
    // _A8 for a yuv plane
    DX_ENSURE_OK(d3ddev->CreateTexture(w, h, 1,
                                        D3DUSAGE_RENDERTARGET,
                                        has_alpha ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8,
                                        D3DPOOL_DEFAULT,
                                        &dx_texture,
                                        &share_handle) , false);
    DX_ENSURE_OK(dx_texture->GetSurfaceLevel(0, &dx_surface), false);
    // required by d3d9 not d3d10&11: https://www.opengl.org/registry/specs/NV/DX_interop2.txt
    WGL_WARN(wgl->DXSetResourceShareHandleNV(dx_surface, share_handle));
    width = w;
    height = h;
    return true;
}
}
}
