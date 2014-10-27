/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_H
#define QTAV_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/version.h>

#include <QtAV/AVError.h>
#include <QtAV/AVClock.h>
#include <QtAV/AVDecoder.h>
#include <QtAV/AVDemuxer.h>
#include <QtAV/AVOutput.h>
#include <QtAV/AVPlayer.h>
#include <QtAV/Packet.h>
#include <QtAV/Statistics.h>

#include <QtAV/AudioDecoder.h>
#include <QtAV/AudioFormat.h>
#include <QtAV/AudioOutput.h>
#include <QtAV/AudioOutputTypes.h>
#include <QtAV/AudioResampler.h>
#include <QtAV/AudioResamplerTypes.h>

#include <QtAV/Filter.h>
#include <QtAV/FilterContext.h>

#include <QtAV/ImageConverter.h>
#include <QtAV/ImageConverterTypes.h>

#include <QtAV/VideoShader.h>
#include <QtAV/OpenGLVideo.h>

#include <QtAV/VideoCapture.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/VideoDecoderTypes.h>
#include <QtAV/VideoFormat.h>
#include <QtAV/VideoFrame.h>
#include <QtAV/VideoFrameExtractor.h>
#include <QtAV/VideoRenderer.h>
#include <QtAV/VideoRendererTypes.h>
#include <QtAV/VideoOutput.h>
//The following renderer headers can be removed
#include <QtAV/QPainterRenderer.h>
#include <QtAV/GraphicsItemRenderer.h>
#include <QtAV/WidgetRenderer.h>
#include <QtAV/GLWidgetRenderer.h>
#include <QtAV/GLWidgetRenderer2.h>

#include <QtAV/Subtitle.h>
#include <QtAV/SubtitleFilter.h>

#endif // QTAV_H
