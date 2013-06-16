/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
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


#ifndef QTAV_EVENTFILTER_H
#define QTAV_EVENTFILTER_H

/*
 * This class is used interally as QtAV's default event filter. It is suite for single player object
 */
#include <QtCore/QObject>
#include <QtCore/QPoint>

class QMenu;
class QPoint;
namespace QtAV {

//for internal use
class EventFilter : public QObject
{
    Q_OBJECT
public:
    explicit EventFilter(QObject *parent = 0);
    virtual ~EventFilter();

public slots:
    void openLocalFile();
    void openUrl();
    void about();
    void aboutFFmpeg();
    void help();

protected:
    virtual bool eventFilter(QObject *, QEvent *);
    void showMenu(const QPoint& p);

private:
    QMenu *menu;
    QPoint gMousePos, iMousePos;
};

} //namespace QtAV
#endif // QTAV_EVENTFILTER_H
