/******************************************************************************
    this file is part of QtAV examples
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

#ifndef QTAV_VIDEOPLAYER_H
#define QTAV_VIDEOPLAYER_H

#include <QtAV/AVPlayer.h>
#include <QtAVWidgets/GraphicsItemRenderer.h>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QGraphicsView;
QT_END_NAMESPACE
class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    VideoPlayer(QWidget *parent = 0);
    ~VideoPlayer();

    QSize sizeHint() const { return QSize(720, 640); }
    void play(const QString& file);

public slots:
    void setOpenGL(bool o = true);

private slots:
    void setOrientation(int value);
    void rotateVideo(int angle);
    void scaleVideo(int value);
    void open();

private:
    QtAV::AVPlayer mediaPlayer;
    QtAV::GraphicsItemRenderer *videoItem;
    QGraphicsView *view;
};

#endif //QTAV_VIDEOPLAYER_H
