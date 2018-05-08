/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

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

#include "VideoDecoderD3D.h"
#include <initguid.h> /* must be last included to not redefine existing GUIDs */

#if (FF_PROFILE_HEVC_MAIN == -1) //libav does not define it
#ifdef _MSC_VER
#pragma message("HEVC will not be supported. Update your FFmpeg")
#else
#warning "HEVC will not be supported. Update your FFmpeg"
#endif
#endif

namespace QtAV {

static bool check_ffmpeg_hevc_dxva2()
{
#if !AVCODEC_STATIC_REGISTER
    avcodec_register_all();
#endif
    AVHWAccel *hwa = av_hwaccel_next(0);
    while (hwa) {
        if (strncmp("hevc_dxva2", hwa->name, 10) == 0)
            return true;
        if (strncmp("hevc_d3d11va", hwa->name, 12) == 0)
            return true;
        hwa = av_hwaccel_next(hwa);
    }
    return false;
}

bool isHEVCSupported()
{
    static const bool support_hevc = check_ffmpeg_hevc_dxva2();
    return support_hevc;
}

// some MS_GUID are defined in mingw but some are not. move to namespace and define all is ok
#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = { l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}

MS_GUID    (DXVA_NoEncrypt,                         0x1b81bed0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

/* Codec capabilities GUID, sorted by codec */
MS_GUID    (DXVA2_ModeMPEG2_MoComp,                 0xe6a9f44b, 0x61b0, 0x4563, 0x9e, 0xa4, 0x63, 0xd2, 0xa3, 0xc6, 0xfe, 0x66);
MS_GUID    (DXVA2_ModeMPEG2_IDCT,                   0xbf22ad00, 0x03ea, 0x4690, 0x80, 0x77, 0x47, 0x33, 0x46, 0x20, 0x9b, 0x7e);
MS_GUID    (DXVA2_ModeMPEG2_VLD,                    0xee27417f, 0x5e28, 0x4e65, 0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9);
DEFINE_GUID(DXVA_ModeMPEG1_A,                       0x1b81be09, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_A,                       0x1b81be0A, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_B,                       0x1b81be0B, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_C,                       0x1b81be0C, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_D,                       0x1b81be0D, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
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
DEFINE_GUID(DXVA_Intel_H264_NoFGT_ClearVideo,       0x604F8E68, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
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

DEFINE_GUID(DXVA_ModeHEVC_VLD_Main,                 0x5b11d51b, 0x2f4c, 0x4452,0xbc,0xc3,0x09,0xf2,0xa1,0x16,0x0c,0xc0);
DEFINE_GUID(DXVA_ModeHEVC_VLD_Main10,               0x107af0e0, 0xef1a, 0x4d19,0xab,0xa8,0x67,0xa1,0x63,0x07,0x3d,0x13);

DEFINE_GUID(DXVA_ModeH264_VLD_Stereo_Progressive_NoFGT,     0xd79be8da, 0x0cf1, 0x4c81,0xb8,0x2a,0x69,0xa4,0xe2,0x36,0xf4,0x3d);
DEFINE_GUID(DXVA_ModeH264_VLD_Stereo_NoFGT,                 0xf9aaccbb, 0xc2b6, 0x4cfc,0x87,0x79,0x57,0x07,0xb1,0x76,0x05,0x52);
DEFINE_GUID(DXVA_ModeH264_VLD_Multiview_NoFGT,              0x705b9d82, 0x76cf, 0x49d6,0xb7,0xe6,0xac,0x88,0x72,0xdb,0x01,0x3c);

DEFINE_GUID(DXVA_ModeH264_VLD_SVC_Scalable_Baseline,                    0xc30700c4, 0xe384, 0x43e0, 0xb9, 0x82, 0x2d, 0x89, 0xee, 0x7f, 0x77, 0xc4);
DEFINE_GUID(DXVA_ModeH264_VLD_SVC_Restricted_Scalable_Baseline,         0x9b8175d4, 0xd670, 0x4cf2, 0xa9, 0xf0, 0xfa, 0x56, 0xdf, 0x71, 0xa1, 0xae);
DEFINE_GUID(DXVA_ModeH264_VLD_SVC_Scalable_High,                        0x728012c9, 0x66a8, 0x422f, 0x97, 0xe9, 0xb5, 0xe3, 0x9b, 0x51, 0xc0, 0x53);
DEFINE_GUID(DXVA_ModeH264_VLD_SVC_Restricted_Scalable_High_Progressive, 0x8efa5926, 0xbd9e, 0x4b04, 0x8b, 0x72, 0x8f, 0x97, 0x7d, 0xc4, 0x4c, 0x36);

DEFINE_GUID(DXVA_ModeH261_A,                        0x1b81be01, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH261_B,                        0x1b81be02, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

DEFINE_GUID(DXVA_ModeH263_A,                        0x1b81be03, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH263_B,                        0x1b81be04, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH263_C,                        0x1b81be05, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH263_D,                        0x1b81be06, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH263_E,                        0x1b81be07, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH263_F,                        0x1b81be08, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);


static const int PROF_MPEG2_SIMPLE[] = { FF_PROFILE_MPEG2_SIMPLE, 0 };
static const int PROF_MPEG2_MAIN[]   = { FF_PROFILE_MPEG2_SIMPLE, FF_PROFILE_MPEG2_MAIN, 0 };
static const int PROF_H264_HIGH[]    = { FF_PROFILE_H264_CONSTRAINED_BASELINE, FF_PROFILE_H264_MAIN, FF_PROFILE_H264_HIGH, 0 };
static const int PROF_HEVC_MAIN[]    = { FF_PROFILE_HEVC_MAIN, 0 };
static const int PROF_HEVC_MAIN10[]  = { FF_PROFILE_HEVC_MAIN, FF_PROFILE_HEVC_MAIN_10, 0 };
// guids are from VLC
struct dxva2_mode_t {
    const char   *name;
    const GUID   *guid;
    int          codec;
    const int    *profiles;
};
/* XXX Prefered modes must come first */
static const dxva2_mode_t dxva2_modes[] = {
    /* MPEG-1/2 */
    { "MPEG-1 decoder, restricted profile A",                                         &DXVA_ModeMPEG1_A,                      0, NULL },
    { "MPEG-2 decoder, restricted profile A",                                         &DXVA_ModeMPEG2_A,                      0, NULL },
    { "MPEG-2 decoder, restricted profile B",                                         &DXVA_ModeMPEG2_B,                      0, NULL },
    { "MPEG-2 decoder, restricted profile C",                                         &DXVA_ModeMPEG2_C,                      0, NULL },
    { "MPEG-2 decoder, restricted profile D",                                         &DXVA_ModeMPEG2_D,                      0, NULL },

    { "MPEG-2 variable-length decoder",                                               &DXVA2_ModeMPEG2_VLD,                   QTAV_CODEC_ID(MPEG2VIDEO), PROF_MPEG2_SIMPLE },
    { "MPEG-2 & MPEG-1 variable-length decoder",                                      &DXVA2_ModeMPEG2and1_VLD,               QTAV_CODEC_ID(MPEG2VIDEO), PROF_MPEG2_MAIN },
    { "MPEG-2 & MPEG-1 variable-length decoder",                                      &DXVA2_ModeMPEG2and1_VLD,               QTAV_CODEC_ID(MPEG1VIDEO), NULL },
    { "MPEG-2 motion compensation",                                                   &DXVA2_ModeMPEG2_MoComp,                0, NULL },
    { "MPEG-2 inverse discrete cosine transform",                                     &DXVA2_ModeMPEG2_IDCT,                  0, NULL },

    /* MPEG-1 http://download.microsoft.com/download/B/1/7/B172A3C8-56F2-4210-80F1-A97BEA9182ED/DXVA_MPEG1_VLD.pdf */
    { "MPEG-1 variable-length decoder, no D pictures",                                &DXVA2_ModeMPEG1_VLD,                   0, NULL },

    /* H.264 http://www.microsoft.com/downloads/details.aspx?displaylang=en&FamilyID=3d1c290b-310b-4ea2-bf76-714063a6d7a6 */
    { "H.264 variable-length decoder, film grain technology",                         &DXVA2_ModeH264_F,                      QTAV_CODEC_ID(H264), PROF_H264_HIGH },
    { "H.264 variable-length decoder, no film grain technology (Intel ClearVideo)",   &DXVA_Intel_H264_NoFGT_ClearVideo,      QTAV_CODEC_ID(H264), PROF_H264_HIGH },
    { "H.264 variable-length decoder, no film grain technology",                      &DXVA2_ModeH264_E,                      QTAV_CODEC_ID(H264), PROF_H264_HIGH },
    { "H.264 variable-length decoder, no film grain technology, FMO/ASO",             &DXVA_ModeH264_VLD_WithFMOASO_NoFGT,    QTAV_CODEC_ID(H264), PROF_H264_HIGH },
    { "H.264 variable-length decoder, no film grain technology, Flash",               &DXVA_ModeH264_VLD_NoFGT_Flash,         QTAV_CODEC_ID(H264), PROF_H264_HIGH },

    { "H.264 inverse discrete cosine transform, film grain technology",               &DXVA2_ModeH264_D,                      0, NULL },
    { "H.264 inverse discrete cosine transform, no film grain technology",            &DXVA2_ModeH264_C,                      0, NULL },
    { "H.264 inverse discrete cosine transform, no film grain technology (Intel)",    &DXVADDI_Intel_ModeH264_C,              0, NULL },

    { "H.264 motion compensation, film grain technology",                             &DXVA2_ModeH264_B,                      0, NULL },
    { "H.264 motion compensation, no film grain technology",                          &DXVA2_ModeH264_A,                      0, NULL },
    { "H.264 motion compensation, no film grain technology (Intel)",                  &DXVADDI_Intel_ModeH264_A,              0, NULL },

    /* http://download.microsoft.com/download/2/D/0/2D02E72E-7890-430F-BA91-4A363F72F8C8/DXVA_H264_MVC.pdf */
    { "H.264 stereo high profile, mbs flag set",                                      &DXVA_ModeH264_VLD_Stereo_Progressive_NoFGT, 0, NULL },
    { "H.264 stereo high profile",                                                    &DXVA_ModeH264_VLD_Stereo_NoFGT,             0, NULL },
    { "H.264 multiview high profile",                                                 &DXVA_ModeH264_VLD_Multiview_NoFGT,          0, NULL },

    /* SVC http://download.microsoft.com/download/C/8/A/C8AD9F1B-57D1-4C10-85A0-09E3EAC50322/DXVA_SVC_2012_06.pdf */
    { "H.264 scalable video coding, Scalable Baseline Profile",                       &DXVA_ModeH264_VLD_SVC_Scalable_Baseline,            0, NULL },
    { "H.264 scalable video coding, Scalable Constrained Baseline Profile",           &DXVA_ModeH264_VLD_SVC_Restricted_Scalable_Baseline, 0, NULL },
    { "H.264 scalable video coding, Scalable High Profile",                           &DXVA_ModeH264_VLD_SVC_Scalable_High,                0, NULL },
    { "H.264 scalable video coding, Scalable Constrained High Profile",               &DXVA_ModeH264_VLD_SVC_Restricted_Scalable_High_Progressive, 0, NULL },

    /* WMV */
    { "Windows Media Video 8 motion compensation",                                    &DXVA2_ModeWMV8_B,                      0, NULL },
    { "Windows Media Video 8 post processing",                                        &DXVA2_ModeWMV8_A,                      0, NULL },

    { "Windows Media Video 9 IDCT",                                                   &DXVA2_ModeWMV9_C,                      0, NULL },
    { "Windows Media Video 9 motion compensation",                                    &DXVA2_ModeWMV9_B,                      0, NULL },
    { "Windows Media Video 9 post processing",                                        &DXVA2_ModeWMV9_A,                      0, NULL },

    /* VC-1 */
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D,                       QTAV_CODEC_ID(VC1), NULL },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D,                       QTAV_CODEC_ID(WMV3), NULL },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D2010,                   QTAV_CODEC_ID(VC1), NULL },
    { "VC-1 variable-length decoder",                                                 &DXVA2_ModeVC1_D2010,                   QTAV_CODEC_ID(WMV3), NULL },
    { "VC-1 variable-length decoder 2 (Intel)",                                       &DXVA_Intel_VC1_ClearVideo_2,           0, NULL },
    { "VC-1 variable-length decoder (Intel)",                                         &DXVA_Intel_VC1_ClearVideo,             0, NULL },

    { "VC-1 inverse discrete cosine transform",                                       &DXVA2_ModeVC1_C,                       0, NULL },
    { "VC-1 motion compensation",                                                     &DXVA2_ModeVC1_B,                       0, NULL },
    { "VC-1 post processing",                                                         &DXVA2_ModeVC1_A,                       0, NULL },

    /* Xvid/Divx: TODO */
    { "MPEG-4 Part 2 nVidia bitstream decoder",                                       &DXVA_nVidia_MPEG4_ASP,                 0, NULL },
    { "MPEG-4 Part 2 variable-length decoder, Simple Profile",                        &DXVA_ModeMPEG4pt2_VLD_Simple,          0, NULL },
    { "MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, no GMC",       &DXVA_ModeMPEG4pt2_VLD_AdvSimple_NoGMC, 0, NULL },
    { "MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, GMC",          &DXVA_ModeMPEG4pt2_VLD_AdvSimple_GMC,   0, NULL },
    { "MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, Avivo",        &DXVA_ModeMPEG4pt2_VLD_AdvSimple_Avivo, 0, NULL },

    /* HEVC */
    { "HEVC Main profile",                                                            &DXVA_ModeHEVC_VLD_Main,                QTAV_CODEC_ID(HEVC), PROF_HEVC_MAIN },
    { "HEVC Main 10 profile",                                                         &DXVA_ModeHEVC_VLD_Main10,              QTAV_CODEC_ID(HEVC), PROF_HEVC_MAIN10 },

    /* H.261 */
    { "H.261 decoder, restricted profile A",                                          &DXVA_ModeH261_A,                       0, NULL },
    { "H.261 decoder, restricted profile B",                                          &DXVA_ModeH261_B,                       0, NULL },

    /* H.263 */
    { "H.263 decoder, restricted profile A",                                          &DXVA_ModeH263_A,                       0, NULL },
    { "H.263 decoder, restricted profile B",                                          &DXVA_ModeH263_B,                       0, NULL },
    { "H.263 decoder, restricted profile C",                                          &DXVA_ModeH263_C,                       0, NULL },
    { "H.263 decoder, restricted profile D",                                          &DXVA_ModeH263_D,                       0, NULL },
    { "H.263 decoder, restricted profile E",                                          &DXVA_ModeH263_E,                       0, NULL },
    { "H.263 decoder, restricted profile F",                                          &DXVA_ModeH263_F,                       0, NULL },

    { NULL, NULL, 0, NULL }
};

static const dxva2_mode_t *Dxva2FindMode(const GUID *guid)
{
    for (unsigned i = 0; dxva2_modes[i].name; i++) {
        if (IsEqualGUID(*dxva2_modes[i].guid, *guid))
            return &dxva2_modes[i];
    }
    return NULL;
}

bool isIntelClearVideo(const GUID *guid)
{
    return IsEqualGUID(*guid, DXVA_Intel_H264_NoFGT_ClearVideo);
}

bool isNoEncrypt(const GUID *guid)
{
    return IsEqualGUID(*guid, DXVA_NoEncrypt);
}

bool checkProfile(const dxva2_mode_t *mode, int profile)
{
    if (!mode->profiles || !mode->profiles[0] || profile <= 0)
        return true;
    for (const int *p = &mode->profiles[0]; *p; ++p) {
        if (*p == profile)
            return true;
    }
    return false;
}

/* XXX Prefered format must come first */
//16-bit: https://msdn.microsoft.com/en-us/library/windows/desktop/bb970578(v=vs.85).aspx
static const d3d_format_t d3d_formats[] = {
    { "NV12",   MAKEFOURCC('N','V','1','2'),    VideoFormat::Format_NV12 },
    { "YV12",   MAKEFOURCC('Y','V','1','2'),    VideoFormat::Format_YUV420P },
    { "IMC3",   MAKEFOURCC('I','M','C','3'),    VideoFormat::Format_YUV420P },
    { "P010",   MAKEFOURCC('P','0','1','0'),    VideoFormat::Format_YUV420P10LE },
    { "P016",   MAKEFOURCC('P','0','1','6'),    VideoFormat::Format_YUV420P16LE }, //FIXME:
    { NULL, 0, VideoFormat::Format_Invalid }
};

static const d3d_format_t *D3dFindFormat(int fourcc)
{
    for (unsigned i = 0; d3d_formats[i].name; i++) {
        if (d3d_formats[i].fourcc == fourcc)
            return &d3d_formats[i];
    }
    return NULL;
}

VideoFormat::PixelFormat pixelFormatFromFourcc(int format)
{
    const d3d_format_t *fmt = D3dFindFormat(format);
    if (fmt)
        return fmt->pixfmt;
    return VideoFormat::Format_Invalid;
}

int getSupportedFourcc(int *formats, UINT nb_formats)
{
    for (const int *f = formats; f < &formats[nb_formats]; ++f) {
        const d3d_format_t *format = D3dFindFormat(*f);
        if (format) {
            qDebug("%s is supported for output", format->name);
        } else {
            qDebug("%d is supported for output (%4.4s)", *f, (const char*)f);
        }
    }
    for (const d3d_format_t *format = d3d_formats; format->name; ++format) {
        bool is_supported = false;
        for (unsigned k = 0; !is_supported && k < nb_formats; k++) {
            if (format->fourcc == formats[k])
                return format->fourcc;
        }
    }
    return 0;
}

VideoDecoderD3D::VideoDecoderD3D(VideoDecoderD3DPrivate &d)
    : VideoDecoderFFmpegHW(d)
{
    // dynamic properties about static property details. used by UI
    // format: detail_property
    setProperty("detail_surfaces", tr("Decoding surfaces") + QStringLiteral(" ") + tr("0: auto"));
    setProperty("threads", 1); //FIXME: mt crash on close
}

void VideoDecoderD3D::setSurfaces(int num)
{
    DPTR_D(VideoDecoderD3D);
    if (d.surface_count == num)
        return;
    d.surface_count = num;
    d.surface_auto = num <= 0;
    Q_EMIT surfacesChanged();
}

int VideoDecoderD3D::surfaces() const
{
    return d_func().surface_count;
}

VideoDecoderD3DPrivate::VideoDecoderD3DPrivate()
    : VideoDecoderFFmpegHWPrivate()
    , surface_auto(true)
    , surface_count(0)
    , surface_width(0)
    , surface_height(0)
    , surface_order(0)
{
}

bool VideoDecoderD3DPrivate::open()
{
    if (!prepare())
        return false;
    if (codec_ctx->codec_id == QTAV_CODEC_ID(HEVC)) {
        // runtime hevc check
        if (!isHEVCSupported()) {
            qWarning("HEVC DXVA2/D3D11VA is not supported by current FFmpeg runtime.");
            return false;
        }
    }
    if (!createDevice())
        return false;
    format_fcc = 0;
    QVector<GUID> codecs = getSupportedCodecs();
    const d3d_format_t *fmt = getFormat(codec_ctx, codecs, &codec_guid);
    if (!fmt)
        return false;
    format_fcc = fmt->fourcc;
    if (!setupSurfaceInterop())
        return false;
    return true;
}

void VideoDecoderD3DPrivate::close()
{
    qDeleteAll(surfaces);
    surfaces.clear();
    restore();
    releaseUSWC();
    destroyDecoder();
    destroyDevice();
}

void* VideoDecoderD3DPrivate::setup(AVCodecContext *avctx)
{
    const int w = codedWidth(avctx);
    const int h = codedHeight(avctx);
    width = avctx->width; // not necessary. set in decode()
    height = avctx->height;
    releaseUSWC();
    destroyDecoder();

    /* Allocates all surfaces needed for the decoder */
    if (surface_auto) {
        switch (codec_ctx->codec_id) {
        case QTAV_CODEC_ID(HEVC):
        case QTAV_CODEC_ID(H264):
            surface_count = 16 + 4;
            break;
        case QTAV_CODEC_ID(MPEG1VIDEO):
        case QTAV_CODEC_ID(MPEG2VIDEO):
            surface_count = 2 + 4;
        default:
            surface_count = 2 + 4;
            break;
        }
        if (avctx->active_thread_type & FF_THREAD_FRAME)
            surface_count += avctx->thread_count;
    }
    qDebug(">>>>>>>>>>>>>>>>>>>>>surfaces: %d, active_thread_type: %d, threads: %d, refs: %d", surface_count, avctx->active_thread_type, avctx->thread_count, avctx->refs);
    if (surface_count == 0) {
        qWarning("internal error: wrong surface count.  %u auto=%d", surface_count, surface_auto);
        surface_count = 16 + 4;
    }
    qDeleteAll(surfaces);
    surfaces.clear();
    hw_surfaces.clear();
    surfaces.resize(surface_count);
    if (!createDecoder(codec_ctx->codec_id, w, h, surfaces))
        return NULL;
    hw_surfaces.resize(surface_count);
    for (int i = 0; i < surfaces.size(); ++i) {
        hw_surfaces[i] = surfaces[i]->getSurface();
    }
    surface_order = 0;
    surface_width = aligned(w);
    surface_height = aligned(h);
    initUSWC(surface_width);
    return setupAVVAContext(); //can not use codec_ctx for threaded mode!
}

/* FIXME it is nearly common with VAAPI */
bool VideoDecoderD3DPrivate::getBuffer(void **opaque, uint8_t **data)//vlc_va_t *external, AVFrame *ff)
{
    if (!checkDevice())
        return false;
    /* Grab an unused surface, in case none are, try the oldest
     * XXX using the oldest is a workaround in case a problem happens with libavcodec */
    int i, old;
    for (i = 0, old = 0; i < surfaces.size(); i++) {
        const va_surface_t *s = surfaces[i];
        if (!s->ref)
            break;
        if (s->order < surfaces[old]->order)
            old = i;
    }
    if (i >= surfaces.size())
        i = old;
    va_surface_t *s = surfaces[i];
    s->ref = 1;
    s->order = surface_order++;
    *data = (uint8_t*)s->getSurface();
    *opaque = s;
    return true;
}

void VideoDecoderD3DPrivate::releaseBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(data);
    va_surface_t *surface = (va_surface_t*)opaque;
    surface->ref--;
}

int VideoDecoderD3DPrivate::aligned(int x)
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

const d3d_format_t* VideoDecoderD3DPrivate::getFormat(const AVCodecContext *avctx, const QVector<GUID> &guids, GUID* selected) const
{
    foreach (const GUID& g, guids) {
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
    const dxva2_mode_t *mode = dxva2_modes;
    for (; mode->name; ++mode) {
        if (!mode->codec || mode->codec != avctx->codec_id) {
            qDebug("codec does not match to %s: %s", avcodec_get_name(avctx->codec_id), avcodec_get_name((AVCodecID)mode->codec));
            continue;
        }
        qDebug("D3D found codec: %s. Check runtime support for the codec.", mode->name);
        bool is_supported = false;
        //TODO: find_if
        foreach (const GUID& g, guids) {
            if (IsEqualGUID(*mode->guid, g)) {
                is_supported = true;
                break;
            }
        }
        if (is_supported) {
            qDebug("Check profile support: %s", AVDecoderPrivate::getProfileName(avctx));
            is_supported = checkProfile(mode, avctx->profile);
        }
        if (!is_supported)
            continue;
        int dxfmt = fourccFor(mode->guid);
        if (!dxfmt)
            continue;
        if (selected)
            *selected = *mode->guid;
        return D3dFindFormat(dxfmt);
    }
    return NULL;
}
} //namespace QtAV
