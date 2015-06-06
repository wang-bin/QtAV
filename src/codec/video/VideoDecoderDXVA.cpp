/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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
/// egl support is added by: Andy Bell <andy.bell@displaynote.com>

#ifdef _MSC_VER
#pragma comment(lib, "ole32.lib") //CoTaskMemFree. why link failed?
#endif
#include "VideoDecoderFFmpegHW.h"
#include "VideoDecoderFFmpegHW_p.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/prepost.h"
//#include "QtAV/private/mkid.h"
#include "utils/Logger.h"
#include "SurfaceInteropDXVA.h"


// d3d9ex: http://dxr.mozilla.org/mozilla-central/source/dom/media/wmf/DXVA2Manager.cpp
// TODO: add to QtAV_Compat.h?
// FF_API_PIX_FMT
#ifdef PixelFormat
#undef PixelFormat
#endif
// AV_CODEC_ID_H265 is a macro defined as AV_CODEC_ID_HEVC. so we can avoid libavcodec version check. (from ffmpeg 2.1)
#ifndef AV_CODEC_ID_H265
#ifdef _MSC_VER
#pragma message("HEVC will not be supported. Update your FFmpeg")
#else
#warning "HEVC will not be supported. Update your FFmpeg"
#endif
#define AV_CODEC_ID_H265 AV_CODEC_ID_NONE //mkid::fourcc<'H','2','6','5'>::value
#define AV_CODEC_ID_HEVC AV_CODEC_ID_NONE
#define FF_PROFILE_HEVC_MAIN -1
#define FF_PROFILE_HEVC_MAIN_10 -1
#endif

template <class T> void SafeRelease(T **ppT)
{
  if (*ppT) {
    (*ppT)->Release();
    *ppT = NULL;
  }
}

#define DX_LOG_COMPONENT "DXVA2"

#ifndef DX_LOG_COMPONENT
#define DX_LOG_COMPONENT "DirectX"
#endif //DX_LOG_COMPONENT
#define DX_ENSURE_OK(f, ...) \
    do { \
        HRESULT hr = f; \
        if (FAILED(hr)) { \
            qWarning() << DX_LOG_COMPONENT " error@" << __LINE__ << ". " #f ": " << QString("(0x%1) ").arg(hr, 0, 16) << qt_error_string(hr); \
            return __VA_ARGS__; \
        } \
    } while (0)

// to use c api, add define COBJMACROS and CINTERFACE
#define DXVA2API_USE_BITFIELDS
extern "C" {
#include <libavcodec/dxva2.h> //will include d3d9.h, dxva2api.h
}
#define VA_DXVA2_MAX_SURFACE_COUNT (64)

#include <d3d9.h>
#include <dxva2api.h>

#include <initguid.h> /* must be last included to not redefine existing GUIDs */

/* dxva2api.h GUIDs: http://msdn.microsoft.com/en-us/library/windows/desktop/ms697067(v=vs100).aspx
 * assume that they are declared in dxva2api.h */
//#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) ///TODO: ???

#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = { l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}
#ifdef __MINGW32__
# include <_mingw.h>

# if !defined(__MINGW64_VERSION_MAJOR)
#  undef MS_GUID
#  define MS_GUID DEFINE_GUID /* dxva2api.h fails to declare those, redefine as static */
#  define DXVA2_E_NEW_VIDEO_DEVICE MAKE_HRESULT(1, 4, 4097)
# else
#  include <dxva.h>
# endif

#endif /* __MINGW32__ */

