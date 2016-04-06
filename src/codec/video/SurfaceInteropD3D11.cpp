#include "SurfaceInteropD3D11.h"
#include "directx/D3D11VP.h"
#include "utils/DirectXHelper.h"
#ifdef QT_OPENGL_ES_2_ANGLE_STATIC
#define CAPI_LINK_EGL
#else
#define EGL_CAPI_NS
#endif //QT_OPENGL_ES_2_ANGLE_STATIC
#include "capi/egl_api.h"
#include <EGL/eglext.h> //include after egl_capi.h to match types

namespace QtAV {
namespace d3d11 {

InteropResource::InteropResource(ComPtr<ID3D11Device> dev)
    : d3ddev(dev)
{
}


void SurfaceInterop::setSurface(ComPtr<ID3D11Texture2D> surface, int index, int frame_w, int frame_h)
{
    m_surface = surface;
    m_index = index;
    frame_width = frame_w;
    frame_height = frame_h;
}

void* SurfaceInterop::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (!handle)
        return NULL;

    if (!m_surface)
        return 0;
    if (type == GLTextureSurface) {
        if (!fmt.isRGB())
            return NULL;
        if (m_resource->map(m_surface, m_index, *((GLuint*)handle), frame_width, frame_height, plane))
            return handle;
    } else if (type == HostMemorySurface) {
        return mapToHost(fmt, handle, plane);
    }
    return NULL;
}

void SurfaceInterop::unmap(void *handle)
{
    m_resource->unmap(*((GLuint*)handle));
}

void* SurfaceInterop::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    return NULL;
}

class EGL {
public:
    EGL() : dpy(EGL_NO_DISPLAY), surface(EGL_NO_SURFACE) {}
    EGLDisplay dpy;
    EGLSurface surface;
};

EGLInteropResource::EGLInteropResource(ComPtr<ID3D11Device> dev)
    : InteropResource(dev)
    , egl(new EGL())
    , vp(new dx::D3D11VP(dev))
{
}

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
    DX_ENSURE(d3ddev->CreateTexture2D(&desc, NULL, d3dtex.ReleaseAndGetAddressOf()), false);
    ComPtr<IDXGIResource> resource;
    DX_ENSURE(d3dtex.As(&resource), false);
    HANDLE share_handle = NULL;
    DX_ENSURE(resource->GetSharedHandle(&share_handle), false);
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
    DYGL(glBindTexture(GL_TEXTURE_2D, tex));
    eglBindTexImage(egl->dpy, egl->surface, EGL_BACK_BUFFER);
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
    return true;
}
} //namespace d3d11
} //namespace QtAV
