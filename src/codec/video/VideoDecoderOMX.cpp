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

class VideoDecoderOMXPrivate : public VideoDecoderPrivate, public omx::component
{
public:
    VideoDecoderOMXPrivate() : VideoDecoderPrivate()
    {}

    OMX_ERRORTYPE onEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData) Q_DECL_OVERRIDE;
    OMX_ERRORTYPE onEmptyBufferDone(OMX_BUFFERHEADERTYPE* pBuffer) Q_DECL_OVERRIDE;
    OMX_ERRORTYPE onFillBufferDone(OMX_BUFFERHEADERTYPE* pBufferHeader) Q_DECL_OVERRIDE;

    bool open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

    QHash<OMX_BUFFERHEADERTYPE*, omx_buf> in_buffers;
    BlockingQueue<omx_buf> in_buffers_free;
    BlockingQueue<OMX_BUFFERHEADERTYPE*> out_buffers;
};

static const struct
{
    AVCodecID codec;
    OMX_VIDEO_CODINGTYPE omx_type;
    const char *role;
} codec_role_map[] = {
    { QTAV_CODEC_ID(MPEG2VIDEO), OMX_VIDEO_CodingMPEG2, "video_decoder.mpeg2" },
    { QTAV_CODEC_ID(MPEG4), OMX_VIDEO_CodingMPEG4, "video_decoder.mpeg4" },
    { QTAV_CODEC_ID(HEVC), OMX_VIDEO_CodingAutoDetect, "video_decoder.hevc" },
    { QTAV_CODEC_ID(H264), OMX_VIDEO_CodingAVC,   "video_decoder.avc"   },
    { QTAV_CODEC_ID(H263), OMX_VIDEO_CodingH263,  "video_decoder.h263"  },
    { QTAV_CODEC_ID(WMV1), OMX_VIDEO_CodingWMV,   "video_decoder.wmv1"  },
    { QTAV_CODEC_ID(WMV2), OMX_VIDEO_CodingWMV,   "video_decoder.wmv2"  },
    { QTAV_CODEC_ID(WMV3), OMX_VIDEO_CodingWMV,   "video_decoder.wmv"   },
    { QTAV_CODEC_ID(VC1), OMX_VIDEO_CodingWMV,   "video_decoder.wmv"   },
    { QTAV_CODEC_ID(MJPEG), OMX_VIDEO_CodingMJPEG, "video_decoder.jpeg"  },
    { QTAV_CODEC_ID(MJPEG), OMX_VIDEO_CodingMJPEG, "video_decoder.mjpeg" },
    { QTAV_CODEC_ID(RV10), OMX_VIDEO_CodingRV,    "video_decoder.rv"    },
    { QTAV_CODEC_ID(RV20), OMX_VIDEO_CodingRV,    "video_decoder.rv"    },
    { QTAV_CODEC_ID(RV30), OMX_VIDEO_CodingRV,    "video_decoder.rv"    },
    { QTAV_CODEC_ID(RV40), OMX_VIDEO_CodingRV,    "video_decoder.rv"    },
    { QTAV_CODEC_ID(VP8), OMX_VIDEO_CodingAutoDetect, "video_decoder.vp8" },
    { QTAV_CODEC_ID(VP9), OMX_VIDEO_CodingAutoDetect, "video_decoder.vp9" },
    { QTAV_CODEC_ID(NONE), OMX_VIDEO_CodingUnused, 0 }
};

const char* CodecToOMXRole(AVCodecID codec, OMX_VIDEO_CODINGTYPE* omx = NULL)
{
    for (int i = 0; codec_role_map[i].role; ++i) {
        if (codec_role_map[i].codec != codec)
            continue;
        if (omx)
            *omx = codec_role_map[i].omx_type;
        return codec_role_map[i].role;
    }
    return NULL;
}

OMX_ERRORTYPE VideoDecoderOMXPrivate::onEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoDecoderOMXPrivate::onEmptyBufferDone(OMX_BUFFERHEADERTYPE *pBuffer)
{
    in_buffers_free.put(in_buffers[pBuffer]);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoDecoderOMXPrivate::onFillBufferDone(OMX_BUFFERHEADERTYPE *pBufferHeader)
{
    return OMX_ErrorNone;
}

bool VideoDecoderOMXPrivate::open()
{
    init();
    const char* role_name = CodecToOMXRole(codec_ctx->codec_id);

    return true;
}

void VideoDecoderOMXPrivate::close()
{
    deinit();
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
    OMX_ENSURE(OMX_EmptyThisBuffer(d.handle(), h), false);
    return true;
}

VideoFrame VideoDecoderOMX::frame()
{
    DPTR_D(VideoDecoderOMX);
    if (d.out_buffers.isEmpty())
        return VideoFrame();
    OMX_BUFFERHEADERTYPE* h = d.out_buffers.take();
    OMX_ENSURE(OMX_FillThisBuffer(d.handle(), h), VideoFrame());
    VideoFrame f;
    return f;
}
} //namespace QtAV