namespace QtAV {

// some MS_GUID are defined in mingw but some are not. move to namespace and define all is ok
MS_GUID(IID_IDirectXVideoDecoderService, 0xfc51a551, 0xd5e7, 0x11d9, 0xaf,0x55,0x00,0x05,0x4e,0x43,0xff,0x02);
MS_GUID(IID_IDirectXVideoAccelerationService, 0xfc51a550, 0xd5e7, 0x11d9, 0xaf,0x55,0x00,0x05,0x4e,0x43,0xff,0x02);
MS_GUID(IID_IDirect3DDevice9Ex, 0xb18b10ce, 0x2649, 0x405a, 0x87, 0xf, 0x95, 0xf7, 0x77, 0xd4, 0x31, 0x3a);

MS_GUID    (DXVA_NoEncrypt,                         0x1b81bed0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

/* Codec capabilities GUID, sorted by codec */
MS_GUID    (DXVA2_ModeMPEG2_MoComp,                 0xe6a9f44b, 0x61b0, 0x4563, 0x9e, 0xa4, 0x63, 0xd2, 0xa3, 0xc6, 0xfe, 0x66);
MS_GUID    (DXVA2_ModeMPEG2_IDCT,                   0xbf22ad00, 0x03ea, 0x4690, 0x80, 0x77, 0x47, 0x33, 0x46, 0x20, 0x9b, 0x7e);
MS_GUID    (DXVA2_ModeMPEG2_VLD,                    0xee27417f, 0x5e28, 0x4e65, 0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9);
DEFINE_GUID(DXVA2_ModeMPEG2and1_VLD,                0x86695f12, 0x340e, 0x4f04, 0x9f, 0xd3, 0x92, 0x53, 0xdd, 0x32, 0x74, 0x60);
DEFINE_GUID(DXVA2_ModeMPEG1_VLD,                    0x6f3ec719, 0x3735, 0x42cc, 0x80, 0x63, 0x65, 0xcc, 0x3c, 0xb3, 0x66, 0x16);

MS_GUID    (DXVA2_ModeH264_A,                       0x1b81be64, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeH264_B,                       0x1b81be65, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeH264_C,                       0x1b81be66, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeH264_D,                       0x1b81be67, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeH264_E,                       0x1b81be68, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeH264_F,                       0x1b81be69, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH264_VLD_Multiview,            0x9901CCD3, 0xca12, 0x4b7e, 0x86, 0x7a, 0xe2, 0x22, 0x3d, 0x92, 0x55, 0xc3); // MVC
DEFINE_GUID(DXVA_ModeH264_VLD_WithFMOASO_NoFGT,     0xd5f04ff9, 0x3418, 0x45d8, 0x95, 0x61, 0x32, 0xa7, 0x6a, 0xae, 0x2d, 0xdd);
DEFINE_GUID(DXVADDI_Intel_ModeH264_A,               0x604F8E64, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_C,               0x604F8E66, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E,               0x604F8E68, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6); // DXVA_Intel_H264_NoFGT_ClearVideo
DEFINE_GUID(DXVA_ModeH264_VLD_NoFGT_Flash,          0x4245F676, 0x2BBC, 0x4166, 0xa0, 0xBB, 0x54, 0xE7, 0xB8, 0x49, 0xC3, 0x80);

MS_GUID    (DXVA2_ModeWMV8_A,                       0x1b81be80, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeWMV8_B,                       0x1b81be81, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

MS_GUID    (DXVA2_ModeWMV9_A,                       0x1b81be90, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeWMV9_B,                       0x1b81be91, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeWMV9_C,                       0x1b81be94, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

MS_GUID    (DXVA2_ModeVC1_A,                        0x1b81beA0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeVC1_B,                        0x1b81beA1, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeVC1_C,                        0x1b81beA2, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID    (DXVA2_ModeVC1_D,                        0x1b81beA3, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeVC1_D2010,                    0x1b81beA4, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5); // August 2010 update
DEFINE_GUID(DXVA_Intel_VC1_ClearVideo,              0xBCC5DB6D, 0xA2B6, 0x4AF0, 0xAC, 0xE4, 0xAD, 0xB1, 0xF7, 0x87, 0xBC, 0x89);
DEFINE_GUID(DXVA_Intel_VC1_ClearVideo_2,            0xE07EC519, 0xE651, 0x4CD6, 0xAC, 0x84, 0x13, 0x70, 0xCC, 0xEE, 0xC8, 0x51);

DEFINE_GUID(DXVA_nVidia_MPEG4_ASP,                  0x9947EC6F, 0x689B, 0x11DC, 0xA3, 0x20, 0x00, 0x19, 0xDB, 0xBC, 0x41, 0x84);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_Simple,           0xefd64d74, 0xc9e8, 0x41d7, 0xa5, 0xe9, 0xe9, 0xb0, 0xe3, 0x9f, 0xa3, 0x19);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_AdvSimple_NoGMC,  0xed418a9f, 0x010d, 0x4eda, 0x9a, 0xe3, 0x9a, 0x65, 0x35, 0x8d, 0x8d, 0x2e);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_AdvSimple_GMC,    0xab998b5b, 0x4258, 0x44a9, 0x9f, 0xeb, 0x94, 0xe5, 0x97, 0xa6, 0xba, 0xae);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_AdvSimple_Avivo,  0x7C74ADC6, 0xe2ba, 0x4ade, 0x86, 0xde, 0x30, 0xbe, 0xab, 0xb4, 0x0c, 0xc1);

// HEVC
DEFINE_GUID(DXVA_ModeHEVC_VLD_Main,                 0x5b11d51b, 0x2f4c, 0x4452, 0xbc, 0xc3, 0x9, 0xf2, 0xa1, 0x16, 0xc, 0xc0);
DEFINE_GUID(DXVA_ModeHEVC_VLD_Main10,               0x107af0e0, 0xef1a, 0x4d19, 0xab, 0xa8, 0x67, 0xa1, 0x63, 0x7, 0x3d, 0x13);

class VideoDecoderDXVAPrivate;
class VideoDecoderDXVA : public VideoDecoderFFmpegHW
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderDXVA)
    Q_PROPERTY(int surfaces READ surfaces WRITE setSurfaces)
public:
    VideoDecoderDXVA();
    virtual VideoDecoderId id() const;
    virtual QString description() const;
    virtual VideoFrame frame();
    // properties
    void setSurfaces(int num);
    int surfaces() const;
};

extern VideoDecoderId VideoDecoderId_DXVA;
FACTORY_REGISTER_ID_AUTO(VideoDecoder, DXVA, "DXVA")

void RegisterVideoDecoderDXVA_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoDecoder, DXVA, "DXVA")
}

