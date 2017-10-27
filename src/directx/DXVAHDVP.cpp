/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2017)

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

#include "DXVAHDVP.h"
#define DX_LOG_COMPONENT "D3D9VP"
#include "utils/DirectXHelper.h"
#include "utils/Logger.h"
#include "directx/DXVAHDVP.h"

namespace QtAV {
namespace dx {

DXVAHDVP::DXVAHDVP(ComPtr<IDirect3DDevice9> dev)
    : m_dev(NULL)
    , m_w(0)
    , m_h(0)
    , m_cs(ColorSpace_BT709)
    , m_range(ColorRange_Limited)
    , fDXVAHD_CreateDevice(NULL)
{
    DX_ENSURE(dev.As(&m_dev));
    fDXVAHD_CreateDevice = (PDXVAHD_CreateDevice)GetProcAddress(GetModuleHandle(TEXT("dxva2.dll")), "DXVAHD_CreateDevice");
}

void DXVAHDVP::setOutput(IDirect3DSurface9 *surface)
{
    m_out = surface;
}

void DXVAHDVP::setSourceRect(const QRect &r)
{
    m_srcRect = r;
}

void DXVAHDVP::setColorSpace(ColorSpace value)
{
    m_cs = value;
}

void DXVAHDVP::setColorRange(ColorRange value)
{
    m_range = value;
}

bool DXVAHDVP::process(IDirect3DSurface9 *surface)
{
    if (!surface || !m_out)
        return false;
    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);
    if (!ensureResource(desc.Width, desc.Height, desc.Format))
        return false;

    if (!m_srcRect.isEmpty()) {
        const RECT sr = {m_srcRect.x(), m_srcRect.y(), m_srcRect.width(), m_srcRect.height()};
        DXVAHD_STREAM_STATE_SOURCE_RECT_DATA r;
        r.Enable = 1;
        r.SourceRect = sr;
        DX_ENSURE(m_vp->SetVideoProcessStreamState(0/*stream index*/, DXVAHD_STREAM_STATE_SOURCE_RECT, sizeof(r), &r), false);
    }
#if 0
    DXVAHD_STREAM_STATE_D3DFORMAT_DATA d3df = {m_fmt};
    DX_ENSURE(m_vp->SetVideoProcessStreamState(0/*stream index*/, DXVAHD_STREAM_STATE_D3DFORMAT, sizeof(d3df), &d3df), false);
    DXVAHD_STREAM_STATE_FRAME_FORMAT_DATA ff = {DXVAHD_FRAME_FORMAT_PROGRESSIVE};
    DX_ENSURE(m_vp->SetVideoProcessStreamState(0/*stream index*/, DXVAHD_STREAM_STATE_FRAME_FORMAT, sizeof(ff), &ff), false);
    DXVAHD_STREAM_STATE_DESTINATION_RECT_DATA dstr = { true, {0, 0, desc.Width, desc.Height} };
    DX_ENSURE(m_vp->SetVideoProcessStreamState(0/*stream index*/, DXVAHD_STREAM_STATE_DESTINATION_RECT, sizeof(dstr), &dstr), false);
    DXVAHD_BLT_STATE_TARGET_RECT_DATA tgtr = {true, {0, 0, desc.Width, desc.Height} };
    DX_ENSURE(m_vp->SetVideoProcessBltState(DXVAHD_BLT_STATE_TARGET_RECT, sizeof(tgtr), &tgtr), false);
    for (int i = 0; i < 7; ++i) {
        DXVAHD_STREAM_STATE_FILTER_DATA flt = {false, DXVAHD_FILTER(i)};
        DXVAHD_STREAM_STATE state = static_cast<DXVAHD_STREAM_STATE>(DXVAHD_STREAM_STATE_FILTER_BRIGHTNESS + i);
        DX_ENSURE(m_vp->SetVideoProcessStreamState(0/*stream index*/, state, sizeof(flt), &flt), false);
    }
#endif

    DXVAHD_STREAM_STATE_OUTPUT_RATE_DATA rate;
    ZeroMemory(&rate, sizeof(rate));
    rate.OutputRate = DXVAHD_OUTPUT_RATE_NORMAL;
    DX_ENSURE(m_vp->SetVideoProcessStreamState(0/*stream index*/, DXVAHD_STREAM_STATE_OUTPUT_RATE, sizeof(rate), &rate), false);

    DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE_DATA ics;
    ZeroMemory(&ics, sizeof(ics));
    ics.Type = 0; // 0: video, 1: graphics
    ics.RGB_Range = 0; // full
    ics.YCbCr_Matrix = m_cs == ColorSpace_BT601 ? 0 : 1; //0: bt601, 1: bt709
    ics.YCbCr_xvYCC = m_range == ColorRange_Full ? 1 : 0;
    DX_ENSURE(m_vp->SetVideoProcessStreamState(0/*stream index*/, DXVAHD_STREAM_STATE_INPUT_COLOR_SPACE, sizeof(ics), &ics), false);

    DXVAHD_BLT_STATE_OUTPUT_COLOR_SPACE_DATA cs;
    ZeroMemory(&cs, sizeof(cs));
    cs.Usage = 0; // output usage. 0: for playback (default), 1: video processing (e.g. video editor)
    cs.RGB_Range = 1; //
    cs.YCbCr_Matrix = 1; //0: bt601, 1: bt709
    cs.YCbCr_xvYCC = 1;
    DX_ENSURE(m_vp->SetVideoProcessBltState(DXVAHD_BLT_STATE_OUTPUT_COLOR_SPACE, sizeof(cs), &cs), false);

    DXVAHD_STREAM_DATA stream;
    ZeroMemory(&stream, sizeof(stream));
    stream.Enable = true;
    stream.OutputIndex = 0;
    stream.InputFrameOrField = 0;
    stream.pInputSurface = surface;
    DX_ENSURE(m_vp->VideoProcessBltHD(m_out.Get(), 0, 1, &stream), false);
    return true;
}

bool DXVAHDVP::ensureResource(UINT width, UINT height, D3DFORMAT format)
{
    const bool dirty = width != m_w || height != m_h || m_fmt != format;
    if (dirty || !m_viddev) {
        if (!fDXVAHD_CreateDevice)
            return false;
        DXVAHD_RATIONAL fps = {0, 0};
        DXVAHD_CONTENT_DESC desc;
        desc.InputFrameFormat = DXVAHD_FRAME_FORMAT_PROGRESSIVE;
        desc.InputFrameRate = fps;
        desc.InputWidth = width;
        desc.InputHeight = height;
        desc.OutputFrameRate = fps;
        desc.OutputWidth = width;
        desc.OutputHeight = height;
        DX_ENSURE(fDXVAHD_CreateDevice(m_dev.Get(), &desc, DXVAHD_DEVICE_USAGE_PLAYBACK_NORMAL, NULL, &m_viddev), false);
    }

    // TODO: check when format is changed, or record supported formats
    DXVAHD_VPDEVCAPS caps;
    DX_ENSURE(m_viddev->GetVideoProcessorDeviceCaps(&caps), false);
    QScopedPointer<DXVAHD_VPCAPS, QScopedPointerArrayDeleter<DXVAHD_VPCAPS>> pVPCaps(new (std::nothrow) DXVAHD_VPCAPS[caps.VideoProcessorCount]);
    DX_ENSURE(m_viddev->GetVideoProcessorCaps(caps.VideoProcessorCount, pVPCaps.data()), false);
    QScopedPointer<D3DFORMAT, QScopedPointerArrayDeleter<D3DFORMAT>> fmts(new (std::nothrow) D3DFORMAT[caps.InputFormatCount]);
    DX_ENSURE(m_viddev->GetVideoProcessorInputFormats(caps.InputFormatCount, fmts.data()), false);
    bool fmt_found = false;
    for (UINT i = 0; i < caps.InputFormatCount; ++i) {
        if (fmts.data()[i] == format) {
            fmt_found = true;
            break;
        }
    }
    if (!fmt_found) {
        qDebug("input format is not supported by DXVAHD");
        return false;
    }
    if (dirty || !m_vp)
        DX_ENSURE(m_viddev->CreateVideoProcessor(&pVPCaps.data()[0].VPGuid, &m_vp), false);
    m_w = width;
    m_h = height;
    m_fmt = format;
    return true;
}
} //namespace dx
} //namespace QtAV
