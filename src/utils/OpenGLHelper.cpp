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

#include "OpenGLHelper.h"

namespace QtAV {
namespace OpenGLHelper {


bool videoFormatToGL(const VideoFormat& fmt, GLint* internal_format, GLenum* data_format, GLenum* data_type)
{
    struct fmt_entry {
        VideoFormat::PixelFormat pixfmt;
        GLint internal_format;
        GLenum format;
        GLenum type;
    };
    // Very special formats, for which OpenGL happens to have direct support
    static const struct fmt_entry pixfmt_to_gl_formats[] = {
#ifdef QT_OPENGL_ES_2
        {VideoFormat::Format_ARGB32, GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_RGB32,  GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE },
#else
        {VideoFormat::Format_RGB32,  GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_ARGB32, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE },
#endif
        {VideoFormat::Format_RGB24,  GL_RGB,  GL_RGB,  GL_UNSIGNED_BYTE },
    #ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
        {VideoFormat::Format_RGB555, GL_RGBA, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
    #endif
        {VideoFormat::Format_RGB565, GL_RGB,  GL_RGB,  GL_UNSIGNED_SHORT_5_6_5}, //GL_UNSIGNED_SHORT_5_6_5_REV?
        //{VideoFormat::Format_BGRA32, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE },
        //{VideoFormat::Format_BGR32,  GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_BGR24,  GL_RGB,  GL_BGR,  GL_UNSIGNED_BYTE },
    #ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
        {VideoFormat::Format_BGR555, GL_RGBA, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
    #endif
        {VideoFormat::Format_BGR565, GL_RGB,  GL_RGB,  GL_UNSIGNED_SHORT_5_6_5}, // need swap r b?
    };

    for (unsigned int i = 0; i < sizeof(pixfmt_to_gl_formats)/sizeof(pixfmt_to_gl_formats[0]); ++i) {
        if (pixfmt_to_gl_formats[i].pixfmt == fmt.pixelFormat()) {
            *internal_format = pixfmt_to_gl_formats[i].internal_format;
            *data_format = pixfmt_to_gl_formats[i].format;
            *data_type = pixfmt_to_gl_formats[i].type;
            return true;
        }
    }
    return false;
}

// TODO: format + datatype? internal format == format?
//https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_format_BGRA8888.txt
int bytesOfGLFormat(GLenum format, GLenum dataType)
{
    switch (format)
      {
#ifdef GL_BGRA //ifndef GL_ES
      case GL_BGRA:
#endif
      case GL_RGBA:
        return 4;
#ifdef GL_BGR //ifndef GL_ES
      case GL_BGR:
#endif
      case GL_RGB:
        switch (dataType) {
        case GL_UNSIGNED_SHORT_5_6_5:
            return 2;
        default:
            return 3;
        }
        return 3;
      case GL_LUMINANCE_ALPHA:
        return 2;
      case GL_LUMINANCE:
      case GL_ALPHA:
        return 1;
#ifdef GL_LUMINANCE16
    case GL_LUMINANCE16:
        return 2;
#endif //GL_LUMINANCE16
#ifdef GL_ALPHA16
    case GL_ALPHA16:
        return 2;
#endif //GL_ALPHA16
      default:
        qWarning("bytesOfGLFormat - Unknown format %u", format);
        return 1;
      }
}

GLint GetGLInternalFormat(GLint data_format, int bpp)
{
    if (bpp != 2)
        return data_format;
    switch (data_format) {
#ifdef GL_ALPHA16
    case GL_ALPHA:
        return GL_ALPHA16;
#endif //GL_ALPHA16
#ifdef GL_LUMINANCE16
    case GL_LUMINANCE:
        return GL_LUMINANCE16;
#endif //GL_LUMINANCE16
    default:
        return data_format;
    }
}

} //namespace OpenGLHelper
} //namespace QtAV