typedef struct {
    IDirect3DSurface9 *d3d;
    int refcount;
    unsigned int order;
} va_surface_t;
/* */
typedef struct {
    const char   *name;
    const GUID   *guid;
    int          codec;
} dxva2_mode_t;
/* XXX Prefered modes must come first */
static const dxva2_mode_t dxva2_modes[] = {
    /* MPEG-1/2 */
    { "MPEG-2 variable-length decoder",                                               &DXVA2_ModeMPEG2_VLD,                   QTAV_CODEC_ID(MPEG2VIDEO) },
    { "MPEG-2 & MPEG-1 variable-length decoder",                                      &DXVA2_ModeMPEG2and1_VLD,               QTAV_CODEC_ID(MPEG2VIDEO) },
    { "MPEG-2 motion compensation",                                                   &DXVA2_ModeMPEG2_MoComp,                0 },
    { "MPEG-2 inverse discrete cosine transform",                                     &DXVA2_ModeMPEG2_IDCT,                  0 },

    { "MPEG-1 variable-length decoder",                                               &DXVA2_ModeMPEG1_VLD,                   0 },

    /* H.264 */
    { "H.264 variable-length decoder, film grain technology",                         &DXVA2_ModeH264_F,                      QTAV_CODEC_ID(H264) },
    { "H.264 variable-length decoder, no film grain technology",                      &DXVA2_ModeH264_E,                      QTAV_CODEC_ID(H264) },
    { "H.264 variable-length decoder, no film grain technology (Intel ClearVideo)",   &DXVADDI_Intel_ModeH264_E,              QTAV_CODEC_ID(H264) },
    { "H.264 variable-length decoder, no film grain technology, FMO/ASO",             &DXVA_ModeH264_VLD_WithFMOASO_NoFGT,    QTAV_CODEC_ID(H264) },
    { "H.264 variable-length decoder, no film grain technology, Flash",               &DXVA_ModeH264_VLD_NoFGT_Flash,         QTAV_CODEC_ID(H264) },

    { "H.264 inverse discrete cosine transform, film grain technology",               &DXVA2_ModeH264_D,                      0 },
    { "H.264 inverse discrete cosine transform, no film grain technology",            &DXVA2_ModeH264_C,                      0 },
    { "H.264 inverse discrete cosine transform, no film grain technology (Intel)",    &DXVADDI_Intel_ModeH264_C,              0 },

    { "H.264 motion compensation, film grain technology",                             &DXVA2_ModeH264_B,                      0 },
    { "H.264 motion compensation, no film grain technology",                          &DXVA2_ModeH264_A,                      0 },
    { "H.264 motion compensation, no film grain technology (Intel)",                  &DXVADDI_Intel_ModeH264_A,              0 },

    /* WMV */
    { "Windows Media Video 8 motion compensation",                                    &DXVA2_ModeWMV8_B,                      0 },
    { "Windows Media Video 8 post processing",                                        &DXVA2_ModeWMV8_A,                      0 },

    { "Windows Media Video 9 IDCT",                                                   &DXVA2_ModeWMV9_C,                      0 },
    { "Windows Media Video 9 motion compensation",                                    &DXVA2_ModeWMV9_B,                      0 },
    { "Windows Media Video 9 post processing",                                        &DXVA2_ModeWMV9_A,                      0 },

    /* VC-1 */
    //https://github.com/afedchin/xbmc/commit/dd4dd69528e10696f8b1b23367d6630adc01e618
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D2010,                   QTAV_CODEC_ID(VC1) },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D2010,                   QTAV_CODEC_ID(WMV3) },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D,                       QTAV_CODEC_ID(VC1) },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D,                       QTAV_CODEC_ID(WMV3) },
    { "VC-1 variable-length decoder 2 (Intel)",                                       &DXVA_Intel_VC1_ClearVideo_2,           0 },
    { "VC-1 variable-length decoder (Intel)",                                         &DXVA_Intel_VC1_ClearVideo,             0 },

    { "VC-1 inverse discrete cosine transform",                                       &DXVA2_ModeVC1_C,                       0 },
    { "VC-1 motion compensation",                                                     &DXVA2_ModeVC1_B,                       0 },
    { "VC-1 post processing",                                                         &DXVA2_ModeVC1_A,                       0 },

    /* Xvid/Divx: TODO */
    { "MPEG-4 Part 2 nVidia bitstream decoder",                                       &DXVA_nVidia_MPEG4_ASP,                 0 },
    { "MPEG-4 Part 2 variable-length decoder, Simple Profile",                        &DXVA_ModeMPEG4pt2_VLD_Simple,          0 },
    { "MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, no GMC",       &DXVA_ModeMPEG4pt2_VLD_AdvSimple_NoGMC, 0 },
    { "MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, GMC",          &DXVA_ModeMPEG4pt2_VLD_AdvSimple_GMC,   0 },
    { "MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, Avivo",        &DXVA_ModeMPEG4pt2_VLD_AdvSimple_Avivo, 0 },

    /* HEVC / H.265 */
    { "HEVC / H.265 variable-length decoder, main",                                   &DXVA_ModeHEVC_VLD_Main,                QTAV_CODEC_ID(HEVC) },
    { "HEVC / H.265 variable-length decoder, main10",                                 &DXVA_ModeHEVC_VLD_Main10,              QTAV_CODEC_ID(HEVC) },

    { NULL, 0, 0 }
};

static const dxva2_mode_t *Dxva2FindMode(const GUID *guid)
{
    for (unsigned i = 0; dxva2_modes[i].name; i++) {
        if (IsEqualGUID(*dxva2_modes[i].guid, *guid))
            return &dxva2_modes[i];
    }
    return NULL;
}

typedef struct {
    const char    *name;
    D3DFORMAT     format;
    AVPixelFormat avpixfmt;
} d3d_format_t;
/* XXX Prefered format must come first */
//16-bit: https://msdn.microsoft.com/en-us/library/windows/desktop/bb970578(v=vs.85).aspx
static const d3d_format_t d3d_formats[] = {
    { "YV12",   (D3DFORMAT)MAKEFOURCC('Y','V','1','2'),    QTAV_PIX_FMT_C(YUV420P) },
    { "NV12",   (D3DFORMAT)MAKEFOURCC('N','V','1','2'),    QTAV_PIX_FMT_C(NV12) },
    { "IMC3",   (D3DFORMAT)MAKEFOURCC('I','M','C','3'),    QTAV_PIX_FMT_C(YUV420P) },
    { "P010",   (D3DFORMAT)MAKEFOURCC('P','0','1','0'),    QTAV_PIX_FMT_C(YUV420P10LE) },
    { "P016",   (D3DFORMAT)MAKEFOURCC('P','0','1','6'),    QTAV_PIX_FMT_C(YUV420P16LE) },
    { NULL, D3DFMT_UNKNOWN, QTAV_PIX_FMT_C(NONE) }
};

static const d3d_format_t *D3dFindFormat(D3DFORMAT format)
{
    for (unsigned i = 0; d3d_formats[i].name; i++) {
        if (d3d_formats[i].format == format)
            return &d3d_formats[i];
    }
    return NULL;
}

static const char* getVendorName(D3DADAPTER_IDENTIFIER9 *id) //vlc_va_dxva2_t *va
{
    static const struct {
        unsigned id;
        char     name[32];
    } vendors [] = {
        { 0x1002, "ATI" },
        { 0x10DE, "NVIDIA" },
        { 0x1106, "VIA" },
        { 0x8086, "Intel" },
        { 0x5333, "S3 Graphics" },
        { 0, "" }
    };
    const char *vendor = "Unknown";
    for (int i = 0; vendors[i].id != 0; i++) {
        if (vendors[i].id == id->VendorId) {
            vendor = vendors[i].name;
            break;
        }
    }
    return vendor;
}


