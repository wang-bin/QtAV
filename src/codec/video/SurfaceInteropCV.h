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

#ifndef QTAV_SURFACEINTEROPCV_H
#define QTAV_SURFACEINTEROPCV_H

#include <QtCore/qglobal.h>
#include <CoreVideo/CoreVideo.h>
#include <QtAV/SurfaceInterop.h>

namespace QtAV {
namespace cv {

VideoFormat::PixelFormat format_from_cv(int cv);

typedef uint32_t GLuint; // define here to avoid including gl headers which are not required by decoder
typedef uint32_t GLenum;
typedef int32_t  GLint;

// FIXME: not texture if change from InteropCVOpenGLES to InteropCVPixelBuffer
enum InteropType {
    InteropCVPixelBuffer,   // macOS+ios
    InteropIOSurface,       // macOS
    InteropCVOpenGL,        // macOS, not implemented
    InteropCVOpenGLES,       // ios
    InteropAuto
};

class InteropResource
{
public:
    InteropResource();
    // Must have CreateInteropXXX in each implemention
    static InteropResource* create(InteropType type);
    virtual ~InteropResource() {}
    /*!
     * \brief stridesForWidth
     * The stride used by opengl can be different in some interop because frame display format can change (outFmt), for example we can use rgb for uyvy422. The default value use the origial pixel format to comupte the strides.
     * If the input strides has a valid value, use this value to compute the output strides. Otherwise, use width to compute the output strides without taking care of padded data size.
     */
    virtual bool stridesForWidth(int cvfmt, int width, int* strides/*in_out*/, VideoFormat::PixelFormat* outFmt);
    virtual bool mapToTexture2D() const { return true;}
    // egl supports yuv extension
    /*!
     * \brief map
     * \param buf vt decoded buffer
     * \param texInOut opengl texture. You can modify it
     * \param w frame width(visual width) without alignment, <= dxva surface width
     * \param h frame height(visual height)
     * \param plane useless now
     * \return true if success
     */
    virtual bool map(CVPixelBufferRef buf, GLuint *texInOut, int w, int h, int plane) = 0;
    virtual bool unmap(CVPixelBufferRef buf, GLuint tex) {
        Q_UNUSED(buf);
        Q_UNUSED(tex);
        return true;
    }
    virtual GLuint createTexture(CVPixelBufferRef, const VideoFormat &fmt, int plane, int planeWidth, int planeHeight) {
        Q_UNUSED(fmt);
        Q_UNUSED(plane);
        Q_UNUSED(planeWidth);
        Q_UNUSED(planeHeight);
        return 0;
    }
    void getParametersGL(OSType cvpixfmt, GLint* internalFormat, GLenum* format, GLenum* dataType, int plane = 0);
private:
    OSType m_cvfmt;
    GLint m_iformat[4];
    GLenum m_format[4];
    GLenum m_dtype[4];
};
typedef QSharedPointer<InteropResource> InteropResourcePtr;

class SurfaceInteropCV Q_DECL_FINAL: public VideoSurfaceInterop
{
public:
    SurfaceInteropCV(const InteropResourcePtr& res) : frame_width(0), frame_height(0), m_resource(res) {}
    ~SurfaceInteropCV();
    void setSurface(CVPixelBufferRef buf, int w, int h);
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
    virtual void* createHandle(void* handle, SurfaceType type, const VideoFormat &fmt, int plane, int planeWidth, int planeHeight) Q_DECL_OVERRIDE;
protected:
    void* mapToHost(const VideoFormat &format, void *handle, int plane);
private:
    int frame_width, frame_height;
    InteropResourcePtr m_resource;
    CVPixelBufferRef m_surface;
};
} // namespace cv
} // namespace QtAV
#endif //QTAV_SURFACEINTEROPCV_H
