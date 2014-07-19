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
#include <qopengl.h>
#else
#include <qgl.h>
#endif
#include "QtAV/VideoFormat.h"

namespace QtAV {
namespace OpenGLHelper {

//TODO: glActiveTexture() for Qt4?
bool videoFormatToGL(const VideoFormat& fmt, GLint* internal_format, GLenum* data_format, GLenum* data_type);
int bytesOfGLFormat(GLenum format, GLenum dataType = GL_UNSIGNED_BYTE);
GLint GetGLInternalFormat(GLint data_format, int bpp);

} //namespace OpenGLHelper
} //namespace QtAV

#endif // QTAV_OPENGLHELPER_H
