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

#include "SurfaceInteropCV.h"
#include <CoreVideo/CVOpenGLESTextureCache.h>
#include <CoreVideo/CVOpenGLESTexture.h>
#import <OpenGLES/EAGL.h>
#include "QtAV/VideoFrame.h"
#include "utils/OpenGLHelper.h"

namespace QtAV {
namespace cv {
VideoFormat::PixelFormat format_from_cv(int cv);
// https://www.opengl.org/registry/specs/APPLE/rgb_422.txt
class InteropResourceCVOpenGLES Q_DECL_FINAL : public InteropResource
{
public:
    InteropResourceCVOpenGLES() : InteropResource()
      , texture_cache(NULL)
    {
        textures.reserve(4);
    }
    ~InteropResourceCVOpenGLES() {
        if (!textures.isEmpty()) {
            foreach (CVOpenGLESTextureRef t, textures) {
                if (t)
                    CFRelease(t);
            }
            textures.clear();
        }
        if (texture_cache) {
            CFRelease(texture_cache);
            texture_cache = NULL;
        }
    }
    bool stridesForWidth(int cvfmt, int width, int* strides, VideoFormat::PixelFormat* outFmt) Q_DECL_OVERRIDE;
    bool map(CVPixelBufferRef buf, GLuint *texInOut, int w, int h, int plane) Q_DECL_OVERRIDE;
private:
    bool ensureResource();

    CVOpenGLESTextureCacheRef texture_cache;
    QVector<CVOpenGLESTextureRef> textures;
    QVector<GLuint> tex_mapped;
};

InteropResource* CreateInteropCVOpenGLES()
{
    return new InteropResourceCVOpenGLES();
}

bool InteropResourceCVOpenGLES::stridesForWidth(int cvfmt, int width, int *strides, VideoFormat::PixelFormat* outFmt)
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

bool InteropResourceCVOpenGLES::map(CVPixelBufferRef buf, GLuint *texInOut, int w, int h, int plane)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
    if (!ensureResource())
        return false;
    GLint iformat[4]; //TODO: as member and compute only when format change
    GLenum format[4];
    GLenum dtype[4];
    const OSType pixfmt = CVPixelBufferGetPixelFormatType(buf);
    const VideoFormat fmt(format_from_cv(pixfmt));
    OpenGLHelper::videoFormatToGL(fmt, iformat, format, dtype);
    qDebug("map plane%d gl orig %d %d %d", plane, iformat[plane], format[plane], dtype[plane]);

    // TODO: move the followings to videoFormatToGL()?
    if (plane > 1 && format[2] == GL_LUMINANCE && fmt.bytesPerPixel(1) == 1) { // QtAV uses the same shader for planar and semi-planar yuv format
        iformat[2] = format[2] = GL_ALPHA;
        if (plane == 4)
            iformat[3] = format[3] = format[2]; // vec4(,,,A)
    }
    switch (pixfmt) {
    case '2vuy':
    case 'yuvs':
        iformat[plane] = GL_RGB_422_APPLE; // ES2 requires internal format and format are the same. desktop can use internal format GL_RGB or sized GL_RGB8
        format[plane] = GL_RGB_422_APPLE;
        dtype[plane] = pixfmt == '2vuy' ? GL_UNSIGNED_SHORT_8_8_APPLE : GL_UNSIGNED_SHORT_8_8_REV_APPLE;
        break;
    case 'BGRA': // GL_BGRA error?
        iformat[plane] = GL_RGBA;
        format[plane] = GL_RGBA;
        break;
    default: // TODO: rgb24
        break;
    }

    if (textures.size() <= fmt.planeCount()) {
        textures.resize(fmt.planeCount());
    }
    CVOpenGLESTextureCacheFlush(texture_cache, NULL);
    CVOpenGLESTextureRef &tex = textures[plane];
    if (tex) {
        CFRelease(tex);
        tex = NULL;
    }

    const int planeW = CVPixelBufferGetWidthOfPlane(buf, plane);
    const int planeH = CVPixelBufferGetHeightOfPlane(buf, plane);
    //const int texture_w = CVPixelBufferGetBytesPerRowOfPlane(buf, plane)/OpenGLHelper::bytesOfGLFormat(format[plane], dtype[plane]);
    //qDebug("map plane%d. %dx%d, gl %d %d %d", plane, planeW, planeH, iformat[plane], format[plane], dtype[plane]);
    CVReturn err = CVOpenGLESTextureCacheCreateTextureFromImage(
                kCFAllocatorDefault
                , texture_cache
                , buf
                , NULL
                , GL_TEXTURE_2D
                , iformat[plane]
                , planeW //Why not texture width?
                , planeH
                , format[plane], dtype[plane]
                , plane
                , &tex);
    if (err != kCVReturnSuccess) {
        qDebug("Failed to create OpenGL texture(%d)", err);
        return false;
    }
    //CVOpenGLESTextureGetCleanTexCoords
    if (tex_mapped.indexOf(*texInOut) < 0) {
        DYGL(glDeleteTextures(1, texInOut));
        qDebug("delete texture generated from VideoShader: %u", *texInOut);
    } else {
        tex_mapped.removeAll(*texInOut);
    }
    *texInOut = CVOpenGLESTextureGetName(tex);
    tex_mapped.append(*texInOut);
    DYGL(glBindTexture(GL_TEXTURE_2D, *texInOut));
    DYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    DYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    // This is necessary for non-power-of-two textures
    DYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    DYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)); //TODO: remove. only call once
    return true;
}

bool InteropResourceCVOpenGLES::ensureResource()
{
    if (texture_cache)
        return true;
    CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, [EAGLContext currentContext], NULL, &texture_cache);
    if (err != kCVReturnSuccess) {
        qWarning("Failed to create OpenGL ES texture Cache(%d)", err);
        return false;
    }
    return true;
}
} // namespace cv
} // namespace QtAV
