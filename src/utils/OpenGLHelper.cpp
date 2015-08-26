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

#include "OpenGLHelper.h"
#include <string.h> //strstr
#include <QtCore/QCoreApplication>
#include <QtGui/QMatrix4x4>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
#include <QtOpenGL/QGLFunctions>
#endif
#endif
#include "utils/Logger.h"

namespace QtAV {
namespace OpenGLHelper {

bool isOpenGLES()
{
#ifdef QT_OPENGL_DYNAMIC
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    // desktop can create es compatible context
    return qApp->testAttribute(Qt::AA_UseOpenGLES) || (ctx ? ctx->isOpenGLES() : QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL); //
#endif //QT_OPENGL_DYNAMIC
#ifdef QT_OPENGL_ES_2
    return true;
#endif //QT_OPENGL_ES_2
#if defined(QT_OPENGL_ES_2_ANGLE_STATIC) || defined(QT_OPENGL_ES_2_ANGLE)
    return true;
#endif //QT_OPENGL_ES_2_ANGLE_STATIC
    return false;
}

bool hasExtension(const char *exts[])
{
    const QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning("no gl context for hasExtension");
        return false;
    }
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    const char *ext = (const char*)glGetString(GL_EXTENSIONS);
    if (!ext)
        return false;
#endif
    for (int i = 0; exts[i]; ++i) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        if (ctx->hasExtension(exts[i]))
#else
        if (strstr(ext, exts[i]))
#endif
            return true;
    }
    return false;
}

bool isPBOSupported() {
    // check pbo support
    static bool support = false;
    static bool pbo_checked = false;
    if (pbo_checked)
        return support;
    const QOpenGLContext *ctx = QOpenGLContext::currentContext();
    Q_ASSERT(ctx);
    if (!ctx)
        return false;
    const char* exts[] = {
        "GL_ARB_pixel_buffer_object",
        "GL_EXT_pixel_buffer_object",
        "GL_NV_pixel_buffer_object", //OpenGL ES
        NULL
    };
    support = hasExtension(exts);
    qDebug() << "PBO: " << support;
    pbo_checked = true;
    return support;
}

// glActiveTexture in Qt4 on windows release mode crash for me
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#ifndef QT_OPENGL_ES
// APIENTRY may be not defined(why? linux es2). or use QOPENGLF_APIENTRY
// use QGLF_APIENTRY for Qt4 crash, why? APIENTRY is defined in windows header
#ifndef APIENTRY
// QGLF_APIENTRY is in Qt4,8+
#if defined(QGLF_APIENTRY)
#define APIENTRY QGLF_APIENTRY
#elif defined(GL_APIENTRY)
#define APIENTRY GL_APIENTRY
#endif //QGLF_APIENTRY
#endif //APIENTRY
typedef void (APIENTRY *type_glActiveTexture) (GLenum);
static type_glActiveTexture qtav_glActiveTexture = 0;

static void qtavResolveActiveTexture()
{
    const QGLContext *context = QGLContext::currentContext();
    qtav_glActiveTexture = (type_glActiveTexture)context->getProcAddress(QLatin1String("glActiveTexture"));
    if (!qtav_glActiveTexture) {
        qDebug("resolve glActiveTextureARB");
        qtav_glActiveTexture = (type_glActiveTexture)context->getProcAddress(QLatin1String("glActiveTextureARB"));
    }
    //Q_ASSERT(qtav_glActiveTexture);
}
#endif //QT_OPENGL_ES
#endif //QT_VERSION

void glActiveTexture(GLenum texture)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#ifndef QT_OPENGL_ES
    if (!qtav_glActiveTexture)
        qtavResolveActiveTexture();
    if (!qtav_glActiveTexture)
        return;
    qtav_glActiveTexture(texture);
#else
    ::glActiveTexture(texture);
#endif //QT_OPENGL_ES
#else
    QOpenGLContext::currentContext()->functions()->glActiveTexture(texture);
