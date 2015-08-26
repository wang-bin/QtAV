/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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
#include <QtGui/QMatrix4x4>
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
#ifndef GL_BGRA_EXT
#if defined QT_OPENGL_ES_2
#include <GLES2/gl2ext.h>
//#include <GLES/glext.h> //maemo 5 define there
#elif defined QT_OPENGL_ES
#include <GLES/glext.h>
#else
#include <GL/glext.h> //GL_BGRA_EXT for OpenGL<=1.1 //TODO Apple include <OpenGL/xxx>
#endif
#endif //GL_BGRA_EXT
//TODO: glPixelStorei(GL_PACK_SWAP_BYTES, ) to swap rgba?
#ifndef GL_BGRA //it may be defined in glext.h
#ifdef GL_BGRA_EXT
#define GL_BGRA GL_BGRA_EXT
#endif //GL_BGRA_EXT
#ifdef GL_BGR_EXT
#define GL_BGR GL_BGR_EXT
#endif //GL_BGR_EXT
#endif //GL_BGRA
#endif //GL_BGRA
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_BGR
#define GL_BGR 0x80E0
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

namespace QtAV {
namespace OpenGLHelper {

bool isOpenGLES();
/*!
 * \brief hasExtension
 * Current OpenGL context must be valid.
 * \param exts Ends with NULL
 */
bool hasExtension(const char* exts[]);
bool isPBOSupported();
void glActiveTexture(GLenum texture);
bool videoFormatToGL(const VideoFormat& fmt, GLint* internal_format, GLenum* data_format, GLenum* data_type, QMatrix4x4* mat = NULL);
int bytesOfGLFormat(GLenum format, GLenum dataType = GL_UNSIGNED_BYTE);
GLint GetGLInternalFormat(GLint data_format, int bpp);

} //namespace OpenGLHelper
} //namespace QtAV

#endif // QTAV_OPENGLHELPER_H