class VideoDecoderDXVAPrivate : public VideoDecoderFFmpegHWPrivate
{
public:
    VideoDecoderDXVAPrivate():
        VideoDecoderFFmpegHWPrivate()
    {
        // TODO: qt5.4 mingw dygl can not resolve egl symbols and crash
#if defined(_MSC_VER)
        if (OpenGLHelper::isOpenGLES())
            copy_mode = VideoDecoderFFmpegHW::ZeroCopy;
#endif
        hd3d9_dll = 0;
        hdxva2_dll = 0;
        d3dobj = 0;
        d3dobj_ex = 0;
        d3ddev = 0;
        token = 0;
        devmng = 0;
        device = 0;
        vs = 0;
        render = D3DFMT_UNKNOWN;
        decoder = 0;
        surface_order = 0;
        surface_width = surface_height = 0;
        available = loadDll();
        // set by user. don't reset in when call destroy
        surface_auto = true;
        surface_count = 0;
        ZeroMemory(&d3dai, sizeof(d3dai));
    }
    virtual ~VideoDecoderDXVAPrivate()
    {
        unloadDll();
    }

    bool loadDll();
    bool unloadDll();
    bool D3dCreateDevice();
    bool D3dCreateDeviceEx();
    bool D3dCreateDeviceFallback();
    void D3dDestroyDevice();
    bool D3dCreateDeviceManager();
    void D3dDestroyDeviceManager();
    bool DxCreateVideoService();
    void DxDestroyVideoService();
    bool DxFindVideoServiceConversion(GUID *input, D3DFORMAT *output);
    bool DxCreateVideoDecoder(int codec_id, int w, int h);
    void DxDestroyVideoDecoder();
    bool DxResetVideoDecoder();
    bool isHEVCSupported() const;

    bool setup(AVCodecContext *avctx);
    bool open();
    void close();
    // get aligned value depending on codec
    int aligned(int x);

    bool getBuffer(void **opaque, uint8_t **data);
    void releaseBuffer(void *opaque, uint8_t *data);
    AVPixelFormat vaPixelFormat() const { return QTAV_PIX_FMT_C(DXVA2_VLD);}
    /* DLL */
    HINSTANCE hd3d9_dll;
    HINSTANCE hdxva2_dll;

    /* Direct3D */
    IDirect3D9 *d3dobj;
    IDirect3D9Ex *d3dobj_ex;
    D3DADAPTER_IDENTIFIER9 d3dai;
    IDirect3DDevice9 *d3ddev; // can be Ex
    /* Device manager */
    UINT                     token;
    IDirect3DDeviceManager9  *devmng;
    HANDLE                   device;

    /* Video service */
    IDirectXVideoDecoderService  *vs;
    GUID                         input;
    D3DFORMAT                    render;

    /* Video decoder */
    DXVA2_ConfigPictureDecode    cfg;
    IDirectXVideoDecoder         *decoder;

    struct dxva_context hw_ctx;
    bool surface_auto;
    unsigned     surface_count;
    unsigned     surface_order;
    int          surface_width;
    int          surface_height;
    //AVPixelFormat surface_chroma;

    va_surface_t surfaces[VA_DXVA2_MAX_SURFACE_COUNT];
    IDirect3DSurface9* hw_surfaces[VA_DXVA2_MAX_SURFACE_COUNT];

    QString vendor;
#if QTAV_HAVE(DXVA_EGL)
    dxva::InteropResourcePtr interop_res; //may be still used in video frames when decoder is destroyed
#endif //QTAV_HAVE(DXVA_EGL)
};

VideoDecoderDXVA::VideoDecoderDXVA()
    : VideoDecoderFFmpegHW(*new VideoDecoderDXVAPrivate())
{
    // dynamic properties about static property details. used by UI
    // format: detail_property
    setProperty("detail_surfaces", tr("Decoding surfaces.") + " " + tr("0: auto"));
}

VideoDecoderId VideoDecoderDXVA::id() const
{
    return VideoDecoderId_DXVA;
}

QString VideoDecoderDXVA::description() const
{
    DPTR_D(const VideoDecoderDXVA);
    if (!d.description.isEmpty())
        return d.description;
    return "DirectX Video Acceleration";
}

VideoFrame VideoDecoderDXVA::frame()
{
    DPTR_D(VideoDecoderDXVA);
    if (!d.frame->opaque || !d.frame->data[0])
        return VideoFrame();
    if (d.width <= 0 || d.height <= 0 || !d.codec_ctx)
        return VideoFrame();

    IDirect3DSurface9 *d3d = (IDirect3DSurface9*)(uintptr_t)d.frame->data[3];
#if QTAV_HAVE(DXVA_EGL)
    if (copyMode() == ZeroCopy) {
        dxva::SurfaceInteropDXVA *interop = new dxva::SurfaceInteropDXVA(d.interop_res);
        interop->setSurface(d3d);
        VideoFrame f(d.width, d.height, VideoFormat::Format_RGB32); //p->width()
        f.setBytesPerLine(d.width * 4); //used by gl to compute texture size
        f.setMetaData("surface_interop", QVariant::fromValue(VideoSurfaceInteropPtr(interop)));
        f.setTimestamp(d.frame->pkt_pts);
        return f;
    }
#endif //QTAV_HAVE(DXVA_EGL)
    class ScopedD3DLock {
        IDirect3DSurface9 *mpD3D;
    public:
        ScopedD3DLock(IDirect3DSurface9* d3d, D3DLOCKED_RECT *rect) : mpD3D(d3d) {
            if (FAILED(mpD3D->LockRect(rect, NULL, D3DLOCK_READONLY))) {
                qWarning("Failed to lock surface");
                mpD3D = 0;
            }
        }
        ~ScopedD3DLock() {
            if (mpD3D)
                mpD3D->UnlockRect();
        }
    };

    //picth >= desc.Width
    //D3DSURFACE_DESC desc;
    //d3d->GetDesc(&desc);
    D3DLOCKED_RECT lock;
    ScopedD3DLock(d3d, &lock);
    if (lock.Pitch == 0) {
        return VideoFrame();
    }

    const VideoFormat fmt = VideoFormat((int)D3dFindFormat(d.render)->avpixfmt);
    if (!fmt.isValid()) {
        qWarning("unsupported dxva pixel format: %#x", d.render);
        return VideoFrame();
    }
    //YV12 need swap, not imc3?
    // imc3 U V pitch == Y pitch, but half of the U/V plane is space. we convert to yuv420p here
    // nv12 bpp(1)==1
    // 3rd plane is not used for nv12
    int pitch[3] = { lock.Pitch, 0, 0}; //compute chroma later
    uint8_t *src[] = { (uint8_t*)lock.pBits, 0, 0}; //compute chroma later
    const bool swap_uv = d.render ==  MAKEFOURCC('I','M','C','3');
    return copyToFrame(fmt, d.surface_height, src, pitch, swap_uv);
}

