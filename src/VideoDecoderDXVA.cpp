/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#pragma comment(lib, "Ole32.lib") //CoTaskMemFree. why link failed?
#include "QtAV/VideoDecoderFFmpeg.h"
#include "private/VideoDecoder_p.h"
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include "prepost.h"

/* LIBAVCODEC_VERSION_CHECK checks for the right version of libav and FFmpeg
 * a is the major version
 * b and c the minor and micro versions of libav
 * d and e the minor and micro versions of FFmpeg */
#define LIBAVCODEC_VERSION_CHECK( a, b, c, d, e ) \
    ( (LIBAVCODEC_VERSION_MICRO <  100 && LIBAVCODEC_VERSION_INT >= AV_VERSION_INT( a, b, c ) ) || \
      (LIBAVCODEC_VERSION_MICRO >= 100 && LIBAVCODEC_VERSION_INT >= AV_VERSION_INT( a, d, e ) ) )

template <class T> void SafeRelease(T **ppT)
{
  if (*ppT) {
    (*ppT)->Release();
    *ppT = NULL;
  }
}

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

MS_GUID(IID_IDirectXVideoDecoderService, 0xfc51a551, 0xd5e7, 0x11d9, 0xaf,0x55,0x00,0x05,0x4e,0x43,0xff,0x02);
MS_GUID(IID_IDirectXVideoAccelerationService, 0xfc51a550, 0xd5e7, 0x11d9, 0xaf,0x55,0x00,0x05,0x4e,0x43,0xff,0x02);

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

namespace QtAV {

class VideoDecoderDXVAPrivate;
class VideoDecoderDXVA : public VideoDecoder
{
    DPTR_DECLARE_PRIVATE(VideoDecoderDXVA)
public:
    VideoDecoderDXVA();
    virtual ~VideoDecoderDXVA();
    virtual bool prepare();
    virtual bool open();
    virtual bool close();
    virtual bool decode(const QByteArray &encoded);
    virtual VideoFrame frame();
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
    { "MPEG-2 variable-length decoder",                                               &DXVA2_ModeMPEG2_VLD,                   CODEC_ID_MPEG2VIDEO },
    { "MPEG-2 & MPEG-1 variable-length decoder",                                      &DXVA2_ModeMPEG2and1_VLD,               CODEC_ID_MPEG2VIDEO },
    { "MPEG-2 motion compensation",                                                   &DXVA2_ModeMPEG2_MoComp,                0 },
    { "MPEG-2 inverse discrete cosine transform",                                     &DXVA2_ModeMPEG2_IDCT,                  0 },

    { "MPEG-1 variable-length decoder",                                               &DXVA2_ModeMPEG1_VLD,                   0 },

    /* H.264 */
    { "H.264 variable-length decoder, film grain technology",                         &DXVA2_ModeH264_F,                      CODEC_ID_H264 },
    { "H.264 variable-length decoder, no film grain technology",                      &DXVA2_ModeH264_E,                      CODEC_ID_H264 },
    { "H.264 variable-length decoder, no film grain technology (Intel ClearVideo)",   &DXVADDI_Intel_ModeH264_E,              CODEC_ID_H264 },
    { "H.264 variable-length decoder, no film grain technology, FMO/ASO",             &DXVA_ModeH264_VLD_WithFMOASO_NoFGT,    CODEC_ID_H264 },
    { "H.264 variable-length decoder, no film grain technology, Flash",               &DXVA_ModeH264_VLD_NoFGT_Flash,         CODEC_ID_H264 },

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
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D,                       CODEC_ID_VC1 },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D,                       CODEC_ID_WMV3 },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D2010,                   CODEC_ID_VC1 },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D2010,                   CODEC_ID_WMV3 },
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

