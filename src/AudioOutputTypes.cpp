#include "QtAV/AudioOutputTypes.h"
#include "QtAV/FactoryDefine.h"
#include "factory.h"

namespace QtAV {

AudioOutputId AudioOutputId_PortAudio = 1;
AudioOutputId AudioOutputId_OpenAL = 2;
AudioOutputId AudioOutputId_OpenSL = 3;

QVector<AudioOutputId> GetRegistedAudioOutputIds()
{
    return QVector<AudioOutputId>::fromStdVector(AudioOutputFactory::registeredIds());
}


FACTORY_DEFINE(AudioOutput)

extern void RegisterAudioOutputPortAudio_Man();
extern void RegisterAudioOutputOpenAL_Man();
extern void RegisterAudioOutputOpenSL_Man();

void AudioOutput_RegisterAll()
{
#if QTAV_HAVE(PORTAUDIO)
    RegisterAudioOutputPortAudio_Man();
#endif //QTAV_HAVE(PORTAUDIO)
#if QTAV_HAVE(OPENAL)
    RegisterAudioOutputOpenAL_Man();
#endif //QTAV_HAVE(OPENAL)
#if QTAV_HAVE(OPENSL)
    RegisterAudioOutputOpenSL_Man();
#endif //QTAV_HAVE(OPENSL)
}

} //namespace QtAV
