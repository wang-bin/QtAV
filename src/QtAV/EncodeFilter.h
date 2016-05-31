/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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
     * \brief setAsync
     * Enable async encoding. Default is disabled.
     */
    void setAsync(bool value = true);
    bool isAsync() const;
    /*!
     * \brief createEncoder
     * Destroy old encoder and create a new one. Filter has the ownership.
     * Encoder will open when encoding first valid frame, and set width/height as frame's.
     * \param name registered encoder name, for example "FFmpeg"
     * \return null if failed
     */
    AudioEncoder* createEncoder(const QString& name = QLatin1String("FFmpeg"));
    /*!
     * \brief encoder
     * Use this to set encoder properties and options
     * \return Encoder instance or null if createEncoder failed
     */
    AudioEncoder* encoder() const;
    // TODO: async property

    /*!
     * \brief startTime
     * start to encode after startTime()
     */
    qint64 startTime() const;
    void setStartTime(qint64 value);
public Q_SLOTS:
    /*!
     * \brief finish
     * Tell the encoder no more frames to encode. Signal finished() will be emitted when all frames are encoded
     */
    void finish();
Q_SIGNALS:
    void finished();
    /*!
     * \brief readyToEncode
     * Emitted when encoder is open. All parameters are set and muxer can set codec properties now.
     * close the encoder() to reset and reopen.
     */
    void readyToEncode();
    void frameEncoded(const QtAV::Packet& packet);
    void startTimeChanged(qint64 value);
    // internal use only
    void requestToEncode(const AudioFrame& frame);
protected Q_SLOTS:
    void encode(const QtAV::AudioFrame& frame = AudioFrame());
protected:
    virtual void process(Statistics* statistics, AudioFrame* frame = 0) Q_DECL_OVERRIDE;
};

class VideoEncoder;
class VideoEncodeFilterPrivate;
class Q_AV_EXPORT VideoEncodeFilter : public VideoFilter
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoEncodeFilter)
public:
    VideoEncodeFilter(QObject* parent = 0);
    /*!
     * \brief setAsync
     * Enable async encoding. Default is disabled.
     */
    void setAsync(bool value = true);
    bool isAsync() const;
    bool isSupported(VideoFilterContext::Type t) const Q_DECL_OVERRIDE { return t == VideoFilterContext::None;}
    /*!
     * \brief createEncoder
     * Destroy old encoder and create a new one. Filter has the ownership.
     * Encoder will open when encoding first valid frame, and set width/height as frame's.
     * \param name registered encoder name, for example "FFmpeg"
     * \return null if failed
     */
    VideoEncoder* createEncoder(const QString& name = QLatin1String("FFmpeg"));
    /*!
     * \brief encoder
     * Use this to set encoder properties and options
     * \return Encoder instance or null if createEncoder failed
     */
    VideoEncoder* encoder() const;
    // TODO: async property

    /*!
     * \brief startTime
     * start to encode after startTime()
     */
    qint64 startTime() const;
    void setStartTime(qint64 value);
public Q_SLOTS:
    /*!
     * \brief finish
     * Tell the encoder no more frames to encode. Signal finished() will be emitted when all frames are encoded
     */
    void finish();
Q_SIGNALS:
    void finished();
    /*!
     * \brief readyToEncode
     * Emitted when encoder is open. All parameters are set and muxer can set codec properties now.
     * close the encoder() to reset and reopen.
     */
    void readyToEncode();
    void frameEncoded(const QtAV::Packet& packet);
    void startTimeChanged(qint64 value);
    // internal use only
    void requestToEncode(const QtAV::VideoFrame& frame);
protected Q_SLOTS:
    void encode(const QtAV::VideoFrame& frame = VideoFrame());
protected:
    virtual void process(Statistics* statistics, VideoFrame* frame = 0) Q_DECL_OVERRIDE;
};
} //namespace QtAV
#endif // QTAV_ENCODEFILTER_H

