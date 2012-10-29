#include <QtAV/AudioOutput.h>
#include <portaudio.h>

namespace QtAV {
AudioOutput::AudioOutput()
    :AVOutput()
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
}
