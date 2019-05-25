/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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

#include "SurfaceInteropCV.h"
#include "QtAV/VideoFrame.h"
#include "opengl/OpenGLHelper.h"

namespace QtAV {
typedef struct {
    int cv_pixfmt;
    VideoFormat::PixelFormat pixfmt;
} cv_format;
//https://developer.apple.com/library/Mac/releasenotes/General/MacOSXLionAPIDiffs/CoreVideo.html
/* use fourcc '420v', 'yuvs' for NV12 and yuyv to avoid build time version check
 * qt4 targets 10.6, so those enum values is not valid in build time, while runtime is supported.
 */
static const cv_format cv_formats[] = {
    { 'y420', VideoFormat::Format_YUV420P }, //kCVPixelFormatType_420YpCbCr8Planar
    { '2vuy', VideoFormat::Format_UYVY }, //kCVPixelFormatType_422YpCbCr8
//#ifdef OSX_TARGET_MIN_LION
    { '420f' , VideoFormat::Format_NV12 }, // kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
    { '420v', VideoFormat::Format_NV12 }, //kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
    { 'yuvs', VideoFormat::Format_YUYV }, //kCVPixelFormatType_422YpCbCr8_yuvs
//#endif
    { 'BGRA', VideoFormat::Format_BGRA32 },
    { kCVPixelFormatType_24RGB, VideoFormat::Format_RGB24 },
    { 0, VideoFormat::Format_Invalid }
};
namespace cv {
VideoFormat::PixelFormat format_from_cv(int cv)
{
    for (int i = 0; cv_formats[i].cv_pixfmt; ++i) {
        if (cv_formats[i].cv_pixfmt == cv)
            return cv_formats[i].pixfmt;
    }
    return VideoFormat::Format_Invalid;
}

extern InteropResource* CreateInteropCVPixelbuffer();
extern InteropResource* CreateInteropIOSurface();
extern InteropResource* CreateInteropCVOpenGL();
extern InteropResource* CreateInteropCVOpenGLES();
InteropResource* InteropResource::create(InteropType type)
{
    if (type == InteropAuto) {
        type = InteropCVOpenGLES;
#if defined(Q_OS_MACX)
        type = InteropIOSurface;
#endif
#if defined(__builtin_available)
        if (__builtin_available(iOS 11, macOS 10.6, *))
            type = InteropIOSurface;
#endif
    }
    switch (type) {
    case InteropCVPixelBuffer: return CreateInteropCVPixelbuffer();
#if defined(Q_OS_MACX) || defined(__IPHONE_11_0)
    case InteropIOSurface: return CreateInteropIOSurface();
    //case InteropCVOpenGL: return CreateInteropCVOpenGL();
#else
    case InteropCVOpenGLES: return CreateInteropCVOpenGLES();
#endif
    default: return NULL;
    }
    return NULL;
}

InteropResource::InteropResource()
    : m_cvfmt(0)
{
    memset(m_iformat, 0, sizeof(m_iformat));
    memset(m_format, 0, sizeof(m_format));
    memset(m_dtype, 0, sizeof(m_dtype));
}

bool InteropResource::stridesForWidth(int cvfmt, int width, int *strides, VideoFormat::PixelFormat *outFmt)
{
    *outFmt = format_from_cv(cvfmt);
    switch (cvfmt) {
    case '2vuy':
    case 'yuvs': {
        if (strides[0] <= 0)
            strides[0] = 2*width;
    }
        break;
    case '420v':
    case '420f': {
        if (strides[1] <= 0)
            strides[1] = width;
    }
        break;
    case 'y420': {
        if (strides[1] <= 0)
            strides[1] = strides[2] = width/2;
    }
        break;
    default:
        return false;
    }
    if (strides[0] <= 0)
        strides[0] = width;
    return true;
}

void InteropResource::getParametersGL(OSType cvpixfmt, GLint *internalFormat, GLenum *format, GLenum *dataType, int plane)
{
    if (cvpixfmt != m_cvfmt) {
        const VideoFormat fmt(format_from_cv(cvpixfmt));
        OpenGLHelper::videoFormatToGL(fmt, m_iformat, m_format, m_dtype);
        m_cvfmt = cvpixfmt;
    }
    *internalFormat = m_iformat[plane];
    *format = m_format[plane];
    *dataType = m_dtype[plane];
}

void SurfaceInteropCV::setSurface(CVPixelBufferRef buf, int w, int h)
{
    m_surface = buf;
    CVPixelBufferRetain(buf); // videotoolbox need it for map and CVPixelBufferRelease
    frame_width = w;
    frame_height = h;
}

SurfaceInteropCV::~SurfaceInteropCV()
{
    if (m_surface)
        CVPixelBufferRelease(m_surface);
}

void* SurfaceInteropCV::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (!handle)
        return NULL;
    if (!m_surface)
        return 0;
    if (type == GLTextureSurface) {
        if (m_resource->map(m_surface, (GLuint*)handle, frame_width, frame_height, plane))
            return handle;
    } else if (type == HostMemorySurface) {
        return mapToHost(fmt, handle, plane);
    } else if (type == SourceSurface) {
        CVPixelBufferRef *b = reinterpret_cast<CVPixelBufferRef*>(handle);
        *b = m_surface;
        return handle;
    }
    return NULL;
}

void SurfaceInteropCV::unmap(void *handle)
{
    // TODO: surface type
    m_resource->unmap(m_surface, *((GLuint*)handle));
}

void* SurfaceInteropCV::createHandle(void *handle, SurfaceType type, const VideoFormat &fmt, int plane, int planeWidth, int planeHeight)
{
    if (type != GLTextureSurface)
        return NULL;
    GLuint tex = m_resource->createTexture(m_surface, fmt, plane, planeWidth, planeHeight);
    if (tex == 0)
        return NULL;
    *((GLuint*)handle) = tex;
    return handle;
}

void* SurfaceInteropCV::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    Q_UNUSED(plane);
    CVPixelBufferLockBaseAddress(m_surface, kCVPixelBufferLock_ReadOnly);
    const VideoFormat fmt(format_from_cv(CVPixelBufferGetPixelFormatType(m_surface)));
    if (!fmt.isValid()) {
        CVPixelBufferUnlockBaseAddress(m_surface, kCVPixelBufferLock_ReadOnly);
        return NULL;
    }
    const int w = CVPixelBufferGetWidth(m_surface);
    const int h = CVPixelBufferGetHeight(m_surface);
    uint8_t *src[3];
    int pitch[3];
    for (int i = 0; i < fmt.planeCount(); ++i) {
        // get address results in internal copy
        src[i] = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(m_surface, i);
        pitch[i] = CVPixelBufferGetBytesPerRowOfPlane(m_surface, i);
    }
    CVPixelBufferUnlockBaseAddress(m_surface, kCVPixelBufferLock_ReadOnly);
    //CVPixelBufferRelease(cv_buffer); // release when video frame is destroyed
    VideoFrame frame(VideoFrame::fromGPU(fmt, w, h, h, src, pitch));
    if (fmt != format)
        frame = frame.to(format);
    VideoFrame *f = reinterpret_cast<VideoFrame*>(handle);
    frame.setTimestamp(f->timestamp());
    frame.setDisplayAspectRatio(f->displayAspectRatio());
    *f = frame;
    return f;
}

