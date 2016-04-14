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

#include "QtAV/VideoDecoder.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVDecoder_p.h"
#include "QtAV/private/factory.h"
#include <QtCore/QQueue>
#include "QtAV/private/AVCompat.h"
#include "utils/BlockingQueue.h"
#include "openmax/OMXHelper.h"
#include "utils/Logger.h"

namespace QtAV {

class VideoDecoderOMXPrivate;
class VideoDecoderOMX : public VideoDecoder
{
    DPTR_DECLARE_PRIVATE(VideoDecoderOMX)
public:
    VideoDecoderOMX();
    bool decode(const Packet& packet) Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;

};

struct omx_buf {
    OMX_BUFFERHEADERTYPE *header;
    QByteArray data;
};

class VideoDecoderOMXPrivate : public VideoDecoderPrivate, protected omx::core
{
public:
    static OMX_ERRORTYPE DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
    static OMX_ERRORTYPE DecoderEmptyBufferDone(OMX_HANDLETYPE hComponent,  OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);
    static OMX_ERRORTYPE DecoderFillBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader);

    bool open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

    OMX_HANDLETYPE dec;
    QHash<OMX_BUFFERHEADERTYPE*, omx_buf> in_buffers;
    BlockingQueue<omx_buf> in_buffers_free;
    BlockingQueue<OMX_BUFFERHEADERTYPE*> out_buffers;
};

OMX_ERRORTYPE VideoDecoderOMXPrivate::DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoDecoderOMXPrivate::DecoderEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE *pBuffer)
{
    Q_UNUSED(hComponent);
    VideoDecoderOMXPrivate *p = static_cast<VideoDecoderOMXPrivate*>(pAppData);
    p->in_buffers_free.put(p->in_buffers[pBuffer]);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoDecoderOMXPrivate::DecoderFillBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE *pBufferHeader)
{
    return OMX_ErrorNone;
}

bool VideoDecoderOMXPrivate::open()
{

    OMX_ENSURE(OMX_Init(), false);
    static OMX_CALLBACKTYPE cb = {
        DecoderEventHandler,
        DecoderEmptyBufferDone,
        DecoderFillBufferDone
    };
    OMX_ENSURE(OMX_GetHandle(&dec, "...", this, &cb), false);
    return true;
}

void VideoDecoderOMXPrivate::close()
{
    OMX_WARN(OMX_Deinit());
}

bool VideoDecoderOMX::decode(const Packet &packet)
{
    DPTR_D(VideoDecoderOMX);
    omx_buf buf = d.in_buffers_free.take();
    buf.data = packet.data;
    OMX_BUFFERHEADERTYPE* h = buf.header;
    h->nFlags = packet.isEOF() ? OMX_BUFFERFLAG_EOS : 0;
    h->nOffset = 0;
    h->pBuffer = (OMX_U8*)buf.data.constData();
    h->nAllocLen = buf.data.size();
    h->nFilledLen = buf.data.size();
    h->nTimeStamp = packet.pts*1000.0*1000.0; //us
    h->pAppPrivate = &d;
    //h->nInputPortIndex = ;
    OMX_ENSURE(OMX_EmptyThisBuffer(d.dec, h), false);
    return true;
}

VideoFrame VideoDecoderOMX::frame()
{
    DPTR_D(VideoDecoderOMX);
    if (d.out_buffers.isEmpty())
        return VideoFrame();
    OMX_BUFFERHEADERTYPE* h = d.out_buffers.take();
    OMX_ENSURE(OMX_FillThisBuffer(d.dec, h), VideoFrame());
    VideoFrame f;
    return f;
}
} //namespace QtAV
