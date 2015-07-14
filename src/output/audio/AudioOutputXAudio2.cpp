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
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

#ifdef __GNUC__
// macros used by XAudio 2.7 (June 2010 SDK)
#define __in
#define __in_ecount(size)
#define __in_bcount(size)
#define __out
#define __out_ecount(size)
#define __out_bcount(size)
#define __inout
#define __deref_out
#define __in_opt
#define __reserved
#endif
//TODO: check _DXSDK_BUILD_MAJOR, win8sdk, dxsdk
// do not add dxsdk xaudio2.h dir to INCLUDE for other file build with winsdk to avoid runtime crash
// currently you can use dxsdk 2010 header for mingw
#include <xaudio2.h>

#ifndef _WIN32_WINNT_WIN8
#define _WIN32_WINNT_WIN8 0x0602
#endif
/* sdk check
 * winsdk: defines XAUDIO2_DLL
 * dxsdk: defines XAUDIO2_DEBUG_ENGINE
 */
#define XA2_WINSDK (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
namespace DXSDK {
#if defined(__WINRT__)
typedef ::IXAudio2 IXAudio2;
#elif XA2_WINSDK
// Used in XAUDIO2_DEVICE_DETAILS below to describe the types of applications
// that the user has specified each device as a default for.  0 means that the
// device isn't the default for any role.
typedef enum XAUDIO2_DEVICE_ROLE
{
    NotDefaultDevice            = 0x0,
    DefaultConsoleDevice        = 0x1,
    DefaultMultimediaDevice     = 0x2,
    DefaultCommunicationsDevice = 0x4,
    DefaultGameDevice           = 0x8,
    GlobalDefaultDevice         = 0xf,
    InvalidDeviceRole = ~GlobalDefaultDevice
} XAUDIO2_DEVICE_ROLE;
// Returned by IXAudio2::GetDeviceDetails
typedef struct XAUDIO2_DEVICE_DETAILS
{
    WCHAR DeviceID[256];                // String identifier for the audio device.
    WCHAR DisplayName[256];             // Friendly name suitable for display to a human.
    XAUDIO2_DEVICE_ROLE Role;           // Roles that the device should be used for.
    WAVEFORMATEXTENSIBLE OutputFormat;  // The device's native PCM audio output format.
} XAUDIO2_DEVICE_DETAILS;

// from wine idl. If use code from dx header. TODO: why crash if use code from dx header?
interface IXAudio2MasteringVoice : public IXAudio2Voice
{
    virtual void STDMETHODCALLTYPE GetChannelMask(
        DWORD *pChannelMask) = 0;

};
DEFINE_GUID(IID_IXAudio27, 0x8bcf1f58, 0x9fe7, 0x4583, 0x8a,0xc6, 0xe2,0xad,0xc4,0x65,0xc8,0xbb);
MIDL_INTERFACE("8bcf1f58-9fe7-4583-8ac6-e2adc465c8bb")
IXAudio2 : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetDeviceCount(
        UINT32 *pCount) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDeviceDetails(
        UINT32 Index,
        XAUDIO2_DEVICE_DETAILS *pDeviceDetails) = 0;

    virtual HRESULT STDMETHODCALLTYPE Initialize(
        UINT32 Flags = 0,
        XAUDIO2_PROCESSOR XAudio2Processor = XAUDIO2_DEFAULT_PROCESSOR) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterForCallbacks(
        IXAudio2EngineCallback *pCallback) = 0;

    virtual void STDMETHODCALLTYPE UnregisterForCallbacks(
        IXAudio2EngineCallback *pCallback) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSourceVoice(
        IXAudio2SourceVoice **ppSourceVoice,
        const WAVEFORMATEX *pSourceFormat,
        UINT32 Flags = 0,
        float MaxFrequencyRatio = XAUDIO2_DEFAULT_FREQ_RATIO,
        IXAudio2VoiceCallback *pCallback = 0,
        const XAUDIO2_VOICE_SENDS *pSendList = 0,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain = 0) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSubmixVoice(
        IXAudio2SubmixVoice **ppSubmixVoice,
        UINT32 InputChannels,
        UINT32 InputSampleRate,
        UINT32 Flags = 0,
        UINT32 ProcessingStage = 0,
        const XAUDIO2_VOICE_SENDS *pSendList = 0,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain = 0) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateMasteringVoice(
        IXAudio2MasteringVoice **ppMasteringVoice,
        UINT32 InputChannels = XAUDIO2_DEFAULT_CHANNELS,
        UINT32 InputSampleRate = XAUDIO2_DEFAULT_SAMPLERATE,
        UINT32 Flags = 0,
        UINT32 DeviceIndex = 0,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain = 0) = 0;

    virtual HRESULT STDMETHODCALLTYPE StartEngine(
        ) = 0;

