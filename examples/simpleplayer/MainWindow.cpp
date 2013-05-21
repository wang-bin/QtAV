#include "MainWindow.h"
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QResizeEvent>
#include <QtAV/AVPlayer.h>
#include <QtAV/VideoRendererTypes.h>
#include <QtAV/WidgetRenderer.h>
#include <QLayout>
#include <QPushButton>
#include <QFileDialog>
#include "Button.h"
#include "Slider.h"

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
    mainLayout->setSpacing(0);
    mainLayout->setMargin(0);
    setLayout(mainLayout);
    layout()->setMargin(0);

    mpPlayerLayout = new QVBoxLayout(this);
    mpTimeSlider = new Slider(this);
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
    mPlayPixmap = QPixmap(":/theme/button-play-pause.png");
    int w = mPlayPixmap.width(), h = mPlayPixmap.height();
    mPausePixmap = mPlayPixmap.copy(QRect(w/2, 0, w/2, h));
    mPlayPixmap = mPlayPixmap.copy(QRect(0, 0, w/2, h));
    qDebug("%d x %d", mPlayPixmap.width(), mPlayPixmap.height());
    mpPlayPauseBtn = new Button(this);
    mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    mpStopBtn = new Button(this);
    mpStopBtn->setIconWithSates(QPixmap(":/theme/button-stop.png"));
    mpBackwardBtn = new Button(this);
    mpBackwardBtn->setIconWithSates(QPixmap(":/theme/button-rewind.png"));
    mpForwardBtn = new Button(this);
    mpForwardBtn->setIconWithSates(QPixmap(":/theme/button-fastforward.png"));
    mpOpenBtn = new Button(this);
    mpOpenBtn->setIconWithSates(QPixmap(":/theme/open.png"));

    mainLayout->addLayout(mpPlayerLayout);
    mainLayout->addWidget(mpTimeSlider);
    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->setSpacing(0);
    buttonLayout->setMargin(0);
    mainLayout->addLayout(buttonLayout);
    QSpacerItem *space = new QSpacerItem(mpPlayPauseBtn->width(), mpPlayPauseBtn->height(), QSizePolicy::MinimumExpanding);
    buttonLayout->addSpacerItem(space);
    buttonLayout->addWidget(mpPlayPauseBtn);
    buttonLayout->addWidget(mpStopBtn);
    buttonLayout->addWidget(mpBackwardBtn);
    buttonLayout->addWidget(mpForwardBtn);
    buttonLayout->addWidget(mpOpenBtn);

    connect(mpOpenBtn, SIGNAL(clicked()), SLOT(openFile()));
    connect(mpPlayPauseBtn, SIGNAL(clicked()), SLOT(togglePlayPause()));
    connect(mpStopBtn, SIGNAL(clicked()), mpPlayer, SLOT(stop()));
    connect(mpForwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekForward()));
    connect(mpBackwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekBackward()));

    connect(mpPlayer, SIGNAL(started()), this, SLOT(onStartPlay()));
    connect(mpPlayer, SIGNAL(stopped()), this, SLOT(onStopPlay()));
    connect(mpPlayer, SIGNAL(paused(bool)), this, SLOT(onPaused(bool)));
    //valueChanged can be triggered by non-mouse event
    //TODO: connect sliderMoved(int) to preview(int)
    //connect(mpTimeSlider, SIGNAL(sliderMoved(int)), this, SLOT(seekToMSec(int)));
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

void MainWindow::togglePlayPause()
{
    qDebug("%s", __FUNCTION__);
    if (mpPlayer->isPlaying()) {
        qDebug("isPaused = %d", mpPlayer->isPaused());
        mpPlayer->pause(!mpPlayer->isPaused());
    } else {
        if (mFile.isEmpty())
            return;
        mpPlayer->play();
        mpPlayPauseBtn->setIconWithSates(mPausePixmap);
    }
}

void MainWindow::onPaused(bool p)
{
    if (p) {
        qDebug("start pausing...");
        if (mTimerId)
            killTimer(mTimerId);
        mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    } else {
        qDebug("stop pausing...");
        mTimerId = startTimer(kSliderUpdateInterval);
        mpPlayPauseBtn->setIconWithSates(mPausePixmap);
    }
}

void MainWindow::onStartPlay()
{
    mpPlayPauseBtn->setIconWithSates(mPausePixmap);
    mpTimeSlider->setMaximum(mpPlayer->duration()*1000);
    mpTimeSlider->setValue(0);
    qDebug(">>>>>>>>>>>>>>enable slider");
    mpTimeSlider->setEnabled(true);
    mTimerId = startTimer(kSliderUpdateInterval);
}

void MainWindow::onStopPlay()
{
    mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    mpTimeSlider->setValue(0);
    qDebug(">>>>>>>>>>>>>>disable slider");
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
    Q_UNUSED(e);
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