void VideoDecoderDXVA::setSurfaces(int num)
{
    DPTR_D(VideoDecoderDXVA);
    d.surface_count = num;
    d.surface_auto = num <= 0;
}

int VideoDecoderDXVA::surfaces() const
{
    return d_func().surface_count;
}

/* FIXME it is nearly common with VAAPI */
bool VideoDecoderDXVAPrivate::getBuffer(void **opaque, uint8_t **data)//vlc_va_t *external, AVFrame *ff)
{
    // check pix fmt DXVA2_VLD and h264 profile?
    HRESULT hr = devmng->TestDevice(device);
    if (hr == DXVA2_E_NEW_VIDEO_DEVICE) {
        if (!DxResetVideoDecoder())
            return false;
    } else if (FAILED(hr)) {
        qWarning() << "IDirect3DDeviceManager9.TestDevice (" << hr << "): " << qt_error_string(hr);
        return false;
    }
    /* Grab an unused surface, in case none are, try the oldest
     * XXX using the oldest is a workaround in case a problem happens with libavcodec */
    unsigned i, old;
    for (i = 0, old = 0; i < surface_count; i++) {
        va_surface_t *surface = &surfaces[i];
        if (!surface->refcount)
            break;
        if (surface->order < surfaces[old].order)
            old = i;
    }
    if (i >= surface_count)
        i = old;
    va_surface_t *surface = &surfaces[i];
    surface->refcount = 1;
    surface->order = surface_order++;
    *data = (uint8_t*)surface->d3d;/* Yummie */
    *opaque = surface;
    return true;
}

//(void *opaque, uint8_t *data)
void VideoDecoderDXVAPrivate::releaseBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(data);
    va_surface_t *surface = (va_surface_t*)opaque;
    surface->refcount--;
}

bool VideoDecoderDXVAPrivate::loadDll() {
    hd3d9_dll = LoadLibrary(TEXT("D3D9.DLL"));
    if (!hd3d9_dll) {
        qWarning("cannot load d3d9.dll");
        return false;
    }
    hdxva2_dll = LoadLibrary(TEXT("DXVA2.DLL"));
    if (!hdxva2_dll) {
        qWarning("cannot load dxva2.dll");
        FreeLibrary(hd3d9_dll);
        return false;
    }
    return true;
}

bool VideoDecoderDXVAPrivate::unloadDll() {
    if (hdxva2_dll)
        FreeLibrary(hdxva2_dll);
    if (hd3d9_dll)
        FreeLibrary(hd3d9_dll);
    return true;
}

bool VideoDecoderDXVAPrivate::D3dCreateDevice()
{
    if (D3dCreateDeviceEx())
        return true;
    return D3dCreateDeviceFallback();
}

bool VideoDecoderDXVAPrivate::D3dCreateDeviceEx()
{
    //http://msdn.microsoft.com/en-us/library/windows/desktop/bb219676(v=vs.85).aspx
    typedef HRESULT (WINAPI *Create9ExFunc)(UINT SDKVersion, IDirect3D9Ex **ppD3D); //IDirect3D9Ex: void is ok
    Create9ExFunc Create9Ex = (Create9ExFunc)GetProcAddress(hd3d9_dll, "Direct3DCreate9Ex");
    if (!Create9Ex) {
        qWarning("Does not support Direct3DCreate9Ex");
        return false;
    }
    if (FAILED(Create9Ex(D3D_SDK_VERSION, &d3dobj_ex))) {
        d3dobj_ex = 0;
        qWarning("Can not create IDirect3D9Ex");
        return false;
    }
    if (FAILED(d3dobj_ex->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &d3dai))) {
        qWarning("IDirect3D9Ex->GetAdapterIdentifier failed. Fallback to IDirect3D9->GetAdapterIdentifier");
        ZeroMemory(&d3dai, sizeof(d3dai));
        return false;
    }
    vendor = getVendorName(&d3dai);
    description = QString().sprintf("DXVA2 (%.*s, vendor %lu(%s), device %lu, revision %lu)",
                                    sizeof(d3dai.Description), d3dai.Description,
                                    d3dai.VendorId, qPrintable(vendor), d3dai.DeviceId, d3dai.Revision);
    //if (copy_uswc)
      //  copy_uswc = vendor.toLower() == "intel";
    qDebug("DXVA2 description:  %s", description.toUtf8().constData());

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    // use mozilla's parameters
    d3dpp.Flags                  = D3DPRESENTFLAG_VIDEO;
    d3dpp.Windowed               = TRUE;
    d3dpp.hDeviceWindow          = ::GetShellWindow(); //NULL;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    //d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
    //d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_DEFAULT;
    d3dpp.BackBufferCount        = 1; //0;                  /* FIXME what to put here */
    d3dpp.BackBufferFormat       = D3DFMT_UNKNOWN; //D3DFMT_X8R8G8B8;    /* FIXME what to put here */
    d3dpp.BackBufferWidth        = 1; //0;
    d3dpp.BackBufferHeight       = 1; //0;
    //d3dpp.EnableAutoDepthStencil = FALSE;

    DWORD flags = D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_MIXED_VERTEXPROCESSING;
    // old: D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED
    // mpv:
    /* Direct3D needs a HWND to create a device, even without using ::Present
    this HWND is used to alert Direct3D when there's a change of focus window.
    For now, use GetDesktopWindow, as it looks harmless */
    if (FAILED(d3dobj_ex->CreateDeviceEx(D3DADAPTER_DEFAULT,
                                         D3DDEVTYPE_HAL, GetShellWindow(),// GetDesktopWindow(), //GetShellWindow()?
                                         flags,
                                         &d3dpp,
                                         NULL,
                                         (IDirect3DDevice9Ex**)(&d3ddev)))) {
        qWarning("IDirect3D9Ex->CreateDeviceEx failed. Fallback to IDirect3D9->CreateDevice");
        d3ddev = 0;
        return false;
    }
    qDebug("IDirect3DDevice9Ex created....");
    return true;
}

