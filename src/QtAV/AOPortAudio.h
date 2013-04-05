/******************************************************************************
	QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
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
    ~AOPortAudio();

    bool open();
    bool close();

protected:
    bool write();
};

} //namespace QtAV
#endif // QTAV_AOPORTAUDIO_H
