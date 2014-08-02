/******************************************************************************
    AudioOutputDSound.cpp: description
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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


#include "QtAV/AudioOutput.h"
#include "QtAV/private/AudioOutput_p.h"
#include "prepost.h"
#include <QtCore/QLibrary>
#include <QtCore/QVector>

#include <windows.h>
#define DIRECTSOUND_VERSION 0x0600
#include <dsound.h>
#include "QtAV/private/AVCompat.h"

namespace QtAV {
class AudioOutputDSoundPrivate;
class AudioOutputDSound : public AudioOutput
{
    DPTR_DECLARE_PRIVATE(AudioOutputDSound)
public:
    AudioOutputDSound();
    //AudioOutputId id() const
    virtual bool open();
    virtual bool close();
    virtual bool isSupported(AudioFormat::SampleFormat sampleFormat) const;
    virtual Feature supportedFeatures() const;
    virtual bool play();
protected:
    virtual bool write(const QByteArray& data);
    virtual int getPlayingBytes();
};

extern AudioOutputId AudioOutputId_DSound;
FACTORY_REGISTER_ID_AUTO(AudioOutput, DSound, "DirectSound")

void RegisterAudioOutputDSound_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioOutput, DSound, "DirectSound")
}


template <class T> void SafeRelease(T **ppT) {
  if (*ppT) {
    (*ppT)->Release();
    *ppT = NULL;
  }
}

// use the definitions from the win32 api headers when they define these
#define WAVE_FORMAT_IEEE_FLOAT      0x0003
#define WAVE_FORMAT_DOLBY_AC3_SPDIF 0x0092
#define WAVE_FORMAT_EXTENSIBLE      0xFFFE
/* GUID SubFormat IDs */
/* We need both b/c const variables are not compile-time constants in C, giving
 * us an error if we use the const GUID in an enum */
