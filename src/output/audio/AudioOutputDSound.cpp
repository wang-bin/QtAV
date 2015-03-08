/******************************************************************************
    AudioOutputDSound.cpp: description
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/private/prepost.h"
#include <QtCore/QLibrary>
#include <math.h>
#include <windows.h>
#define DIRECTSOUND_VERSION 0x0600
#include <dsound.h>
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

namespace QtAV {
class AudioOutputDSoundPrivate;
class AudioOutputDSound : public AudioOutput
{
    DPTR_DECLARE_PRIVATE(AudioOutputDSound)
public:
    AudioOutputDSound(QObject *parent = 0);
    //AudioOutputId id() const
    bool open() Q_DECL_FINAL;
    bool close() Q_DECL_FINAL;
    bool isSupported(AudioFormat::SampleFormat sampleFormat) const Q_DECL_FINAL;
protected:
    BufferControl bufferControl() const Q_DECL_FINAL;
    bool write(const QByteArray& data) Q_DECL_FINAL;
    bool play() Q_DECL_FINAL;
    int getOffsetByBytes() Q_DECL_FINAL;

    bool deviceSetVolume(qreal value) Q_DECL_FINAL;
    qreal deviceGetVolume() const Q_DECL_FINAL;
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

#define DX_LOG_COMPONENT "DSound"

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

class  AudioOutputDSoundPrivate : public AudioOutputPrivate
{
public:
    AudioOutputDSoundPrivate()
        : AudioOutputPrivate()
        , dll(NULL)
        , dsound(NULL)
        , prim_buf(NULL)
        , stream_buf(NULL)
        , write_offset(0)
    {
    }
    ~AudioOutputDSoundPrivate() {
        destroy();
    }
    bool loadDll();
    bool unloadDll();
    bool init();
    bool destroy() {
        SafeRelease(&prim_buf);
        SafeRelease(&stream_buf);
        SafeRelease(&dsound);
        unloadDll();
        return true;
    }
    bool createDSoundBuffers();

    HINSTANCE dll;
    LPDIRECTSOUND dsound;              ///direct sound object
    LPDIRECTSOUNDBUFFER prim_buf;      ///primary direct sound buffer
    LPDIRECTSOUNDBUFFER stream_buf;    ///secondary direct sound buffer (stream buffer)
    int write_offset;               ///offset of the write cursor in the direct sound buffer
};

AudioOutputDSound::AudioOutputDSound(QObject *parent)
    :AudioOutput(DeviceFeatures()|SetVolume, *new AudioOutputDSoundPrivate(), parent)
{
    setDeviceFeatures(DeviceFeatures()|SetVolume);
}

bool AudioOutputDSound::open()
{
    DPTR_D(AudioOutputDSound);
    d.available = false;
    resetStatus();
    if (!d.init())
        goto error;
    if (!d.createDSoundBuffers())
        goto error;
    d.available = true;
/*
    for (int i = 0; i < bufferCount(); ++i) {
        write(QByteArray(bufferSize(), 0));
        d.bufferAdded();
    }*/
    return true;
error:
    d.unloadDll();
    SafeRelease(&d.dsound);
    return false;
}

bool AudioOutputDSound::close()
{
    DPTR_D(AudioOutputDSound);
    resetStatus();
    d.destroy();
    return true;
}

bool AudioOutputDSound::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    return sampleFormat == AudioFormat::SampleFormat_Signed16
            || sampleFormat == AudioFormat::SampleFormat_Float;
}

AudioOutput::BufferControl AudioOutputDSound::bufferControl() const
{
    return OffsetBytes;
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
        qWarning() << "Can not lock secondary buffer (" << res << "): " << qt_error_string(res);
        return false;
    }
    memcpy(dst1, data.constData(), size1);
    if (dst2)
        memcpy(dst2, data.constData() + size1, size2);
    d.write_offset += size1 + size2;
    if (d.write_offset >= d.bufferSizeTotal())
        d.write_offset = size2;
    DX_ENSURE_OK(d.stream_buf->Unlock(dst1, size1, dst2, size2), false);
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

int AudioOutputDSound::getOffsetByBytes()
{
    DPTR_D(AudioOutputDSound);
    DWORD read_offset = 0;
    d.stream_buf->GetCurrentPosition(&read_offset /*play*/, NULL /*write*/); //what's this write_offset?
    return (int)read_offset;
}

bool AudioOutputDSound::deviceSetVolume(qreal value)
{
    DPTR_D(AudioOutputDSound);
    // dsound supports [0, 1]
    const LONG vol = value <= 0 ? DSBVOLUME_MIN : LONG(log10(value*100.0) * 5000.0) + DSBVOLUME_MIN;
    // +DSBVOLUME_MIN == -100dB
    DX_ENSURE_OK(d.stream_buf->SetVolume(vol), false);
    return true;
}

qreal AudioOutputDSound::deviceGetVolume() const
{
    DPTR_D(const AudioOutputDSound);
    LONG vol = 0;
    DX_ENSURE_OK(d.stream_buf->GetVolume(&vol), 1.0);
    return pow(10.0, double(vol - DSBVOLUME_MIN)/5000.0)/100.0;
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
    DX_ENSURE_OK(dsound_create(NULL/*dev guid*/, &dsound, NULL), false);
    /*  DSSCL_EXCLUSIVE: can modify the settings of the primary buffer, only the sound of this app will be hearable when it will have the focus.
     */
    DX_ENSURE_OK(dsound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_EXCLUSIVE), false);
    qDebug("DirectSound initialized.");
    DSCAPS dscaps;
    memset(&dscaps, 0, sizeof(DSCAPS));
    dscaps.dwSize = sizeof(DSCAPS);
    DX_ENSURE_OK(dsound->GetCaps(&dscaps), false);
    if (dscaps.dwFlags & DSCAPS_EMULDRIVER)
        qDebug("DirectSound is emulated");

    write_offset = 0;
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
    DX_ENSURE_OK(dsound->CreateSoundBuffer(&dsbpridesc, &prim_buf, NULL), (destroy() && false));
    HRESULT res = prim_buf->SetFormat((WAVEFORMATEX *)&wformat);
    if (res != DS_OK) {
        qWarning() << "Cannot set primary buffer format (" << res << "): " << qt_error_string(res) << ". using standard setting (bad quality)";
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
    // now create the stream buffer (secondary buffer)
    res = dsound->CreateSoundBuffer(&dsbdesc, &stream_buf, NULL);
    if (res != DS_OK) {
        if (dsbdesc.dwFlags & DSBCAPS_LOCHARDWARE) {
            // Try without DSBCAPS_LOCHARDWARE
            dsbdesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
            DX_ENSURE_OK(dsound->CreateSoundBuffer(&dsbdesc, &stream_buf, NULL), (destroy() && false));
        }
    }
    qDebug( "Secondary (stream)buffer created");
    return true;
}

} // namespace QtAV
