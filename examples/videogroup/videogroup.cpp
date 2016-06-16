/******************************************************************************
    VideoGroup:  this file is part of QtAV examples
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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
#include <QtAVWidgets>

using namespace QtAV;

VideoGroup::VideoGroup(QObject *parent) :
    QObject(parent)
  , m1Window(false)
  , m1Frame(true)
  , mFrameless(false)
  , r(3),c(3),view(0)
  , vid(QString::fromLatin1("qpainter"))
{
    mpPlayer = new AVPlayer(this);
    //mpPlayer->setPlayerEventFilter(0);

    mpBar = new QWidget(0, Qt::WindowStaysOnTopHint);
    mpBar->setMaximumSize(400, 60);
    mpBar->show();
    mpBar->setLayout(new QHBoxLayout);
    mpOpen = new QPushButton(tr("Open"));
    mpPlay = new QPushButton(tr("Play"));
    mpStop = new QPushButton(tr("Stop"));
    mpPause = new QPushButton(tr("Pause"));
    mpPause->setCheckable(true);
    mpAdd = new QPushButton(QString::fromLatin1("+"));
    mpRemove = new QPushButton(QString::fromLatin1("-"));
    mp1Window = new QPushButton(tr("Single Window"));
    mp1Window->setCheckable(true);
    mp1Frame = new QPushButton(tr("Single Frame"));
    mp1Frame->setCheckable(true);
    mp1Frame->setChecked(true);
    mpFrameless = new QPushButton(tr("no window frame"));
    mpFrameless->setCheckable(true);
    connect(mpOpen, SIGNAL(clicked()), SLOT(openLocalFile()));
    connect(mpPlay, SIGNAL(clicked()), mpPlayer, SLOT(play()));
    connect(mpStop, SIGNAL(clicked()), mpPlayer, SLOT(stop()));
    connect(mpPause, SIGNAL(toggled(bool)), mpPlayer, SLOT(pause(bool)));
    connect(mpAdd, SIGNAL(clicked()), SLOT(addRenderer()));
    connect(mpRemove, SIGNAL(clicked()), SLOT(removeRenderer()));
    connect(mp1Window, SIGNAL(toggled(bool)), SLOT(setSingleWindow(bool)));
    connect(mp1Frame, SIGNAL(toggled(bool)), SLOT(toggleSingleFrame(bool)));
    connect(mpFrameless, SIGNAL(toggled(bool)), SLOT(toggleFrameless(bool)));

    mpBar->layout()->addWidget(mpOpen);
    mpBar->layout()->addWidget(mpPlay);
    mpBar->layout()->addWidget(mpStop);
    mpBar->layout()->addWidget(mpPause);
    mpBar->layout()->addWidget(mpAdd);
    mpBar->layout()->addWidget(mpRemove);
    mpBar->layout()->addWidget(mp1Window);
    mpBar->layout()->addWidget(mp1Frame);
    //mpBar->layout()->addWidget(mpFrameless);
    mpBar->resize(200, 25);
}

VideoGroup::~VideoGroup()
{
    delete view;
    delete mpBar;
}

void VideoGroup::setSingleWindow(bool s)
{
    m1Window = s;
    mRenderers = mpPlayer->videoOutputs();
    if (mRenderers.isEmpty())
        return;
    if (!s) {
        if (!view)
            return;
        foreach(VideoRenderer *vo, mRenderers) {
            view->layout()->removeWidget(vo->widget());
            vo->widget()->setParent(0);
            Qt::WindowFlags wf = vo->widget()->windowFlags();
            if (mFrameless) {
                wf &= ~Qt::FramelessWindowHint;
            } else {
                wf |= Qt::FramelessWindowHint;
            }
            vo->widget()->setWindowFlags(wf);
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

void VideoGroup::toggleSingleFrame(bool s)
{
    if (m1Frame == s)
        return;
    m1Frame = s;
    updateROI();
}

void VideoGroup::toggleFrameless(bool f)
{
    mFrameless = f;
    if (mRenderers.isEmpty())
        return;
    VideoRenderer *renderer = mRenderers.first();
    Qt::WindowFlags wf = renderer->widget()->windowFlags();
    if (f) {
        wf &= ~Qt::FramelessWindowHint;
    } else {
        wf |= Qt::FramelessWindowHint;
    }
    foreach (VideoRenderer *rd, mRenderers) {
        rd->widget()->setWindowFlags(wf);
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
    if (vid == QLatin1String("gl"))
        v = VideoRendererId_GLWidget2;
    else if (vid == QLatin1String("d2d"))
        v = VideoRendererId_Direct2D;
    else if (vid == QLatin1String("gdi"))
        v = VideoRendererId_GDI;
    else if (vid == QLatin1String("xv"))
        v = VideoRendererId_XV;
    VideoRenderer* renderer = VideoRenderer::create(v);
    mRenderers = mpPlayer->videoOutputs();
    mRenderers.append(renderer);
    renderer->widget()->setAttribute(Qt::WA_DeleteOnClose);
    Qt::WindowFlags wf = renderer->widget()->windowFlags();
    if (mFrameless) {
        wf &= ~Qt::FramelessWindowHint;
    } else {
        wf |= Qt::FramelessWindowHint;
    }
    renderer->widget()->setWindowFlags(wf);
    int w = view ? view->frameGeometry().width()/c : qApp->desktop()->width()/c;
    int h = view ? view->frameGeometry().height()/r : qApp->desktop()->height()/r;
    renderer->widget()->resize(w, h);
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
    updateROI();
}

void VideoGroup::removeRenderer()
{
    mRenderers = mpPlayer->videoOutputs();
    if (mRenderers.isEmpty())
        return;
    VideoRenderer *r = mRenderers.takeLast();
    mpPlayer->removeVideoRenderer(r);
    if (view) {
        view->layout()->removeWidget(r->widget());
    }
    delete r;
    updateROI();
}

void VideoGroup::updateROI()
{
    if (mRenderers.isEmpty())
        return;
    if (!m1Frame) {
        foreach (VideoRenderer *renderer, mRenderers) {
            renderer->setRegionOfInterest(0, 0, 0, 0);
        }
        return;
    }
    int W = view ? view->frameGeometry().width() : qApp->desktop()->width();
    int H = view ? view->frameGeometry().height() : qApp->desktop()->height();
    int w = W / c;
    int h = H / r;
    for (int i = 0; i < mRenderers.size(); ++i) {
        VideoRenderer *renderer = mRenderers.at(i);
        renderer->setRegionOfInterest(qreal((i%c)*w)/qreal(W), qreal((i/c)*h)/qreal(H), qreal(w)/qreal(W), qreal(h)/qreal(H));
    }
}
