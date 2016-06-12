/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
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

#ifndef QTAV_AUDIOFRAME_H
#define QTAV_AUDIOFRAME_H

#include <QtAV/Frame.h>
#include <QtAV/AudioFormat.h>

namespace QtAV {
class AudioResampler;
class AudioFramePrivate;
class Q_AV_EXPORT AudioFrame : public Frame
{
    Q_DECLARE_PRIVATE(AudioFrame)
public:
    //data must be complete
    /*!
     * \brief AudioFrame
     * construct an audio frame from a given buffer and format
     */
    AudioFrame(const AudioFormat& format = AudioFormat(), const QByteArray& data = QByteArray());
    AudioFrame(const AudioFrame &other);
    virtual ~AudioFrame();
    AudioFrame &operator =(const AudioFrame &other);

    bool isValid() const;
    operator bool() const { return isValid();}

    /*!
     * \brief data
     * Audio data. clone is called if frame is not constructed with a QByteArray.
     * \return
     */
    QByteArray data();
    virtual int channelCount() const;
    /*!
     * Deep copy. If you want to copy data from somewhere, knowing the format, width and height,
     * then you can use clone().
     */
    AudioFrame clone() const;
    AudioFormat format() const;
    void setSamplesPerChannel(int samples);
    // may change after resampling
    int samplesPerChannel() const;
    AudioFrame to(const AudioFormat& fmt) const;
    //AudioResamplerId
    void setAudioResampler(AudioResampler *conv); //TODO: remove
    /*!
        Returns the number of microseconds represented by \a bytes in this format.
        Returns 0 if this format is not valid.
        Note that some rounding may occur if \a bytes is not an exact multiple
        of the number of bytes per frame.
    */
    qint64 duration() const;
};
} //namespace QtAV
Q_DECLARE_METATYPE(QtAV::AudioFrame)
#endif // QTAV_AUDIOFRAME_H
