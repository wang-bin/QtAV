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

#ifndef QTAV_AUDIOFORMAT_H
#define QTAV_AUDIOFORMAT_H

#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtAV/QtAV_Global.h>

class QDebug;
namespace QtAV {

class AudioFormatPrivate;
class Q_AV_EXPORT AudioFormat
{
public:
    //TODO: what about paInt24
    enum SampleFormat {
        SampleFormat_Unknown = -1,
        SampleFormat_Input = SampleFormat_Unknown,
        SampleFormat_Unsigned8,
        SampleFormat_Signed16,
        SampleFormat_Signed32,
        SampleFormat_Float,
        SampleFormat_Double,
        SampleFormat_Unsigned8Planar,
        SampleFormat_Signed16Planar,
        SampleFormat_Signed32Planar,
        SampleFormat_FloatPlanar,
        SampleFormat_DoublePlanar
    };
    enum ChannelLayout {
        ChannelLayout_Left,
        ChannelLayout_Right,
        ChannelLayout_Center,
        ChannelLayout_Mono = ChannelLayout_Center,
        ChannelLayout_Stero,
        ChannelLayout_Unsupported //ok. now it's not complete
    };
    //typedef qint64 ChannelLayout; //currently use latest FFmpeg's

    static ChannelLayout channelLayoutFromFFmpeg(qint64 clff);
    static qint64 channelLayoutToFFmpeg(ChannelLayout cl);
    static bool isPlanar(SampleFormat format);

    AudioFormat();
    AudioFormat(const AudioFormat &other);
    ~AudioFormat();

    AudioFormat& operator=(const AudioFormat &other);
    bool operator==(const AudioFormat &other) const;
    bool operator!=(const AudioFormat &other) const;

    bool isValid() const;
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
