/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

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

#include "SurfaceInteropD3D11.h"
#include "directx/D3D11VP.h"
#define DX_LOG_COMPONENT "D3D11EGL Interop"
#include "utils/DirectXHelper.h"

#include "opengl/OpenGLHelper.h"
#if defined(QT_OPENGL_ES_2_ANGLE_STATIC) || defined(Q_OS_WINRT)
#ifndef CAPI_LINK_EGL
#define CAPI_LINK_EGL
#endif //CAPI_LINK_EGL
#else
#define EGL_CAPI_NS
#endif //QT_OPENGL_ES_2_ANGLE_STATIC
#include "capi/egl_api.h"
#include <EGL/eglext.h> //include after egl_capi.h to match types

namespace QtAV {
namespace d3d11 {

class EGL {
public:
    EGL() : dpy(EGL_NO_DISPLAY), surface(EGL_NO_SURFACE) {}
    EGLDisplay dpy;
    EGLSurface surface;
};

class EGLInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    EGLInteropResource()
        : egl(new EGL())
        , vp(0)
        , boundTex(0)
    {}
    ~EGLInteropResource();
    VideoFormat::PixelFormat format(DXGI_FORMAT) const Q_DECL_OVERRIDE { return VideoFormat::Format_RGB32;}
    bool map(ComPtr<ID3D11Texture2D> surface, int index, GLuint tex, int w, int h, int) Q_DECL_OVERRIDE;

private:
    void releaseEGL();
    bool ensureSurface(int w, int h);

    EGL* egl;
    dx::D3D11VP *vp;
    ComPtr<ID3D11Texture2D> d3dtex;
    GLuint boundTex;
};

InteropResource* CreateInteropEGL() { return new EGLInteropResource();}

EGLInteropResource::~EGLInteropResource()
{
    if (vp) {
        delete vp;
        vp = 0;
    }
    releaseEGL();
    if (egl) {
        delete egl;
        egl = 0;
    }
}

void EGLInteropResource::releaseEGL() {
    if (egl->surface != EGL_NO_SURFACE) {
        eglReleaseTexImage(egl->dpy, egl->surface, EGL_BACK_BUFFER);
        eglDestroySurface(egl->dpy, egl->surface);
        egl->surface = EGL_NO_SURFACE;
    }
}

bool EGLInteropResource::ensureSurface(int w, int h) {
    if (egl->surface && width == w && height == h)
        return true;
    qDebug("update egl and dx");
    egl->dpy = eglGetCurrentDisplay();
    if (!egl->dpy) {
        qWarning("Failed to get current EGL display");
        return false;
    }
    qDebug("EGL version: %s, client api: %s", eglQueryString(egl->dpy, EGL_VERSION), eglQueryString(egl->dpy, EGL_CLIENT_APIS));
    // TODO: check runtime egl>=1.4 for eglGetCurrentContext()
    // check extensions
    QList<QByteArray> extensions = QByteArray(eglQueryString(egl->dpy, EGL_EXTENSIONS)).split(' ');
    // TODO: strstr is enough
    const bool kEGL_ANGLE_d3d_share_handle_client_buffer = extensions.contains("EGL_ANGLE_d3d_share_handle_client_buffer");
    if (!kEGL_ANGLE_d3d_share_handle_client_buffer) {
        qWarning("EGL extension 'ANGLE_d3d_share_handle_client_buffer' is required!");
        return false;
    }
    //GLint has_alpha = 1; //QOpenGLContext::currentContext()->format().hasAlpha()
    EGLint cfg_attribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, //
        EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_NONE
    };
    EGLint nb_cfgs;
    EGLConfig egl_cfg;
    if (!eglChooseConfig(egl->dpy, cfg_attribs, &egl_cfg, 1, &nb_cfgs)) {
        qWarning("Failed to create EGL configuration");
        return false;
    }
    // TODO: check alpha?
    CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_B8G8R8A8_UNORM, w, h, 1, 1);
    // why crash if only set D3D11_BIND_RENDER_TARGET?
    desc.BindFlags |= D3D11_BIND_RENDER_TARGET; // also required by VideoProcessorOutputView https://msdn.microsoft.com/en-us/library/windows/desktop/hh447791(v=vs.85).aspx
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    DX_ENSURE(d3ddev->CreateTexture2D(&desc, NULL, &d3dtex), false);
    ComPtr<IDXGIResource> resource;
    DX_ENSURE(d3dtex.As(&resource), false);
    HANDLE share_handle = NULL;
    DX_ENSURE(resource->GetSharedHandle(&share_handle), false);

    if (!vp)
        vp = new dx::D3D11VP(d3ddev);
    vp->setOutput(d3dtex.Get());

    releaseEGL();
    EGLint attribs[] = {
        EGL_WIDTH, w,
        EGL_HEIGHT, h,
        EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
        EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
        EGL_NONE
    };
    // egl surface size must match d3d texture's
    EGL_ENSURE((egl->surface = eglCreatePbufferFromClientBuffer(egl->dpy, EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE, share_handle, egl_cfg, attribs)), false);
    qDebug("pbuffer surface from client buffer: %p", egl->surface);

    width = w;
    height = h;
    return true;
}

bool EGLInteropResource::map(ComPtr<ID3D11Texture2D> surface, int index, GLuint tex, int w, int h, int)
{
    if (!ensureSurface(w, h)) {
        releaseEGL();
        return false;
    }
    vp->setSourceRect(QRect(0, 0, w, h));
    if (!vp->process(surface.Get(), index))
        return false;
    if (boundTex == tex)
        return true;
    DYGL(glBindTexture(GL_TEXTURE_2D, tex));
    if (boundTex)
        EGL_WARN(eglReleaseTexImage(egl->dpy, egl->surface, EGL_BACK_BUFFER));
    EGL_WARN(eglBindTexImage(egl->dpy, egl->surface, EGL_BACK_BUFFER));
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
    boundTex = tex;
    return true;
}
} //namespace d3d11
} //namespace QtAV
