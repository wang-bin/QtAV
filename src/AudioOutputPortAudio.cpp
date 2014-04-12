/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>
    
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


#include <QtAV/AudioOutputPortAudio.h>
#include <QtAV/private/AudioOutput_p.h>
#include "QtAV/prepost.h"
#include <portaudio.h>
#include <QtCore/QString>

namespace QtAV {

extern AudioOutputId AudioOutputId_PortAudio;
FACTORY_REGISTER_ID_AUTO(AudioOutput, PortAudio, "PortAudio")

void RegisterAudioOutputPortAudio_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioOutput, PortAudio, "PortAudio")
}

class AudioOutputPortAudioPrivate : public AudioOutputPrivate
{
public:
    AudioOutputPortAudioPrivate():
        initialized(false)
      ,outputParameters(new PaStreamParameters)
      ,stream(0)
    {
        PaError err = paNoError;
        if ((err = Pa_Initialize()) != paNoError) {
            qWarning("Error when init portaudio: %s", Pa_GetErrorText(err));
            available = false;
            return;
        }
        initialized = true;

        int numDevices = Pa_GetDeviceCount();
        for (int i = 0 ; i < numDevices ; ++i) {
            const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
            if (deviceInfo) {
                const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
                QString name = QString(hostApiInfo->name) + ": " + QString::fromLocal8Bit(deviceInfo->name);
                qDebug("audio device %d: %s", i, name.toUtf8().constData());
                qDebug("max in/out channels: %d/%d", deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels);
            }
        }
        memset(outputParameters, 0, sizeof(PaStreamParameters));
        outputParameters->device = Pa_GetDefaultOutputDevice();
        if (outputParameters->device == paNoDevice) {
            qWarning("PortAudio get device error!");
            available = false;
            return;
        }
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(outputParameters->device);
        max_channels = deviceInfo->maxOutputChannels;
        qDebug("DEFAULT max in/out channels: %d/%d", deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels);
        qDebug("audio device: %s", QString::fromLocal8Bit(Pa_GetDeviceInfo(outputParameters->device)->name).toUtf8().constData());
        outputParameters->hostApiSpecificStreamInfo = NULL;
        outputParameters->suggestedLatency = Pa_GetDeviceInfo(outputParameters->device)->defaultHighOutputLatency;
    }
    ~AudioOutputPortAudioPrivate() {
        if (initialized)
            Pa_Terminate(); //Do NOT call this if init failed. See document
        if (outputParameters) {
            delete outputParameters;
            outputParameters = 0;
        }
    }

    bool initialized;
    PaStreamParameters *outputParameters;
    PaStream *stream;
    double outputLatency;
};

AudioOutputPortAudio::AudioOutputPortAudio()
    :AudioOutput(*new AudioOutputPortAudioPrivate())
{
}

AudioOutputPortAudio::~AudioOutputPortAudio()
{
    close();
}

bool AudioOutputPortAudio::write()
{
    DPTR_D(AudioOutputPortAudio);
    QMutexLocker lock(&d.mutex);
    Q_UNUSED(lock);
    if (!d.available)
        return false;
    if (Pa_IsStreamStopped(d.stream))
        Pa_StartStream(d.stream);
#if KNOW_WHY
#ifndef Q_OS_MAC //?
    int diff = Pa_GetStreamWriteAvailable(d.stream) - d.outputLatency * d.sample_rate;
    if (diff > 0) {
        int newsize = diff * d.channels * sizeof(float);
        static char *a = new char[newsize];
        memset(a, 0, newsize);
        Pa_WriteStream(d.stream, a, diff);
    }
#endif
#endif //KNOW_WHY
    PaError err = Pa_WriteStream(d.stream, d.data.constData(), d.data.size()/audioFormat().channels()/audioFormat().bytesPerSample());
    if (err == paUnanticipatedHostError) {
        qWarning("Write portaudio stream error: %s", Pa_GetErrorText(err));
        return   false;
    }
    return true;
}
//TODO: what about planar, int8, int24 etc that FFmpeg or Pa not support?
static int toPaSampleFormat(AudioFormat::SampleFormat format)
{
    switch (format) {
    case AudioFormat::SampleFormat_Unsigned8:
        return paUInt8;
    case AudioFormat::SampleFormat_Signed16:
        return paInt16;
    case AudioFormat::SampleFormat_Signed32:
        return paInt32;
    case AudioFormat::SampleFormat_Float:
        return paFloat32;
    default:
        return paCustomFormat;
    }
}

//TODO: call open after audio format changed?
bool AudioOutputPortAudio::open()
{
    DPTR_D(AudioOutputPortAudio);
    QMutexLocker lock(&d.mutex);
    Q_UNUSED(lock);
    d.outputParameters->sampleFormat = toPaSampleFormat(audioFormat().sampleFormat());
    d.outputParameters->channelCount = audioFormat().channels();
    PaError err = Pa_OpenStream(&d.stream, NULL, d.outputParameters, audioFormat().sampleRate(), 0, paNoFlag, NULL, NULL);
    if (err == paNoError) {
        d.outputLatency = Pa_GetStreamInfo(d.stream)->outputLatency;
        d.available = true;
    } else {
        qWarning("Open portaudio stream error: %s", Pa_GetErrorText(err));
        d.available = false;
    }
    return err == paNoError;
}

bool AudioOutputPortAudio::close()
{
    DPTR_D(AudioOutputPortAudio);
    QMutexLocker lock(&d.mutex);
    Q_UNUSED(lock);
    bool available_old = d.available;
    d.available = false;
    PaError err = paNoError;
    if (!d.stream) {
        return true;
    }
    err = Pa_StopStream(d.stream); //may be already stopped: paStreamIsStopped
    if (err != paNoError) {
        d.available = available_old;
        qWarning("Stop portaudio stream error: %s", Pa_GetErrorText(err));
        //return err == paStreamIsStopped;
    }
    err = Pa_CloseStream(d.stream);
    if (err != paNoError) {
        d.available = available_old;
        qWarning("Close portaudio stream error: %s", Pa_GetErrorText(err));
        return false;
    }
    d.stream = NULL;
    return true;
}

} //namespace QtAV
