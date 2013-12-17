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

#include "QtAV/Statistics.h"

namespace QtAV {

Statistics::Common::Common():
    available(false)
  , bit_rate(0)
  , frames(0)
  , size(0)
{
}

Statistics::AudioOnly::AudioOnly():
    sample_rate(0)
  , channels(0)
  , frame_size(0)
  , frame_number(0)
  , block_align(0)
{
}

Statistics::VideoOnly::VideoOnly()
    : fps_guess(0)
    , fps(0)
    , avg_frame_rate(0)
  , width(0)
  , height(0)
  , coded_width(0)
  , coded_height(0)
  , gop_size(0)
{
}

void Statistics::VideoOnly::putPts(qreal pts)
{
    ptsHistory.push_back(pts);
    if (ptsHistory.size() < 2)
        return;
    if (pts - ptsHistory.at(0) > 1) {
        ptsHistory.pop_front();
    }
}

qreal Statistics::VideoOnly::currentDisplayFPS() const
{
    if (ptsHistory.size() < 2)
        return 0;
    return (qreal)ptsHistory.size()/(ptsHistory.last() - ptsHistory.first());
}

Statistics::Statistics()
{
}

Statistics::~Statistics()
{
}

void Statistics::reset()
{
    url = "";
    audio = Common();
    video = Common();
    audio_only = AudioOnly();
    video_only = VideoOnly();
}

} //namespace QtAV
