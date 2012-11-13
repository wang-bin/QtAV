/******************************************************************************
    AOPortAudio_p.h: description
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


#ifndef QTAV_AOPORTAUDIO_P_H
#define QTAV_AOPORTAUDIO_P_H

#include <private/AudioOutput_p.h>
#include <portaudio.h>

namespace QtAV {

class Q_EXPORT AOPortAudioPrivate : public AudioOutputPrivate
{
public:
    AOPortAudioPrivate():outputParameters(new PaStreamParameters)
      ,stream(0)
    {
        Pa_Initialize();

        memset(outputParameters, 0, sizeof(PaStreamParameters));
        outputParameters->device = Pa_GetDefaultOutputDevice();
        outputParameters->sampleFormat = paFloat32;
        outputParameters->hostApiSpecificStreamInfo = NULL;
        outputParameters->suggestedLatency = Pa_GetDeviceInfo(outputParameters->device)->defaultHighOutputLatency;

    }
    ~AOPortAudioPrivate() {
        Pa_Terminate();
        if (outputParameters) {
            delete outputParameters;
            outputParameters = 0;
        }
    }

    PaStreamParameters *outputParameters;
    PaStream *stream;
#ifdef Q_OS_LINUX
    bool autoFindMultichannelDevice;
#endif
    double outputLatency;
};

} //namespace QtAV
#endif // QTAV_AOPORTAUDIO_P_H
