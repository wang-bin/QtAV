/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
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


#ifndef QTAV_EVENTFILTER_H
#define QTAV_EVENTFILTER_H

/*
 * This class is used interally as QtAV's default event filter. It is suite for single player object
 */
#include <QtCore/QObject>

namespace QtAV {

//for internal use
class EventFilter : public QObject
{
    Q_OBJECT
public:
    explicit EventFilter(QObject *parent = 0);
    virtual ~EventFilter();
protected:
    virtual bool eventFilter(QObject *, QEvent *);
};

} //namespace QtAV
#endif // QTAV_EVENTFILTER_H
