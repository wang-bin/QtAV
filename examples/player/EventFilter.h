/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

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
#include <QtCore/QPoint>

class QMenu;
class QPoint;
namespace QtAV {
class AVPlayer;
}
//for internal use
class EventFilter : public QObject
{
    Q_OBJECT
public:
    explicit EventFilter(QtAV::AVPlayer *player);
    virtual ~EventFilter();

signals:
    void helpRequested();

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
};


class WindowEventFilter : public QObject
{
    Q_OBJECT
public:
    WindowEventFilter(QWidget *window);

signals:
    void fullscreenChanged();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event);

private:
    QWidget *mpWindow;
};

#endif // QTAV_EVENTFILTER_H
