/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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

#ifndef QTAV_D3D11VP_H
#define QTAV_D3D11VP_H
#include <d3d11.h>
#include <wrl/client.h>
#include <QtAV/VideoFormat.h>
using namespace Microsoft::WRL;
namespace QtAV {
namespace dx {

class D3D11VP
{
public:
    // brightness, contrast, hue, saturation, rotation, color space, range, source/dest rect
    D3D11VP(ComPtr<ID3D11Device> dev);
    void setOutput(ID3D11Texture2D* tex);

    bool process(ID3D11Texture2D *texture);
private:
    ComPtr<ID3D11Device> m_dev;
    ID3D11Texture2D *m_out;
    ComPtr<ID3D11VideoDevice> m_viddev;
    ComPtr<ID3D11VideoProcessorEnumerator> m_enum;
    ComPtr<ID3D11VideoProcessor> m_vp;
    ComPtr<ID3D11VideoProcessorOutputView> m_outview;
};
} //namespace dx
} //namespace QtAV
#endif //QTAV_D3D11VP_H
