/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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

#include "QtAV/private/AudioOutputBackend.h"
#include "QtAV/private/mkid.h"
#include "QtAV/private/factory.h"
#include <QtCore/QLibrary>
#include <QtCore/QSemaphore>
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"
#define DX_LOG_COMPONENT "XAudio2"
#include "utils/DirectXHelper.h"
#include "xaudio2_compat.h"

// ref: DirectXTK, SDL, wine
namespace QtAV {

static const char kName[] = "XAudio2";
class AudioOutputXAudio2 Q_DECL_FINAL: public AudioOutputBackend, public IXAudio2VoiceCallback
{
public:
    AudioOutputXAudio2(QObject *parent = 0);
    ~AudioOutputXAudio2();
    QString name() const Q_DECL_OVERRIDE { return QString::fromLatin1(kName);}
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

    bool setVolume(qreal value) Q_DECL_OVERRIDE;
    qreal getVolume() const Q_DECL_OVERRIDE;
public:
    STDMETHOD_(void, OnVoiceProcessingPassStart)(THIS_ UINT32 bytesRequired) Q_DECL_OVERRIDE {Q_UNUSED(bytesRequired);}
    STDMETHOD_(void, OnVoiceProcessingPassEnd)(THIS) Q_DECL_OVERRIDE {}
    STDMETHOD_(void, OnStreamEnd)(THIS) Q_DECL_OVERRIDE {}
    STDMETHOD_(void, OnBufferStart)(THIS_ void* bufferContext) Q_DECL_OVERRIDE { Q_UNUSED(bufferContext);}
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
    bool xaudio2_winsdk;
    bool uninit_com;
    // TODO: com ptr
    IXAudio2SourceVoice* source_voice;
    union {
        struct {
            DXSDK::IXAudio2* xaudio;
            DXSDK::IXAudio2MasteringVoice* master;
        } dxsdk;
        struct {
            WinSDK::IXAudio2* xaudio;
            WinSDK::IXAudio2MasteringVoice* master;
        } winsdk;
    };
    QSemaphore sem;
    int queue_data_write;
    QByteArray queue_data;

