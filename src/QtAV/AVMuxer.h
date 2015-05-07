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

#ifndef QAV_AVMUXER_H
#define QAV_AVMUXER_H

#include <QtAV/AVError.h>
#include <QtAV/Packet.h>
#include <QtCore/QObject>
#include <QtCore/QIODevice>
#include <QtCore/QScopedPointer>

namespace QtAV {

class VideoEncoder;
class Q_AV_EXPORT AVMuxer : public QObject
{
    Q_OBJECT
public:
    /// Supported ffmpeg/libav input protocols(not complete). A static string list
    static const QStringList& supportedProtocols();

    AVMuxer(QObject *parent = 0);
    ~AVMuxer();
    QString fileName() const;
    QIODevice* ioDevice() const;
    /// not null for QIODevice, custom protocols
    //AVWriter* writer() const;
    /*!
     * \brief setMedia
     * \return whether the media is changed
     */
    bool setMedia(const QString& fileName);
    bool setMedia(QIODevice* dev);
    //bool setMedia(AVWriter* writer);
    /*!
     * \brief setFormat
     * Force the output format.
     * formatForced() is reset if media changed. So you have to call setFormat() for every media
     * you want to force the format.
     * Also useful for custom io
     */
    void setFormat(const QString& fmt);
    QString formatForced() const;

    bool open();
    bool close();
    bool isOpen() const;
    void setAudioCodecProperties(void* avctx);
    void setVideoCodecProperties(void* avctx);
    // TODO: multiple streams. Packet.type,stream
    bool writeAudio(const Packet& packet);
    bool writeVideo(const Packet& packet);

    void copyProperties(VideoEncoder* enc);
private:
    class Private;
    QScopedPointer<Private> d;
};
} //namespace QtAV
#endif //QAV_AVMUXER_H
