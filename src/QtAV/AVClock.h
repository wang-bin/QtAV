/******************************************************************************
    AVClock.h: description
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


#ifndef AVCLOCK_H
#define AVCLOCK_H

#include <qglobal.h>

namespace QtAV {

class AVClock
{
public:
    typedef enum {
        AudioClock, VideoClock, ExternalClock
    } ClockType;

    AVClock(ClockType c = AudioClock);

    inline qreal value() const;
    inline void updateValue(qreal pts);

private:
    ClockType clock_type;
    qreal t;


};

qreal AVClock::value() const
{
    return t;
}

void AVClock::updateValue(qreal pts)
{
    t = pts;
}

} //namespace QtAV
#endif // AVCLOCK_H