#undef DEFINE_GUID
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) EXTERN_C const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
DEFINE_GUID(_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID(_KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID(_KSDATAFORMAT_SUBTYPE_PCM, WAVE_FORMAT_PCM, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(_KSDATAFORMAT_SUBTYPE_UNKNOWN, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

#ifndef _WAVEFORMATEXTENSIBLE_
typedef struct {
   WAVEFORMATEX    Format;
   union {
      WORD wValidBitsPerSample;       /* bits of precision  */
      WORD wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
      WORD wReserved;                 /* If neither applies, set to zero. */
   } Samples;
   DWORD           dwChannelMask;      /* which channels are */
   /* present in stream  */
   GUID            SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;
#endif

/* Microsoft speaker definitions. key/values are equal to FFmpeg's */
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000

static int channelMaskToMS(qint64 av) {
    if (av >= (qint64)SPEAKER_RESERVED)
        return 0;
    return (int)av;
}

static int channelLayoutToMS(qint64 av) {
    return channelMaskToMS(av);
}

static const char* dserr2str(int err) {
   switch (err) {
      case DS_OK: return "DS_OK";
      case DS_NO_VIRTUALIZATION: return "DS_NO_VIRTUALIZATION";
      case DSERR_ALLOCATED: return "DS_NO_VIRTUALIZATION";
      case DSERR_CONTROLUNAVAIL: return "DSERR_CONTROLUNAVAIL";
      case DSERR_INVALIDPARAM: return "DSERR_INVALIDPARAM";
      case DSERR_INVALIDCALL: return "DSERR_INVALIDCALL";
      case DSERR_GENERIC: return "DSERR_GENERIC";
      case DSERR_PRIOLEVELNEEDED: return "DSERR_PRIOLEVELNEEDED";
      case DSERR_OUTOFMEMORY: return "DSERR_OUTOFMEMORY";
      case DSERR_BADFORMAT: return "DSERR_BADFORMAT";
      case DSERR_UNSUPPORTED: return "DSERR_UNSUPPORTED";
      case DSERR_NODRIVER: return "DSERR_NODRIVER";
      case DSERR_ALREADYINITIALIZED: return "DSERR_ALREADYINITIALIZED";
      case DSERR_NOAGGREGATION: return "DSERR_NOAGGREGATION";
      case DSERR_BUFFERLOST: return "DSERR_BUFFERLOST";
      case DSERR_OTHERAPPHASPRIO: return "DSERR_OTHERAPPHASPRIO";
      case DSERR_UNINITIALIZED: return "DSERR_UNINITIALIZED";
      case DSERR_NOINTERFACE: return "DSERR_NOINTERFACE";
      case DSERR_ACCESSDENIED: return "DSERR_ACCESSDENIED";
      default: return "unknown";
   }
}

class  AudioOutputDSoundPrivate : public AudioOutputPrivate
{
public:
    AudioOutputDSoundPrivate()
        : AudioOutputPrivate()
        , dll(NULL)
        , prim_buf(NULL)
        , stream_buf(NULL)
        , write_offset(0)
        , read_offset(0)
        , read_size_queued(0)
    {
    }
    ~AudioOutputDSoundPrivate() {
        destroy();
    }
    bool loadDll();
    bool unloadDll();
    bool init();
    void destroy() {
        SafeRelease(&prim_buf);
        SafeRelease(&stream_buf);
        SafeRelease(&dsound);
        unloadDll();
    }
    bool createDSoundBuffers();

    HINSTANCE dll;
    LPDIRECTSOUND dsound;              ///direct sound object
    LPDIRECTSOUNDBUFFER prim_buf;      ///primary direct sound buffer
    LPDIRECTSOUNDBUFFER stream_buf;    ///secondary direct sound buffer (stream buffer)
    int write_offset;               ///offset of the write cursor in the direct sound buffer
    int read_offset;
    int read_size_queued;
};

AudioOutputDSound::AudioOutputDSound()
    :AudioOutput(*new AudioOutputDSoundPrivate())
{
    setFeature(GetPlayingBytes);
}

bool AudioOutputDSound::open()
{
    DPTR_D(AudioOutputDSound);
    d.resetBuffers();
    if (!d.init())
        return false;
    if (!d.createDSoundBuffers())
        return false;
    return true;
    QByteArray feed(d.bufferSizeTotal(), 0);
    write(feed);
    for (quint32 i = 0; i < d.nb_buffers; ++i) {
        d.bufferAdded();
    }
    qDebug("==========d.write_offset=%d", d.write_offset);
    return true;
}

bool AudioOutputDSound::close()
{
    DPTR_D(AudioOutputDSound);
    d.resetBuffers();
    d.destroy();
    return true;
}

bool AudioOutputDSound::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    return sampleFormat == AudioFormat::SampleFormat_Signed16
            || sampleFormat == AudioFormat::SampleFormat_Float;
}

AudioOutput::Feature AudioOutputDSound::supportedFeatures() const
{
    return GetPlayingBytes;
}

bool AudioOutputDSound::write(const QByteArray &data)
{
    DPTR_D(AudioOutputDSound);
    LPVOID dst1= NULL, dst2 = NULL;
    DWORD size1 = 0, size2 = 0;
    if (d.write_offset >= d.bufferSizeTotal()) ///!!!>=
        d.write_offset = 0;
    HRESULT res = d.stream_buf->Lock(d.write_offset, data.size(), &dst1, &size1, &dst2, &size2, 0); //DSBLOCK_ENTIREBUFFER
    if (res == DSERR_BUFFERLOST) {
        d.stream_buf->Restore();
        res = d.stream_buf->Lock(d.write_offset, data.size(), &dst1, &size1, &dst2, &size2, 0);
    }
    if (res != DS_OK) {
        qWarning("Can not lock secondary buffer (%s)", dserr2str(res));
        return false;
    }
    memcpy(dst1, data.constData(), size1);
    if (dst2)
        memcpy(dst2, data.constData() + size1, size2);
    d.write_offset += size1 + size2;
    if (d.write_offset >= d.bufferSizeTotal())
        d.write_offset = size2;
    res = d.stream_buf->Unlock(dst1, size1, dst2, size2);
    if (res != DS_OK) {
        qWarning("Unloack error (%s)",dserr2str(res));
        //return false;
    }
    return true;
}

bool AudioOutputDSound::play()
{
    DPTR_D(AudioOutputDSound);
    DWORD status;
    d.stream_buf->GetStatus(&status);
    if (!(status & DSBSTATUS_PLAYING)) {
        // we don't need looping here. otherwise sound is always playing repeatly if no data feeded
        d.stream_buf->Play(0, 0, 0);// DSBPLAY_LOOPING);
    }
    return true;
}

int AudioOutputDSound::getPlayingBytes()
{
    DPTR_D(AudioOutputDSound);
    DWORD read_offset = 0;
    d.stream_buf->GetCurrentPosition(&read_offset /*play*/, NULL /*write*/); //what's this write_offset?
    return (int)read_offset;
}

bool AudioOutputDSoundPrivate::loadDll()
{
    dll = LoadLibrary(TEXT("dsound.dll"));
    if (!dll) {
        qWarning("Can not load dsound.dll");
        return false;
    }
    return true;
}

bool AudioOutputDSoundPrivate::unloadDll()
{
    if (dll)
        FreeLibrary(dll);
    return true;
}

bool AudioOutputDSoundPrivate::init()
{
    if (!loadDll())
        return false;
    typedef HRESULT (WINAPI *DirectSoundCreateFunc)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
    //typedef HRESULT (WINAPI *DirectSoundEnumerateFunc)(LPDSENUMCALLBACKA, LPVOID);
    DirectSoundCreateFunc dsound_create = (DirectSoundCreateFunc)GetProcAddress(dll, "DirectSoundCreate");
    //DirectSoundEnumerateFunc dsound_enumerate = (DirectSoundEnumerateFunc)GetProcAddress(dll, "DirectSoundEnumerateA");
    if (!dsound_create) {
        qWarning("Failed to resolve 'DirectSoundCreate'");
        unloadDll();
        return false;
    }
    if (FAILED(dsound_create(NULL/*dev guid*/, &dsound, NULL))){
        unloadDll();
        return false;
    }
    /*  DSSCL_EXCLUSIVE: can modify the settings of the primary buffer, only the sound of this app will be hearable when it will have the focus.
     */
    if (FAILED(dsound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_EXCLUSIVE))) {
        qWarning("Cannot set direct sound cooperative level");
        SafeRelease(&dsound);
        return false;
    }
    qDebug("DirectSound initialized.");
    DSCAPS dscaps;
    memset(&dscaps, 0, sizeof(DSCAPS));
    dscaps.dwSize = sizeof(DSCAPS);
    if (FAILED(dsound->GetCaps(&dscaps))) {
       qWarning("Cannot get device capabilities.");
       SafeRelease(&dsound);
       return false;
    }
    if (dscaps.dwFlags & DSCAPS_EMULDRIVER)
        qDebug("DirectSound is emulated");

    write_offset = 0;
    read_offset = 0;
    read_size_queued = 0;
    return true;
}

/**
 * Creates a DirectSound buffer of the required format.
 *
 * This function creates the buffer we'll use to play audio.
 * In DirectSound there are two kinds of buffers:
 * - the primary buffer: which is the actual buffer that the soundcard plays
 * - the secondary buffer(s): these buffers are the one actually used by
 *    applications and DirectSound takes care of mixing them into the primary.
 *
 * Once you create a secondary buffer, you cannot change its format anymore so
 * you have to release the current one and create another.
 */
bool AudioOutputDSoundPrivate::createDSoundBuffers()
{
    WAVEFORMATEXTENSIBLE wformat;
    // TODO:  Dolby Digital AC3
    ZeroMemory(&wformat, sizeof(WAVEFORMATEXTENSIBLE));
    wformat.Format.cbSize =  0;
    wformat.Format.nChannels = format.channels();
    wformat.Format.nSamplesPerSec = format.sampleRate();
    wformat.Format.wFormatTag = WAVE_FORMAT_PCM;
    wformat.Format.wBitsPerSample = format.bytesPerSample() * 8;
    wformat.Samples.wValidBitsPerSample = wformat.Format.wBitsPerSample; //
    wformat.Format.nBlockAlign = wformat.Format.nChannels * format.bytesPerSample();
    wformat.Format.nAvgBytesPerSec = wformat.Format.nSamplesPerSec * wformat.Format.nBlockAlign;
    wformat.dwChannelMask = channelLayoutToMS(format.channelLayoutFFmpeg());
    if (format.channels() > 2) {
        wformat.Format.cbSize =  sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        wformat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wformat.SubFormat = _KSDATAFORMAT_SUBTYPE_PCM;
        //wformat.Samples.wValidBitsPerSample = wformat.Format.wBitsPerSample;
    }
    switch (format.sampleFormat()) {
    case AudioFormat::SampleFormat_Float:
        wformat.Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wformat.SubFormat = _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        break;
    case AudioFormat::SampleFormat_Signed16:
        wformat.SubFormat = _KSDATAFORMAT_SUBTYPE_PCM;
        break;
    default:
        break;
    }
    // fill in primary sound buffer descriptor
    DSBUFFERDESC dsbpridesc;
    memset(&dsbpridesc, 0, sizeof(DSBUFFERDESC));
    dsbpridesc.dwSize = sizeof(DSBUFFERDESC);
    dsbpridesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbpridesc.dwBufferBytes = 0;
    dsbpridesc.lpwfxFormat = NULL;
    // create primary buffer and set its format
    HRESULT res = dsound->CreateSoundBuffer(&dsbpridesc, &prim_buf, NULL);
    if (res != DS_OK) {
        destroy();
        qWarning("Cannot create primary buffer (%s)", dserr2str(res));
        return false;
    }
    res = prim_buf->SetFormat((WAVEFORMATEX *)&wformat);
    if (res != DS_OK) {
        qWarning("Cannot set primary buffer format (%s), using standard setting (bad quality)", dserr2str(res));
    }
    qDebug("primary buffer created");

    // fill in the secondary sound buffer (=stream buffer) descriptor
    DSBUFFERDESC dsbdesc;
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 /** Better position accuracy */
                      | DSBCAPS_GLOBALFOCUS       /** Allows background playing */
                      | DSBCAPS_CTRLVOLUME;       /** volume control enabled */
    dsbdesc.dwBufferBytes = bufferSizeTotal();
    dsbdesc.lpwfxFormat = (WAVEFORMATEX *)&wformat;
    // Needed for 5.1 on emu101k - shit soundblaster
    if (format.channels() > 2)
        dsbdesc.dwFlags |= DSBCAPS_LOCHARDWARE;
    // now create the stream buffer
    res = dsound->CreateSoundBuffer(&dsbdesc, &stream_buf, NULL);
    if (res != DS_OK) {
        if (dsbdesc.dwFlags & DSBCAPS_LOCHARDWARE) {
            // Try without DSBCAPS_LOCHARDWARE
            dsbdesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
            res = dsound->CreateSoundBuffer(&dsbdesc, &stream_buf, NULL);
        }
        if (res != DS_OK) {
            destroy();
            qWarning("Cannot create secondary (stream)buffer (%s)", dserr2str(res));
            return false;
        }
    }
    qDebug( "Secondary (stream)buffer created");
    return true;
}

} // namespace QtAV
