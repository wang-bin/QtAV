/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#ifndef QTAV_VIDEOFRAMEEXTRACTOR_H
#define QTAV_VIDEOFRAMEEXTRACTOR_H

#include <QtCore/QObject>
#include <QtAV/VideoFrame.h>

//TODO: extract all streams
namespace QtAV {
class VideoFrameExtractorPrivate;
class Q_AV_EXPORT VideoFrameExtractor : public QObject
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoFrameExtractor)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool autoExtract READ autoExtract WRITE setAutoExtract NOTIFY autoExtractChanged)
    Q_PROPERTY(bool async READ async WRITE setAsync NOTIFY asyncChanged)
    Q_PROPERTY(int precision READ precision WRITE setPrecision NOTIFY precisionChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
public:
    explicit VideoFrameExtractor(QObject *parent = 0);
    /*!
     * \brief setSource
     * Set the video file. If video changes, current loaded video will be unloaded.
     */
    void setSource(const QString url);
    QString source() const;
    /*!
     * \brief setAsync
     * Extract video frames in another thread. Default is true.
     * In async mode, if current extraction is not finished, new
     * setPosition() will be ignored.
     */
    void setAsync(bool value);
    bool async() const;
    void setAutoExtract(bool value);
    bool autoExtract() const;
    /*!
     * \brief setPrecision
     * if the difference between the next requested position is less than the value, previous
     * one is used and no positionChanged() and frameExtracted() signals to emit.
     * \param value < 0: auto. Real value depends on video duration and fps, but always 20 <= value <=500
     * Default is auto.
     */
    void setPrecision(int value);
    int precision() const;
    void setPosition(qint64 value);
    qint64 position() const;

    virtual bool event(QEvent *e);
Q_SIGNALS:
    void frameExtracted(const QtAV::VideoFrame& frame); // parameter: VideoFrame, bool changed?
    void sourceChanged();
    void asyncChanged();
    void error(); // clear preview image in a slot
    void autoExtractChanged();
    /*!
     * \brief positionChanged
     * If not autoExtract, positionChanged() => extract() in a slot
     */
    void positionChanged();
    void precisionChanged();

    void aboutToExtract(qint64 pos);

public Q_SLOTS:
    /*!
     * \brief extract
     * If last extracted frame can be use, use it.
     * If there is a key frame in [position, position+precision], the nearest key frame
     * before position+precision will be extracted. Otherwise, the given position frame will be extracted.
     */
    void extract();
private Q_SLOTS:
    void extractInternal(qint64 pos);

protected:
    //VideoFrameExtractor(VideoFrameExtractorPrivate &d, QObject* parent = 0);
    DPTR_DECLARE(VideoFrameExtractor)
};

} //namespace QtAV
#endif // QTAV_VIDEOFRAMEEXTRACTOR_H