/* */
typedef struct {
    const char    *name;
    D3DFORMAT     format;
    AVPixelFormat codec;
} d3d_format_t;
/* XXX Prefered format must come first */
static const d3d_format_t d3d_formats[] = {
    { "YV12",   (D3DFORMAT)MAKEFOURCC('Y','V','1','2'),    QTAV_PIX_FMT_C(YUV420P) },
    { "NV12",   (D3DFORMAT)MAKEFOURCC('N','V','1','2'),    QTAV_PIX_FMT_C(NV12) },
//    { "IMC3",   (D3DFORMAT)MAKEFOURCC('I','M','C','3'),    QTAV_PIX_FMT_C(YV12) },

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

static QString DxDescribe(D3DADAPTER_IDENTIFIER9 *id) //vlc_va_dxva2_t *va
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
    return QString().sprintf("DXVA2 (%.*s, vendor %lu(%s), device %lu, revision %lu)",
                 sizeof(id->Description), id->Description,
                 id->VendorId, vendor, id->DeviceId, id->Revision);
}


class VideoDecoderDXVAPrivate : public VideoDecoderPrivate
{
public:
    VideoDecoderDXVAPrivate():
        VideoDecoderPrivate()
    {
        hd3d9_dll = 0;
        hdxva2_dll = 0;
        d3dobj = 0;
        d3ddev = 0;
        token = 0;
        devmng = 0;
        device = 0;
        vs = 0;
        render = D3DFMT_UNKNOWN;
        decoder = 0;
        output = D3DFMT_UNKNOWN;
        surface_count = 0;
        surface_order = 0;
        surface_width = surface_height = 0;
        available = loadDll();
    }
    virtual ~VideoDecoderDXVAPrivate()
    {
        unloadDll();
    }

    bool loadDll();
    bool unloadDll();
    bool D3dCreateDevice();
    void D3dDestroyDevice();
    bool D3dCreateDeviceManager();
    void D3dDestroyDeviceManager();
    bool DxCreateVideoService();
    void DxDestroyVideoService();
    bool DxFindVideoServiceConversion(GUID *input, D3DFORMAT *output);
    bool DxCreateVideoDecoder(int codec_id, int w, int h);
    void DxDestroyVideoDecoder();
    bool DxResetVideoDecoder();
    void DxCreateVideoConversion();
    void DxDestroyVideoConversion();

    bool setup(void **hwctx, AVPixelFormat *chroma, int w, int h);
    bool open();
    bool close();

    AVPixelFormat getFormat(struct AVCodecContext *p_context, const AVPixelFormat * pi_fmt);
    int getBuffer(struct AVCodecContext *c, AVFrame *ff);
    void releaseBuffer(struct AVCodecContext *c, AVFrame *ff);

    /* DLL */
    HINSTANCE hd3d9_dll;
    HINSTANCE hdxva2_dll;

    /* Direct3D */
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3D9 *d3dobj;
    D3DADAPTER_IDENTIFIER9 d3dai;
    IDirect3DDevice9 *d3ddev;

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

    /* Option conversion */
    D3DFORMAT                    output;
    //copy_cache_t                 surface_cache;

    struct dxva_context hw;

    unsigned     surface_count;
    unsigned     surface_order;
    int          surface_width;
    int          surface_height;
    //AVPixelFormat surface_chroma;

    va_surface_t surfaces[VA_DXVA2_MAX_SURFACE_COUNT];
    IDirect3DSurface9* hw_surfaces[VA_DXVA2_MAX_SURFACE_COUNT];

    QString description;
};

static AVPixelFormat ffmpeg_get_dxva2_format(struct AVCodecContext *c, const AVPixelFormat * ff)
{
    VideoDecoderDXVAPrivate *va = (VideoDecoderDXVAPrivate*)c->opaque;
    return va->getFormat(c, ff);
}

static int ffmpeg_get_dxva2_buffer(struct AVCodecContext *c, AVFrame *ff)//vlc_va_t *external, AVFrame *ff)
{
    VideoDecoderDXVAPrivate *va = (VideoDecoderDXVAPrivate*)c->opaque;
    return va->getBuffer(c, ff);
}

static void ffmpeg_release_buffer(struct AVCodecContext *c, AVFrame *ff)
{
    VideoDecoderDXVAPrivate *va = (VideoDecoderDXVAPrivate*)c->opaque;
    va->releaseBuffer(c, ff);
}