    QLibrary dll;
};

typedef AudioOutputXAudio2 AudioOutputBackendXAudio2;
static const AudioOutputBackendId AudioOutputBackendId_XAudio2 = mkid::id32base36_6<'X', 'A', 'u', 'd', 'i', 'o'>::value;
FACTORY_REGISTER(AudioOutputBackend, XAudio2, kName)

AudioOutputXAudio2::AudioOutputXAudio2(QObject *parent)
    : AudioOutputBackend(AudioOutput::DeviceFeatures()|AudioOutput::SetVolume, parent)
    , xaudio2_winsdk(true)
    , uninit_com(false)
    , source_voice(NULL)
    , queue_data_write(0)
{
    memset(&dxsdk, 0, sizeof(dxsdk));
    available = false;
    //setDeviceFeatures(AudioOutput::DeviceFeatures()|AudioOutput::SetVolume);
#ifdef Q_OS_WINRT
    qDebug("XAudio2 for WinRT");
    // winrt can only load package dlls
    DX_ENSURE(XAudio2Create(&winsdk.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
#else
    // https://github.com/wang-bin/QtAV/issues/518
    // already initialized in qtcore for main thread. If RPC_E_CHANGED_MODE no ref is added, CoUninitialize can lead to crash
    uninit_com = CoInitializeEx(NULL, COINIT_MULTITHREADED) != RPC_E_CHANGED_MODE;
    // load dll. <win8: XAudio2_7.DLL, <win10: XAudio2_8.DLL, win10: XAudio2_9.DLL. also defined by XAUDIO2_DLL_A in xaudio2.h
    int ver = 9;
    for (; ver >= 7; ver--) {
        dll.setFileName(QStringLiteral("XAudio2_%1").arg(ver));
        qDebug() << dll.fileName();
        if (!dll.load()) {
            qWarning() << dll.errorString();
            continue;
        }
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
        // defined as an inline function
        qDebug("Build with XAudio2 from DXSDK");
#else
        qDebug("Build with XAudio2 from Win8 or later SDK");
#endif
        bool ready = false;
        if (!ready && ver >= 8) {
            xaudio2_winsdk = true;
            qDebug("Try symbol 'XAudio2Create' from WinSDK dll");
            typedef HRESULT (__stdcall *XAudio2Create_t)(WinSDK::IXAudio2** ppXAudio2, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor);
            XAudio2Create_t XAudio2Create = (XAudio2Create_t)dll.resolve("XAudio2Create");
            if (XAudio2Create)
                ready = SUCCEEDED(XAudio2Create(&winsdk.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
        }
        if (!ready && ver < 8) {
            xaudio2_winsdk = false;
#ifdef _XBOX // xbox < win8 is inline XAudio2Create
            qDebug("Try symbol 'XAudio2Create' from DXSDK dll (XBOX)");
            typedef HRESULT (__stdcall *XAudio2Create_t)(DXSDK::IXAudio2** ppXAudio2, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor);
            XAudio2Create_t XAudio2Create = (XAudio2Create_t)dll.resolve("XAudio2Create");
            if (XAudio2Create)
                ready = SUCCEEDED(XAudio2Create(&dxsdk.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
#else
            // try xaudio2 from dxsdk without symbol
            qDebug("Try inline function 'XAudio2Create' from DXSDK");
            ready = SUCCEEDED(DXSDK::XAudio2Create(&dxsdk.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
#endif
        }
        if (ready)
            break;
        dll.unload();
    }
#endif //Q_OS_WINRT
    qDebug("xaudio2: %p", winsdk.xaudio);
    available = !!(winsdk.xaudio);
}

AudioOutputXAudio2::~AudioOutputXAudio2()
{
    qDebug();
    if (xaudio2_winsdk)
        SafeRelease(&winsdk.xaudio);
    else
        SafeRelease(&dxsdk.xaudio);
#ifndef Q_OS_WINRT
    //again, for COM. not for winrt
    if (uninit_com)
        CoUninitialize();
#endif //Q_OS_WINRT
}

bool AudioOutputXAudio2::open()
{
    if (!available)
        return false;
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8) // TODO: also check runtime version before call
//    XAUDIO2_DEVICE_DETAILS details;
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
    if (xaudio2_winsdk) {
        // TODO: device Id property
        // TODO: parameters now default.
        DX_ENSURE_OK(winsdk.xaudio->CreateMasteringVoice(&winsdk.master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE), false);
        DX_ENSURE_OK(winsdk.xaudio->CreateSourceVoice(&source_voice, &wf, flags, XAUDIO2_DEFAULT_FREQ_RATIO, this, NULL, NULL), false);
        DX_ENSURE_OK(winsdk.xaudio->StartEngine(), false);
    } else {
        DX_ENSURE_OK(dxsdk.xaudio->CreateMasteringVoice(&dxsdk.master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE), false);
        DX_ENSURE_OK(dxsdk.xaudio->CreateSourceVoice(&source_voice, &wf, flags, XAUDIO2_DEFAULT_FREQ_RATIO, this, NULL, NULL), false);
        DX_ENSURE_OK(dxsdk.xaudio->StartEngine(), false);
    }
    DX_ENSURE_OK(source_voice->Start(0, XAUDIO2_COMMIT_NOW), false);
    qDebug("source_voice:%p", source_voice);

    queue_data.resize(buffer_size*buffer_count);
    sem.release(buffer_count - sem.available());
    return true;
}

bool AudioOutputXAudio2::close()
{
    qDebug("source_voice: %p, master: %p", source_voice, winsdk.master);
    if (source_voice) {
        source_voice->Stop(0, XAUDIO2_COMMIT_NOW);
        source_voice->FlushSourceBuffers();
        source_voice->DestroyVoice();
        source_voice = NULL;
    }
    if (xaudio2_winsdk) {
        if (winsdk.master) {
            winsdk.master->DestroyVoice();
            winsdk.master = NULL;
        }
        if (winsdk.xaudio)
            winsdk.xaudio->StopEngine();
    } else {
        if (dxsdk.master) {
            dxsdk.master->DestroyVoice();
            dxsdk.master = NULL;
        }
        if (dxsdk.xaudio)
            dxsdk.xaudio->StopEngine();
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
    return !IsPlanar(sampleFormat) && RawSampleSize(sampleFormat) < sizeof(double); // TODO: what about s64?
}

// FIXME:
bool AudioOutputXAudio2::isSupported(AudioFormat::ChannelLayout channelLayout) const
{
    return channelLayout == AudioFormat::ChannelLayout_Mono || channelLayout == AudioFormat::ChannelLayout_Stereo;
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
    XAUDIO2_BUFFER xb; //IMPORTANT! wrong value(playbegin/length, loopbegin/length) will result in commit sourcebuffer fail
    memset(&xb, 0, sizeof(XAUDIO2_BUFFER));
    xb.AudioBytes = data.size();
    //xb.Flags = XAUDIO2_END_OF_STREAM;
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

bool AudioOutputXAudio2::setVolume(qreal value)
{
    // master or source?
    DX_ENSURE_OK(source_voice->SetVolume(value), false);
    return true;
}

qreal AudioOutputXAudio2::getVolume() const
{
    FLOAT value;
    source_voice->GetVolume(&value);
    return value;
}

} // namespace QtAV
