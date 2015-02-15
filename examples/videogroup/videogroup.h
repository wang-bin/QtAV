/******************************************************************************
    VideoGroup:  this file is part of QtAV examples
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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
#include <QtAV>
#include <QtAVWidgets>
#include <QPushButton>

class VideoGroup : public QObject
{
    Q_OBJECT
public:
    explicit VideoGroup(QObject *parent = 0);
    ~VideoGroup();
    void setVideoRendererTypeString(const QString& vt);
    void setRows(int n);
    void setCols(int n);
    int rows() const;
    int cols() const;
    void show();
    void play(const QString& file);
    void updateROI();

public slots:
    void setSingleWindow(bool s);
    void toggleSingleFrame(bool s);
    void toggleFrameless(bool f);
    void openLocalFile();
    void addRenderer();
    void removeRenderer();

private:
    bool m1Window;
    bool m1Frame;
    bool mFrameless;
    int r, c;
    int timer_id;
    QtAV::AVPlayer *mpPlayer;
    QWidget *view;
    QWidget *mpBar;
    QPushButton *mpAdd, *mpRemove, *mpOpen, *mpPlay, *mpStop, *mpPause, *mp1Window;
    QPushButton *mp1Frame;
    QPushButton *mpFrameless;
    QString vid;
    QList<QtAV::VideoRenderer*> mRenderers;
};

#endif // QTAV_VIDEOWALL_H
