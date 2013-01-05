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


#include <QtAV/AOPortAudio.h>
#include <private/AOPortAudio_p.h>

namespace QtAV {

AOPortAudio::AOPortAudio()
    :AudioOutput(*new AOPortAudioPrivate())
{
}

AOPortAudio::~AOPortAudio()
{
    close();
}

bool AOPortAudio::write()
{
    DPTR_D(AOPortAudio);
    if (Pa_IsStreamStopped(d.stream))
        Pa_StartStream(d.stream);

#ifndef Q_OS_MAC //?
    int diff = Pa_GetStreamWriteAvailable(d.stream) - d.outputLatency * d.sample_rate;
    if (diff > 0) {
        int newsize = diff * d.channels * sizeof(float);
        char a[newsize];
        memset(a, 0, newsize);
        Pa_WriteStream(d.stream, a, diff);
    }
#endif
#ifdef Q_OS_LINUX
    int chn = d.channels;
    if (chn == 6 || chn == 8) {
		float *audio_buffer = (float *)d.data.data();
		int size_per_chn = d.data.size() >> 2;
        for (int i = 0 ; i < size_per_chn; i += chn) {
            float tmp = audio_buffer[i+2];
            audio_buffer[i+2] = audio_buffer[i+4];
            audio_buffer[i+4] = tmp;
            tmp = audio_buffer[i+3];
            audio_buffer[i+3] = audio_buffer[i+5];
            audio_buffer[i+5] = tmp;
        }
    }
#endif

	PaError err = Pa_WriteStream(d.stream, d.data.data(), d.data.size()/d.channels/sizeof(float));
    if (err == paUnanticipatedHostError) {
        qWarning("Write portaudio stream error: %s", Pa_GetErrorText(err));
		return false;
    }
	return true;
}

bool AOPortAudio::open()
{
    DPTR_D(AOPortAudio);
    //
    d.outputParameters->channelCount = d.channels;
    PaError err = Pa_OpenStream(&d.stream, NULL, d.outputParameters, d.sample_rate, 0, paNoFlag, NULL, NULL);
    if (err == paNoError) {
        d.outputLatency = Pa_GetStreamInfo(d.stream)->outputLatency;
        d.available = true;
    } else {
        qWarning("Open portaudio stream error: %s", Pa_GetErrorText(err));
        d.available = false;
    }
    return err == paNoError;
}

bool AOPortAudio::close()
{
    DPTR_D(AOPortAudio);
    PaError err = paNoError;
    if (!d.stream) {
        return true;
    }
    err = Pa_StopStream(d.stream);
    if (err != paNoError)
        qWarning("Stop portaudio stream error: %s", Pa_GetErrorText(err));
    err = Pa_CloseStream(d.stream);
    d.stream = NULL;
    if (err != paNoError)
        qWarning("Close portaudio stream error: %s", Pa_GetErrorText(err));
    return err == paNoError;
}

} //namespace QtAV