VideoDecoderDXVA::VideoDecoderDXVA()
    : VideoDecoder(*new VideoDecoderDXVAPrivate())
{
}

VideoDecoderDXVA::~VideoDecoderDXVA()
{
    setCodecContext(0);
}

bool VideoDecoderDXVA::prepare()
{
    return VideoDecoder::prepare();
}

bool VideoDecoderDXVA::open()
{
    DPTR_D(VideoDecoderDXVA);
    if (!d.codec_ctx) {
        qWarning("FFmpeg codec context not ready");
        return false;
    }
    AVCodec *codec = 0;
    if (!d.name.isEmpty()) {
        codec = avcodec_find_decoder_by_name(d.name.toUtf8().constData());
    } else {
        codec = avcodec_find_decoder(d.codec_ctx->codec_id);
    }
    if (!codec) {
        if (d.name.isEmpty()) {
            qWarning("No codec could be found with id %d", d.codec_ctx->codec_id);
        } else {
            qWarning("No codec could be found with name %s", d.name.toUtf8().constData());
        }
        return false;
    }
    if (!d.open()) {
        qWarning("Open DXVA2 decoder failed!");
        return false;
    }
    //TODO: neccesary?
#if 0
    if (!d.setup(&d.codec_ctx->hwaccel_context, &d.codec_ctx->pix_fmt, d.codec_ctx->width, d.codec_ctx->height)) {
        qWarning("Setup DXVA2 failed.");
        return false;
    }
#endif
    d.codec_ctx->opaque = &d; //is it ok?
    d.codec_ctx->get_format = ffmpeg_get_dxva2_format;
    d.codec_ctx->get_buffer = ffmpeg_get_dxva2_buffer; //deprecated
    d.codec_ctx->reget_buffer = avcodec_default_reget_buffer; //deprecated
    //d.codec_ctx->get_buffer2 = //lavfilter
    d.codec_ctx->release_buffer = ffmpeg_release_buffer; //deprecated
    d.codec_ctx->thread_count = 1;
    //d.codec_ctx->strict_std_compliance = FF_COMPLIANCE_STRICT;
    //d.codec_ctx->slice_flags |= SLICE_FLAG_ALLOW_FIELD;
// lavfilter
    //d.codec_ctx->slice_flags |= SLICE_FLAG_ALLOW_FIELD; //lavfilter
    //d.codec_ctx->strict_std_compliance = FF_COMPLIANCE_STRICT;

    d.codec_ctx->pix_fmt = QTAV_PIX_FMT_C(DXVA2_VLD);

    int ret = avcodec_open2(d.codec_ctx, codec, NULL);
    if (ret < 0) {
        qWarning("open video codec failed: %s", av_err2str(ret));
        return false;
    }
    d.is_open = true;
    return true;
}

bool VideoDecoderDXVA::close()
{
    if (!isOpen()) {
        return true;
    }
    DPTR_D(VideoDecoderDXVA);
    d.is_open = false;
    d.close();
    return VideoDecoder::close();
}

bool VideoDecoderDXVA::decode(const QByteArray &encoded)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoder);
    AVPacket packet;
    av_new_packet(&packet, encoded.size());
    memcpy(packet.data, encoded.data(), encoded.size());
//TODO: use AVPacket directly instead of Packet?
    //AVStream *stream = format_context->streams[stream_idx];

    //TODO: some decoders might in addition need other fields like flags&AV_PKT_FLAG_KEY
    int ret = avcodec_decode_video2(d.codec_ctx, d.frame, &d.got_frame_ptr, &packet);
    //TODO: decoded format is YUV420P, YUV422P?
    av_free_packet(&packet);
    if (ret < 0) {
        qWarning("[VideoDecoder] %s", av_err2str(ret));
        return false;
    }
    if (!d.got_frame_ptr) {
        qWarning("no frame could be decompressed: %s", av_err2str(ret));
        return false; //FIXME
    }
    // TODO: wait key frame?
    if (!d.codec_ctx->width || !d.codec_ctx->height)
        return false;
    d.width = d.codec_ctx->width;
    d.height = d.codec_ctx->height;
    return true;
}