#endif
}

typedef struct {
    GLint internal_format;
    GLenum format;
    GLenum type;
} gl_fmt_t;

// es formats:  ALPHA, RGB, RGBA, LUMINANCE, LUMINANCE_ALPHA
// es types:  UNSIGNED_BYTE, UNSIGNED_SHORT_5_6_5, UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_5_5_5_1
/*!
  c: number of channels(components) in the plane
  b: componet size
    result is gl_fmts1[(c-1)+4*(b-1)]
*/
static const gl_fmt_t gl_fmts1[] = { // it's legacy
    { GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE},
    { GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE},
    { GL_RGB, GL_RGB, GL_UNSIGNED_BYTE},
    { GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},
    { GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_SHORT}, //internal format XXX16?
    { GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT},
    { GL_RGB, GL_RGB, GL_UNSIGNED_SHORT},
    { GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT},
};

typedef struct {
    VideoFormat::PixelFormat pixfmt;
    quint8 channels[4];
} reorder_t;
// use with gl_fmts1
static const reorder_t gl_channel_maps[] = {
    { VideoFormat::Format_ARGB32, {1, 2, 3, 0}},
    { VideoFormat::Format_ABGR32, {3, 2, 1, 0}}, // R->gl.?(a)->R
    { VideoFormat::Format_BGR24,  {2, 1, 0, 3}},
    { VideoFormat::Format_BGR565, {2, 1, 0, 3}},
    { VideoFormat::Format_BGRA32, {2, 1, 0, 3}},
    { VideoFormat::Format_BGR32,  {2, 1, 0, 3}},
    { VideoFormat::Format_BGR48LE,{2, 1, 0, 3}},
    { VideoFormat::Format_BGR48BE,{2, 1, 0, 3}},
    { VideoFormat::Format_BGR48,  {2, 1, 0, 3}},
    { VideoFormat::Format_BGR555, {2, 1, 0, 3}},
    { VideoFormat::Format_Invalid,{0, 1, 2, 3}}
};

static QMatrix4x4 channelMap(const VideoFormat& fmt)
{
    if (fmt.isPlanar()) //currently only for planar
        return QMatrix4x4();
    switch (fmt.pixelFormat()) {
    case VideoFormat::Format_UYVY:
        return QMatrix4x4(0.0f, 0.5f, 0.0f, 0.5f,
                                 1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f);
    case VideoFormat::Format_YUYV:
        return QMatrix4x4(0.5f, 0.0f, 0.5f, 0.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f);
    case VideoFormat::Format_VYUY:
        return QMatrix4x4(0.0f, 0.5f, 0.0f, 0.5f,
                                 0.0f, 0.0f, 1.0f, 0.0f,
                                 1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f);
    case VideoFormat::Format_YVYU:
        return QMatrix4x4(0.5f, 0.0f, 0.5f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f);
    default:
        break;
    }

    const quint8 *channels = NULL;//{ 0, 1, 2, 3};
    for (int i = 0; gl_channel_maps[i].pixfmt != VideoFormat::Format_Invalid; ++i) {
        if (gl_channel_maps[i].pixfmt == fmt.pixelFormat()) {
            channels = gl_channel_maps[i].channels;
            break;
        }
    }
    QMatrix4x4 m;
    if (!channels)
        return m;
    m.fill(0);
    for (int i = 0; i < 4; ++i) {
        m(i, channels[i]) = 1;
    }
    qDebug() << m;
    return m;
}

