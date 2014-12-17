/******************************************************************************
    Simple Player:  this file is part of QtAV examples
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

#include "playerwindow.h"
#include <QPushButton>
#include <QSlider>
#include <QLayout>
#include <QMessageBox>
#include <QFileDialog>

using namespace QtAV;

PlayerWindow::PlayerWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("QtAV simple player example");
    m_player = new AVPlayer(this);
    QVBoxLayout *vl = new QVBoxLayout();
    setLayout(vl);
    const VideoRendererId vo_ids[] = {
        VideoRendererId_OpenGLWidget, // Qt >= 5.4
        VideoRendererId_GLWidget2,
        VideoRendererId_GLWidget,
        VideoRendererId_Widget,
        0
    };
    for (int i = 0; vo_ids[i]; ++i) {
        m_vo = new VideoOutput(vo_ids[i]);
        if (m_vo->widget())
            break;
    }
    if (!m_vo->widget()) {
        QMessageBox::warning(0, "QtAV error", "Can not create video renderer");
        return;
    }
    m_player->setRenderer(m_vo);
    vl->addWidget(m_vo->widget());
    m_slider = new QSlider();
    m_slider->setOrientation(Qt::Horizontal);
    connect(m_slider, SIGNAL(sliderMoved(int)), SLOT(seek(int)));
    connect(m_player, SIGNAL(positionChanged(qint64)), SLOT(updateSlider()));
    connect(m_player, SIGNAL(started()), SLOT(updateSlider()));

    vl->addWidget(m_slider);
    QHBoxLayout *hb = new QHBoxLayout();
    vl->addLayout(hb);
    m_openBtn = new QPushButton("Open");
    m_playBtn = new QPushButton("Play/Pause");
    m_stopBtn = new QPushButton("Stop");
    hb->addWidget(m_openBtn);
    hb->addWidget(m_playBtn);
    hb->addWidget(m_stopBtn);
    connect(m_openBtn, SIGNAL(clicked()), SLOT(openMedia()));
    connect(m_playBtn, SIGNAL(clicked()), SLOT(playPause()));
    connect(m_stopBtn, SIGNAL(clicked()), m_player, SLOT(stop()));
}

PlayerWindow::~PlayerWindow()
{

}

void PlayerWindow::openMedia()
{
    QString file = QFileDialog::getOpenFileName(0, "Open a video");
    if (file.isEmpty())
        return;
    m_player->play(file);
}

void PlayerWindow::seek(int pos)
{
    if (!m_player->isPlaying())
        return;
    m_player->seek(pos*1000LL); // to msecs
}

void PlayerWindow::playPause()
{
    if (!m_player->isPlaying()) {
        m_player->play();
        return;
    }
    m_player->pause(!m_player->isPaused());
}

void PlayerWindow::updateSlider()
{
    m_slider->setRange(0, int(m_player->duration()/1000LL));
    m_slider->setValue(int(m_player->position()/1000LL));
}