bool VideoDecoderDXVAPrivate::D3dCreateDeviceFallback()
{
    qDebug("Fallback to d3d9");
    typedef IDirect3D9* (WINAPI *Create9Func)(UINT SDKVersion);
    Create9Func Create9 = (Create9Func)GetProcAddress(hd3d9_dll, "Direct3DCreate9");
    if (!Create9) {
        qWarning("Cannot locate reference to Direct3DCreate9 ABI in DLL");
        return false;
    }
    d3dobj = Create9(D3D_SDK_VERSION);
    if (!d3dobj) {
        qWarning("Direct3DCreate9 failed");
        return false;
    }
    if (FAILED(d3dobj->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &d3dai))) {
        qWarning("IDirect3D9->GetAdapterIdentifier failed");
        ZeroMemory(&d3dai, sizeof(d3dai));
        return false;
    }
    vendor = getVendorName(&d3dai);
    description = QString().sprintf("DXVA2 (%.*s, vendor %lu(%s), device %lu, revision %lu)",
                                    sizeof(d3dai.Description), d3dai.Description,
                                    d3dai.VendorId, qPrintable(vendor), d3dai.DeviceId, d3dai.Revision);
    //if (copy_uswc)
      //  copy_uswc = vendor.toLower() == "intel";
    qDebug("DXVA2 description:  %s", description.toUtf8().constData());

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    // use mozilla's parameters
    d3dpp.Flags                  = D3DPRESENTFLAG_VIDEO;
    d3dpp.Windowed               = TRUE;
    d3dpp.hDeviceWindow          = ::GetShellWindow(); //NULL;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    //d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
    //d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_DEFAULT;
    d3dpp.BackBufferCount        = 1; //0;                  /* FIXME what to put here */
    d3dpp.BackBufferFormat       = D3DFMT_UNKNOWN; //D3DFMT_X8R8G8B8;    /* FIXME what to put here */
    d3dpp.BackBufferWidth        = 1; //0;
    d3dpp.BackBufferHeight       = 1; //0;
    //d3dpp.EnableAutoDepthStencil = FALSE;
    DWORD flags = D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_MIXED_VERTEXPROCESSING;
    if (FAILED(d3dobj->CreateDevice(D3DADAPTER_DEFAULT,
                                   D3DDEVTYPE_HAL, GetShellWindow(),// GetDesktopWindow(), //GetShellWindow()?
                                   flags,
                                   &d3dpp, &d3ddev))) {
        qWarning("IDirect3D9->CreateDevice failed");
        d3ddev = 0;
        return false;
    }
    qDebug("IDirect3DDevice9 created....");
    return true;
}

/**
 * It releases a Direct3D device and its resources.
 */
void VideoDecoderDXVAPrivate::D3dDestroyDevice()
{
    SafeRelease(&d3ddev);
    SafeRelease(&d3dobj);
    SafeRelease(&d3dobj_ex);
    ZeroMemory(&d3dai, sizeof(d3dai));
}
/**
 * It creates a Direct3D device manager
 */
bool VideoDecoderDXVAPrivate::D3dCreateDeviceManager()
{
    typedef HRESULT (WINAPI *CreateDeviceManager9Func)(UINT *pResetToken, IDirect3DDeviceManager9 **);
    CreateDeviceManager9Func CreateDeviceManager9 = (CreateDeviceManager9Func)GetProcAddress(hdxva2_dll, "DXVA2CreateDirect3DDeviceManager9");
    if (!CreateDeviceManager9) {
        qWarning("cannot load function DXVA2CreateDirect3DDeviceManager9");
        return false;
    }
    qDebug("OurDirect3DCreateDeviceManager9 Success!");
    DX_ENSURE_OK(CreateDeviceManager9(&token, &devmng), false);
    qDebug("obtained IDirect3DDeviceManager9");
    //http://msdn.microsoft.com/en-us/library/windows/desktop/ms693525%28v=vs.85%29.aspx
    DX_ENSURE_OK(devmng->ResetDevice(d3ddev, token), false);
    return true;
}
void VideoDecoderDXVAPrivate::D3dDestroyDeviceManager()
{
    SafeRelease(&devmng);
}

bool VideoDecoderDXVAPrivate::DxCreateVideoService()
{
    DX_ENSURE_OK(devmng->OpenDeviceHandle(&device), false);
    DX_ENSURE_OK(devmng->GetVideoService(device, IID_IDirectXVideoDecoderService, (void**)&vs), false);
    return true;
}
void VideoDecoderDXVAPrivate::DxDestroyVideoService()
{
    if (devmng && device && device != INVALID_HANDLE_VALUE) {
        devmng->CloseDeviceHandle(device);
        device = 0;
    }
    SafeRelease(&vs);
}
/**
 * Find the best suited decoder mode GUID and render format.
 */
