/******************************************************************************
    VideoGroup:  this file is part of QtAV examples
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

#include "videogroup.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QtCore/QUrl>
#include <QtAV/AudioOutput.h>
#include <QtAV/OSDFilter.h>
#include <QtAV/VideoRendererTypes.h>

using namespace QtAV;

VideoGroup::VideoGroup(QObject *parent) :
    QObject(parent)
  , mSingle(false)
  , r(3),c(3),view(0)
  , vid("qpainter")
{
    mpPlayer = new AVPlayer(this);
    mpPlayer->setPlayerEventFilter(0);

    mpBar = new QWidget(0, Qt::WindowStaysOnTopHint);
    mpBar->setMaximumSize(400, 60);
    mpBar->show();
    mpBar->setLayout(new QHBoxLayout);
    mpOpen = new QPushButton("Open");
    mpPlay = new QPushButton("Play");
    mpStop = new QPushButton("Stop");
    mpPause = new QPushButton("Pause");
    mpPause->setCheckable(true);
    mpAdd = new QPushButton("+");
    mpRemove = new QPushButton("-");
    mpSingle = new QPushButton("Single");
    mpSingle->setCheckable(mpSingle);
    connect(mpOpen, SIGNAL(clicked()), SLOT(openLocalFile()));
    connect(mpPlay, SIGNAL(clicked()), mpPlayer, SLOT(play()));
    connect(mpStop, SIGNAL(clicked()), mpPlayer, SLOT(stop()));
    connect(mpPause, SIGNAL(toggled(bool)), mpPlayer, SLOT(pause(bool)));
    connect(mpAdd, SIGNAL(clicked()), SLOT(addRenderer()));
    connect(mpRemove, SIGNAL(clicked()), SLOT(removeRenderer()));
    connect(mpSingle, SIGNAL(toggled(bool)), SLOT(setSingleWindow(bool)));

    mpBar->layout()->addWidget(mpOpen);
    mpBar->layout()->addWidget(mpPlay);
    mpBar->layout()->addWidget(mpStop);
    mpBar->layout()->addWidget(mpPause);
    mpBar->layout()->addWidget(mpAdd);
    mpBar->layout()->addWidget(mpRemove);
    mpBar->layout()->addWidget(mpSingle);
    mpBar->resize(200, 25);
}

VideoGroup::~VideoGroup()
{
    delete view;
    delete mpBar;
}

void VideoGroup::setSingleWindow(bool s)
{
    mSingle = s;
    if (!s) {
        if (!view)
            return;
        foreach(VideoRenderer *vo, mRenderers) {
            view->layout()->removeWidget(vo->widget());
            vo->widget()->setParent(0);
            vo->widget()->show();
        }
        delete view;
        view = 0;
    } else {
        if (view)
            return;
        view = new QWidget;
        view->resize(qApp->desktop()->size());
        QGridLayout *layout = new QGridLayout;
        layout->setSizeConstraint(QLayout::SetMaximumSize);
        layout->setSpacing(1);
        layout->setMargin(0);
        layout->setContentsMargins(0, 0, 0, 0);
        view->setLayout(layout);

        for (int i = 0; i < mRenderers.size(); ++i) {
            int x = i/cols();
            int y = i%cols();
            ((QGridLayout*)view->layout())->addWidget(mRenderers.at(i)->widget(), x, y);
        }
        view->show();
        mRenderers.last()->widget()->showFullScreen();
    }

}

void VideoGroup::setVideoRendererTypeString(const QString &vt)
{
    vid = vt.toLower();
}

void VideoGroup::setRows(int n)
{
    r = n;
}

void VideoGroup::setCols(int n)
{
    c = n;
}

int VideoGroup::rows() const
{
    return r;
}

int VideoGroup::cols() const
{
    return c;
}

void VideoGroup::show()
{
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < c; ++j) {
            addRenderer();
        }
    }
}

void VideoGroup::play(const QString &file)
{
    mpPlayer->play(file);
}

void VideoGroup::openLocalFile()
{
    QString file = QFileDialog::getOpenFileName(0, tr("Open a video"));
    if (file.isEmpty())
        return;
    mpPlayer->stop();
    mpPlayer->play(file);
}

void VideoGroup::addRenderer()
{
    VideoRendererId v = VideoRendererId_Widget;
    if (vid == "gl")
        v = VideoRendererId_GLWidget;
    else if (vid == "d2d")
        v = VideoRendererId_Direct2D;
    else if (vid == "gdi")
        v = VideoRendererId_GDI;
    else if (vid == "xv")
        v = VideoRendererId_XV;
    VideoRenderer* renderer = VideoRendererFactory::create(v);
    mRenderers.append(renderer);
    //renderer->widget()->setAttribute(Qt::WA_DeleteOnClose);
    int w = view ? view->frameGeometry().width()/c : qApp->desktop()->width()/c;
    int h = view ? view->frameGeometry().height()/r : qApp->desktop()->height()/r;
    renderer->widget()->resize(w, h);
    if (renderer->osdFilter()) {
        renderer->osdFilter()->setShowType(OSD::ShowNone);
    }
    mpPlayer->addVideoRenderer(renderer);
    int i = (mRenderers.size()-1)/cols();
    int j = (mRenderers.size()-1)%cols();
    if (view) {
        view->resize(qApp->desktop()->size());
        ((QGridLayout*)view->layout())->addWidget(renderer->widget(), i, j);
        view->show();
    } else {
        renderer->widget()->move(j*w, i*h);
        renderer->widget()->show();
    }
}

void VideoGroup::removeRenderer()
{
    if (mRenderers.isEmpty())
        return;
    VideoRenderer *r = mRenderers.takeLast();
    mpPlayer->removeVideoRenderer(r);
    if (view) {
        view->layout()->removeWidget(r->widget());
    }
    delete r;
}
