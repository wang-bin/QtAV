/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include <qglobal.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#else
#include <qgl.h>
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

#include "QtAV/VideoFormat.h"

namespace QtAV {
namespace OpenGLHelper {

void glActiveTexture(GLenum texture);
bool videoFormatToGL(const VideoFormat& fmt, GLint* internal_format, GLenum* data_format, GLenum* data_type);
int bytesOfGLFormat(GLenum format, GLenum dataType = GL_UNSIGNED_BYTE);
GLint GetGLInternalFormat(GLint data_format, int bpp);

} //namespace OpenGLHelper
} //namespace QtAV

#endif // QTAV_OPENGLHELPER_H
