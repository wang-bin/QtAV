/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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
#include "gl_api.h"

namespace QtAV {
typedef void *(*GetProcAddress_t)(const char *);
static GetProcAddress_t sGetProcAddress;

void* GetProcAddress_Qt(const char *name)
{
    if (!QOpenGLContext::currentContext())
        return 0;
    void* p = (void*)QOpenGLContext::currentContext()->getProcAddress(QByteArray((const char*)name));
    if (!p) {
#if defined(Q_OS_WIN) && defined(QT_OPENGL_DYNAMIC)
        HMODULE handle = (HMODULE)QOpenGLContext::openGLModuleHandle();
        if (handle)
            p = (void*)GetProcAddress(handle, name);
#endif
    }
    //fallback to QOpenGLFunctions_1_0?
    return p;
}

#ifdef QT_OPENGL_DYNAMIC
#define GETPROCADDRESS_RESOLVE
// GL_RESOLVE_ES_X_X will link to lib or use getProcAddress
#define GL_RESOLVE_ES_2_0(name) GL_RESOLVE(name)
#define GL_RESOLVE_ES_3_0(name) GL_RESOLVE(name)
#define GL_RESOLVE_ES_3_1(name) GL_RESOLVE(name)
#else
#ifdef GL_ES_VERSION_2_0
#define GL_RESOLVE_ES_2_0(name) GL_RESOLVE(name)
#define GL_RESOLVE_ES_3_0(name) GL_RESOLVE_NONE(name)
#define GL_RESOLVE_ES_3_1(name) GL_RESOLVE_NONE(name)
#elif GL_ES_VERSION_3_0
#define GL_RESOLVE_ES_2_0(name) GL_RESOLVE(name)
#define GL_RESOLVE_ES_3_0(name) GL_RESOLVE(name)
#define GL_RESOLVE_ES_3_1(name) GL_RESOLVE_NONE(name)
#elif GL_ES_VERSION_3_1
#define GL_RESOLVE_ES_2_0(name) GL_RESOLVE(name)
#define GL_RESOLVE_ES_3_0(name) GL_RESOLVE(name)
#define GL_RESOLVE_ES_3_1(name) GL_RESOLVE(name)
#else
#define GETPROCADDRESS_RESOLVE
// GL_RESOLVE_ES_X_X will link to lib or use getProcAddress
#define GL_RESOLVE_ES_2_0(name) GL_RESOLVE(name)
#define GL_RESOLVE_ES_3_0(name) GL_RESOLVE(name)
#define GL_RESOLVE_ES_3_1(name) GL_RESOLVE(name)
#endif
#endif //QT_OPENGL_DYNAMIC

#define GL_RESOLVE_NONE(name) do { name = NULL;}while(0)
#ifdef GETPROCADDRESS_RESOLVE
#define GL_RESOLVE(name) do {\
    void** fp = (void**)(&name); \
    *fp = sGetProcAddress("gl" # name); \
    if (!*fp) \
        *fp = sGetProcAddress("gl" # name "ARB"); \
} while(0)
#else
#define GL_RESOLVE(name)  do {\
    name = ::gl##name; \
} while(0)
#endif

void api::resolve()
{
    //memset(g, 0, sizeof(g));
    sGetProcAddress = GetProcAddress_Qt;
    GL_RESOLVE(GetString);
    GL_RESOLVE(GetError);
    GL_RESOLVE(ActiveTexture);
    GL_RESOLVE(GetUniformLocation);
    GL_RESOLVE(Uniform1f);
    GL_RESOLVE(Uniform2f);
    GL_RESOLVE(Uniform3f);
    GL_RESOLVE(Uniform4f);
    GL_RESOLVE(Uniform1fv);
    GL_RESOLVE(Uniform2fv);
    GL_RESOLVE(Uniform3fv);
    GL_RESOLVE(Uniform4fv);
    GL_RESOLVE(Uniform1iv);
    GL_RESOLVE(Uniform2iv);
    GL_RESOLVE(Uniform3iv);
    GL_RESOLVE(Uniform4iv);
    GL_RESOLVE(UniformMatrix2fv);
    GL_RESOLVE(UniformMatrix3fv);
    GL_RESOLVE(UniformMatrix4fv);

    GL_RESOLVE_ES_3_1(GetTexLevelParameteriv);
}

api& gl() {
    static api g;
    if (!sGetProcAddress) {
        g.resolve();
    }
    return g;
}
} //namespace QtAV