    virtual void STDMETHODCALLTYPE StopEngine(
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE CommitChanges(
        UINT32 OperationSet) = 0;

    virtual void STDMETHODCALLTYPE GetPerformanceData(
        XAUDIO2_PERFORMANCE_DATA *pPerfData) = 0;

    virtual void STDMETHODCALLTYPE SetDebugConfiguration(
        const XAUDIO2_DEBUG_CONFIGURATION *pDebugConfiguration,
        void *pReserved = 0) = 0;

};

#ifndef GUID_SECT
    #define GUID_SECT
#endif
#define __DEFINE_CLSID(n,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) static const CLSID n GUID_SECT = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}
#define __DEFINE_IID(n,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) static const IID n GUID_SECT = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#define DEFINE_CLSID(className, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    __DEFINE_CLSID(CLSID_##className, 0x##l, 0x##w1, 0x##w2, 0x##b1, 0x##b2, 0x##b3, 0x##b4, 0x##b5, 0x##b6, 0x##b7, 0x##b8)

#define DEFINE_IID(interfaceName, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    __DEFINE_IID(IID_##interfaceName, 0x##l, 0x##w1, 0x##w2, 0x##b1, 0x##b2, 0x##b3, 0x##b4, 0x##b5, 0x##b6, 0x##b7, 0x##b8)

// XAudio 2.7 (June 2010 SDK)
DEFINE_CLSID(XAudio2, 5a508685, a254, 4fba, 9b, 82, 9a, 24, b0, 03, 06, af);
DEFINE_CLSID(XAudio2_Debug, db05ea35, 0329, 4d4b, a5, 3a, 6d, ea, d0, 3d, 38, 52);
DEFINE_IID(IXAudio2, 8bcf1f58, 9fe7, 4583, 8a, c6, e2, ad, c4, 65, c8, bb);

// Flags
// NOTE: XAUDIO2_DEBUG_ENGINE is NOT defined in winsdk!!!
#define XAUDIO2_DEBUG_ENGINE            0x0001        // Used in XAudio2Create on Windows only
HRESULT XAudio2Create(__deref_out IXAudio2** ppXAudio2, UINT32 Flags X2DEFAULT(0),
                               XAUDIO2_PROCESSOR XAudio2Processor X2DEFAULT(XAUDIO2_DEFAULT_PROCESSOR))
{
    // Instantiate the appropriate XAudio2 engine
    IXAudio2* pXAudio2;
    HRESULT hr = CoCreateInstance((Flags & XAUDIO2_DEBUG_ENGINE) ? CLSID_XAudio2_Debug : CLSID_XAudio2,
                                  NULL, CLSCTX_INPROC_SERVER, IID_IXAudio2, (void**)&pXAudio2);
    if (SUCCEEDED(hr))
    {
        hr = pXAudio2->Initialize(Flags, XAudio2Processor);

        if (SUCCEEDED(hr))
        {
            *ppXAudio2 = pXAudio2;
        }
        else
        {
            pXAudio2->Release();
        }
    }
    return hr;
}
#else
#ifndef _XBOX
HRESULT XAudio2Create(__deref_out IXAudio2** ppXAudio2, UINT32 Flags X2DEFAULT(0),
                               XAUDIO2_PROCESSOR XAudio2Processor X2DEFAULT(XAUDIO2_DEFAULT_PROCESSOR))
{
    return ::XAudio2Create(ppXAudio2, Flags, XAudio2Processor);
}
#endif //_XBOX
typedef ::IXAudio2 IXAudio2;
typedef ::IXAudio2MasteringVoice IXAudio2MasteringVoice;
#endif
// At last, define the same types. MUST BE AT LAST to avoid ambigous, for example IXAudio2MasteringVoice is a ::IXAudio2Voice but we can use IXAudio2Voice
typedef ::IXAudio2Voice IXAudio2Voice;
typedef ::IXAudio2SourceVoice IXAudio2SourceVoice;
}
namespace WINSDK {
#if defined(__WINRT__)
typedef ::IXAudio2 IXAudio2;
#elif XA2_WINSDK
typedef ::IXAudio2 IXAudio2;
typedef ::IXAudio2MasteringVoice IXAudio2MasteringVoice;
#else
typedef enum _AUDIO_STREAM_CATEGORY
{
    AudioCategory_Other = 0,
    AudioCategory_ForegroundOnlyMedia,
    AudioCategory_BackgroundCapableMedia,
    AudioCategory_Communications,
    AudioCategory_Alerts,
    AudioCategory_SoundEffects,
    AudioCategory_GameEffects,
    AudioCategory_GameMedia,
} AUDIO_STREAM_CATEGORY;
// from wine idl. If use code from dx header. TODO: why crash if use code from dx header?
interface IXAudio2MasteringVoice : public IXAudio2Voice
{
    virtual void STDMETHODCALLTYPE GetChannelMask(
        DWORD *pChannelMask) = 0;

};
DEFINE_GUID(IID_IXAudio2, 0x60d8dac8, 0x5aa1, 0x4e8e, 0xb5,0x97, 0x2f,0x5e,0x28,0x83,0xd4,0x84);
MIDL_INTERFACE("60d8dac8-5aa1-4e8e-b597-2f5e2883d484")
IXAudio2 : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE RegisterForCallbacks(
        IXAudio2EngineCallback *pCallback) = 0;

