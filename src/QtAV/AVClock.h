/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef QTAV_AVCLOCK_H
#define QTAV_AVCLOCK_H

#include <QtAV/QtAV_Global.h>

namespace QtAV {

class Q_EXPORT AVClock
{
public:
    typedef enum {
        AudioClock, VideoClock, ExternalClock
    } ClockType;

    AVClock(ClockType c = AudioClock);

    inline double value() const;
    inline void updateValue(double pts);

    inline double delay() const; //playing audio spends some time
    inline void updateDelay(double delay);

private:
    ClockType clock_type;
    double t;
    double delay_;
};

double AVClock::value() const
{
    return t;
}

void AVClock::updateValue(double pts)
{
    t = pts;
}

double AVClock::delay() const
{
    return delay_;
}

void AVClock::updateDelay(double delay)
{
    delay_ = delay;
}

} //namespace QtAV
#endif // QTAV_AVCLOCK_H
