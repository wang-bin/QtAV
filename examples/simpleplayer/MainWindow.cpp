#include "MainWindow.h"
#include <QLabel>
#include <QSlider>
#include <QGraphicsOpacityEffect>
#include <QResizeEvent>
#include <QtAV/AVPlayer.h>
#include <QtAV/VideoRendererTypes.h>
#include <QtAV/WidgetRenderer.h>
#include <QLayout>

//TODO: custome slider, mouse pressevent -> <-

#define AVDEBUG() \
    qDebug("%s %s @%d", __FILE__, __FUNCTION__, __LINE__);

const int kSliderUpdateInterval = 500;
using namespace QtAV;

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent)
  , mTimerId(0)
  , mpRenderer(0)
{

    QHBoxLayout *hlayout = new QHBoxLayout(this);
    hlayout->setMargin(0);
    setLayout(hlayout);

    mpTimeSlider = new QSlider(this);
    mpTimeSlider->setTracking(true);
    mpTimeSlider->setMaximumHeight(22);
    mpTimeSlider->setOrientation(Qt::Horizontal);
    mpTimeSlider->setMinimum(0);
    QGraphicsOpacityEffect *oe = new QGraphicsOpacityEffect(this);
    oe->setOpacity(0.5);
    mpTimeSlider->setGraphicsEffect(oe);

    mpPlayer = new AVPlayer(this);

    connect(mpPlayer, SIGNAL(started()), this, SLOT(onStartPlay()));
    connect(mpPlayer, SIGNAL(paused(bool)), this, SLOT(onPaused(bool)));
    //valueChanged can be triggered by non-mouse event
    connect(mpTimeSlider, SIGNAL(sliderMoved(int)), this, SLOT(seekToMSec(int)));
    connect(mpTimeSlider, SIGNAL(sliderPressed()), SLOT(seek()));
    connect(mpTimeSlider, SIGNAL(sliderReleased()), SLOT(seek()));
}

void MainWindow::setRenderer(QtAV::VideoRenderer *renderer)
{
    if (!renderer)
        return;
    int old_pos = 0;
    int old_total = 0;
    if (mpTimeSlider) {
        old_pos = mpTimeSlider->value();
        old_total = mpTimeSlider->maximum();
        mpTimeSlider->hide();
    } else {
    }
    QWidget *r = 0;
    if (mpRenderer)
        r = mpRenderer->widget();
    if (r) {
        layout()->removeWidget(r);
        r->close();
        delete r;
        r = 0;
    }
    mpRenderer = renderer;
    mpPlayer->setRenderer(mpRenderer);
    if (mpTimeSlider) {
        mpTimeSlider->setParent(mpRenderer->widget());
        mpTimeSlider->show();
    }
    layout()->addWidget(mpRenderer->widget());
    resize(mpRenderer->widget()->size());
}

void MainWindow::play(const QString &name)
{
    mFile = name;
    mpPlayer->play(name);
}

void MainWindow::onPaused(bool p)
{
    if (p) {
        if (mTimerId)
            killTimer(mTimerId);
    } else {
        mTimerId = startTimer(kSliderUpdateInterval);
    }
}

void MainWindow::onStartPlay()
{
    mpTimeSlider->setMaximum(mpPlayer->duration()*1000);
    mpTimeSlider->setValue(0);
    mTimerId = startTimer(kSliderUpdateInterval);
}

void MainWindow::seekToMSec(int msec)
{
    mpPlayer->seek(qreal(msec)/qreal(mpTimeSlider->maximum()));
}

void MainWindow::seek()
{
    mpPlayer->seek(qreal(mpTimeSlider->value())/qreal(mpTimeSlider->maximum()));
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    int m = 4;
    QWidget *w = static_cast<QWidget*>(mpTimeSlider->parent());
    if (w) {
        mpTimeSlider->resize(w->width() - m*2, 44);
        qDebug("%d %d %d", m, w->height() - mpTimeSlider->height() - m, w->width() - m*2);
        mpTimeSlider->move(m, w->height() - mpTimeSlider->height() - m);
    }
}

void MainWindow::timerEvent(QTimerEvent *)
{
    mpTimeSlider->setValue(int(mpPlayer->masterClock()->value()*1000.0));
}
