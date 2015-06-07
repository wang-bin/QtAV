/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/VideoFrame.h"
// no need to check qt4 because no ANGLE there
#if defined(QT_OPENGL_DYNAMIC) || defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_ES_2_ANGLE)
#define QTAV_HAVE_DXVA_EGL 1
#endif
#if QTAV_HAVE(DXVA_EGL)
# if QTAV_HAVE(GUI_PRIVATE)
#include <qpa/qplatformnativeinterface.h>
#include <QtGui/QGuiApplication>
# endif //QTAV_HAVE(GUI_PRIVATE)
#endif
#include "utils/Logger.h"
#define WGL_WGLEXT_PROTOTYPES
#include "SurfaceInteropDXVA.h"

namespace QtAV {
extern VideoFormat::PixelFormat pixelFormatFromD3D(D3DFORMAT format);

namespace dxva {

template <class T> void SafeRelease(T **ppT)
{
  if (*ppT) {
    (*ppT)->Release();
    *ppT = NULL;
  }
}

#define DX_LOG_COMPONENT "DXVA2"

#ifndef DX_LOG_COMPONENT
#define DX_LOG_COMPONENT "DirectX"
#endif //DX_LOG_COMPONENT
#define DX_ENSURE_OK(f, ...) \
    do { \
        HRESULT hr = f; \
        if (FAILED(hr)) { \
            qWarning() << DX_LOG_COMPONENT " error@" << __LINE__ << ". " #f ": " << QString("(0x%1) ").arg(hr, 0, 16) << qt_error_string(hr); \
            return __VA_ARGS__; \
        } \
    } while (0)

SurfaceInteropDXVA::~SurfaceInteropDXVA()
{
    SafeRelease(&m_surface);
}

void* SurfaceInteropDXVA::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (!handle)
        return NULL;
    if (!fmt.isRGB())
        return 0;

    if (!m_surface)
        return 0;
    if (type == GLTextureSurface) {
        if (m_resource->map(m_surface, *((GLuint*)handle), plane))
            return handle;
        return NULL;
    } else if (type == HostMemorySurface) {
        return mapToHost(fmt, handle, plane);
    }
    return NULL;
}

void SurfaceInteropDXVA::unmap(void *handle)
{
    m_resource->unmap(*((GLuint*)handle));
}

void* SurfaceInteropDXVA::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    Q_UNUSED(plane);
    class ScopedD3DLock {
        IDirect3DSurface9 *mpD3D;
    public:
        ScopedD3DLock(IDirect3DSurface9* d3d, D3DLOCKED_RECT *rect) : mpD3D(d3d) {
            if (FAILED(mpD3D->LockRect(rect, NULL, D3DLOCK_READONLY))) {
                qWarning("Failed to lock surface");
                mpD3D = 0;
            }
        }
        ~ScopedD3DLock() {
            if (mpD3D)
                mpD3D->UnlockRect();
        }
    };

    D3DLOCKED_RECT lock;
    ScopedD3DLock(m_surface, &lock);
    if (lock.Pitch == 0)
        return NULL;

    //picth >= desc.Width
    D3DSURFACE_DESC desc;
    m_surface->GetDesc(&desc);
    const VideoFormat fmt = VideoFormat(pixelFormatFromD3D(desc.Format));
    if (!fmt.isValid()) {
        qWarning("unsupported dxva pixel format: %#x", desc.Format);
        return NULL;
    }
    //YV12 need swap, not imc3?
    // imc3 U V pitch == Y pitch, but half of the U/V plane is space. we convert to yuv420p here
    // nv12 bpp(1)==1
    // 3rd plane is not used for nv12
    int pitch[3] = { lock.Pitch, 0, 0}; //compute chroma later
    quint8 *src[] = { (quint8*)lock.pBits, 0, 0}; //compute chroma later
    Q_ASSERT(src[0] && pitch[0] > 0);
    const int nb_planes = fmt.planeCount();
    const int chroma_pitch = nb_planes > 1 ? fmt.bytesPerLine(pitch[0], 1) : 0;
    const int chroma_h = fmt.chromaHeight(desc.Height);
    int h[] = { (int)desc.Height, 0, 0};
    for (int i = 1; i < nb_planes; ++i) {
        h[i] = chroma_h;
        // set chroma address and pitch if not set
        if (pitch[i] <= 0)
            pitch[i] = chroma_pitch;
        if (!src[i])
            src[i] = src[i-1] + pitch[i-1]*h[i-1];
    }
    const bool swap_uv = desc.Format ==  MAKEFOURCC('I','M','C','3');
    if (swap_uv) {
        std::swap(src[1], src[2]);
        std::swap(pitch[1], pitch[2]);
    }
    VideoFrame frame = VideoFrame(desc.Width, desc.Height, fmt); //FIXME: not surface size
    frame.setBits(src);
    frame.setBytesPerLine(pitch);
    frame = frame.to(format);

