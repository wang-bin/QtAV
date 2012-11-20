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

    inline double pts() const;
    inline double value() const; //the real timestamp: pts + delay
    inline void updateValue(double pts); //update the pts
    inline void updateVideoPts(double pts);
    inline double videoPts() const;
    inline double delay() const; //playing audio spends some time
    inline void updateDelay(double delay);

private:
    ClockType clock_type;
    double pts_, pts_v;
    double delay_;
};

double AVClock::value() const
{
    return pts_ + delay_;
}

void AVClock::updateValue(double pts)
{
    pts_ = pts;
}

void AVClock::updateVideoPts(double pts)
{
    pts_v = pts;
}

double AVClock::videoPts() const
{
    return pts_v;
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