VideoFrame VideoDecoderDXVA::frame()
{
    DPTR_D(VideoDecoderDXVA);
    if (d.width <= 0 || d.height <= 0 || !d.codec_ctx)
        return VideoFrame();
/*
    if (!va->surface_cache.buffer)
        return VLC_EGENERIC;
*/
    Q_ASSERT(d.output == MAKEFOURCC('Y','V','1','2'));
//    qDebug("...........output: %d yv12=%d, size=%dx%d", d.output, MAKEFOURCC('Y','V','1','2'), d.width, d.height);

    class ScopedD3DLock {
    public:
        ScopedD3DLock(IDirect3DSurface9* d3d, D3DLOCKED_RECT *rect)
            : mpD3D(d3d)
        {
            if (FAILED(mpD3D->LockRect(rect, NULL, D3DLOCK_READONLY))) {
                qWarning("Failed to lock surface");
                mpD3D = 0;
            }
        }
        ~ScopedD3DLock() {
            if (mpD3D)
                mpD3D->UnlockRect();
        }
    private:
        IDirect3DSurface9 *mpD3D;
    };

    IDirect3DSurface9 *d3d = (IDirect3DSurface9*)(uintptr_t)d.frame->data[3];
    //picth >= desc.Width
    //D3DSURFACE_DESC desc;
    //d3d->GetDesc(&desc);
    D3DLOCKED_RECT lock;
    ScopedD3DLock(d3d, &lock);
    if (lock.Pitch == 0) {
        return VideoFrame();
    }
    if (d.render == MAKEFOURCC('Y','V','1','2') ||
        d.render == MAKEFOURCC('I','M','C','3')) {
        bool imc3 = d.render == MAKEFOURCC('I','M','C','3');
        int chroma_pitch = imc3 ? lock.Pitch : (lock.Pitch / 2);
        int pitch[3] = {
            lock.Pitch,
            chroma_pitch,
            chroma_pitch,
        };
        uint8_t *plane[3] = {
            (uint8_t*)lock.pBits,
            (uint8_t*)lock.pBits + pitch[0] * d.surface_height,
            (uint8_t*)lock.pBits + pitch[0] * d.surface_height
                                 + pitch[1] * d.surface_height / 2,
        };
        if (imc3) {
            uint8_t *V = plane[1];
            plane[1] = plane[2];
            plane[2] = V;
        }
        //DO NOT make frame as a memeber, because VideoFrame is explictly shared!
        //FIXME: FFmpeg has no YV12. YUV420P is ok?
        VideoFrame frame(d.width, d.height, VideoFormat(VideoFormat::Format_YUV420P));//VideoFormat((int)d.codec_ctx->pix_fmt));
        frame.setBits(plane);
        frame.setBytesPerLine(pitch);
        return frame;
    } else {
        Q_ASSERT(d.render == MAKEFOURCC('N','V','1','2'));
        uint8_t *plane[2] = {
            (uint8_t *)lock.pBits,
            (uint8_t*)lock.pBits + lock.Pitch * d.surface_height
        };
        int pitch[2] = {
            lock.Pitch,
            lock.Pitch
        };
        VideoFrame frame(d.width, d.height, VideoFormat(VideoFormat::Format_NV12));//VideoFormat((int)d.codec_ctx->pix_fmt));
        frame.setBits(plane);
        frame.setBytesPerLine(pitch);
        return frame;
    }
    return VideoFrame();
}


