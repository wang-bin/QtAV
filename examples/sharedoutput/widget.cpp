/******************************************************************************
    Shared output:  shared renderer test
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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

#include "widget.h"
#include <QtAV/AVPlayer.h>
#include <QtAV/WidgetRenderer.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QTimer>

/*
    rtsp://122.192.35.80:554/live/tv01 江苏城市
    rtsp://122.192.35.80:554/live/tv02 江苏卫视
    rtsp://122.192.35.80:554/live/tv03 江苏综艺
    rtsp://122.192.35.80:554/live/tv07 CCTV4
    rtsp://122.192.35.80:554/live/tv08 旅游卫视
    rtsp://122.192.35.80:554/live/tv08 NJTV新闻综合
    rtsp://122.192.35.80:554/live/tv10 湖南卫视
    rtsp://122.192.35.80:554/live/tv11 东方卫视
    rtsp://122.192.35.80:554/live/tv12 档案（北京）
    rtsp://122.192.35.80:554/live/tv14 CCTV1
    rtsp://122.192.35.80:554/live/tv16 辽宁卫视
    rtsp://122.192.35.80:554/live/tv20 MursTv
    rtsp://122.192.35.80:554/live/tv28 幸福Tv
    rtsp://122.192.35.80:554/live/tv30 CCTV3
    rtsp://122.192.35.80:554/live/tv31 CCTV6
    rtsp://122.192.35.80:554/live/tv50 靓妆
    rtsp://122.192.35.80:554/live/tv51 江苏公共
    rtsp://122.192.35.80:554/live/tv60 新闻综合（江苏）
    rtsp://122.192.35.80:554/live/tv61 新闻
    rtsp://122.192.35.80:554/live/tv62 盐城
    rtsp://122.192.35.80:554/live/tv63
    rtsp://122.192.35.80:554/live/tv73 新闻综合

    rtmp://mirtv.cdnvideo.ru/mirtv-live/mirtv300.sdp
*/

using namespace QtAV;

Widget::Widget(QWidget *parent) :
    QWidget(parent)
{
    setWindowTitle("A test for shared video renderer. QtAV" + QtAV_Version_String_Long() + " wbsecg1@gmail.com");
    QVBoxLayout *main_layout = new QVBoxLayout;
    QHBoxLayout *btn_layout = new QHBoxLayout;
    renderer = new WidgetRenderer;
    renderer->setFocusPolicy(Qt::StrongFocus);
    renderer->resizeRenderer(640, 480);
    for (int i = 0; i < 2; ++i) {
        player[i] = new AVPlayer(this);
        player[i]->setRenderer(renderer);
        QVBoxLayout *vb = new QVBoxLayout;
        play_btn[i] = new QPushButton(this);
        play_btn[i]->setText(QString("Play-%1").arg(i));
        file_btn[i] = new QPushButton(this);
        file_btn[i]->setText(QString("Choose video-%1").arg(i));
        connect(play_btn[i], SIGNAL(clicked()), SLOT(playVideo()));
        connect(file_btn[i], SIGNAL(clicked()), SLOT(setVideo()));
        vb->addWidget(play_btn[i]);
        vb->addWidget(file_btn[i]);
        btn_layout->addLayout(vb);
    }
    QPushButton *net_btn = new QPushButton(tr("Test online video(rtsp)"));
    connect(net_btn, SIGNAL(clicked()), SLOT(testRTSP()));
    main_layout->addWidget(renderer);
    main_layout->addWidget(net_btn);
    main_layout->addLayout(btn_layout);
    setLayout(main_layout);
    resize(720, 600);
}

Widget::~Widget()
{
}

void Widget::playVideo()
{
    for (int i = 0; i < 2; ++i)
        player[i]->pause(true);
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    int idx = btn->text().section('-', 1).toInt();
    player[idx]->pause(false);
}

void Widget::setVideo()
{
    QString v = QFileDialog::getOpenFileName(0, "Select a video");
    if (v.isEmpty())
        return;

    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    int idx = btn->text().section('-', 1).toInt();
    QString oldv = player[idx]->file();
    if (v == oldv)
        return;
    for (int i = 0; i < 2; ++i)
        player[i]->pause(true);
    player[idx]->stop();
    player[idx]->setFile(v);
    player[idx]->play();
}

void Widget::testRTSP()
{
    for (int i = 0; i < 2; ++i)
        player[i]->stop();
    player[0]->play("rtsp://122.192.35.80:554/live/tv11");
    player[0]->pause(true);

    player[1]->play("rtsp://122.192.35.80:554/live/tv10");
}
