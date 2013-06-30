/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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
#include <QtAV/QtAV_Global.h>

namespace QtAV {

class AudioFormatPrivate;
class Q_EXPORT AudioFormat
{
public:
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
    //enum ChannelLayout {
    //};
    typedef qint64 ChannelLayout; //currently use latest FFmpeg's

    static bool isPlanar(SampleFormat format);

    AudioFormat();
    AudioFormat(const AudioFormat &other);
    ~AudioFormat();

    AudioFormat& operator=(const AudioFormat &other);
    bool operator==(const AudioFormat &other) const;
    bool operator!=(const AudioFormat &other) const;

    bool isValid() const;
    bool isPlanar() const;

    void setSampleRate(int sampleRate);
    int sampleRate() const;

    //currently use FFmpeg's. see avutil/channel_layout.h
    void setChannelLayout(ChannelLayout layout);
    ChannelLayout channelLayout() const;

    void setChannels(int channels);
    int channels() const;

    void setSampleFormat(SampleFormat sampleFormat);
    SampleFormat sampleFormat() const;
    void setSampleFormatFFmpeg(int ffSampleFormat);
    int sampleFormatFFmpeg() const;

    // Helper functions
    qint32 bytesForDuration(qint64 duration) const;
    qint64 durationForBytes(qint32 byteCount) const;

    qint32 bytesForFrames(qint32 frameCount) const;
    qint32 framesForBytes(qint32 byteCount) const;

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

private:
    QSharedDataPointer<AudioFormatPrivate> d;
};

//operator<<

} //namespace QtAV

#endif // QTAV_AUDIOFORMAT_H
