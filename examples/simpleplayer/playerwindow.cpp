/******************************************************************************
    Simple Player:  this file is part of QtAV examples
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#include "playerwindow.h"
#include <QPushButton>
#include <QSlider>
#include <QLayout>
#include <QMessageBox>
#include <QFileDialog>

using namespace QtAV;

PlayerWindow::PlayerWindow(QWidget *parent) : QWidget(parent)
{
    m_unit = 1000;
    setWindowTitle(QString::fromLatin1("QtAV simple player example"));
    m_player = new AVPlayer(this);
    QVBoxLayout *vl = new QVBoxLayout();
    setLayout(vl);
    m_vo = new VideoOutput(this);
    if (!m_vo->widget()) {
        QMessageBox::warning(0, QString::fromLatin1("QtAV error"), tr("Can not create video renderer"));
        return;
    }
    m_player->setRenderer(m_vo);
    vl->addWidget(m_vo->widget());
    m_slider = new QSlider();
    m_slider->setOrientation(Qt::Horizontal);
    connect(m_slider, SIGNAL(sliderMoved(int)), SLOT(seekBySlider(int)));
    connect(m_slider, SIGNAL(sliderPressed()), SLOT(seekBySlider()));
    connect(m_player, SIGNAL(positionChanged(qint64)), SLOT(updateSlider(qint64)));
    connect(m_player, SIGNAL(started()), SLOT(updateSlider()));
    connect(m_player, SIGNAL(notifyIntervalChanged()), SLOT(updateSliderUnit()));

    vl->addWidget(m_slider);
    QHBoxLayout *hb = new QHBoxLayout();
    vl->addLayout(hb);
    m_openBtn = new QPushButton(tr("Open"));
    m_playBtn = new QPushButton(tr("Play/Pause"));
    m_stopBtn = new QPushButton(tr("Stop"));
    hb->addWidget(m_openBtn);
    hb->addWidget(m_playBtn);
    hb->addWidget(m_stopBtn);
    connect(m_openBtn, SIGNAL(clicked()), SLOT(openMedia()));
    connect(m_playBtn, SIGNAL(clicked()), SLOT(playPause()));
    connect(m_stopBtn, SIGNAL(clicked()), m_player, SLOT(stop()));
}

void PlayerWindow::openMedia()
{
    QString file = QFileDialog::getOpenFileName(0, tr("Open a video"));
    if (file.isEmpty())
        return;
    m_player->play(file);
}

void PlayerWindow::seekBySlider(int value)
{
    if (!m_player->isPlaying())
        return;
    m_player->seek(qint64(value*m_unit));
}

void PlayerWindow::seekBySlider()
{
    seekBySlider(m_slider->value());
}

void PlayerWindow::playPause()
{
    if (!m_player->isPlaying()) {
        m_player->play();
        return;
    }
    m_player->pause(!m_player->isPaused());
}

void PlayerWindow::updateSlider(qint64 value)
{
    m_slider->setRange(0, int(m_player->duration()/m_unit));
    m_slider->setValue(int(value/m_unit));
}

void PlayerWindow::updateSlider()
{
    updateSlider(m_player->position());
}

void PlayerWindow::updateSliderUnit()
{
    m_unit = m_player->notifyInterval();
    updateSlider();
}
