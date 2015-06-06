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
#include "SurfaceInteropDXVA.h"

namespace QtAV {
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
    return NULL;
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
    : d3ddev(d3device)
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
        // TODO: can not display if destroyed. eglCreatePbufferSurface always return the same address even if attributes changed. because of the same context?
        //eglReleaseTexImage(egl_dpy, egl_surface, EGL_BACK_BUFFER);
        //eglDestroySurface(egl_dpy, egl_surface);
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
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
    return true;
}
#endif //QTAV_HAVE(DXVA_EGL)

} //namespace dxva
} //namespace QtAV
