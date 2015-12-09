/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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

#ifndef QTAV_ENCODEFILTER_H
#define QTAV_ENCODEFILTER_H

#include <QtAV/Filter.h>
#include <QtAV/Packet.h>
#include <QtAV/AudioFrame.h>
#include <QtAV/VideoFrame.h>

namespace QtAV {

class AudioEncoder;
class AudioEncodeFilterPrivate;
class Q_AV_EXPORT AudioEncodeFilter : public AudioFilter
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(AudioEncodeFilter)
public:
    AudioEncodeFilter(QObject *parent = 0);

    /*!
     * \brief createEncoder
     * Destroy old encoder and create a new one. Filter has the ownership.
     * Encoder will open when encoding first valid frame, and set width/height as frame's.
     * \param name registered encoder name, for example "FFmpeg"
     * \return null if failed
     */
    AudioEncoder* createEncoder(const QString& name = QStringLiteral("FFmpeg"));
    /*!
     * \brief encoder
     * Use this to set encoder properties and options
     * \return Encoder instance or null if createEncoder failed
     */
    AudioEncoder* encoder() const;
    // TODO: async property

Q_SIGNALS:
    /*!
     * \brief readyToEncode
     * Emitted when encoder is open. All parameters are set and muxer can set codec properties now.
     * close the encoder() to reset and reopen.
     */
    void readyToEncode();
    void frameEncoded(const QtAV::Packet& packet);

protected:
    virtual void process(Statistics* statistics, AudioFrame* frame = 0) Q_DECL_OVERRIDE;
    void encode(const AudioFrame& frame = AudioFrame());
};

class VideoEncoder;
class VideoEncodeFilterPrivate;
class Q_AV_EXPORT VideoEncodeFilter : public VideoFilter
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoEncodeFilter)
public:
    VideoEncodeFilter(QObject* parent = 0);
    bool isSupported(VideoFilterContext::Type t) const Q_DECL_OVERRIDE { return t == VideoFilterContext::None;}
    /*!
     * \brief createEncoder
     * Destroy old encoder and create a new one. Filter has the ownership.
     * Encoder will open when encoding first valid frame, and set width/height as frame's.
     * \param name registered encoder name, for example "FFmpeg"
     * \return null if failed
     */
    VideoEncoder* createEncoder(const QString& name = QStringLiteral("FFmpeg"));
    /*!
     * \brief encoder
     * Use this to set encoder properties and options
     * \return Encoder instance or null if createEncoder failed
     */
    VideoEncoder* encoder() const;
    // TODO: async property

Q_SIGNALS:
    /*!
     * \brief readyToEncode
     * Emitted when encoder is open. All parameters are set and muxer can set codec properties now.
     * close the encoder() to reset and reopen.
     */
    void readyToEncode();
    void frameEncoded(const QtAV::Packet& packet);

protected:
    virtual void process(Statistics* statistics, VideoFrame* frame = 0) Q_DECL_OVERRIDE;
    void encode(const VideoFrame& frame = VideoFrame());
};

} //namespace QtAV
#endif // QTAV_ENCODEFILTER_H

