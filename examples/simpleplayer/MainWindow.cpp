#include "MainWindow.h"
#include <QLabel>
#include <QSlider>
#include <QGraphicsOpacityEffect>
#include <QResizeEvent>
#include <QtAV/AVPlayer.h>
#include <QtAV/VideoRendererTypes.h>
#include <QtAV/WidgetRenderer.h>
#include <QLayout>
#include <QPushButton>
#include <QFileDialog>

#define SLIDER_ON_VO 0
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
    mpPlayer = new AVPlayer(this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    layout()->setMargin(0);

    mpPlayerLayout = new QVBoxLayout(this);
    mpTimeSlider = new QSlider(this);
    mpTimeSlider->setDisabled(true);
    //mpTimeSlider->setFixedHeight(8);
    mpTimeSlider->setMaximumHeight(8);
    mpTimeSlider->setTracking(true);
    mpTimeSlider->setOrientation(Qt::Horizontal);
    mpTimeSlider->setMinimum(0);
#if SLIDER_ON_VO
    QGraphicsOpacityEffect *oe = new QGraphicsOpacityEffect(this);
    oe->setOpacity(0.5);
    mpTimeSlider->setGraphicsEffect(oe);
#endif //SLIDER_ON_VO
    mpPlayStopBtn = new QPushButton(tr("Play"), this);
    mpPauseBtn = new QPushButton(tr("Pause"), this);
    mpBackwardBtn = new QPushButton("<<", this);
    mpForwardBtn = new QPushButton(">>", this);
    mpOpenBtn = new QPushButton(tr("Open"), this);

    mainLayout->addLayout(mpPlayerLayout);
    mainLayout->addWidget(mpTimeSlider);
    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    mainLayout->addLayout(buttonLayout);
    QSpacerItem *space = new QSpacerItem(mpPlayStopBtn->width(), mpPlayStopBtn->height(), QSizePolicy::MinimumExpanding);
    buttonLayout->addSpacerItem(space);
    buttonLayout->addWidget(mpPlayStopBtn);
    buttonLayout->addWidget(mpPauseBtn);
    buttonLayout->addWidget(mpBackwardBtn);
    buttonLayout->addWidget(mpForwardBtn);
    buttonLayout->addWidget(mpOpenBtn);

    connect(mpOpenBtn, SIGNAL(clicked()), SLOT(openFile()));
    connect(mpPlayStopBtn, SIGNAL(clicked()), SLOT(togglePlayStop()));
    connect(mpPauseBtn, SIGNAL(clicked()), mpPlayer, SLOT(togglePause()));
    connect(mpForwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekForward()));
    connect(mpBackwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekBackward()));

    connect(mpPlayer, SIGNAL(started()), this, SLOT(onStartPlay()));
    connect(mpPlayer, SIGNAL(stopped()), this, SLOT(onStopPlay()));
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
#if SLIDER_ON_VO
    int old_pos = 0;
    int old_total = 0;

    if (mpTimeSlider) {
        old_pos = mpTimeSlider->value();
        old_total = mpTimeSlider->maximum();
        mpTimeSlider->hide();
    } else {
    }
#endif //SLIDER_ON_VO
    QWidget *r = 0;
    if (mpRenderer)
        r = mpRenderer->widget();
    if (r) {
        mpPlayerLayout->removeWidget(r);
        r->close();
        delete r;
        r = 0;
    }
    mpRenderer = renderer;
    mpPlayer->setRenderer(mpRenderer);
#if SLIDER_ON_VO
    if (mpTimeSlider) {
        mpTimeSlider->setParent(mpRenderer->widget());
        mpTimeSlider->show();
    }
#endif //SLIDER_ON_VO
    mpPlayerLayout->addWidget(mpRenderer->widget());
    resize(mpRenderer->widget()->size());
}

void MainWindow::play(const QString &name)
{
    mFile = name;
    mpPlayer->play(name);
}

void MainWindow::openFile()
{
    QString file = QFileDialog::getOpenFileName(0, tr("Open a media file"));
    if (file.isEmpty())
        return;
    mFile = file;
    setWindowTitle(mFile);
    mpPlayer->play(mFile);
}

void MainWindow::togglePlayStop()
{
    if (mpPlayer->isPlaying())
        mpPlayer->stop();
    else
        mpPlayer->play();
}

void MainWindow::onPaused(bool p)
{
    if (p) {
        if (mTimerId)
            killTimer(mTimerId);
        mpPauseBtn->setText(tr("Resume"));
    } else {
        mTimerId = startTimer(kSliderUpdateInterval);
        mpPauseBtn->setText(tr("Pause"));
    }
}

void MainWindow::onStartPlay()
{
    mpTimeSlider->setEnabled(true);
    mpTimeSlider->setMaximum(mpPlayer->duration()*1000);
    mpTimeSlider->setValue(0);
    mTimerId = startTimer(kSliderUpdateInterval);
    mpPlayStopBtn->setText(tr("Stop"));
}

void MainWindow::onStopPlay()
{
    mpPlayStopBtn->setText(tr("Play"));
    mpTimeSlider->setValue(0);
    mpTimeSlider->setDisabled(true);
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
#if SLIDER_ON_VO
    int m = 4;
    QWidget *w = static_cast<QWidget*>(mpTimeSlider->parent());
    if (w) {
        mpTimeSlider->resize(w->width() - m*2, 44);
        qDebug("%d %d %d", m, w->height() - mpTimeSlider->height() - m, w->width() - m*2);
        mpTimeSlider->move(m, w->height() - mpTimeSlider->height() - m);
    }
#endif //SLIDER_ON_VO
}

void MainWindow::timerEvent(QTimerEvent *)
{
    mpTimeSlider->setValue(int(mpPlayer->masterClock()->value()*1000.0));
}
