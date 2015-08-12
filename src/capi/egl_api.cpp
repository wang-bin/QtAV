/******************************************************************************
    mkapi dynamic load code generation for capi template
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>
    https://github.com/wang-bin/mkapi
    https://github.com/wang-bin/capi

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
#define CAPI_LINKAGE EGLAPIENTRY // for functions defined in namespace egl::capi
#define EGL_CAPI_BUILD
#define DEBUG_RESOLVE
#define DEBUG_LOAD
//#define CAPI_IS_LAZY_RESOLVE 0
#ifndef CAPI_LINK_EGL
#include <QtCore/QLibrary>
#include "capi.h"
#endif //CAPI_LINK_EGL
#include "egl_api.h" //include last to avoid covering types later

namespace egl {
#ifdef CAPI_LINK_EGL
api::api(){dll=0;}
api::~api(){}
bool api::loaded() const{return true;}
#else
static const char* names[] = {
    "EGL",
#ifdef Q_OS_WIN
#ifndef QT_NO_DEBUG
    "libEGLd",
    "libEGL",
#else
    "libEGL",
    "libEGLd",
#endif //QT_NO_DEBUG
#else
    "libEGL",
#endif
    NULL
};

class EGLLib
{
    QLibrary m_lib;
public:
    explicit EGLLib() : m_lib() {}
    explicit EGLLib(const QString& fileName) : m_lib(fileName) {}
    explicit EGLLib(const QString& fileName, int verNum) : m_lib(fileName, verNum) {}
    void* resolve(const char *symbol) {
        // from qwindowseglcontext.cpp
#ifdef __MINGW32__
        QString baseNameStr = QString::fromLatin1(symbol);
        QString nameStr;
        void *proc = 0;

        // Play nice with 32-bit mingw: Try func first, then func@0, func@4,
        // func@8, func@12, ..., func@64. The def file does not provide any aliases
        // in libEGL and libGLESv2 in these builds which results in exporting
        // function names like eglInitialize@12. This cannot be fixed without
        // breaking binary compatibility. So be flexible here instead.

        int argSize = -1;
        while (!proc && argSize <= 64) {
            nameStr = baseNameStr;
            if (argSize >= 0)
                nameStr += QLatin1Char('@') + QString::number(argSize);
            argSize = argSize < 0 ? 0 : argSize + 4;
            proc = (void*)m_lib.resolve(nameStr.toLatin1().constData());
        }
        if (argSize > 0)
            fprintf(stderr, "%s=>%s\n", symbol, nameStr.toLatin1().constData());
        return proc;
#else
        return (void*)m_lib.resolve(symbol);
#endif
    }

    bool load() {return m_lib.load();}
    bool unload() {return m_lib.unload();}
    bool isLoaded() const {return m_lib.isLoaded();}

    void setFileName(const QString &fileName) {m_lib.setFileName(fileName);}
    QString fileName() const {return m_lib.fileName();}

    void setFileNameAndVersion(const QString &fileName, int verNum) {m_lib.setFileNameAndVersion(fileName, verNum);}
    QString errorString() const {return m_lib.errorString();}
};

# if CAPI_HAS_EGL_VERSION
static const int versions[] = {
    ::capi::NoVersion,
// the following line will be replaced by the content of config/egl/version if exists
    %VERSIONS%
    , ::capi::EndVersion
};
CAPI_BEGIN_DLL_VER(names, versions, EGLLib)
# else
CAPI_BEGIN_DLL(names, EGLLib)
# endif //CAPI_HAS_EGL_VERSION
// CAPI_DEFINE_RESOLVER(argc, return_type, name, argv_no_name)
// mkapi code generation BEGIN
#ifdef EGL_VERSION_1_0
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglChooseConfig, CAPI_ARG5(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglCopyBuffers, CAPI_ARG3(EGLDisplay, EGLSurface, EGLNativePixmapType))
CAPI_DEFINE_M_ENTRY(EGLContext, EGLAPIENTRY, eglCreateContext, CAPI_ARG4(EGLDisplay, EGLConfig, EGLContext, const EGLint *))
CAPI_DEFINE_M_ENTRY(EGLSurface, EGLAPIENTRY, eglCreatePbufferSurface, CAPI_ARG3(EGLDisplay, EGLConfig, const EGLint *))
CAPI_DEFINE_M_ENTRY(EGLSurface, EGLAPIENTRY, eglCreatePixmapSurface, CAPI_ARG4(EGLDisplay, EGLConfig, EGLNativePixmapType, const EGLint *))
CAPI_DEFINE_M_ENTRY(EGLSurface, EGLAPIENTRY, eglCreateWindowSurface, CAPI_ARG4(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint *))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglDestroyContext, CAPI_ARG2(EGLDisplay, EGLContext))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglDestroySurface, CAPI_ARG2(EGLDisplay, EGLSurface))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglGetConfigAttrib, CAPI_ARG4(EGLDisplay, EGLConfig, EGLint, EGLint *))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglGetConfigs, CAPI_ARG4(EGLDisplay, EGLConfig *, EGLint, EGLint *))
CAPI_DEFINE_M_ENTRY(EGLDisplay, EGLAPIENTRY, eglGetCurrentDisplay, CAPI_ARG0())
CAPI_DEFINE_M_ENTRY(EGLSurface, EGLAPIENTRY, eglGetCurrentSurface, CAPI_ARG1(EGLint))
CAPI_DEFINE_M_ENTRY(EGLDisplay, EGLAPIENTRY, eglGetDisplay, CAPI_ARG1(EGLNativeDisplayType))
CAPI_DEFINE_M_ENTRY(EGLint, EGLAPIENTRY, eglGetError, CAPI_ARG0())
CAPI_DEFINE_M_ENTRY(__eglMustCastToProperFunctionPointerType, EGLAPIENTRY, eglGetProcAddress, CAPI_ARG1(const char *))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglInitialize, CAPI_ARG3(EGLDisplay, EGLint *, EGLint *))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglMakeCurrent, CAPI_ARG4(EGLDisplay, EGLSurface, EGLSurface, EGLContext))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglQueryContext, CAPI_ARG4(EGLDisplay, EGLContext, EGLint, EGLint *))
CAPI_DEFINE_M_ENTRY(const char *, EGLAPIENTRY, eglQueryString, CAPI_ARG2(EGLDisplay, EGLint))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglQuerySurface, CAPI_ARG4(EGLDisplay, EGLSurface, EGLint, EGLint *))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglSwapBuffers, CAPI_ARG2(EGLDisplay, EGLSurface))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglTerminate, CAPI_ARG1(EGLDisplay))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglWaitGL, CAPI_ARG0())
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglWaitNative, CAPI_ARG1(EGLint))
#endif //EGL_VERSION_1_0
#ifdef EGL_VERSION_1_1
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglBindTexImage, CAPI_ARG3(EGLDisplay, EGLSurface, EGLint))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglReleaseTexImage, CAPI_ARG3(EGLDisplay, EGLSurface, EGLint))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglSurfaceAttrib, CAPI_ARG4(EGLDisplay, EGLSurface, EGLint, EGLint))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglSwapInterval, CAPI_ARG2(EGLDisplay, EGLint))
#endif //EGL_VERSION_1_1
#ifdef EGL_VERSION_1_2
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglBindAPI, CAPI_ARG1(EGLenum))
CAPI_DEFINE_M_ENTRY(EGLenum, EGLAPIENTRY, eglQueryAPI, CAPI_ARG0())
CAPI_DEFINE_M_ENTRY(EGLSurface, EGLAPIENTRY, eglCreatePbufferFromClientBuffer, CAPI_ARG5(EGLDisplay, EGLenum, EGLClientBuffer, EGLConfig, const EGLint *))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglReleaseThread, CAPI_ARG0())
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglWaitClient, CAPI_ARG0())
#endif //EGL_VERSION_1_2
#ifdef EGL_VERSION_1_4
CAPI_DEFINE_M_ENTRY(EGLContext, EGLAPIENTRY, eglGetCurrentContext, CAPI_ARG0())
#endif //EGL_VERSION_1_4
#ifdef EGL_VERSION_1_5
CAPI_DEFINE_M_ENTRY(EGLSync, EGLAPIENTRY, eglCreateSync, CAPI_ARG3(EGLDisplay, EGLenum, const EGLAttrib *))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglDestroySync, CAPI_ARG2(EGLDisplay, EGLSync))
CAPI_DEFINE_M_ENTRY(EGLint, EGLAPIENTRY, eglClientWaitSync, CAPI_ARG4(EGLDisplay, EGLSync, EGLint, EGLTime))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglGetSyncAttrib, CAPI_ARG4(EGLDisplay, EGLSync, EGLint, EGLAttrib *))
CAPI_DEFINE_M_ENTRY(EGLDisplay, EGLAPIENTRY, eglGetPlatformDisplay, CAPI_ARG3(EGLenum, void *, const EGLAttrib *))
CAPI_DEFINE_M_ENTRY(EGLSurface, EGLAPIENTRY, eglCreatePlatformWindowSurface, CAPI_ARG4(EGLDisplay, EGLConfig, void *, const EGLAttrib *))
CAPI_DEFINE_M_ENTRY(EGLSurface, EGLAPIENTRY, eglCreatePlatformPixmapSurface, CAPI_ARG4(EGLDisplay, EGLConfig, void *, const EGLAttrib *))
CAPI_DEFINE_M_ENTRY(EGLBoolean, EGLAPIENTRY, eglWaitSync, CAPI_ARG3(EGLDisplay, EGLSync, EGLint))
#endif //EGL_VERSION_1_5
// mkapi code generation END
CAPI_END_DLL()
CAPI_DEFINE_DLL
// CAPI_DEFINE(argc, return_type, name, argv_no_name)
// mkapi code generation BEGIN
#ifdef EGL_VERSION_1_0
CAPI_DEFINE(EGLBoolean, eglChooseConfig, CAPI_ARG5(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *))
CAPI_DEFINE(EGLBoolean, eglCopyBuffers, CAPI_ARG3(EGLDisplay, EGLSurface, EGLNativePixmapType))
CAPI_DEFINE(EGLContext, eglCreateContext, CAPI_ARG4(EGLDisplay, EGLConfig, EGLContext, const EGLint *))
CAPI_DEFINE(EGLSurface, eglCreatePbufferSurface, CAPI_ARG3(EGLDisplay, EGLConfig, const EGLint *))
CAPI_DEFINE(EGLSurface, eglCreatePixmapSurface, CAPI_ARG4(EGLDisplay, EGLConfig, EGLNativePixmapType, const EGLint *))
CAPI_DEFINE(EGLSurface, eglCreateWindowSurface, CAPI_ARG4(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint *))
CAPI_DEFINE(EGLBoolean, eglDestroyContext, CAPI_ARG2(EGLDisplay, EGLContext))
CAPI_DEFINE(EGLBoolean, eglDestroySurface, CAPI_ARG2(EGLDisplay, EGLSurface))
CAPI_DEFINE(EGLBoolean, eglGetConfigAttrib, CAPI_ARG4(EGLDisplay, EGLConfig, EGLint, EGLint *))
CAPI_DEFINE(EGLBoolean, eglGetConfigs, CAPI_ARG4(EGLDisplay, EGLConfig *, EGLint, EGLint *))
CAPI_DEFINE(EGLDisplay, eglGetCurrentDisplay, CAPI_ARG0())
CAPI_DEFINE(EGLSurface, eglGetCurrentSurface, CAPI_ARG1(EGLint))
CAPI_DEFINE(EGLDisplay, eglGetDisplay, CAPI_ARG1(EGLNativeDisplayType))
CAPI_DEFINE(EGLint, eglGetError, CAPI_ARG0())
CAPI_DEFINE(__eglMustCastToProperFunctionPointerType, eglGetProcAddress, CAPI_ARG1(const char *))
CAPI_DEFINE(EGLBoolean, eglInitialize, CAPI_ARG3(EGLDisplay, EGLint *, EGLint *))
CAPI_DEFINE(EGLBoolean, eglMakeCurrent, CAPI_ARG4(EGLDisplay, EGLSurface, EGLSurface, EGLContext))
CAPI_DEFINE(EGLBoolean, eglQueryContext, CAPI_ARG4(EGLDisplay, EGLContext, EGLint, EGLint *))
CAPI_DEFINE(const char *, eglQueryString, CAPI_ARG2(EGLDisplay, EGLint))
CAPI_DEFINE(EGLBoolean, eglQuerySurface, CAPI_ARG4(EGLDisplay, EGLSurface, EGLint, EGLint *))
CAPI_DEFINE(EGLBoolean, eglSwapBuffers, CAPI_ARG2(EGLDisplay, EGLSurface))
CAPI_DEFINE(EGLBoolean, eglTerminate, CAPI_ARG1(EGLDisplay))
CAPI_DEFINE(EGLBoolean, eglWaitGL, CAPI_ARG0())
CAPI_DEFINE(EGLBoolean, eglWaitNative, CAPI_ARG1(EGLint))
#endif //EGL_VERSION_1_0
#ifdef EGL_VERSION_1_1
CAPI_DEFINE(EGLBoolean, eglBindTexImage, CAPI_ARG3(EGLDisplay, EGLSurface, EGLint))
CAPI_DEFINE(EGLBoolean, eglReleaseTexImage, CAPI_ARG3(EGLDisplay, EGLSurface, EGLint))
CAPI_DEFINE(EGLBoolean, eglSurfaceAttrib, CAPI_ARG4(EGLDisplay, EGLSurface, EGLint, EGLint))
CAPI_DEFINE(EGLBoolean, eglSwapInterval, CAPI_ARG2(EGLDisplay, EGLint))
#endif //EGL_VERSION_1_1
#ifdef EGL_VERSION_1_2
CAPI_DEFINE(EGLBoolean, eglBindAPI, CAPI_ARG1(EGLenum))
CAPI_DEFINE(EGLenum, eglQueryAPI, CAPI_ARG0())
CAPI_DEFINE(EGLSurface, eglCreatePbufferFromClientBuffer, CAPI_ARG5(EGLDisplay, EGLenum, EGLClientBuffer, EGLConfig, const EGLint *))
CAPI_DEFINE(EGLBoolean, eglReleaseThread, CAPI_ARG0())
CAPI_DEFINE(EGLBoolean, eglWaitClient, CAPI_ARG0())
#endif //EGL_VERSION_1_2
#ifdef EGL_VERSION_1_4
CAPI_DEFINE(EGLContext, eglGetCurrentContext, CAPI_ARG0())
#endif //EGL_VERSION_1_4
#ifdef EGL_VERSION_1_5
CAPI_DEFINE(EGLSync, eglCreateSync, CAPI_ARG3(EGLDisplay, EGLenum, const EGLAttrib *))
CAPI_DEFINE(EGLBoolean, eglDestroySync, CAPI_ARG2(EGLDisplay, EGLSync))
CAPI_DEFINE(EGLint, eglClientWaitSync, CAPI_ARG4(EGLDisplay, EGLSync, EGLint, EGLTime))
CAPI_DEFINE(EGLBoolean, eglGetSyncAttrib, CAPI_ARG4(EGLDisplay, EGLSync, EGLint, EGLAttrib *))
CAPI_DEFINE(EGLDisplay, eglGetPlatformDisplay, CAPI_ARG3(EGLenum, void *, const EGLAttrib *))
CAPI_DEFINE(EGLSurface, eglCreatePlatformWindowSurface, CAPI_ARG4(EGLDisplay, EGLConfig, void *, const EGLAttrib *))
CAPI_DEFINE(EGLSurface, eglCreatePlatformPixmapSurface, CAPI_ARG4(EGLDisplay, EGLConfig, void *, const EGLAttrib *))
CAPI_DEFINE(EGLBoolean, eglWaitSync, CAPI_ARG3(EGLDisplay, EGLSync, EGLint))
#endif //EGL_VERSION_1_5
// mkapi code generation END
#endif //CAPI_LINK_EGL
} //namespace egl
//this file is generated by "mkapi.sh -I /Users/wangbin/dev/qtbase/src/3rdparty/angle/include -name egl -template capi /Users/wangbin/dev/qtbase/src/3rdparty/angle/include/EGL/egl.h"
