/******************************************************************************
	QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
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


#ifndef QTAV_AUDIOOUTPUT_P_H
#define QTAV_AUDIOOUTPUT_P_H

#include <private/AVOutput_p.h>

namespace QtAV {

class Q_EXPORT AudioOutputPrivate : public AVOutputPrivate
{
public:
    AudioOutputPrivate():mute(false),channels(2)
      ,vol(1),sample_rate(44100)
    {
    }
    virtual ~AudioOutputPrivate(){}
    bool mute;
    int channels;
    qreal vol;
    int sample_rate;
	QByteArray data;
};

} //namespace QtAV
#endif // QTAV_AUDIOOUTPUT_P_H
