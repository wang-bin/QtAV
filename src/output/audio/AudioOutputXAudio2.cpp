/******************************************************************************
    AudioOutputXAudio2.cpp: description
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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


#include "QtAV/private/AudioOutputBackend.h"
#include "QtAV/private/mkid.h"
#include "QtAV/private/prepost.h"
#include <QtCore/QLibrary>
#include <QtCore/QSemaphore>
#include <windows.h>
#include <xaudio2.h> //TODO: check _DXSDK_BUILD_MAJOR, win8sdk, dxsdk. use compatible header and no link?
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

// ref: DirectXTK, SDL

namespace QtAV {

template <class T> void SafeRelease(T **ppT) {
  if (*ppT) {
    (*ppT)->Release();
    *ppT = NULL;
  }
}

static const char kName[] = "XAudio2";
class AudioOutputXAudio2 Q_DECL_FINAL: public AudioOutputBackend, public IXAudio2VoiceCallback
{
public:
    AudioOutputXAudio2(QObject *parent = 0);
    ~AudioOutputXAudio2();
    QString name() const Q_DECL_OVERRIDE { return kName;}
    bool open() Q_DECL_OVERRIDE;
    bool close() Q_DECL_OVERRIDE;
    // TODO: check channel layout. xaudio2 supports channels>2
    bool isSupported(const AudioFormat& format) const Q_DECL_OVERRIDE;
    bool isSupported(AudioFormat::SampleFormat sampleFormat) const Q_DECL_OVERRIDE;
    bool isSupported(AudioFormat::ChannelLayout channelLayout) const Q_DECL_OVERRIDE;
    BufferControl bufferControl() const Q_DECL_OVERRIDE;
    void onCallback() Q_DECL_OVERRIDE;
    bool write(const QByteArray& data) Q_DECL_OVERRIDE;
    bool play() Q_DECL_OVERRIDE;

   // bool setVolume(qreal value) Q_DECL_OVERRIDE;
    //qreal getVolume() const Q_DECL_OVERRIDE;
public:
    STDMETHOD_(void, OnVoiceProcessingPassStart)(THIS_ UINT32 bytesRequired) Q_DECL_OVERRIDE {Q_UNUSED(bytesRequired);}
    STDMETHOD_(void, OnVoiceProcessingPassEnd)(THIS) Q_DECL_OVERRIDE {}
    STDMETHOD_(void, OnStreamEnd)(THIS) Q_DECL_OVERRIDE {}
    STDMETHOD_(void, OnBufferStart)(THIS_ void* bufferContext) Q_DECL_OVERRIDE {
        Q_UNUSED(bufferContext);
    }
    STDMETHOD_(void, OnBufferEnd)(THIS_ void* bufferContext) Q_DECL_OVERRIDE {
        AudioOutputXAudio2 *ao = reinterpret_cast<AudioOutputXAudio2*>(bufferContext);
        if (ao->bufferControl() & AudioOutputBackend::CountCallback) {
            ao->onCallback();
        }
    }
    STDMETHOD_(void, OnLoopEnd)(THIS_ void* bufferContext) Q_DECL_OVERRIDE { Q_UNUSED(bufferContext);}
    STDMETHOD_(void, OnVoiceError)(THIS_ void* bufferContext, HRESULT error) Q_DECL_OVERRIDE {
        Q_UNUSED(bufferContext);
        qWarning() << __FUNCTION__ << ": (" << error << ") " << qt_error_string(error);
    }

private:
    // TODO: com ptr
    IXAudio2* xaudio;
    IXAudio2MasteringVoice* master_voice;
    IXAudio2SourceVoice* source_voice;

    QSemaphore sem;
    int queue_data_write;
    QByteArray queue_data;
};

typedef AudioOutputXAudio2 AudioOutputBackendXAudio2;
static const AudioOutputBackendId AudioOutputBackendId_XAudio2 = mkid::id32base36_6<'X', 'A', 'u', 'd', 'i', 'o'>::value;
FACTORY_REGISTER_ID_AUTO(AudioOutputBackend, XAudio2, kName)

void RegisterAudioOutputXAudio2_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioOutputBackend, XAudio2, kName)
}

#define DX_LOG_COMPONENT "XAudio2"

#ifndef DX_LOG_COMPONENT
#define DX_LOG_COMPONENT "DirectX"
#endif //DX_LOG_COMPONENT
#define DX_ENSURE_OK(f, ...) \
    do { \
        HRESULT hr = f; \
        if (FAILED(hr)) { \
            qWarning() << DX_LOG_COMPONENT " error@" << __LINE__ << ". " #f ": " << QString("(0x%1) ").arg(hr, 0, 16) << qt_error_string(hr); \
            return __VA_ARGS__; \
        } \
    } while (0)

AudioOutputXAudio2::AudioOutputXAudio2(QObject *parent)
    : AudioOutputBackend(AudioOutput::DeviceFeatures()/*|AudioOutput::SetVolume*/, parent)
    , xaudio(NULL)
    , master_voice(NULL)
    , source_voice(NULL)
    , queue_data_write(0)
{
    //setDeviceFeatures(AudioOutput::DeviceFeatures()|AudioOutput::SetVolume);

    // TODO: load dll. <win8: XAudio2_7.DLL, <win10: XAudio2_8.DLL, win10: XAudio2_9.DLL. also defined by XAUDIO2_DLL_A in xaudio2.h
    //required by XAudio2. This simply starts up the COM library on this thread
    // TODO: not required by winrt
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    //create the engine
    DX_ENSURE_OK(XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
}

AudioOutputXAudio2::~AudioOutputXAudio2()
{
    qDebug();
    SafeRelease(&xaudio);
    //again, for COM. not for winrt
    CoUninitialize();
}

bool AudioOutputXAudio2::open()
{
    if (!xaudio)
        return false;
    // TODO: device Id property
    // TODO: parameters now default.
    DX_ENSURE_OK(xaudio->CreateMasteringVoice(&master_voice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE), false);
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
    XAUDIO2_DEVICE_DETAILS details;
#endif

    WAVEFORMATEX wf;
    wf.cbSize = 0; //sdl: sizeof(wf)
    wf.nChannels = format.channels();
    wf.nSamplesPerSec = format.sampleRate(); // FIXME: use supported values
    wf.wFormatTag = format.isFloat() ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
    wf.wBitsPerSample = format.bytesPerSample() * 8;
    wf.nBlockAlign = wf.nChannels * format.bytesPerSample();
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    // dwChannelMask
    // TODO: channels >2, see dsound
    const UINT32 flags = 0; //XAUDIO2_VOICE_NOSRC | XAUDIO2_VOICE_NOPITCH;
    // TODO: sdl freq 1.0
    qDebug("master_voice:%p",master_voice);
    DX_ENSURE_OK(xaudio->CreateSourceVoice(&source_voice, &wf, flags, XAUDIO2_DEFAULT_FREQ_RATIO, this, NULL, NULL), false);
    DX_ENSURE_OK(xaudio->StartEngine(), false);
    DX_ENSURE_OK(source_voice->Start(0, XAUDIO2_COMMIT_NOW), false);
    qDebug("source_voice:%p", source_voice);

    queue_data.resize(buffer_size*buffer_count);
    sem.release(buffer_count - sem.available());
    return true;
}

bool AudioOutputXAudio2::close()
{
    qDebug("source_voice: %p, master_voice: %p", source_voice, master_voice);
    if (source_voice) {
        source_voice->Stop(0, XAUDIO2_COMMIT_NOW);
        source_voice->FlushSourceBuffers();
        source_voice->DestroyVoice();
        source_voice = NULL;
    }
    if (master_voice) {
        master_voice->DestroyVoice();
        master_voice = NULL;
    }
    if (xaudio) {
        xaudio->StopEngine();
    }
    queue_data.clear();
    queue_data_write = 0;
    return true;
}

bool AudioOutputXAudio2::isSupported(const AudioFormat& format) const
{
    return isSupported(format.sampleFormat()) && isSupported(format.channelLayout());
}

bool AudioOutputXAudio2::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    return !AudioFormat::isPlanar(sampleFormat);
}