    VideoFrame *f = reinterpret_cast<VideoFrame*>(handle);
    frame.setTimestamp(f->timestamp());
    *f = frame;
    return f;
}

#if QTAV_HAVE(DXVA_EGL)
#define EGL_ENSURE(x, ...) \
    do { \
        if (!(x)) { \
            EGLint err = eglGetError(); \
            qWarning("EGL error@%d<<%s. " #x ": %#x %s", __LINE__, __FILE__, err, eglQueryString(eglGetCurrentDisplay(), err)); \
            return __VA_ARGS__; \
        } \
    } while(0)

EGLInteropResource::EGLInteropResource(IDirect3DDevice9 * d3device)
    : InteropResource()
    , d3ddev(d3device)
    , dx_texture(NULL)
    , dx_surface(NULL)
    , egl_dpy(EGL_NO_DISPLAY)
    , egl_surface(EGL_NO_SURFACE)
    , width(0)
    , height(0)
{
    d3ddev->AddRef();
}

EGLInteropResource::~EGLInteropResource()
{
    releaseResource();
    SafeRelease(&d3ddev);
}

void EGLInteropResource::releaseResource() {
    SafeRelease(&dx_surface);
    SafeRelease(&dx_texture);
    if (egl_surface != EGL_NO_SURFACE) {
        eglReleaseTexImage(egl_dpy, egl_surface, EGL_BACK_BUFFER);
        eglDestroySurface(egl_dpy, egl_surface);
        egl_surface = EGL_NO_SURFACE;
    }
}

bool EGLInteropResource::ensureSurface(int w, int h) {
    if (dx_surface && width == w && height == h)
        return true;
#if QTAV_HAVE(GUI_PRIVATE)
    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
    egl_dpy = static_cast<EGLDisplay>(nativeInterface->nativeResourceForContext("eglDisplay", QOpenGLContext::currentContext()));
    EGLConfig egl_cfg = static_cast<EGLConfig>(nativeInterface->nativeResourceForContext("eglConfig", QOpenGLContext::currentContext()));
#else
#ifdef Q_OS_WIN
#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
#ifdef _MSC_VER
#pragma message("ANGLE version in Qt<5.5 does not support eglQueryContext. You must upgrade your runtime ANGLE libraries")
#else
#warning "ANGLE version in Qt<5.5 does not support eglQueryContext. You must upgrade your runtime ANGLE libraries"
#endif //_MSC_VER
#endif
#endif //Q_OS_WIN
    // eglQueryContext() added (Feb 2015): https://github.com/google/angle/commit/8310797003c44005da4143774293ea69671b0e2a
    egl_dpy = eglGetCurrentDisplay();
    EGLint cfg_id = 0;
    EGL_ENSURE(eglQueryContext(egl_dpy, eglGetCurrentContext(), EGL_CONFIG_ID , &cfg_id) == EGL_TRUE, false);
    qDebug("egl config id: %d", cfg_id);
    EGLint nb_cfg = 0;
    EGL_ENSURE(eglGetConfigs(egl_dpy, NULL, 0, &nb_cfg) == EGL_TRUE, false);
    qDebug("eglGetConfigs number: %d", nb_cfg);
    QVector<EGLConfig> cfgs(nb_cfg); //check > 0
    EGL_ENSURE(eglGetConfigs(egl_dpy, cfgs.data(), cfgs.size(), &nb_cfg) == EGL_TRUE, false);
    EGLConfig egl_cfg = NULL;
    for (int i = 0; i < nb_cfg; ++i) {
        EGLint id = 0;
        eglGetConfigAttrib(egl_dpy, cfgs[i], EGL_CONFIG_ID, &id);
        if (id == cfg_id) {
            egl_cfg = cfgs[i];
            break;
        }
    }
#endif
    qDebug("egl display:%p config: %p", egl_dpy, egl_cfg);

    EGLint attribs[] = {
        EGL_WIDTH, w,
        EGL_HEIGHT, h,
        EGL_TEXTURE_FORMAT, QOpenGLContext::currentContext()->format().hasAlpha() ? EGL_TEXTURE_RGBA : EGL_TEXTURE_RGB,
        EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
        EGL_NONE
    };
    EGL_ENSURE((egl_surface = eglCreatePbufferSurface(egl_dpy, egl_cfg, attribs)) != EGL_NO_SURFACE, false);
    qDebug("pbuffer surface: %p", egl_surface);

    // create dx resources
    PFNEGLQUERYSURFACEPOINTERANGLEPROC eglQuerySurfacePointerANGLE = reinterpret_cast<PFNEGLQUERYSURFACEPOINTERANGLEPROC>(eglGetProcAddress("eglQuerySurfacePointerANGLE"));
    if (!eglQuerySurfacePointerANGLE) {
        qWarning("EGL_ANGLE_query_surface_pointer is not supported");
        return false;
    }
    HANDLE share_handle = NULL;
    EGL_ENSURE(eglQuerySurfacePointerANGLE(egl_dpy, egl_surface, EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE, &share_handle), false);

    SafeRelease(&dx_surface);
    SafeRelease(&dx_texture);
    // _A8 for a yuv plane
    DX_ENSURE_OK(d3ddev->CreateTexture(w, h, 1,
                                        D3DUSAGE_RENDERTARGET,
                                        QOpenGLContext::currentContext()->format().hasAlpha() ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8,
                                        D3DPOOL_DEFAULT,
                                        &dx_texture,
                                        &share_handle) , false);
    DX_ENSURE_OK(dx_texture->GetSurfaceLevel(0, &dx_surface), false);
    width = w;
    height = h;
    return true;
}

bool EGLInteropResource::map(IDirect3DSurface9* surface, GLuint tex, int)
{
    D3DSURFACE_DESC dxvaDesc;
    surface->GetDesc(&dxvaDesc);
    if (!ensureSurface(dxvaDesc.Width, dxvaDesc.Height)) {
        releaseResource();
        return false;
    }
    DYGL(glBindTexture(GL_TEXTURE_2D, tex));
    if (SUCCEEDED(d3ddev->StretchRect(surface, NULL, dx_surface, NULL, D3DTEXF_NONE)))
        eglBindTexImage(egl_dpy, egl_surface, EGL_BACK_BUFFER);
    // Flush the draw command now, so that by the time we come to draw this
      // image, we're less likely to need to wait for the draw operation to
      // complete.
      IDirect3DQuery9 *query = NULL;
      DX_ENSURE_OK(d3ddev->CreateQuery(D3DQUERYTYPE_EVENT, &query), false);
      DX_ENSURE_OK(query->Issue(D3DISSUE_END), false);
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
    return true;
}
#endif //QTAV_HAVE(DXVA_EGL)

#if QTAV_HAVE(DXVA_GL)
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
    : InteropResource()
    , d3ddev(d3device)
    , dx_texture(NULL)
    , dx_surface(NULL)
    , interop_dev(NULL)
    , interop_obj(NULL)
    , width(0)
    , height(0)
    , wgl(0)
{
    d3ddev->AddRef();
}

GLInteropResource::~GLInteropResource()
{
    if (interop_obj) {
        WGL_WARN(wgl->DXUnregisterObjectNV(interop_dev, interop_obj));
        interop_obj = NULL;
    }
    if (interop_dev) {
        WGL_WARN(wgl->DXCloseDeviceNV(interop_dev));
        interop_dev = NULL;
    }
    if (wgl) {
        delete wgl;
        wgl = NULL;
    }
    releaseResource();
    SafeRelease(&d3ddev);
}

void GLInteropResource::releaseResource() {
    SafeRelease(&dx_surface);
    SafeRelease(&dx_texture);
}

#define REGISTER_ON_MAP 1
bool GLInteropResource::map(IDirect3DSurface9 *surface, GLuint tex, int)
{
    D3DSURFACE_DESC dxvaDesc;
    surface->GetDesc(&dxvaDesc);
    if (!ensureResource(dxvaDesc.Width, dxvaDesc.Height, tex)) {
        releaseResource();
        return false;
    }
#if REGISTER_ON_MAP
    // call in ensureResource or in map?
    // TODO: WGL_ACCESS_READ_ONLY_NV?
    WGL_ENSURE((interop_obj = wgl->DXRegisterObjectNV(interop_dev, dx_surface, tex, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV)) != NULL, false);
#endif //REGISTER_ON_MAP
    DX_ENSURE_OK(d3ddev->StretchRect(surface, NULL, dx_surface, NULL, D3DTEXF_NONE), false);
    WGL_ENSURE(wgl->DXLockObjectsNV(interop_dev, 1, &interop_obj), false);
    WGL_ENSURE(wgl->DXObjectAccessNV(interop_obj, WGL_ACCESS_READ_ONLY_NV), false);
    DYGL(glBindTexture(GL_TEXTURE_2D, tex));
    return true;
}

bool GLInteropResource::unmap(GLuint tex)
{
    Q_UNUSED(tex);
    if (!interop_obj)
        return false;
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
    WGL_ENSURE(wgl->DXUnlockObjectsNV(interop_dev, 1, &interop_obj), false);
#if REGISTER_ON_MAP
    WGL_WARN(wgl->DXUnregisterObjectNV(interop_dev, interop_obj));
#endif //REGISTER_ON_MAP
    return true;
}

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
    wgl->DXSetResourceShareHandleNV = (PFNWGLDXSETRESOURCESHAREHANDLENVPROC)ctx->getProcAddress("wglDXSetResourceShareHandleNV");
    wgl->DXOpenDeviceNV = (PFNWGLDXOPENDEVICENVPROC)ctx->getProcAddress("wglDXOpenDeviceNV");
    wgl->DXCloseDeviceNV = (PFNWGLDXCLOSEDEVICENVPROC)ctx->getProcAddress("wglDXCloseDeviceNV");
    wgl->DXRegisterObjectNV = (PFNWGLDXREGISTEROBJECTNVPROC)ctx->getProcAddress("wglDXRegisterObjectNV");
    wgl->DXUnregisterObjectNV = (PFNWGLDXUNREGISTEROBJECTNVPROC)ctx->getProcAddress("wglDXUnregisterObjectNV");
    wgl->DXObjectAccessNV = (PFNWGLDXOBJECTACCESSNVPROC)ctx->getProcAddress("wglDXObjectAccessNV");
    wgl->DXLockObjectsNV = (PFNWGLDXLOCKOBJECTSNVPROC)ctx->getProcAddress("wglDXLockObjectsNV");
    wgl->DXUnlockObjectsNV = (PFNWGLDXUNLOCKOBJECTSNVPROC)ctx->getProcAddress("wglDXUnlockObjectsNV");

    Q_ASSERT(wgl->DXRegisterObjectNV);
    Q_ASSERT(wgl->DXSetResourceShareHandleNV);
    Q_ASSERT(wgl->DXLockObjectsNV);
    Q_ASSERT(wgl->DXObjectAccessNV);
    return true;
}

