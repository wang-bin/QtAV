/******************************************************************************
    Clock control:  external clock controlling test
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
#include <limits>
#include <QtAV/AVPlayer.h>
#include <QtAV/WidgetRenderer.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QTimer>

using namespace QtAV;

Widget::Widget(QWidget *parent) :
    QWidget(parent)
{
    setWindowTitle("A test for [clock controlling]. QtAV" QTAV_VERSION_STR_LONG " wbsecg1@gmail.com");
    clock = new AVClock(this);
    clock->setClockType(AVClock::ExternalClock);
    QVBoxLayout *main_layout = new QVBoxLayout;
    QHBoxLayout *btn_layout = new QHBoxLayout;
    renderer = new WidgetRenderer;
    renderer->setFocusPolicy(Qt::StrongFocus);
    renderer->resizeRenderer(640, 480);
    player = new AVPlayer(this);
    player->setRenderer(renderer);
    player->masterClock()->setClockAuto(false);
    player->masterClock()->setClockType(AVClock::ExternalClock);
    connect(player, SIGNAL(started()), SLOT(slotStarted()));
    connect(player->masterClock(), SIGNAL(paused(bool)), clock, SLOT(pause(bool)));
    connect(player->masterClock(), SIGNAL(started()), clock, SLOT(start()));
    connect(player->masterClock(), SIGNAL(resetted()), clock, SLOT(reset()));

    file_btn = new QPushButton(tr("Choose a video"), this);
    play_btn = new QPushButton(tr("Play"), this);
    pause_btn = new QPushButton(tr("Pause"), this);
    pause_btn->setCheckable(true);
    stop_btn = new QPushButton(tr("Stop"), this);
    connect(file_btn, SIGNAL(clicked()), SLOT(chooseVideo()));
    connect(play_btn, SIGNAL(clicked()), player, SLOT(play()));
    connect(pause_btn, SIGNAL(toggled(bool)), player, SLOT(pause(bool)));
    connect(stop_btn, SIGNAL(clicked()), player, SLOT(stop()));

    time_box = new QSpinBox(this);
    time_box->setMaximum(std::numeric_limits<int>::max());
    time_box->setMinimum(0);
    send_btn = new QPushButton(tr("Send"), this);
    connect(send_btn, SIGNAL(clicked()), SLOT(sendClock()));

    slider = new QSlider(this);
    slider->setMinimum(0);
    connect(slider, SIGNAL(valueChanged(int)), SLOT(seekVideo(int)));

    slider->setOrientation(Qt::Horizontal);
    btn_layout->addWidget(file_btn);
    btn_layout->addWidget(play_btn);
    btn_layout->addWidget(pause_btn);
    btn_layout->addWidget(stop_btn);
    btn_layout->addWidget(time_box);
    btn_layout->addWidget(send_btn);
    main_layout->addWidget(renderer);
    main_layout->addLayout(btn_layout);
    main_layout->addWidget(slider);
    setLayout(main_layout);
    resize(720, 600);
    startTimer(1000);
}

Widget::~Widget()
{
}

void Widget::chooseVideo()
{
    QString v = QFileDialog::getOpenFileName(0, "Select a video");
    if (v.isEmpty())
        return;
    vfile = v;
    player->setFile(vfile);
}

void Widget::sendClock()
{
    player->masterClock()->updateExternalClock(qint64(clock->value()*1000.0) + 1100LL);
    clock->updateExternalClock(qint64(clock->value()*1000.0) + 1100LL);
}

void Widget::seekVideo(int v)
{
    qreal pos = qreal(v)/qreal(slider->maximum());
    player->seek(pos);
    clock->updateValue(player->masterClock()->value());
    clock->updateExternalClock(player->masterClock()->value() * 1000LL); //in msec. ignore usec part using t/1000
}

void Widget::slotStarted()
{
    //slider->setMaximum();
}

void Widget::timerEvent(QTimerEvent *)
{
    if (!clock->isActive())
        return;
    qDebug("timerEvent %d", int(clock->value() * 1000.0f));
    time_box->setValue(int(clock->value() * 1000.0f));
    //slider->setValue(int(clock->value() * 1000.0f));
}
