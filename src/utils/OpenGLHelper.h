/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_OPENGLHELPER_H
#define QTAV_OPENGLHELPER_H

#include "QtAV/VideoFormat.h"
#include <qglobal.h>
#ifndef QT_NO_OPENGL
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>
#elif defined(QT_OPENGL_LIB)
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
#include <QtOpenGL/QGLFunctions>
#endif //4.8
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
#else //used by vaapi even qtopengl module is disabled
#if defined(QT_OPENGL_ES_2)
# if defined(Q_OS_MAC) // iOS
#  include <OpenGLES/ES2/gl.h>
#  include <OpenGLES/ES2/glext.h>
# else // "uncontrolled" ES2 platforms
#  include <GLES2/gl2.h>
# endif // Q_OS_MAC
#else // non-ES2 platforms
# if defined(Q_OS_MAC)
#  include <OpenGL/gl.h>
#  if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
#   include <OpenGL/gl3.h>
#  endif
#  include <OpenGL/glext.h>
# else
#  include <GL/gl.h>
# endif // Q_OS_MAC
#endif // QT_OPENGL_ES_2
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
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
#ifdef QT_OPENGL_DYNAMIC
#define DYGL(glFunc) QOpenGLContext::currentContext()->functions()->glFunc
#else
#define DYGL(glFunc) glFunc
#endif
#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE 0x84F5
#endif

#define EGL_ENSURE(x, ...) \
    do { \
        if (!(x)) { \
            EGLint err = eglGetError(); \
            qWarning("EGL error@%d<<%s. " #x ": %#x %s", __LINE__, __FILE__, err, eglQueryString(eglGetCurrentDisplay(), err)); \
            return __VA_ARGS__; \
        } \
    } while(0)
#define EGL_WARN(x, ...) \
    do { \
        if (!(x)) { \
            EGLint err = eglGetError(); \
            qWarning("EGL error@%d<<%s. " #x ": %#x %s", __LINE__, __FILE__, err, eglQueryString(eglGetCurrentDisplay(), err)); \
        } \
    } while(0)

class QMatrix4x4;
namespace QtAV {
namespace OpenGLHelper {

QByteArray compatibleVertexShaderHeader();
QByteArray compatibleFragmentShaderHeader();
int GLSLVersion();
bool isEGL();
bool isOpenGLES();
/*!
 * \brief hasExtensionEGL
 * Test if any of the given extensions is supported
 * \param exts Ends with NULL
 * \return true if one of extension is supported
 */
bool hasExtensionEGL(const char* exts[]);
bool hasRG();
bool has16BitTexture();
// set by user (environment var "QTAV_TEXTURE16_DEPTH=8 or 16", default now is 8)
int depth16BitTexture();
// set by user (environment var "QTAV_GL_DEPRECATED=1")
bool useDeprecatedFormats();
/*!
 * \brief hasExtension
 * Test if any of the given extensions is supported. Current OpenGL context must be valid.
 * \param exts Ends with NULL
 * \return true if one of extension is supported
 */
bool hasExtension(const char* exts[]);
bool isPBOSupported();
void glActiveTexture(GLenum texture);
/*!
 * \brief videoFormatToGL
 * \param fmt
 * \param internal_format an array with size fmt.planeCount()
 * \param data_format an array with size fmt.planeCount()
 * \param data_type an array with size fmt.planeCount()
 * \param mat channel reorder matrix used in glsl
 * \return false if fmt is not supported
 */
bool videoFormatToGL(const VideoFormat& fmt, GLint* internal_format, GLenum* data_format, GLenum* data_type, QMatrix4x4* mat = NULL);
int bytesOfGLFormat(GLenum format, GLenum dataType = GL_UNSIGNED_BYTE);
} //namespace OpenGLHelper
} //namespace QtAV
#else
namespace QtAV {
namespace OpenGLHelper {
#define DYGL(f) f
inline bool isOpenGLES() {return false;}
} //namespace OpenGLHelper
} //namespace QtAV
#endif //QT_NO_OPENGL
#endif // QTAV_OPENGLHELPER_H