bool GLInteropResource::ensureResource(int w, int h, GLuint tex)
{
    Q_UNUSED(tex);
    if (!ensureWGL())
        return false;
    if (interop_obj && interop_dev && dx_surface && width == w && height == h)
        return true;
    SafeRelease(&dx_surface);
    SafeRelease(&dx_texture);
#if !REGISTER_ON_MAP
    if (interop_obj) {
        WGL_WARN(wgl->DXUnregisterObjectNV(interop_dev, interop_obj));
        interop_obj = NULL;
    }
#endif //!REGISTER_ON_MAP
    if (interop_dev) {
        wgl->DXCloseDeviceNV(interop_dev);
        interop_dev = NULL;
    }
    HANDLE share_handle = NULL;
    // _A8 for a yuv plane
    DX_ENSURE_OK(d3ddev->CreateTexture(w, h, 1,
                                        D3DUSAGE_RENDERTARGET,
                                        QOpenGLContext::currentContext()->format().hasAlpha() ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8,
                                        D3DPOOL_DEFAULT,
                                        &dx_texture,
                                        &share_handle) , false);
    DX_ENSURE_OK(dx_texture->GetSurfaceLevel(0, &dx_surface), false);
    // required by d3d9 not d3d10&11: https://www.opengl.org/registry/specs/NV/DX_interop2.txt
    WGL_WARN(wgl->DXSetResourceShareHandleNV(dx_surface, share_handle));
    WGL_ENSURE((interop_dev = wgl->DXOpenDeviceNV(d3ddev)) != NULL, false);
    // register obj here or map() both works
#if !REGISTER_ON_MAP
    // call in ensureResource or in map?
    // TODO: WGL_ACCESS_READ_ONLY_NV?
    WGL_ENSURE((interop_obj = wgl->DXRegisterObjectNV(interop_dev, dx_surface, tex, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV)) != NULL, false);
#endif //!REGISTER_ON_MAP
    width = w;
    height = h;
    return true;
}
#endif //QTAV_HAVE(DXVA_GL)
} //namespace dxva
} //namespace QtAV
