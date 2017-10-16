/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_AUDIOFORMAT_H
#define QTAV_AUDIOFORMAT_H

#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtAV/QtAV_Global.h>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE
namespace QtAV {

class AudioFormatPrivate;
class Q_AV_EXPORT AudioFormat
{
    enum { kSize = 12, kFloat = 1<<(kSize+1), kUnsigned = 1<<(kSize+2), kPlanar = 1<<(kSize+3), kByteOrder = 1<<(kSize+4) };
public:
    /*!
     * \brief The SampleFormat enum
     * s8, u16, u24, s24, u32 are not listed in ffmpeg sample format and have not planar format.
     * pcm_s24le will be decoded as s32-24bit in ffmpeg, it's encoded as 32 bits, but raw sample has 24 bits
     */
    enum SampleFormat {
        SampleFormat_Unknown = 0,
        SampleFormat_Input = SampleFormat_Unknown,
        SampleFormat_Unsigned8 = 1 | kUnsigned,
        SampleFormat_Signed8 = 1,
        SampleFormat_Unigned16 = 2 | kUnsigned,
        SampleFormat_Signed16 = 2,
        SampleFormat_Unsigned24 = 3 | kUnsigned,
        SampleFormat_Signed24 = 3,
        SampleFormat_Unsigned32 = 4 | kUnsigned,
        SampleFormat_Signed32 = 4,
        SampleFormat_Float = 4 | kFloat,
        SampleFormat_Double = 8 | kFloat,
        SampleFormat_Unsigned8Planar = SampleFormat_Unsigned8 | kPlanar,
        SampleFormat_Signed16Planar = SampleFormat_Signed16 | kPlanar,
        SampleFormat_Signed32Planar = SampleFormat_Signed32 | kPlanar,
        SampleFormat_FloatPlanar = SampleFormat_Float | kPlanar,
        SampleFormat_DoublePlanar = SampleFormat_Double | kPlanar
    };
    enum ChannelLayout {
        ChannelLayout_Left,
        ChannelLayout_Right,
        ChannelLayout_Center,
        ChannelLayout_Mono = ChannelLayout_Center,
        ChannelLayout_Stereo,
        ChannelLayout_Unsupported //ok. now it's not complete
    };

    static const qint64 kHz = 1000000LL;

    //typedef qint64 ChannelLayout; //currently use latest FFmpeg's
    // TODO: constexpr
    friend int RawSampleSize(SampleFormat fmt) { return fmt & ((1<<(kSize+1)) - 1); }
    friend bool IsFloat(SampleFormat fmt) { return !!(fmt & kFloat);}
    friend bool IsPlanar(SampleFormat format) { return !!(format & kPlanar);}
    friend bool IsUnsigned(SampleFormat format) {return !!(format & kUnsigned);}
    friend SampleFormat ToPlanar(SampleFormat fmt) { return IsPlanar(fmt) ? fmt : SampleFormat((int)fmt|kPlanar);}
    friend SampleFormat ToPacked(SampleFormat fmt) { return IsPlanar(fmt) ? SampleFormat((int)fmt^kPlanar) : fmt;}

    static ChannelLayout channelLayoutFromFFmpeg(qint64 clff);
    static qint64 channelLayoutToFFmpeg(ChannelLayout cl);
    static SampleFormat sampleFormatFromFFmpeg(int fffmt);
    static int sampleFormatToFFmpeg(SampleFormat fmt);
    static SampleFormat make(int bytesPerSample, bool isFloat, bool isUnsigned, bool isPlanar);
    AudioFormat();
    AudioFormat(const AudioFormat &other);
    ~AudioFormat();

    AudioFormat& operator=(const AudioFormat &other);
    bool operator==(const AudioFormat &other) const;
    bool operator!=(const AudioFormat &other) const;

    bool isValid() const;
    bool isFloat() const;
    bool isUnsigned() const;
    bool isPlanar() const;
    int planeCount() const;

    void setSampleRate(int sampleRate);
    int sampleRate() const;

    /*!
     * setChannelLayout and setChannelLayoutFFmpeg also sets the correct channels if channels does not match.
     */
    void setChannelLayoutFFmpeg(qint64 layout);
    qint64 channelLayoutFFmpeg() const;
    //currently a limitted set of channel layout is supported. call setChannelLayoutFFmpeg is recommended
    void setChannelLayout(ChannelLayout layout);
    ChannelLayout channelLayout() const;
    QString channelLayoutName() const;
    /*!
     * setChannels also sets the default layout for this channels if channels does not match.
     */
    void setChannels(int channels);
    /*!
     * \brief channels
     *   For planar format, channel count == plane count. For packed format, plane count is 1
     * \return
     */
    int channels() const;

    void setSampleFormat(SampleFormat sampleFormat);
    SampleFormat sampleFormat() const;
    void setSampleFormatFFmpeg(int ffSampleFormat);
    int sampleFormatFFmpeg() const;
    QString sampleFormatName() const;

    // Helper functions
    // in microseconds
    qint32 bytesForDuration(qint64 duration) const;
    qint64 durationForBytes(qint32 byteCount) const;

    qint32 bytesForFrames(qint32 frameCount) const;
    qint32 framesForBytes(qint32 byteCount) const;

    // in microseconds
    qint32 framesForDuration(qint64 duration) const;
    qint64 durationForFrames(qint32 frameCount) const;

    // 1 frame = 1 sample with channels
    /*!
        Returns the number of bytes required to represent one frame (a sample in each channel) in this format.
        Returns 0 if this format is invalid.
    */
    int bytesPerFrame() const;
    /*!
        Returns the current sample size value, in bytes.
        \sa bytesPerFrame()
    */
    int bytesPerSample() const;
    int sampleSize() const; // the same as bytesPerSample()
    int bitRate() const; //bits per second
    int bytesPerSecond() const;
private:
    QSharedDataPointer<AudioFormatPrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
Q_AV_EXPORT QDebug operator<<(QDebug debug, const AudioFormat &fmt);
Q_AV_EXPORT QDebug operator<<(QDebug debug, AudioFormat::SampleFormat sampleFormat);
Q_AV_EXPORT QDebug operator<<(QDebug debug, AudioFormat::ChannelLayout channelLayout);
#endif

} //namespace QtAV

Q_DECLARE_METATYPE(QtAV::AudioFormat)
Q_DECLARE_METATYPE(QtAV::AudioFormat::SampleFormat)
Q_DECLARE_METATYPE(QtAV::AudioFormat::ChannelLayout)

#endif // QTAV_AUDIOFORMAT_H