//template<typename T, size_t N> size_t array_size(const T (&)[N]) { return N;} //does not support local type
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
bool videoFormatToGL(const VideoFormat& fmt, GLint* internal_format, GLenum* data_format, GLenum* data_type, QMatrix4x4* mat)
{
    typedef struct fmt_entry {
        VideoFormat::PixelFormat pixfmt;
        GLint internal_format;
        GLenum format;
        GLenum type;
    } fmt_entry;
    static const fmt_entry pixfmt_to_gles[] = {
        {VideoFormat::Format_RGB32,  GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_Invalid, 0, 0, 0}
    };
    Q_UNUSED(pixfmt_to_gles);
    static const fmt_entry pixfmt_to_desktop[] = {
        {VideoFormat::Format_RGB32,  GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE }, //FIXMEL endian check
        //{VideoFormat::Format_BGRA32,  GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE }, //{2,1,0,3}
        //{VideoFormat::Format_BGR24,   GL_RGB,  GL_BGR,  GL_UNSIGNED_BYTE }, //{0,1,2,3}
    #ifdef GL_UNSIGNED_SHORT_5_6_5_REV
        {VideoFormat::Format_BGR565, GL_RGB,  GL_RGB,  GL_UNSIGNED_SHORT_5_6_5_REV}, // es error, use channel map
    #endif
    #ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
        {VideoFormat::Format_RGB555, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1}, //desktop error
    #endif
    #ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
        {VideoFormat::Format_BGR555, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
    #endif
        {VideoFormat::Format_Invalid, 0, 0, 0}
    };
    Q_UNUSED(pixfmt_to_desktop);
    const fmt_entry *pixfmt_gl_entry = pixfmt_to_desktop;
    if (OpenGLHelper::isOpenGLES())
        pixfmt_gl_entry = pixfmt_to_gles;
    // Very special formats, for which OpenGL happens to have direct support
    static const fmt_entry pixfmt_gl_base[] = {
        {VideoFormat::Format_BGRA32, GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_RGBA32, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE }, // only tested for osx, win, angle
        {VideoFormat::Format_RGB24,  GL_RGB,  GL_RGB,  GL_UNSIGNED_BYTE },
        {VideoFormat::Format_RGB565, GL_RGB,  GL_RGB,  GL_UNSIGNED_SHORT_5_6_5},
        {VideoFormat::Format_BGR32,  GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE }, //rgba(tested) or abgr, depending on endian
        // TODO: not implemeted
        {VideoFormat::Format_RGB48, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT }, //TODO: rgb16?
        {VideoFormat::Format_RGB48LE, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT },
        {VideoFormat::Format_RGB48BE, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT },
        {VideoFormat::Format_BGR48, GL_BGR, GL_BGR, GL_UNSIGNED_SHORT }, //RGB16?
        {VideoFormat::Format_BGR48LE, GL_BGR, GL_BGR, GL_UNSIGNED_SHORT },
        {VideoFormat::Format_BGR48BE, GL_BGR, GL_BGR, GL_UNSIGNED_SHORT },
    };
    const VideoFormat::PixelFormat pixfmt = fmt.pixelFormat();
    // can not use array size because pixfmt_gl_entry is set on runtime
    for (const fmt_entry* e = pixfmt_gl_entry; e->pixfmt != VideoFormat::Format_Invalid; ++e) {
        if (e->pixfmt == pixfmt) {
            *internal_format = e->internal_format;
            *data_format = e->format;
            *data_type = e->type;
            if (mat)
                *mat = QMatrix4x4();
            return true;
        }
    }
    for (size_t i = 0; i < ARRAY_SIZE(pixfmt_gl_base); ++i) {
        const fmt_entry& e = pixfmt_gl_base[i];
        if (e.pixfmt == pixfmt) {
            *internal_format = e.internal_format;
            *data_format = e.format;
            *data_type = e.type;
            if (mat)
                *mat = QMatrix4x4();
            return true;
        }
    }
    static const fmt_entry pixfmt_to_gl_swizzele[] = {
        {VideoFormat::Format_UYVY, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_YUYV, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_VYUY, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_YVYU, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_BGR565, GL_RGB,  GL_RGB,  GL_UNSIGNED_SHORT_5_6_5}, //swizzle
        {VideoFormat::Format_RGB555, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1}, //not working
        {VideoFormat::Format_BGR555, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1}, //not working
    };
    for (size_t i = 0; i < ARRAY_SIZE(pixfmt_to_gl_swizzele); ++i) {
        const fmt_entry& e = pixfmt_to_gl_swizzele[i];
        if (e.pixfmt == pixfmt) {
            *internal_format = e.internal_format;
            *data_format = e.format;
            *data_type = e.type;
            if (mat)
                *mat = channelMap(fmt);
            return true;
        }
    }
    GLint *i_f = internal_format;
    GLenum *d_f = data_format;
    GLenum *d_t = data_type;
    for (int p = 0; p < fmt.planeCount(); ++p) {
        // for packed rgb(swizzle required) and planar formats
        const int c = (fmt.channels(p)-1) + 4*((fmt.bitsPerComponent() + 7)/8 - 1);
        if (c >= (int)ARRAY_SIZE(gl_fmts1))
            return false;
        const gl_fmt_t f = gl_fmts1[c];
        *(i_f++) = f.internal_format;
        *(d_f++) = f.format;
        *(d_t++) = f.type;
    }
    if (mat)
        *mat = channelMap(fmt);
    return true;
}

// TODO: format + datatype? internal format == format?
//https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_format_BGRA8888.txt
int bytesOfGLFormat(GLenum format, GLenum dataType)
{
    int component_size = 0;
    switch (dataType) {
#ifdef GL_UNSIGNED_INT_8_8_8_8_REV
    case GL_UNSIGNED_INT_8_8_8_8_REV:
        return 4;
#endif
#ifdef GL_UNSIGNED_BYTE_3_3_2
    case GL_UNSIGNED_BYTE_3_3_2:
        return 1;
#endif //GL_UNSIGNED_BYTE_3_3_2
#ifdef GL_UNSIGNED_BYTE_2_3_3_REV
    case GL_UNSIGNED_BYTE_2_3_3_REV:
            return 1;
#endif
        case GL_UNSIGNED_SHORT_5_5_5_1:
#ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
        case GL_UNSIGNED_SHORT_1_5_5_5_REV:
#endif //GL_UNSIGNED_SHORT_1_5_5_5_REV
#ifdef GL_UNSIGNED_SHORT_5_6_5_REV
        case GL_UNSIGNED_SHORT_5_6_5_REV:
#endif //GL_UNSIGNED_SHORT_5_6_5_REV
    case GL_UNSIGNED_SHORT_5_6_5: // gles
#ifdef GL_UNSIGNED_SHORT_4_4_4_4_REV
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
#endif //GL_UNSIGNED_SHORT_4_4_4_4_REV
    case GL_UNSIGNED_SHORT_4_4_4_4:
        return 2;
    case GL_UNSIGNED_BYTE:
        component_size = 1;
        break;
        // mpv returns 2
#ifdef GL_UNSIGNED_SHORT_8_8_APPLE
    case GL_UNSIGNED_SHORT_8_8_APPLE:
    case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
        return 2;
#endif
    case GL_UNSIGNED_SHORT:
        component_size = 2;
        break;
    }
    switch (format) {
#ifdef GL_YCBCR_422_APPLE
      case GL_YCBCR_422_APPLE:
        return 2;
#endif
#ifdef GL_RGB_422_APPLE
      case GL_RGB_422_APPLE:
        return 2;
#endif
#ifdef GL_BGRA //ifndef GL_ES
      case GL_BGRA:
#endif
      case GL_RGBA:
        return 4*component_size;
#ifdef GL_BGR //ifndef GL_ES
      case GL_BGR:
#endif
      case GL_RGB:
        return 3*component_size;
      case GL_LUMINANCE_ALPHA:
        // mpv returns 2
        return 2*component_size;
      case GL_LUMINANCE:
      case GL_ALPHA:
        return 1*component_size;
#ifdef GL_LUMINANCE16
    case GL_LUMINANCE16:
        return 2*component_size;
#endif //GL_LUMINANCE16
#ifdef GL_ALPHA16
    case GL_ALPHA16:
        return 2*component_size;
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