// FIXME:
bool AudioOutputXAudio2::isSupported(AudioFormat::ChannelLayout channelLayout) const
{
    return channelLayout == AudioFormat::ChannelLayout_Mono || channelLayout == AudioFormat::ChannelLayout_Stero;
}


AudioOutputBackend::BufferControl AudioOutputXAudio2::bufferControl() const
{
    return CountCallback;
}

void AudioOutputXAudio2::onCallback()
{
    if (bufferControl() & CountCallback)
        sem.release();
}

bool AudioOutputXAudio2::write(const QByteArray &data)
{
    //qDebug("sem: %d, write: %d/%d", sem.available(), queue_data_write, queue_data.size());
    if (bufferControl() & CountCallback)
        sem.acquire();
    const int s = qMin(queue_data.size() - queue_data_write, data.size());
    // assume data.size() <= buffer_size. It's true in QtAV
    if (s < data.size())
        queue_data_write = 0;
    memcpy((char*)queue_data.constData() + queue_data_write, data.constData(), data.size());
    XAUDIO2_BUFFER xb = { 0 }; //IMPORTANT! wrong value(playbegin/length, loopbegin/length) will result in commit sourcebuffer fail
    xb.AudioBytes = data.size();
    //xb.Flags = XAUDIO2_END_OF_STREAM; // TODO: not set in sdl
    xb.pContext = this;
    xb.pAudioData = (const BYTE*)(queue_data.constData() + queue_data_write);
    queue_data_write += data.size();
    if (queue_data_write == queue_data.size())
        queue_data_write = 0;
    DX_ENSURE_OK(source_voice->SubmitSourceBuffer(&xb, NULL), false);
    // TODO: XAUDIO2_E_DEVICE_INVALIDATED
    return true;
}

bool AudioOutputXAudio2::play()
{
    return true;
}
/*
bool AudioOutputXAudio2::setVolume(qreal value)
{
    // master or source?
   // source_voice->SetVolume()
    return false;
}

qreal AudioOutputXAudio2::getVolume() const
{

    return pow(10.0, double(vol - DSBVOLUME_MIN)/5000.0)/100.0;
}
*/
} // namespace QtAV
