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

#include <QtAV/AudioOutput.h>
#include <portaudio.h>
#if 0
//#include <guiddef.h>
typedef struct _GUID
{
unsigned long Data1;
unsigned short Data2;
unsigned short Data3;
unsigned char Data4[8];
} GUID;
extern const GUID GUID_NULL = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
#endif
namespace QtAV {
AudioOutput::AudioOutput()
    :AVOutput(),mute(false),vol(1)
{
    outputParameters = new PaStreamParameters;
    Pa_Initialize();

    stream = NULL;
    sample_rate = 0;
    memset(outputParameters, 0, sizeof(PaStreamParameters));
    outputParameters->device = Pa_GetDefaultOutputDevice();
    outputParameters->sampleFormat = paFloat32;
    outputParameters->hostApiSpecificStreamInfo = NULL;
    outputParameters->suggestedLatency = Pa_GetDeviceInfo(outputParameters->device)->defaultHighOutputLatency;
}

AudioOutput::~AudioOutput()
{
    close();
    Pa_Terminate();
    if (outputParameters) {
        delete outputParameters;
        outputParameters = 0;
    }
}

void AudioOutput::setSampleRate(int rate)
{
    sample_rate = rate;
}

void AudioOutput::setChannels(int channels)
{
    outputParameters->channelCount = channels;
}

int AudioOutput::write(const QByteArray &data)
{
    //qDebug("%s", __PRETTY_FUNCTION__);
    tryPause(); //Pa_AbortStream?
    if (Pa_IsStreamStopped(stream))
        Pa_StartStream(stream);

#ifndef Q_OS_MAC //?
    int diff = Pa_GetStreamWriteAvailable(stream) - outputLatency * sample_rate;
    if (diff > 0) {
        int newsize = diff * outputParameters->channelCount * sizeof(float);
        char a[newsize];
        memset(a, 0, newsize);
        Pa_WriteStream(stream, a, diff);
    }
#endif

#ifdef Q_OS_LINUX
    int chn = outputParameters->channelCount;
	if (chn == 6 || chn == 8) {
		float *audio_buffer = (float *)data.data();
		int size_per_chn = data.size() >> 2;
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

    PaError err = Pa_WriteStream(stream, data.data(), data.size() / outputParameters->channelCount / sizeof(float));
    if (err == paUnanticipatedHostError) {
        qWarning("Write portaudio stream error: %s", Pa_GetErrorText(err));
        return 0;
    }

    return data.size();
}

bool AudioOutput::open()
{
    PaError err = Pa_OpenStream(&stream, NULL, outputParameters, sample_rate, 0, paNoFlag, NULL, NULL);
    if (err == paNoError)
        outputLatency = Pa_GetStreamInfo(stream)->outputLatency;
    else
        qWarning("Open portaudio stream error: %s", Pa_GetErrorText(err));
    return err == paNoError;
}

bool AudioOutput::close()
{
    PaError err = paNoError;
    if (!stream) {
        return true;
    }
    err = Pa_StopStream(stream);
    if (err != paNoError)
        qWarning("Stop portaudio stream error: %s", Pa_GetErrorText(err));
    err = Pa_CloseStream(stream);
    stream = NULL;
    if (err != paNoError)
        qWarning("Close portaudio stream error: %s", Pa_GetErrorText(err));
    return err == paNoError;
}

void AudioOutput::setVolume(qreal volume)
{
    vol = qMax<qreal>(volume, 0);
    mute = vol == 0;
}

qreal AudioOutput::volume() const
{
    return qMax<qreal>(vol, 0);
}

void AudioOutput::setMute(bool yes)
{
    mute = yes;
}

bool AudioOutput::isMute() const
{
    return mute;
}

} //namespace QtAV