    virtual void STDMETHODCALLTYPE UnregisterForCallbacks(
        IXAudio2EngineCallback *pCallback) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSourceVoice(
        IXAudio2SourceVoice **ppSourceVoice,
        const WAVEFORMATEX *pSourceFormat,
        UINT32 Flags = 0,
        float MaxFrequencyRatio = XAUDIO2_DEFAULT_FREQ_RATIO,
        IXAudio2VoiceCallback *pCallback = 0,
        const XAUDIO2_VOICE_SENDS *pSendList = 0,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain = 0) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSubmixVoice(
        IXAudio2SubmixVoice **ppSubmixVoice,
        UINT32 InputChannels,
        UINT32 InputSampleRate,
        UINT32 Flags = 0,
        UINT32 ProcessingStage = 0,
        const XAUDIO2_VOICE_SENDS *pSendList = 0,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain = 0) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateMasteringVoice(
        IXAudio2MasteringVoice **ppMasteringVoice,
        UINT32 InputChannels = XAUDIO2_DEFAULT_CHANNELS,
        UINT32 InputSampleRate = XAUDIO2_DEFAULT_SAMPLERATE,
        UINT32 Flags = 0,
        LPCWSTR DeviceId = 0,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain = 0,
        AUDIO_STREAM_CATEGORY StreamCategory = AudioCategory_GameEffects) = 0;

    virtual HRESULT STDMETHODCALLTYPE StartEngine(
        ) = 0;

    virtual void STDMETHODCALLTYPE StopEngine(
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE CommitChanges(
        UINT32 OperationSet) = 0;

    virtual void STDMETHODCALLTYPE GetPerformanceData(
        XAUDIO2_PERFORMANCE_DATA *pPerfData) = 0;

    virtual void STDMETHODCALLTYPE SetDebugConfiguration(
        const XAUDIO2_DEBUG_CONFIGURATION *pDebugConfiguration,
        void *pReserved = 0) = 0;

};
#endif
// At last, define the same types. MUST BE AT LAST to avoid ambigous, for example IXAudio2MasteringVoice is a ::IXAudio2Voice but we can use IXAudio2Voice
typedef ::IXAudio2Voice IXAudio2Voice;
typedef ::IXAudio2SourceVoice IXAudio2SourceVoice;
}
// ref: DirectXTK, SDL
// TODO: some API delarations in windows sdk and dxsdk are different, how to support different runtime (for win8sdk)?
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
    DXSDK::IXAudio2* xaudio;
    DXSDK::IXAudio2MasteringVoice* master_voice;
    DXSDK::IXAudio2SourceVoice* source_voice;

    union {
        DXSDK::IXAudio2 *xa_dx;
        WINSDK::IXAudio2 *xa_win;
    };
    QSemaphore sem;
    int queue_data_write;
    QByteArray queue_data;

    QLibrary dll;
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
    available = false;
    //setDeviceFeatures(AudioOutput::DeviceFeatures()|AudioOutput::SetVolume);
    //required by XAudio2. This simply starts up the COM library on this thread
    // TODO: not required by winrt
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    // load dll. <win8: XAudio2_7.DLL, <win10: XAudio2_8.DLL, win10: XAudio2_9.DLL. also defined by XAUDIO2_DLL_A in xaudio2.h
    int ver = 9;
    for (; ver >= 0; ver--) {
        dll.setFileName(QString("XAudio2_%1").arg(ver));
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
            qDebug("try xaudio2 symbol from winsdk");
            typedef HRESULT (__stdcall *XAudio2Create_t)(WINSDK::IXAudio2** ppXAudio2, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor);
            XAudio2Create_t XAudio2Create = (XAudio2Create_t)dll.resolve("XAudio2Create");
            //if (XAudio2Create)
                //ready = SUCCEEDED(XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
        }
        if (!ready && ver < 8) {
#ifdef _XBOX // xbox < win8 is inline XAudio2Create
            qDebug("try xaudio2 symbol from dxsdk (XBOX)");
            typedef HRESULT (__stdcall *XAudio2Create_t)(DXSDK::IXAudio2** ppXAudio2, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor);
            XAudio2Create_t XAudio2Create = (XAudio2Create_t)dll.resolve("XAudio2Create");
            if (XAudio2Create)
                ready = SUCCEEDED(XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
#else
            // try xaudio2 from dxsdk without symbol
            qDebug("try xaudio2 symbol from dxsdk");
            ready = SUCCEEDED(DXSDK::XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
#endif
        }
        if (ready)
            break;
        dll.unload();
    }
    qDebug("xaudio2: %p", xaudio);
    available = !!xaudio;
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
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8) // TODO: also check runtime version before call
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
    XAUDIO2_BUFFER xb; //IMPORTANT! wrong value(playbegin/length, loopbegin/length) will result in commit sourcebuffer fail
    memset(&xb, 0, sizeof(XAUDIO2_BUFFER));
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
