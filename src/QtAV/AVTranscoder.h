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

#ifndef QTAV_AVTRANSCODE_H
#define QTAV_AVTRANSCODE_H

#include <QtAV/MediaIO.h>
#include <QtAV/AudioEncoder.h>
#include <QtAV/VideoEncoder.h>

namespace QtAV {

class AVPlayer;
class Q_AV_EXPORT AVTranscoder : public QObject
{
    Q_OBJECT
public:
    AVTranscoder(QObject* parent = 0);
    ~AVTranscoder();

    // TODO: other source (more operations needed, e.g. seek)?
    void setMediaSource(AVPlayer* player);
    AVPlayer* sourcePlayer() const;

    QString outputFile() const;
    QIODevice* outputDevice() const;
    MediaIO* outputMediaIO() const;
    /*!
     * \brief setOutputMedia
     */
    void setOutputMedia(const QString& fileName);
    void setOutputMedia(QIODevice* dev);
    void setOutputMedia(MediaIO* io);
    /*!
     * \brief setOutputFormat
     * Force the output format. Useful for custom io
     */
    void setOutputFormat(const QString& fmt);
    QString outputFormatForced() const;

    void setOutputOptions(const QVariantHash &dict);
    QVariantHash outputOptions() const;

    /*!
     * \brief createEncoder
     * Destroy old encoder and create a new one in filter chain. Filter has the ownership. You shall not manually open it. Transcoder will set the missing parameters open it.
     * \param name registered encoder name, for example "FFmpeg"
     * \return false if failed
     */
    bool createVideoEncoder(const QString& name = QStringLiteral("FFmpeg"));
    /*!
     * \brief encoder
     * Use this to set encoder properties and options.
     * If frameRate is not set, source frame rate will be set if it's valid, otherwise VideoEncoder::defaultFrameRate() will be used internally
     * \return Encoder instance or null if createVideoEncoder failed
     */
    VideoEncoder* videoEncoder() const;
    /*!
     * \brief createEncoder
     * Destroy old encoder and create a new one in filter chain. Filter has the ownership. You shall not manually open it. Transcoder will set the missing parameters open it.
     * \param name registered encoder name, for example "FFmpeg"
     * \return false if failed
     */
    bool createAudioEncoder(const QString& name = QStringLiteral("FFmpeg"));
    /*!
     * \brief encoder
     * Use this to set encoder properties and options
     * \return Encoder instance or null if createAudioEncoder failed
     */
    AudioEncoder* audioEncoder() const;
    /*!
     * \brief isRunning
     * \return true if encoding started
     */
    bool isRunning() const;
    bool isPaused() const;
    qint64 encodedSize() const;
    qreal startTimestamp() const;
    qreal encodedDuration() const;

Q_SIGNALS:
    void videoFrameEncoded(qreal timestamp);
    void audioFrameEncoded(qreal timestamp);
    void started();
    void stopped();
    void paused(bool value);

public Q_SLOTS:
    void start();
    /*!
     * \brief stop
     * Call stop() to encode delayed frames remains in encoder and then stop encoding.
     * It's called internally when sourcePlayer() is stopped
     */
    void stop();
    /*!
     * \brief pause
     * pause the encoders
     * \param value
     */
    void pause(bool value);

private Q_SLOTS:
    void onSourceStarted();
    void prepareMuxer();
    void writeAudio(const QtAV::Packet& packet);
    void writeVideo(const QtAV::Packet& packet);

private:
    class Private;
    QScopedPointer<Private> d;
};
} //namespace QtAV
#endif // QTAV_AVTRANSCODE_H

