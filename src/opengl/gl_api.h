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
#ifndef QTAV_GL_API_H
#define QTAV_GL_API_H

#ifndef QT_NO_OPENGL
#include <qglobal.h>
# if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>
# elif defined(QT_OPENGL_LIB)
#  if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
#include <QtOpenGL/QGLFunctions>
#  endif //4.8
#include <QtOpenGL/QGLBuffer>
#include <QtOpenGL/QGLContext>
#include <QtOpenGL/QGLShaderProgram>
#define QOpenGLShaderProgram QGLShaderProgram
typedef QGLBuffer QOpenGLBuffer;
#define QOpenGLContext QGLContext
#define QOpenGLShaderProgram QGLShaderProgram
#define QOpenGLShader QGLShader
#define QOpenGLFunctions QGLFunctions
#define initializeOpenGLFunctions() initializeGLFunctions()
#include <qgl.h>
# else //used by vaapi even qtopengl module is disabled
#  if defined(QT_OPENGL_ES_2)
#   if defined(Q_OS_MAC) // iOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#   else // "uncontrolled" ES2 platforms
#include <GLES2/gl2.h>
#   endif // Q_OS_MAC
#  else // non-ES2 platforms
#   if defined(Q_OS_MAC)
#include <OpenGL/gl.h>
#    if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
#include <OpenGL/gl3.h>
#    endif
#include <OpenGL/glext.h>
#   else
#include <GL/gl.h>
#   endif // Q_OS_MAC
#  endif // QT_OPENGL_ES_2
# endif

#ifndef GL_APIENTRY
#ifdef Q_OS_WIN
#define GL_APIENTRY __stdcall
#else
#define GL_APIENTRY
#endif
#endif

#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE 0x84F5
#endif

//GL_BGRA is available in OpenGL >= 1.2
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_RG
#define GL_RG 0x8227
#endif
#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_R16
#define GL_R16 0x822A
#endif
#ifndef GL_RG8
#define GL_RG8 0x822B
#endif
#ifndef GL_RG16
#define GL_RG16 0x822C
#endif
#ifndef GL_RGB8
#define GL_RGB8 0x8051
#endif
#ifndef GL_RGB16
#define GL_RGB16 0x8054
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_RGBA16
#define GL_RGBA16 0x805B
#endif

namespace QtAV {
typedef char GLchar; // for qt4 mingw
struct api;
api& gl();
struct api {
    void resolve();
    // TODO: static, so gl::GetString
    const GLubyte *(GL_APIENTRY *GetString)(GLenum);
    GLenum (GL_APIENTRY *GetError)(void);
    void (GL_APIENTRY *ActiveTexture)(GLenum);
    void (GL_APIENTRY *BindFramebuffer)(GLenum target, GLuint framebuffer);
    GLint (GL_APIENTRY *GetUniformLocation)(GLuint, const GLchar *);
    void (GL_APIENTRY *Uniform1f)(GLint, GLfloat);
    void (GL_APIENTRY *Uniform2f)(GLint, GLfloat, GLfloat);
    void (GL_APIENTRY *Uniform3f)(GLint, GLfloat, GLfloat, GLfloat);
    void (GL_APIENTRY *Uniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
    void (GL_APIENTRY *Uniform1fv)(GLint location, GLsizei count, const GLfloat *value);
    void (GL_APIENTRY *Uniform2fv)(GLint location, GLsizei count, const GLfloat *value);
    void (GL_APIENTRY *Uniform3fv)(GLint location, GLsizei count, const GLfloat *value);
    void (GL_APIENTRY *Uniform4fv)(GLint location, GLsizei count, const GLfloat *value);
    void (GL_APIENTRY *Uniform1iv)(GLint location, GLsizei count, const GLint *value);
    void (GL_APIENTRY *Uniform2iv)(GLint location, GLsizei count, const GLint *value);
    void (GL_APIENTRY *Uniform3iv)(GLint location, GLsizei count, const GLint *value);
    void (GL_APIENTRY *Uniform4iv)(GLint location, GLsizei count, const GLint *value);
    void (GL_APIENTRY *UniformMatrix2fv)(GLint, GLsizei, GLboolean, const GLfloat *);
    void (GL_APIENTRY *UniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat *);
    void (GL_APIENTRY *UniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (GL_APIENTRY *BlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);

    // Before using the following members, check null ptr first because they are not valid everywhere
// ES3.1
    void (GL_APIENTRY *GetTexLevelParameteriv)(GLenum, GLint, GLenum, GLint *);

#if defined(Q_OS_WIN32)
    //#include <GL/wglext.h> //not found in vs2013
    //https://www.opengl.org/registry/specs/NV/DX_interop.txt
#ifndef WGL_ACCESS_READ_ONLY_NV
#define WGL_ACCESS_READ_ONLY_NV           0x00000000
#define WGL_ACCESS_READ_WRITE_NV          0x00000001
#define WGL_ACCESS_WRITE_DISCARD_NV       0x00000002
#endif
    BOOL (WINAPI* DXSetResourceShareHandleNV)(void *dxObject, HANDLE shareHandle);
    HANDLE (WINAPI* DXOpenDeviceNV)(void *dxDevice);
    BOOL (WINAPI* DXCloseDeviceNV)(HANDLE hDevice);
    HANDLE (WINAPI* DXRegisterObjectNV)(HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access);
    BOOL (WINAPI* DXUnregisterObjectNV)(HANDLE hDevice, HANDLE hObject);
    BOOL (WINAPI* DXObjectAccessNV)(HANDLE hObject, GLenum access);
    BOOL (WINAPI* DXLockObjectsNV)(HANDLE hDevice, GLint count, HANDLE *hObjects);
    BOOL (WINAPI* DXUnlockObjectsNV)(HANDLE hDevice, GLint count, HANDLE *hObjects);
#endif
};
} //namespace QtAV
#endif //QT_NO_OPENGL
#endif //QTAV_GL_API_H
