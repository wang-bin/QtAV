/******************************************************************************
    VideoWall:  this file is part of QtAV examples
    Copyright (C) 2013-2016 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_VIDEOWALL_H
#define QTAV_VIDEOWALL_H

#include <QtCore/QList>
#include <QtAV/AVPlayer.h>
#include <QtAVWidgets/WidgetRenderer.h>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE
class VideoWall : public QObject
{
    Q_OBJECT
public:
    explicit VideoWall(QObject *parent = 0);
    ~VideoWall();
    void setVideoRendererTypeString(const QString& vt);
    void setRows(int n);
    void setCols(int n);
    int rows() const;
    int cols() const;
    void show();
    void play(const QString& file);

public slots:
    void stop();
    void openLocalFile();
    void openUrl();
    void about();
    void help();

protected:
    virtual bool eventFilter(QObject *, QEvent *);
    virtual void timerEvent(QTimerEvent *e);

private Q_SLOTS:
    void changeClockType();

private:
    int r, c;
    int timer_id;
    QtAV::AVClock *clock;
    QList<QtAV::AVPlayer*> players;
    QWidget *view;
    QMenu *menu;
    QString vid;
};

#endif // QTAV_VIDEOWALL_H
