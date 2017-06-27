/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

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
#include "utils/ring.h"

namespace QtAV {

Statistics::Common::Common():
    available(false)
  , bit_rate(0)
  , frames(0)
  , frame_rate(0)
{
}

Statistics::AudioOnly::AudioOnly():
    sample_rate(0)
  , channels(0)
  , frame_size(0)
  , block_align(0)
{
}

class Statistics::VideoOnly::Private : public QSharedData {
public:
    Private()
        : pts(0)
        , history(ring<qreal>(30))
    {}
    qreal pts;
    ring<qreal> history;
};

Statistics::VideoOnly::VideoOnly():
    width(0)
  , height(0)
  , coded_width(0)
  , coded_height(0)
  , gop_size(0)
  , rotate(0)
  , d(new Private())
{
}

Statistics::VideoOnly::VideoOnly(const VideoOnly& v)
  : width(v.width)
  , height(v.height)
  , coded_width(v.coded_width)
  , coded_height(v.coded_height)
  , gop_size(v.gop_size)
  , rotate(v.rotate)
  , d(v.d)
{
}

Statistics::VideoOnly& Statistics::VideoOnly::operator =(const VideoOnly& v)
{
    width = v.width;
    height = v.height;
    coded_width = v.coded_width;
    coded_height = v.coded_height;
    gop_size = v.gop_size;
    rotate = v.rotate;
    d = v.d;
    return *this;
}

Statistics::VideoOnly::~VideoOnly()
{
}

qreal Statistics::VideoOnly::pts() const
{
    return d->pts;
}

qint64 Statistics::VideoOnly::frameDisplayed(qreal pts)
{
    d->pts = pts;
    const qint64 msecs = QDateTime::currentMSecsSinceEpoch();
    const qreal t = (double)msecs/1000.0;
    d->history.push_back(t);
    return msecs;
}
// d->history is not thread safe!
qreal Statistics::VideoOnly::currentDisplayFPS() const
{
    if (d->history.empty())
        return 0;
    // DO NOT use d->history.last-first
    const qreal dt = (double)QDateTime::currentMSecsSinceEpoch()/1000.0 - d->history.front();
    // dt should be always > 0 because history stores absolute time
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
    url = QString();
    audio = Common();
    video = Common();
    audio_only = AudioOnly();
    video_only = VideoOnly();
    metadata.clear();
}

} //namespace QtAV
