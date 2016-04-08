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

#ifndef QT_NO_OPENGL
#include "QtAV/VideoFormat.h"
#include "opengl/gl_api.h"
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
#ifdef QT_OPENGL_DYNAMIC
#define DYGL(glFunc) QOpenGLContext::currentContext()->functions()->glFunc
#else
#define DYGL(glFunc) glFunc
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

QT_BEGIN_NAMESPACE
class QMatrix4x4;
QT_END_NAMESPACE
namespace QtAV {
namespace OpenGLHelper {
QString removeComments(const QString& code);
QByteArray compatibleShaderHeader(QOpenGLShader::ShaderType type);
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
