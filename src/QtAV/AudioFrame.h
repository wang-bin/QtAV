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
    AudioFrame(); //invalid frame
    //data must be complete
    AudioFrame(const QByteArray& data, const AudioFormat& format);
    AudioFrame(const AudioFrame &other);
    virtual ~AudioFrame();

    AudioFrame &operator =(const AudioFrame &other);

    /*!
     * Deep copy. If you want to copy data from somewhere, knowing the format, width and height,
     * then you can use clone().
     */
    AudioFrame clone() const;
    /*!
     * Allocate memory with given format, width and height. planes and bytesPerLine will be set.
     * The memory can be initialized by user
     */
    virtual int allocate();
    AudioFormat format() const;
    void setSamplesPerChannel(int samples);
    // may change after resampling
    int samplesPerChannel() const;
    //AudioResamplerId
    void setAudioResampler(AudioResampler *conv);
    /*!
     * \brief convertTo
     * \code
     *   AudioFrame af(data, fmt1);
     *   af.convertTo(fmt2);
     * \param fmt
     * \return
     */
    bool convertTo(const AudioFormat& fmt);
private:
    /*
     * call this only when setBytesPerLine() and setBits() will not be called
     */
    void init();
};

} //namespace QtAV

#endif // QTAV_AUDIOFRAME_H