/* FIXME it is nearly common with VAAPI */
int VideoDecoderDXVAPrivate::getBuffer(struct AVCodecContext *c, AVFrame *ff)//vlc_va_t *external, AVFrame *ff)
{
    ff->reordered_opaque = c->reordered_opaque;
    ff->opaque = NULL;
#if ! LIBAVCODEC_VERSION_CHECK(54, 34, 0, 79, 101)
    ff->pkt_pts = c->pkt ? c->pkt->pts : AV_NOPTS_VALUE;
#endif
#if LIBAVCODEC_VERSION_MAJOR < 54
    ff->age = 256*256*256*64;
#endif
    // check pix fmt DXVA2_VLD and h264 profile?
    /* hwaccel_context is not present in old ffmpeg version */
    if (!setup(&c->hwaccel_context, &c->pix_fmt,// &p_dec->fmt_out.video.i_chroma,
                c->width, c->height)) {
        qWarning("setup failed" );
        return -1;
    }
    /* Check the device */
    HRESULT hr = devmng->TestDevice(device);
    if (hr == DXVA2_E_NEW_VIDEO_DEVICE) {
        if (!DxResetVideoDecoder())
            return -1;
    } else if (FAILED(hr)) {
        qWarning("IDirect3DDeviceManager9_TestDevice %u", (unsigned)hr);
        return -1;
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
    /* */
    memset(ff->data, 0, sizeof(ff->data));
    memset(ff->linesize, 0, sizeof(ff->linesize));
    //memset(ff->buf, 0, sizeof(ff->buf));
    ff->data[0] = ff->data[3] = (uint8_t*)surface->d3d;/* Yummie */
    ff->type = FF_BUFFER_TYPE_USER; //type: deprecated

    /*
     * 515959913922fc234d0d814e0d0b115f9c293ee5
     * use AVFrame.opaque to store internal per-picture state directly
     */
    ff->opaque = surface;
    return 0;
}

//(void *opaque, uint8_t *data)
void VideoDecoderDXVAPrivate::releaseBuffer(struct AVCodecContext *c, AVFrame *ff)
{
    Q_UNUSED(c);
    va_surface_t *surface = (va_surface_t*)ff->opaque;
    surface->refcount--;
}

AVPixelFormat VideoDecoderDXVAPrivate::getFormat(struct AVCodecContext *p_context, const AVPixelFormat * pi_fmt)
{
    //lavfilter
/*
    while (*pi_fmt != AV_PIX_FMT_NONE && *pi_fmt != AV_PIX_FMT_DXVA2_VLD) {
        ++pi_fmt;
    }
    return fmt[0];
*/
    /* Enumerate available formats */
    bool can_hwaccel = false;
    for (size_t i = 0; pi_fmt[i] != QTAV_PIX_FMT_C(NONE); i++) {
        const AVPixFmtDescriptor *dsc = av_pix_fmt_desc_get(pi_fmt[i]);
        if (dsc == NULL)
            continue;
        bool hwaccel = (dsc->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0;

        qDebug("available %sware decoder output format %d (%s)",
                 hwaccel ? "hard" : "soft", pi_fmt[i], dsc->name);
        if (hwaccel)
            can_hwaccel = true;
    }

    if (!can_hwaccel)
        goto end;
    /* Try too look for a supported hw acceleration */
    //AV_PIX_FMT_NB? read avctx.get_format
    for (size_t i = 0; pi_fmt[i] != QTAV_PIX_FMT_C(NONE); i++) {
        const char *name = av_get_pix_fmt_name(pi_fmt[i]);
        qDebug("Available decoder output format %d (%s)", pi_fmt[i], name ? name : "unknown" );
        /*
         * TODO: check pix_fmt (set on open) to uniform all va. i.e. getFormat can be used for other va
         * such as vaapi, only setup() changes
         */
        if (QTAV_PIX_FMT_C(DXVA2_VLD) != pi_fmt[i])
            continue;
        /* We try to call vlc_va_Setup when possible to detect errors when
         * possible (later is too late) */
        if( p_context->width > 0 && p_context->height > 0
         && !setup(&p_context->hwaccel_context,
                   ///FIXME: fmt_out is required?
                   &p_context->pix_fmt,  ///&p_dec->fmt_out.video.i_chroma,
                   p_context->width, p_context->height ) )
        {
            qWarning("acceleration setup failure");
            break;
        }
        p_context->draw_horiz_band = NULL;
        return pi_fmt[i];
    }
end:
    qWarning("acceleration not available" );
    close();
    return avcodec_default_get_format(p_context, pi_fmt);
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
        qWarning("IDirect3D9_GetAdapterIdentifier failed");
        ZeroMemory(&d3dai, sizeof(d3dai));
    } else {
        description = DxDescribe(&d3dai);
        qDebug("DXVA2 description:  %s", description.toUtf8().constData());
    }
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Flags                  = D3DPRESENTFLAG_VIDEO;
    d3dpp.Windowed               = TRUE;
    d3dpp.hDeviceWindow          = NULL;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
    d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_DEFAULT;
    d3dpp.BackBufferCount        = 0;                  /* FIXME what to put here */
    d3dpp.BackBufferFormat       = D3DFMT_X8R8G8B8;    /* FIXME what to put here */
    d3dpp.BackBufferWidth        = 0;
    d3dpp.BackBufferHeight       = 0;
    d3dpp.EnableAutoDepthStencil = FALSE;

    /* Direct3D needs a HWND to create a device, even without using ::Present
    this HWND is used to alert Direct3D when there's a change of focus window.
    For now, use GetDesktopWindow, as it looks harmless */
    if (FAILED(d3dobj->CreateDevice(D3DADAPTER_DEFAULT,
                                   D3DDEVTYPE_HAL, GetDesktopWindow(), //GetShellWindow()?
                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                                   D3DCREATE_MULTITHREADED,
                                   &d3dpp, &d3ddev))) {
        qWarning("IDirect3D9_CreateDevice failed");
        return false;
    }

    return true;
}

/**
 * It releases a Direct3D device and its resources.
 */
void VideoDecoderDXVAPrivate::D3dDestroyDevice()
{
    SafeRelease(&d3ddev);
    SafeRelease(&d3dobj);
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
    if (FAILED(CreateDeviceManager9(&token, &devmng))) {
        qWarning(" OurDirect3DCreateDeviceManager9 failed");
        return false;
    }
    qDebug("obtained IDirect3DDeviceManager9");
    //http://msdn.microsoft.com/en-us/library/windows/desktop/ms693525%28v=vs.85%29.aspx
    HRESULT hr = devmng->ResetDevice(d3ddev, token);
    if (FAILED(hr)) {
        qWarning("IDirect3DDeviceManager9_ResetDevice failed: %08x", (unsigned)hr);
        return false;
    }
    return true;
}
void VideoDecoderDXVAPrivate::D3dDestroyDeviceManager()
{
    SafeRelease(&devmng);
}

bool VideoDecoderDXVAPrivate::DxCreateVideoService()
{
    typedef HRESULT (WINAPI *CreateVideoServiceFunc)(IDirect3DDevice9 *, REFIID riid, void **ppService);
    CreateVideoServiceFunc CreateVideoService = (CreateVideoServiceFunc)GetProcAddress(hdxva2_dll, "DXVA2CreateVideoService");

    if (!CreateVideoService) {
        qWarning("cannot load function");
        return false;
    }
    qDebug("DXVA2CreateVideoService Success!");

    HRESULT hr = devmng->OpenDeviceHandle(&device);
    if (FAILED(hr)) {
        device = 0;
        qWarning("OpenDeviceHandle failed");
        return false;
    }
    //c++: devmng->GetVideoService
    hr = devmng->GetVideoService(device, IID_IDirectXVideoDecoderService, (void**)&vs);
    if (FAILED(hr)) {
        qWarning("GetVideoService failed");
        return false;
    }
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
    if (FAILED(vs->GetDecoderDeviceGuids(&input_count, &input_list))) {
        qWarning("IDirectXVideoDecoderService_GetDecoderDeviceGuids failed");
        return false;
    }
    for (unsigned i = 0; i < input_count; i++) {
        const GUID &g = input_list[i];
        const dxva2_mode_t *mode = Dxva2FindMode(&g);
        if (mode) {
            qDebug("- '%s' is supported by hardware", mode->name);
        } else {
            qWarning("- Unknown GUID = %08X-%04x-%04x-XXXX",
                     (unsigned)g.Data1, g.Data2, g.Data3);
        }
    }
    /* Try all supported mode by our priority */
    for (unsigned i = 0; dxva2_modes[i].name; i++) {
        const dxva2_mode_t *mode = &dxva2_modes[i];
        if (!mode->codec || mode->codec != codec_ctx->codec_id)
            continue;
        bool is_suported = false;
        for (unsigned count = 0; !is_suported && count < input_count; count++) {
            const GUID &g = input_list[count];
            is_suported = IsEqualGUID(*mode->guid, g); ///
        }
        if (!is_suported)
            continue;

        qDebug("Trying to use '%s' as input", mode->name);
        UINT      output_count = 0;
        D3DFORMAT *output_list = NULL;
        if (FAILED(vs->GetDecoderRenderTargets(*mode->guid, &output_count, &output_list))) {
            qWarning("IDirectXVideoDecoderService_GetDecoderRenderTargets failed");
            continue;
        }
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
    if (!codec_ctx) {
        qWarning("AVCodecContext not ready!");
        return false;
    }
    qDebug("DxCreateVideoDecoder id %d %dx%d", codec_id, w, h);
    width = w;
    height = h;
    /* Allocates all surfaces needed for the decoder */
    surface_width = FFALIGN(width, 16);
    surface_height = FFALIGN(height, 16);
    switch (codec_id) {
    case AV_CODEC_ID_H264:
        surface_count = 16 + 1;
        break;
    default:
        surface_count = 2 + 1;
        break;
    }
    IDirect3DSurface9* surface_list[VA_DXVA2_MAX_SURFACE_COUNT];
    qDebug("%s @%d vs=%p surface_count=%d surface_width=%d surface_height=%d"
           , __FUNCTION__, __LINE__, vs, surface_count, surface_width, surface_height);
    if (FAILED(vs->CreateSurface(surface_width,
                                 surface_height,
                                 surface_count - 1,
                                 render,
                                 D3DPOOL_DEFAULT,
                                 0,
                                 DXVA2_VideoDecoderRenderTarget,
                                 surface_list,
                                 NULL))) {
        qWarning("IDirectXVideoAccelerationService_CreateSurface failed");
        surface_count = 0;
        return false;
    }
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
    dsc.SampleWidth     = width;
    dsc.SampleHeight    = height;
    dsc.Format          = render;
    /*if (fmt->i_frame_rate > 0 && fmt->i_frame_rate_base > 0) {
        dsc.InputSampleFreq.Numerator   = fmt->i_frame_rate;
        dsc.InputSampleFreq.Denominator = fmt->i_frame_rate_base;
    } else {*/
        dsc.InputSampleFreq.Numerator   = 0;
        dsc.InputSampleFreq.Denominator = 0;
    //}
    dsc.OutputFrameFreq = dsc.InputSampleFreq;
    dsc.UABProtectionLevel = FALSE;
    dsc.Reserved = 0;

    /* FIXME I am unsure we can let unknown everywhere */
    DXVA2_ExtendedFormat *ext = &dsc.SampleFormat;
    ext->SampleFormat = 0;//DXVA2_SampleUnknown;
    ext->VideoChromaSubsampling = 0;//DXVA2_VideoChromaSubsampling_Unknown;
    ext->NominalRange = 0;//DXVA2_NominalRange_Unknown;
    ext->VideoTransferMatrix = 0;//DXVA2_VideoTransferMatrix_Unknown;
    ext->VideoLighting = 0;//DXVA2_VideoLighting_Unknown;
    ext->VideoPrimaries = 0;//DXVA2_VideoPrimaries_Unknown;
    ext->VideoTransferFunction = 0;//DXVA2_VideoTransFunc_Unknown;

    /* List all configurations available for the decoder */
    UINT                      cfg_count = 0;
    DXVA2_ConfigPictureDecode *cfg_list = NULL;
    if (FAILED(vs->GetDecoderConfigurations(input,
                                            &dsc,
                                            NULL,
                                            &cfg_count,
                                            &cfg_list))) {
        qWarning("IDirectXVideoDecoderService_GetDecoderConfigurations failed");
        return false;
    }
    qDebug("we got %d decoder configurations", cfg_count);
    /* Select the best decoder configuration */
    int cfg_score = 0;
    for (unsigned i = 0; i < cfg_count; i++) {
        const DXVA2_ConfigPictureDecode *cfg = &cfg_list[i];
        qDebug("configuration[%d] ConfigBitstreamRaw %d", i, cfg->ConfigBitstreamRaw);
        int score;
        if (cfg->ConfigBitstreamRaw == 1)
            score = 1;
        else if (codec_id == AV_CODEC_ID_H264 && cfg->ConfigBitstreamRaw == 2)
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
    if (FAILED(vs->CreateVideoDecoder(input,
                                      &dsc,
                                      &cfg,
                                      surface_list,
                                      surface_count,
                                      &decoder))) {
        qWarning("IDirectXVideoDecoderService_CreateVideoDecoder failed");
        return false;
    }
    qDebug("IDirectXVideoDecoderService_CreateVideoDecoder succeed. decoder=%p", decoder);
    return true;
}

void VideoDecoderDXVAPrivate::DxDestroyVideoDecoder()
{
    SafeRelease(&decoder);
    for (unsigned i = 0; i < surface_count; i++) {
        SafeRelease(&surfaces[i].d3d);
    }
    surface_count = 0;
}

bool VideoDecoderDXVAPrivate::DxResetVideoDecoder()
{
    qWarning("DxResetVideoDecoder unimplemented");
    return false;
}

void VideoDecoderDXVAPrivate::DxCreateVideoConversion()
{
    switch (render) {
    case MAKEFOURCC('N','V','1','2'):
    case MAKEFOURCC('I','M','C','3'):
        output = (D3DFORMAT)MAKEFOURCC('Y','V','1','2');
        break;
    default:
        output = render;
        break;
    }
    ///CopyInitCache(&surface_cache, surface_width); //FIXME:
}

void VideoDecoderDXVAPrivate::DxDestroyVideoConversion()
{
    ///CopyCleanCache(&surface_cache); //FIXME:
}

// hwaccel_context
bool VideoDecoderDXVAPrivate::setup(void **hwctx, AVPixelFormat *chroma, int w, int h)
{
    if (w <= 0 || h <= 0)
        return false;
    if (!decoder || ((width != w || height != h)/* &&
            (surface_width != FFALIGN(w, 16) || surface_height != FFALIGN(h, 16))//)
        */)) {
        DxDestroyVideoConversion();
        DxDestroyVideoDecoder();
        *hwctx = NULL;
        *chroma = QTAV_PIX_FMT_C(NONE);
        /* FIXME transmit a video_format_t by VaSetup directly */
        if (!DxCreateVideoDecoder(codec_ctx->codec_id, w, h))
            return false;
        hw.decoder = decoder;
        hw.cfg = &cfg;
        hw.surface_count = surface_count;
        hw.surface = hw_surfaces;
        for (unsigned i = 0; i < surface_count; i++)
            hw.surface[i] = surfaces[i].d3d;
        DxCreateVideoConversion();
    }
    *hwctx = &hw;
    const d3d_format_t *outfmt = D3dFindFormat(output);
    *chroma = outfmt ? outfmt->codec : QTAV_PIX_FMT_C(NONE);
    return true;
}

bool VideoDecoderDXVAPrivate::open() {
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
    /* TODO print the hardware name/vendor for debugging purposes */
    return true;
error:
    close();
    return false;
}

bool VideoDecoderDXVAPrivate::close() {
    DxDestroyVideoConversion();
    DxDestroyVideoDecoder();
    DxDestroyVideoService();
    D3dDestroyDeviceManager();
    D3dDestroyDevice();
    return true;
}

} //namespace QtAV
