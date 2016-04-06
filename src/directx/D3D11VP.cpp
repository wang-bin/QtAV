#include "D3D11VP.h"
#include "utils/DirectXHelper.h"
#include "utils/Logger.h"
namespace QtAV {
namespace dx {

D3D11VP::D3D11VP(ComPtr<ID3D11Device> dev)
    : m_dev(dev)
{
    DX_ENSURE(m_dev.As(&m_viddev));
}

void D3D11VP::setOutput(ID3D11Texture2D *tex)
{
    m_out = tex;
}

void D3D11VP::setSourceRect(const QRect &r)
{
    m_srcRect = r;
}

bool D3D11VP::process(ID3D11Texture2D *texture, int index)
{
    if (!texture || !m_out)
        return false;
    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);
    if (!m_enum) { // TODO: check input size change?
        D3D11_VIDEO_PROCESSOR_CONTENT_DESC videoProcessorDesc = {
            D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE,
            { 0 }, desc.Width, desc.Height,
            { 0 }, desc.Width, desc.Height,
            D3D11_VIDEO_USAGE_PLAYBACK_NORMAL //D3D11_VIDEO_USAGE_OPTIMAL_SPEED
        };
        DX_ENSURE(m_viddev->CreateVideoProcessorEnumerator(&videoProcessorDesc, &m_enum), false);
        UINT flags;
        // TODO: check when format is changed, or record supported formats
        DX_ENSURE(m_enum->CheckVideoProcessorFormat(desc.Format, &flags), false);
        if (!(flags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT)) {
            qWarning("unsupported input format for d3d11 video processor: %d", desc.Format);
            return false;
        }
    }
    if (!m_vp)
        DX_ENSURE(m_viddev->CreateVideoProcessor(m_enum.Get(), 0, &m_vp), false);
    if (!m_outview) {
        D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outputDesc = { D3D11_VPOV_DIMENSION_TEXTURE2D };
        DX_ENSURE(m_viddev->CreateVideoProcessorOutputView(m_out.Get(), m_enum.Get(), &outputDesc, &m_outview), false);
    }
    D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inputViewDesc = {
        0, D3D11_VPIV_DIMENSION_TEXTURE2D, { 0, (UINT)index }
    };
    ComPtr<ID3D11VideoProcessorInputView> inputView;
    DX_ENSURE(m_viddev->CreateVideoProcessorInputView(
                texture, m_enum.Get(), &inputViewDesc, &inputView), false);
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11VideoContext> videoContext;
    m_dev->GetImmediateContext(&context);
    DX_ENSURE(context.As(&videoContext), false);
    if (!m_srcRect.isEmpty()) {
        const RECT r = {m_srcRect.x(), m_srcRect.y(), m_srcRect.width(), m_srcRect.height()};
        videoContext->VideoProcessorSetStreamSourceRect(m_vp.Get(), 0, TRUE, &r);
    }
    D3D11_VIDEO_PROCESSOR_STREAM stream = { TRUE };
    stream.pInputSurface = inputView.Get();
    DX_ENSURE(videoContext->VideoProcessorBlt(m_vp.Get(), m_outview.Get(), 0, 1, &stream), false);
    return true;
}
} //namespace dx
} //namespace QtAV
