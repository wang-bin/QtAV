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


#ifndef QTAV_AOPORTAUDIO_H
#define QTAV_AOPORTAUDIO_H

#include <QtAV/AudioOutput.h>

namespace QtAV {

class AOPortAudioPrivate;
class Q_EXPORT AOPortAudio : public AudioOutput
{
    DPTR_DECLARE_PRIVATE(AOPortAudio)
public:
    AOPortAudio();
    virtual ~AOPortAudio();

    virtual int write(const QByteArray& data);

    virtual bool open();
    virtual bool close();

};

} //namespace QtAV
#endif // QTAV_AOPORTAUDIO_H
