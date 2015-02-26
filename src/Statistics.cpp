/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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
  , d(new Private())
{
}

Statistics::AudioOnly::AudioOnly():
    sample_rate(0)
  , channels(0)
  , frame_size(0)
  , block_align(0)
  , d(new Private())
{
}

Statistics::VideoOnly::Private::Private()
    : pts(0)
{
}

Statistics::VideoOnly::VideoOnly():
    frame_rate(0)
  , width(0)
  , height(0)
  , coded_width(0)
  , coded_height(0)
  , gop_size(0)
  , d(new Private())
{
}

qreal Statistics::VideoOnly::pts() const
{
    return d->pts;
}

void Statistics::VideoOnly::frameDisplayed(qreal pts)
{
    d->pts = pts;
    const qreal t = (double)QDateTime::currentMSecsSinceEpoch()/1000.0;
    d->history.push_back(t);
    if (d->history.size() > 60) {
        d->history.pop_front();
    }
    if (t - d->history.at(0) > 1.0) {
        d->history.pop_front();
    }
}
// d->history is not thread safe!
qreal Statistics::VideoOnly::currentDisplayFPS() const
{
    if (d->history.isEmpty())
        return 0;
    // DO NOT use d->history.last-first
    const qreal dt = (double)QDateTime::currentMSecsSinceEpoch()/1000.0 - d->history.first();
    if (qFuzzyIsNull(dt))
        return 0;
    return (qreal)d->history.size()/dt;
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