bool VideoDecoderDXVAPrivate::DxFindVideoServiceConversion(GUID *input, D3DFORMAT *output)
{
    /* Retreive supported modes from the decoder service */
    UINT input_count = 0;
    GUID *input_list = NULL;
    DX_ENSURE_OK(vs->GetDecoderDeviceGuids(&input_count, &input_list), false);
    for (unsigned i = 0; i < input_count; i++) {
        const GUID &g = input_list[i];
        const dxva2_mode_t *mode = Dxva2FindMode(&g);
        if (mode) {
            qDebug("- '%s' is supported by hardware", mode->name);
        } else {
            qDebug("- Unknown GUID = %08X-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
                     (unsigned)g.Data1, g.Data2, g.Data3
                   , g.Data4[0], g.Data4[1]
                   , g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
        }
    }
    /* Try all supported mode by our priority */
    for (unsigned i = 0; dxva2_modes[i].name; i++) {
        const dxva2_mode_t *mode = &dxva2_modes[i];
        if (!mode->codec || mode->codec != codec_ctx->codec_id) {
            qDebug("codec does not match to %s: %s", avcodec_get_name(codec_ctx->codec_id), avcodec_get_name((AVCodecID)mode->codec));
            continue;
        }
        if (codec_ctx->profile == FF_PROFILE_HEVC_MAIN_10 && !IsEqualGUID(*mode->guid, DXVA_ModeHEVC_VLD_Main10)) {
            qDebug("profile (%s) does not match to HEVC 10-bit", mode->name);
            continue;
        }
        qDebug("DXVA found codec: %s. Check support for the codec.", mode->name);
        bool is_suported = false;
        for (unsigned count = 0; !is_suported && count < input_count; count++) {
            const GUID &g = input_list[count];
            is_suported = IsEqualGUID(*mode->guid, g); ///
        }
        if (!is_suported)
            continue;

        UINT output_count = 0;
        D3DFORMAT *output_list = NULL;
        if (FAILED(vs->GetDecoderRenderTargets(*mode->guid, &output_count, &output_list))) {
            qWarning("IDirectXVideoDecoderService_GetDecoderRenderTargets failed");
            continue;
        }
        qDebug("supprted output count: %d", output_count);
        for (unsigned j = 0; j < output_count; j++) {
            const D3DFORMAT f = output_list[j];
            const d3d_format_t *format = D3dFindFormat(f);
            if (format) {
                qDebug("%s is supported for output", format->name);
            } else {
                qDebug("%d is supported for output (%4.4s)", f, (const char*)&f);
            }
        }

        for (unsigned j = 0; d3d_formats[j].name; j++) {
            const d3d_format_t *format = &d3d_formats[j];
            bool is_suported = false;
            for (unsigned k = 0; !is_suported && k < output_count; k++) {
                is_suported = format->format == output_list[k];
            }
            if (!is_suported)
                continue;

            /* We have our solution */
            qDebug("Using '%s' to decode to '%s'", mode->name, format->name);
            *input  = *mode->guid;
            *output = format->format;
            CoTaskMemFree(output_list);
            CoTaskMemFree(input_list);
            return true;
        }
        CoTaskMemFree(output_list);
    }
    CoTaskMemFree(input_list);
    return false;
}

bool VideoDecoderDXVAPrivate::DxCreateVideoDecoder(int codec_id, int w, int h)
{
    qDebug("DxCreateVideoDecoder id %d %dx%d, surfaces: %u", codec_id, w, h, surface_count);
    /* Allocates all surfaces needed for the decoder */
    surface_width = aligned(w);
    surface_height = aligned(h);
    if (surface_auto) {
        switch (codec_id) {
        case QTAV_CODEC_ID(H264):
            surface_count = 16 + 2 + codec_ctx->thread_count;
            break;
        case QTAV_CODEC_ID(MPEG1VIDEO):
        case QTAV_CODEC_ID(MPEG2VIDEO):
            surface_count = 2 + 2;
        default:
            surface_count = 16 + 4;
            break;
        }
    }
    qDebug(">>>>>>>>>>>>>>>>>>>>>surfaces: %d, threads: %d, refs: %d", surface_count, codec_ctx->thread_count, codec_ctx->refs);
    if (surface_count == 0) {
        qWarning("internal error: wrong surface count.  %u auto=%d", surface_count, surface_auto);
        surface_count = 16 + 4;
    }

    IDirect3DSurface9* surface_list[VA_DXVA2_MAX_SURFACE_COUNT];
    qDebug("%s @%d vs=%p surface_count=%d surface_width=%d surface_height=%d"
           , __FUNCTION__, __LINE__, vs, surface_count, surface_width, surface_height);
    DX_ENSURE_OK(vs->CreateSurface(surface_width,
                                 surface_height,
                                 surface_count - 1,
                                 render,
                                 D3DPOOL_DEFAULT,
                                 0,
                                 DXVA2_VideoDecoderRenderTarget,
                                 surface_list,
                                 NULL)
            , false);
    memset(surfaces, 0, sizeof(surfaces));
    for (unsigned i = 0; i < surface_count; i++) {
        va_surface_t *surface = &this->surfaces[i];
        surface->d3d = surface_list[i];
        surface->refcount = 0;
        surface->order = 0;
    }
    qDebug("IDirectXVideoAccelerationService_CreateSurface succeed with %d surfaces (%dx%d)", surface_count, w, h);

    /* */
    DXVA2_VideoDesc dsc;
    ZeroMemory(&dsc, sizeof(dsc));
    dsc.SampleWidth     = w; //coded_width
    dsc.SampleHeight    = h; //coded_height
    dsc.Format          = render;
    dsc.InputSampleFreq.Numerator   = 0;
    dsc.InputSampleFreq.Denominator = 0;
    dsc.OutputFrameFreq = dsc.InputSampleFreq;
    dsc.UABProtectionLevel = FALSE;
    dsc.Reserved = 0;
// see xbmc
    /* FIXME I am unsure we can let unknown everywhere */
    DXVA2_ExtendedFormat *ext = &dsc.SampleFormat;
    ext->SampleFormat = 0;//DXVA2_SampleProgressiveFrame;//xbmc. DXVA2_SampleUnknown;
    ext->VideoChromaSubsampling = 0;//DXVA2_VideoChromaSubsampling_Unknown;
    ext->NominalRange = 0;//DXVA2_NominalRange_Unknown;
    ext->VideoTransferMatrix = 0;//DXVA2_VideoTransferMatrix_Unknown;
    ext->VideoLighting = 0;//DXVA2_VideoLighting_dim;//xbmc. DXVA2_VideoLighting_Unknown;
    ext->VideoPrimaries = 0;//DXVA2_VideoPrimaries_Unknown;
    ext->VideoTransferFunction = 0;//DXVA2_VideoTransFunc_Unknown;

    /* List all configurations available for the decoder */
    UINT                      cfg_count = 0;
    DXVA2_ConfigPictureDecode *cfg_list = NULL;
    DX_ENSURE_OK(vs->GetDecoderConfigurations(input,
                                            &dsc,
                                            NULL,
                                            &cfg_count,
                                            &cfg_list)
                 , false);
    qDebug("we got %d decoder configurations", cfg_count);
    /* Select the best decoder configuration */
    int cfg_score = 0;
    for (unsigned i = 0; i < cfg_count; i++) {
        const DXVA2_ConfigPictureDecode *cfg = &cfg_list[i];
        qDebug("configuration[%d] ConfigBitstreamRaw %d", i, cfg->ConfigBitstreamRaw);
        int score;
        if (cfg->ConfigBitstreamRaw == 1)
            score = 1;
        else if (codec_id ==  QTAV_CODEC_ID(H264) && cfg->ConfigBitstreamRaw == 2)
            score = 2;
        else
            continue;
        if (IsEqualGUID(cfg->guidConfigBitstreamEncryption, DXVA_NoEncrypt))
            score += 16;

        if (cfg_score < score) {
            this->cfg = *cfg;
            cfg_score = score;
        }
    }
    CoTaskMemFree(cfg_list);
    if (cfg_score <= 0) {
        qWarning("Failed to find a supported decoder configuration");
        return false;
    }
    /* Create the decoder */
    DX_ENSURE_OK(vs->CreateVideoDecoder(input, &dsc, &cfg, surface_list, surface_count, &decoder), false);
    qDebug("IDirectXVideoDecoderService_CreateVideoDecoder succeed. decoder=%p", decoder);
    return true;
}

void VideoDecoderDXVAPrivate::DxDestroyVideoDecoder()
{
    SafeRelease(&decoder);
    for (unsigned i = 0; i < surface_count; i++) {
        SafeRelease(&surfaces[i].d3d);
    }
}

bool VideoDecoderDXVAPrivate::DxResetVideoDecoder()
{
    qWarning("DxResetVideoDecoder unimplemented");
    return false;
}

static bool check_ffmpeg_hevc_dxva2()
{
    avcodec_register_all();
    AVHWAccel *hwa = av_hwaccel_next(0);
    while (hwa) {
        if (strncmp("hevc_dxva2", hwa->name, 10) == 0)
            return true;
        hwa = av_hwaccel_next(hwa);
    }
    return false;
}

bool VideoDecoderDXVAPrivate::isHEVCSupported() const
{
    static const bool support_hevc = check_ffmpeg_hevc_dxva2();
    return support_hevc;
}

// hwaccel_context
bool VideoDecoderDXVAPrivate::setup(AVCodecContext *avctx)
{
    const int w = codedWidth(avctx);
    const int h = codedHeight(avctx);
    if (decoder && surface_width == aligned(w) && surface_height == aligned(h)) {
        avctx->hwaccel_context = &hw_ctx;
        return true;
    }
    width = avctx->width; // not necessary. set in decode()
    height = avctx->height;
    releaseUSWC();
    DxDestroyVideoDecoder();
    avctx->hwaccel_context = NULL;
    /* FIXME transmit a video_format_t by VaSetup directly */
    if (!DxCreateVideoDecoder(avctx->codec_id, w, h))
        return false;
    avctx->hwaccel_context = &hw_ctx;
    hw_ctx.decoder = decoder;
    hw_ctx.cfg = &cfg;
    hw_ctx.surface_count = surface_count;
    hw_ctx.surface = hw_surfaces;
    memset(hw_surfaces, 0, sizeof(hw_surfaces));
    for (unsigned i = 0; i < surface_count; i++)
        hw_ctx.surface[i] = surfaces[i].d3d;
    initUSWC(surface_width);
    return true;
}

bool VideoDecoderDXVAPrivate::open()
{
    if (codec_ctx->codec_id == QTAV_CODEC_ID(HEVC)) {
        // runtime hevc check
        if (isHEVCSupported()) {
            qWarning("HEVC DXVA2 is supported by current FFmpeg runtime.");
        } else {
            qWarning("HEVC DXVA2 is not supported by current FFmpeg runtime.");
            return false;
        }
    }
    if (!D3dCreateDevice()) {
        qWarning("Failed to create Direct3D device");
        goto error;
    }
    qDebug("D3dCreateDevice succeed");
    if (!D3dCreateDeviceManager()) {
        qWarning("D3dCreateDeviceManager failed");
        goto error;
    }
    if (!DxCreateVideoService()) {
        qWarning("DxCreateVideoService failed");
        goto error;
    }
    if (!DxFindVideoServiceConversion(&input, &render)) {
        qWarning("DxFindVideoServiceConversion failed");
        goto error;
    }
    IDirect3DDevice9Ex *devEx;
    d3ddev->QueryInterface(IID_IDirect3DDevice9Ex, (void**)&devEx);
    qDebug("using D3D9Ex: %d", !!devEx);
    SafeRelease(&devEx);
#if QTAV_HAVE(DXVA_EGL)
    interop_res = dxva::InteropResourcePtr(new dxva::EGLInteropResource(d3ddev));
#endif //QTAV_HAVE(DXVA_EGL)
    return true;
error:
    close();
    return false;
}

void VideoDecoderDXVAPrivate::close()
{
    restore();
    releaseUSWC();
    DxDestroyVideoDecoder();
    DxDestroyVideoService();
    D3dDestroyDeviceManager();
    D3dDestroyDevice();
}

int VideoDecoderDXVAPrivate::aligned(int x)
{
    // from lavfilters
    int align = 16;
    // MPEG-2 needs higher alignment on Intel cards, and it doesn't seem to harm anything to do it for all cards.
    if (codec_ctx->codec_id == QTAV_CODEC_ID(MPEG2VIDEO))
      align <<= 1;
    else if (codec_ctx->codec_id == QTAV_CODEC_ID(HEVC))
      align = 128;
    return FFALIGN(x, align);
}

} //namespace QtAV

#include "VideoDecoderDXVA.moc"
