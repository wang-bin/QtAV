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

#ifndef QTAV_SUBTITLEPROCESSOR_H
#define QTAV_SUBTITLEPROCESSOR_H

#include <QtCore/QList>
#include <QtGui/QImage>
#include <QtAV/QtAV_Global.h>
#include <QtAV/FactoryDefine.h>
#include <QtAV/Subtitle.h>

namespace QtAV {

typedef QString SubtitleProcessorId;
class SubtitleProcessor;
FACTORY_DECLARE(SubtitleProcessor)


class Q_AV_PRIVATE_EXPORT SubtitleProcessor
{
public:
    enum SourceType {
        RawData,
        File
    };
    //virtual ~SubtitleProcessor() = 0;
    virtual SubtitleProcessorId id() const = 0;
    virtual QString name() const = 0;
    virtual bool isSupported(SourceType) const { return true;}
    /*!
     * \brief process
     * process subtitle from QIODevice. SourceType RawData must be supported
     * \param dev dev is open and you don't have to close it
     * \return false if failed, e.g. does not support sequential device
     */
    virtual bool process(QIODevice* dev) = 0;
    // isSupported(File) must be true. default behavior is calling process(QFile*)
    virtual bool process(const QString& path);
    /*!
     * \brief timestamps
     * call this after process(). SubtitleFrame.text must be set
     * \return
     */
    virtual QList<SubtitleFrame> frames() const = 0;
    // add to higher level api Subtitle
    // return false if not supported
    //virtual bool setHeader(const QByteArray& data) = 0;
    // return timestamp, add to class Subtitle
    //virtual qreal process(const QByteArray& data, qreal pts = -1) = 0;
    virtual QString getText(qreal pts) const = 0;
    virtual QImage getImage(qreal pts, int width, int height) = 0;
};

} //namespace QtAV
#endif // QTAV_SUBTITLEPROCESSOR_H
