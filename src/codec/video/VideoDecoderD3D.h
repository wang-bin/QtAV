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

#ifndef QTAV_VIDEODECODERD3D_H
#define QTAV_VIDEODECODERD3D_H

#include <windows.h>
#include <inttypes.h>
#include "VideoDecoderFFmpegHW.h"
#include "VideoDecoderFFmpegHW_p.h"

namespace QtAV {

bool isHEVCSupported();

struct dxva2_mode_t;
typedef int D3DFormat;
struct d3d_format_t {
    const char    *name;
    D3DFormat     format;
    VideoFormat::PixelFormat pixfmt;
};
bool checkProfile(const dxva2_mode_t* mode, int profile);
const dxva2_mode_t *Dxva2FindMode(const GUID *guid);
bool isIntelClearVideo(const GUID *guid);
bool isNoEncrypt(const GUID* guid);
D3DFormat select_d3d_format(D3DFormat *formats, UINT nb_formats);
VideoFormat::PixelFormat pixelFormatFromD3D(int format);

class VideoDecoderD3DPrivate;
class VideoDecoderD3D : public VideoDecoderFFmpegHW
{
    DPTR_DECLARE_PRIVATE(VideoDecoderD3D)
    Q_OBJECT
    Q_PROPERTY(int surfaces READ surfaces WRITE setSurfaces NOTIFY surfacesChanged)
public:
    // properties
    void setSurfaces(int num);
    int surfaces() const;
Q_SIGNALS:
    void surfacesChanged();
protected:
    VideoDecoderD3D(VideoDecoderD3DPrivate& d);
};

class VideoDecoderD3DPrivate : public VideoDecoderFFmpegHWPrivate
{
public:
    VideoDecoderD3DPrivate();

    bool setup(AVCodecContext *avctx) Q_DECL_OVERRIDE;

    int aligned(int x);
    virtual void setupAVVAContext(AVCodecContext* avctx) = 0;
    /// width and height are coded value, maybe not aligned for d3d surface
    virtual bool ensureResources(AVCodecID codec, int width, int height, int nb_surfaces) = 0;
    virtual void releaseResources() = 0;
    virtual D3DFormat formatFor(const GUID *guid) const = 0;
    const d3d_format_t *getFormat(const GUID *supported_guids, UINT nb_guids, GUID *selected) const;


    // set by user. don't reset in when call destroy
    bool surface_auto;
    int surface_count;

    int surface_width;
    int surface_height;
};

} //namespace QtAV
#endif //QTAV_VIDEODECODERD3D_H