/*!
 * \brief The InteropResourceCVPixelBuffer class
 * The mapping is not 0-copy. Use CVPixelBufferGetBaseAddressOfPlane to upload video frame to opengl.
 */
class InteropResourceCVPixelBuffer Q_DECL_FINAL : public InteropResource
{
public:
    bool map(CVPixelBufferRef buf, GLuint *tex, int w, int h, int plane) Q_DECL_OVERRIDE;
};

InteropResource* CreateInteropCVPixelbuffer()
{
    return new InteropResourceCVPixelBuffer();
}

bool InteropResourceCVPixelBuffer::map(CVPixelBufferRef buf, GLuint *tex, int w, int h, int plane)
{
    Q_UNUSED(h);
    Q_UNUSED(w);
    CVPixelBufferLockBaseAddress(buf, kCVPixelBufferLock_ReadOnly);
    GLint iformat;
    GLenum format, dtype;
    getParametersGL(CVPixelBufferGetPixelFormatType(buf), &iformat, &format, &dtype, plane); //TODO: call once when format changed
    const int texture_w = CVPixelBufferGetBytesPerRowOfPlane(buf, plane)/OpenGLHelper::bytesOfGLFormat(format, dtype);
    //qDebug("cv plane%d width: %d, stride: %d, tex width: %d", plane, CVPixelBufferGetWidthOfPlane(buf, plane), CVPixelBufferGetBytesPerRowOfPlane(buf, plane), texture_w);
    // get address results in internal copy
    DYGL(glBindTexture(GL_TEXTURE_2D, *tex));
    DYGL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0
                         , texture_w
                         , CVPixelBufferGetHeightOfPlane(buf, plane)
                         , format
                         , dtype
                         , (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(buf, plane)));
    CVPixelBufferUnlockBaseAddress(buf, kCVPixelBufferLock_ReadOnly);
    return true;
}
} // namespace cv
} // namespace QtAV
