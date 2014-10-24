/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_VIDEOFRAMEEXTRACTOR_H
#define QTAV_VIDEOFRAMEEXTRACTOR_H

#include <QtCore/QObject>
#include <QtAV/VideoFrame.h>

namespace QtAV {

class VideoFrameExtractorPrivate;
class Q_AV_EXPORT VideoFrameExtractor : public QObject
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoFrameExtractor)
public:
    explicit VideoFrameExtractor(QObject *parent = 0);
    void setSource(const QString value);
    QString source() const;
    void setAutoExtract(bool value);
    bool autoExtract() const;
    /*!
     * \brief setPrecision
     * if the difference of the next requested position is less than the value, the
     * last one is used and not positionChanged() signal to emit.
     * Default is 500ms.
     */
    void setPrecision(int value);
    int precision() const;
    void setPosition(qint64 value);
    qint64 position() const;
    /*!
     * \brief frame
     * \return the last video frame extracted
     */
    VideoFrame frame();

signals:
    void frameExtracted(); // parameter: VideoFrame, bool changed?
    void sourceChanged();
    void error(); // clear preview image in a slot
    void autoExtractChanged();
    /*!
     * \brief positionChanged
     * If not autoExtract, positionChanged() => extract() in a slot
     */
    void positionChanged();
    void precisionChanged();

public slots:
    /*!
     * \brief extract
     * If last extracted frame can be use, use it.
     * If there is a key frame in [position-precision, position+precision], then the nearest key frame will be extracted.
     * Otherwise, the given position frame will be extracted.
     */
    void extract();

protected:
    VideoFrameExtractor(VideoFrameExtractorPrivate &d, QObject* parent = 0);
    DPTR_DECLARE(VideoFrameExtractor)
};

} //namespace QtAV
#endif // QTAV_VIDEOFRAMEEXTRACTOR_H
