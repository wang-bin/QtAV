/******************************************************************************
    VideoThread.h: description
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


#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include <QtAV/AVThread.h>

namespace QtAV {

class VideoThreadPrivate;
class VideoThread : public AVThread
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(VideoThread)
public:
    explicit VideoThread(QObject *parent = 0);
    virtual void stop();

protected:
    virtual void run();
};

} //namespace QtAV
#endif // VIDEOTHREAD_H
