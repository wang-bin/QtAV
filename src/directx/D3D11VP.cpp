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

#include "D3D11VP.h"
#define DX_LOG_COMPONENT "D3D11VP"
#include "utils/DirectXHelper.h"
#include "utils/Logger.h"

// define __mingw_uuidof
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(ID3D11VideoContext,0x61F21C45,0x3C0E,0x4a74,0x9C,0xEA,0x67,0x10,0x0D,0x9A,0xD5,0xE4)
__CRT_UUID_DECL(ID3D11VideoDevice,0x10EC4D5B,0x975A,0x4689,0xB9,0xE4,0xD0,0xAA,0xC3,0x0F,0xE3,0x33)
#endif //__CRT_UUID_DECL

namespace QtAV {
namespace dx {

D3D11VP::D3D11VP(ComPtr<ID3D11Device> dev)
    : m_dev(dev)
    , m_w(0)
    , m_h(0)
    , m_cs(ColorSpace_BT709)
    , m_range(ColorRange_Limited)
{
    DX_ENSURE(m_dev.As(&m_viddev));
}

void D3D11VP::setOutput(ID3D11Texture2D *tex)
{
    m_out = tex;
    m_outview.Reset();
}

void D3D11VP::setSourceRect(const QRect &r)
{
    m_srcRect = r;
}

void D3D11VP::setColorSpace(ColorSpace value)
{
    m_cs = value;
}

void D3D11VP::setColorRange(ColorRange value)
{
    m_range = value;
}

bool D3D11VP::process(ID3D11Texture2D *texture, int index)
{
    if (!texture || !m_out)
        return false;
    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);
    if (!ensureResource(desc.Width, desc.Height, desc.Format))
        return false;

    D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inviewDesc = {
        0, D3D11_VPIV_DIMENSION_TEXTURE2D, { 0, (UINT)index }
    };
    ComPtr<ID3D11VideoProcessorInputView> inview;
    DX_ENSURE(m_viddev->CreateVideoProcessorInputView(
                texture, m_enum.Get(), &inviewDesc, &inview), false);
    ComPtr<ID3D11DeviceContext> ctx;
    ComPtr<ID3D11VideoContext> videoctx;
    m_dev->GetImmediateContext(&ctx);
    DX_ENSURE(ctx.As(&videoctx), false);
    if (!m_srcRect.isEmpty()) {
        const RECT r = {m_srcRect.x(), m_srcRect.y(), m_srcRect.width(), m_srcRect.height()};
        videoctx->VideoProcessorSetStreamSourceRect(m_vp.Get(), 0, TRUE, &r);
    }
    // disable additional processing. this can fix the output frame is too dark, also make in/out color space parameters work
    videoctx->VideoProcessorSetStreamAutoProcessingMode(m_vp.Get(), 0, FALSE);
    D3D11_VIDEO_PROCESSOR_COLOR_SPACE cs;
    ZeroMemory(&cs, sizeof(cs));
    cs.YCbCr_Matrix = m_cs == ColorSpace_BT601 ? 0 : 1; //0: bt601, 1: bt709
    // D3D11_VIDEO_PROCESSOR_NOMINAL_RANGE_xxx is desktop only?
    cs.Nominal_Range = m_range == ColorRange_Full ? D3D11_VIDEO_PROCESSOR_NOMINAL_RANGE_0_255 : D3D11_VIDEO_PROCESSOR_NOMINAL_RANGE_16_235;
    videoctx->VideoProcessorSetStreamColorSpace(m_vp.Get(), 0, &cs);
#if 0
    cs.RGB_Range = 1; // 0: full, 1: limited
    videoctx->VideoProcessorSetOutputColorSpace(m_vp.Get(), &cs);
#endif
    D3D11_VIDEO_PROCESSOR_STREAM stream;
    ZeroMemory(&stream, sizeof(stream));
    stream.Enable = TRUE;
    stream.pInputSurface = inview.Get();
    DX_ENSURE(videoctx->VideoProcessorBlt(m_vp.Get(), m_outview.Get(), 0, 1, &stream), false);
    return true;
}

bool D3D11VP::ensureResource(UINT width, UINT height, DXGI_FORMAT format)
{
    bool dirty = width != m_w || height != m_h;
    if (dirty || !m_enum) {
        D3D11_VIDEO_PROCESSOR_CONTENT_DESC vpdesc = {
            D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE,
            { 0, 0 }, width, height,
            { 0, 0 }, width, height,
            D3D11_VIDEO_USAGE_PLAYBACK_NORMAL //D3D11_VIDEO_USAGE_OPTIMAL_SPEED
        };
        DX_ENSURE(m_viddev->CreateVideoProcessorEnumerator(&vpdesc, &m_enum), false);
    }

    UINT flags;
    // TODO: check when format is changed, or record supported formats
    DX_ENSURE(m_enum->CheckVideoProcessorFormat(format, &flags), false);
    if (!(flags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT)) {
        qWarning("unsupported input format for d3d11 video processor: %d", format);
        return false;
    }

    if (dirty || !m_vp)
        DX_ENSURE(m_viddev->CreateVideoProcessor(m_enum.Get(), 0, &m_vp), false);
    if (dirty || !m_outview) {
        D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outputDesc;
        ZeroMemory(&outputDesc, sizeof(outputDesc));
        outputDesc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
        DX_ENSURE(m_viddev->CreateVideoProcessorOutputView(m_out.Get(), m_enum.Get(), &outputDesc, &m_outview), false);
    }

    m_w = width;
    m_h = height;
    return true;
}
} //namespace dx
} //namespace QtAV
