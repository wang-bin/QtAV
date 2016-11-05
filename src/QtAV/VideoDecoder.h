/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
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

#ifndef QTAV_VIDEODECODER_H
#define QTAV_VIDEODECODER_H

#include <QtAV/AVDecoder.h>
#include <QtAV/VideoFrame.h>
#include <QtCore/QStringList>

namespace QtAV {
typedef int VideoDecoderId;
/*!
    Useful properties.
    A key is a string, a value can be int, bool or string. Both int and string are valid for enumerate
    properties. Flag properties must use int if more than 1 value is used.
    e.g. decoder->setProperty("display", 1) equals decoder->setProperty("display", "GLX")
    setOptions() also applies the properties.
    avcodec (also for VA-API, DXVA, VDA)
      Use AVCodecContext options
    CUDA
      surfaces: 0 is auto
      deinterlace: 0 "Weave", 1 "Bob", 2 "Adaptive"
    VA-API
      display: 0 "X11", 1 "GLX", 2 "DRM"
    DXVA, VA-API
      surfaces: 0 default
    DXVA, VA-API, VDA:
      sse4: bool
    CedarV
      neon: bool
    FFmpeg
      skip_loop_filter, skip_idct, skip_frame: -16 "None", 0: "Default", 8 "NoRef", 16 "Bidir", 32 "NoKey", 64 "All"
      threads: int, 0 is auto
      vismv(motion vector visualization): flag, 0 "NO", 1 "PF", 2 "BF", 4 "BB"
 */

class VideoDecoderPrivate;
class Q_AV_EXPORT VideoDecoder : public AVDecoder
{
    Q_DISABLE_COPY(VideoDecoder)
    DPTR_DECLARE_PRIVATE(VideoDecoder)
public:
    static QStringList supportedCodecs();
    static VideoDecoder* create(VideoDecoderId id);
    /*!
     * \brief create
     * create a decoder from registered name. FFmpeg decoder will be created for empty name
     * \param name can be "FFmpeg", "CUDA", "VDA", "VAAPI", "DXVA", "Cedarv"
     * \return 0 if not registered
     */
    static VideoDecoder* create(const char* name = "FFmpeg");
    virtual VideoDecoderId id() const = 0;
    QString name() const; //name from factory
    virtual VideoFrame frame() = 0;
public:
    typedef int Id;
    static QVector<VideoDecoderId> registered();
    template<class C> static bool Register(VideoDecoderId id, const char* name) { return Register(id, create<C>, name);}
    /*!
     * \brief next
     * \param id NULL to get the first id address
     * \return address of id or NULL if not found/end
     */
    static VideoDecoderId* next(VideoDecoderId* id = 0);
    static const char* name(VideoDecoderId id);
    static VideoDecoderId id(const char* name);
private:
    template<class C> static VideoDecoder* create() { return new C();}
    typedef VideoDecoder* (*VideoDecoderCreator)();
protected:
    static bool Register(VideoDecoderId id, VideoDecoderCreator, const char *name);
protected:
    VideoDecoder(VideoDecoderPrivate& d);
private:
    VideoDecoder();
};

extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_FFmpeg;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_CUDA;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_DXVA;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_D3D11;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_VAAPI;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_Cedarv;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_VDA;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_VideoToolbox;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_MediaCodec;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_MMAL;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_QSV;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_CrystalHD;
} //namespace QtAV
#endif // QTAV_VIDEODECODER_H
