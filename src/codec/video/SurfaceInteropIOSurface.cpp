/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
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

#include "SurfaceInteropCV.h"
#include <IOSurface/IOSurface.h>
#include "QtAV/VideoFrame.h"
#include "opengl/OpenGLHelper.h"

namespace QtAV {
namespace cv {
VideoFormat::PixelFormat format_from_cv(int cv);

// https://www.opengl.org/registry/specs/APPLE/rgb_422.txt
// https://www.opengl.org/registry/specs/APPLE/ycbcr_422.txt  uyvy: UNSIGNED_SHORT_8_8_REV_APPLE, yuy2: GL_UNSIGNED_SHORT_8_8_APPLE
// check extension GL_APPLE_rgb_422 and rectangle?
class InteropResourceIOSurface Q_DECL_FINAL : public InteropResource
{
public:
    bool stridesForWidth(int cvfmt, int width, int* strides, VideoFormat::PixelFormat* outFmt) Q_DECL_OVERRIDE;
    bool mapToTexture2D() const Q_DECL_OVERRIDE { return false;}
    bool map(CVPixelBufferRef buf, GLuint *tex, int w, int h, int plane) Q_DECL_OVERRIDE;
    GLuint createTexture(CVPixelBufferRef, const VideoFormat &fmt, int plane, int planeWidth, int planeHeight) Q_DECL_OVERRIDE
    {
        Q_UNUSED(fmt);
        Q_UNUSED(plane);
        Q_UNUSED(planeWidth);
        Q_UNUSED(planeHeight);
        GLuint tex = 0;
        DYGL(glGenTextures(1, &tex));
        return tex;
    }
};

InteropResource* CreateInteropIOSurface()
{
    return new InteropResourceIOSurface();
}

bool InteropResourceIOSurface::stridesForWidth(int cvfmt, int width, int *strides, VideoFormat::PixelFormat* outFmt)
{
    switch (cvfmt) {
    case '2vuy':
    case 'yuvs': {
        *outFmt = VideoFormat::Format_VYU;
        if (strides[0] <= 0)
            strides[0] = 4*width; //RGB layout: BRGX
        else
            strides[0] *= 2;
    }
        break;
    default:
        return InteropResource::stridesForWidth(cvfmt, width, strides, outFmt);
    }
    return true;
}

bool InteropResourceIOSurface::map(CVPixelBufferRef buf, GLuint *tex, int w, int h, int plane)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
    const OSType pixfmt = CVPixelBufferGetPixelFormatType(buf);
    GLint iformat;
    GLenum format, dtype;
    getParametersGL(pixfmt, &iformat, &format, &dtype, plane);
    switch (pixfmt) {
    case '2vuy':
    case 'yuvs':
        iformat = GL_RGB8; // ES2 requires internal format and format are the same. OSX can use internal format GL_RGB or sized GL_RGB8
        format = GL_RGB_422_APPLE;
        dtype = pixfmt == '2vuy' ? GL_UNSIGNED_SHORT_8_8_APPLE : GL_UNSIGNED_SHORT_8_8_REV_APPLE;
        break;
        // macOS: GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV
        // GL_YCBCR_422_APPLE: convert to rgb texture internally (bt601). only supports OSX
        // GL_RGB_422_APPLE: raw yuv422 texture
    case 'BGRA':
        iformat = GL_RGBA8;
        format = GL_BGRA;
        dtype = GL_UNSIGNED_INT_8_8_8_8_REV;
        break;
    default:
        break;
    }
    const GLenum target = GL_TEXTURE_RECTANGLE;
    DYGL(glBindTexture(target, *tex));
    const int planeW = CVPixelBufferGetWidthOfPlane(buf, plane);
    const int planeH = CVPixelBufferGetHeightOfPlane(buf, plane);
    //qDebug("map plane%d. %dx%d, gl %d %d %d", plane, planeW, planeH, iformat, format, dtype);

    const IOSurfaceRef surface  = CVPixelBufferGetIOSurface(buf);
    CGLError err = CGLTexImageIOSurface2D(CGLGetCurrentContext(), target, iformat, planeW, planeH, format, dtype, surface, plane);
    if (err != kCGLNoError) {
        qWarning("error creating IOSurface texture at plane %d: %s", plane, CGLErrorString(err));
    }
    DYGL(glBindTexture(target, 0));
    return true;
}
} // namespace cv
} // namespace QtAV
