/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#ifndef QTAV_VIDEOTHREAD_H
#define QTAV_VIDEOTHREAD_H

#include <QtAV/AVThread.h>
#include <QtCore/QSize>

namespace QtAV {

class VideoThreadPrivate;
class VideoThread : public AVThread
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoThread)
public:
    explicit VideoThread(QObject *parent = 0);
    QByteArray currentRawImage() const;
    QSize currentRawImageSize() const; //TODO: remove it? use dec's size outside?
    double currentPts() const;
protected:
    virtual void run();
};

} //namespace QtAV
#endif // QTAV_VIDEOTHREAD_H
