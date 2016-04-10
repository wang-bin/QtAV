/******************************************************************************
    XAudio2 compat layer with both DXSDK and WINSDK support
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_XAUDIO2_COMPAT_H
#define QTAV_XAUDIO2_COMPAT_H

#define PREFER_WINSDK 0 //1 if use xaudio2 from windows sdk, _WIN32_WINNT must >= win8

#if PREFER_WINSDK
#if _WIN32_WINNT < _WIN32_WINNT_WIN8
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#endif
#endif //PREFER_WINSDK
/*!
  IXAudio2 and IXAudio2MasteringVoice are different defined in DXSDK(2010 June) and WINSDK(>=win8).
  You MUST explicitly prefix with namespace DXSDK or WinSDK for IXAudio2MasteringVoice, IXAudio2,
*/
#include <windows.h>
#ifndef _WIN32_WINNT_WIN8
#define _WIN32_WINNT_WIN8 0x0602
#endif
#include "directx/dxcompat.h"
#ifdef __GNUC__
// macros used by XAudio 2.7 (June 2010 SDK)
#ifndef __in
#define __in
#endif
#ifndef __out
#define __out
#endif
#endif
//TODO: winrt test
// do not add dxsdk xaudio2.h dir to INCLUDE for other file build with winsdk to avoid runtime crash
// currently you can use dxsdk 2010 header for mingw
// MinGW64 cross: XAudio2.h
#include <XAudio2.h>

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
//typedef ::IXAudio2Voice IXAudio2Voice;
//typedef ::IXAudio2SourceVoice IXAudio2SourceVoice;
}
namespace WinSDK {
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
//typedef ::IXAudio2Voice IXAudio2Voice;
//typedef ::IXAudio2SourceVoice IXAudio2SourceVoice;
}

#endif //QTAV_XAUDIO2_COMPAT_H
