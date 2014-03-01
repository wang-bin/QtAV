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

#include "videoplayer.h"
#include <QGraphicsView>

#ifndef QT_NO_OPENGL
#include <QtOpenGL/QGLWidget>
#endif

#include <QSlider>
#include <QLayout>
#include <QPushButton>
#include <QFileDialog>

using namespace QtAV;

VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent)
    , videoItem(0)
{
    videoItem = new GraphicsItemRenderer;
    videoItem->resizeRenderer(640, 360);
    videoItem->setOutAspectRatioMode(VideoRenderer::RendererAspectRatio);

    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->addItem(videoItem);

    QGraphicsView *graphicsView = new QGraphicsView(scene);
#if 0
#ifndef QT_NO_OPENGL
    QGLWidget *glw = new QGLWidget(QGLFormat(QGL::SampleBuffers));
    glw->setAutoFillBackground(false);
    graphicsView->setCacheMode(QGraphicsView::CacheNone);
    graphicsView->setViewport(glw);
#endif
#endif //0
    QSlider *rotateSlider = new QSlider(Qt::Horizontal);
    rotateSlider->setRange(-180,  180);
    rotateSlider->setValue(0);

    connect(rotateSlider, SIGNAL(valueChanged(int)),
            this, SLOT(rotateVideo(int)));
    QPushButton *openBtn = new QPushButton;
    openBtn->setText("Open");
    connect(openBtn, SIGNAL(clicked()), SLOT(open()));

    QBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(graphicsView);
    layout->addWidget(rotateSlider);
    layout->addWidget(openBtn);
    setLayout(layout);

    mediaPlayer.setRenderer(videoItem);
}

VideoPlayer::~VideoPlayer()
{
}

void VideoPlayer::play(const QString &file)
{
    mediaPlayer.play(file);
}

void VideoPlayer::rotateVideo(int angle)
{
    //rotate around the center of video element
    qreal x = videoItem->boundingRect().width() / 2.0;
    qreal y = videoItem->boundingRect().height() / 2.0;
    videoItem->setTransform(QTransform().translate(x, y).rotate(angle).translate(-x, -y));
}

void VideoPlayer::open()
{
    QString f = QFileDialog::getOpenFileName(0, "Open a video");
    if (f.isEmpty())
        return;
    play(f);
}
