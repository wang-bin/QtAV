#include "MainWindow.h"
#include <QtCore/QTimer>
#include <QTimeEdit>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QGraphicsOpacityEffect>
#include <QResizeEvent>
#include <QWindowStateChangeEvent>
#include <QtAV/AVPlayer.h>
#include <QtAV/OSDFilter.h>
#include <QtAV/VideoRendererTypes.h>
#include <QtAV/WidgetRenderer.h>
#include <QLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include "Button.h"
#include "Slider.h"

#define SLIDER_ON_VO 0
//TODO: custome slider, mouse pressevent -> <-

#define AVDEBUG() \
    qDebug("%s %s @%d", __FILE__, __FUNCTION__, __LINE__);

const int kSliderUpdateInterval = 500;
using namespace QtAV;

static void QLabelSetElideText(QLabel *label, QString text, int W = 0)
{
    QFontMetrics metrix(label->font());
    int width = label->width() - label->indent() - label->margin();
    if (W || label->parent()) {
        int w = W;
        if (!w)
            w = ((QWidget*)label->parent())->width();
        width = qMax(w - label->indent() - label->margin() - 13*(30), 0); //TODO: why 30?
    }
    QString clippedText = metrix.elidedText(text, Qt::ElideRight, width);
    label->setText(clippedText);
}

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent)
  , mIsReady(false)
  , mHasPendingPlay(false)
  , mTimerId(0)
  , mpRenderer(0)
  , mpTempRenderer(0)
{
    connect(this, SIGNAL(ready()), SLOT(processPendingActions()));
    //QTimer::singleShot(10, this, SLOT(setupUi()));
    setupUi();
}

void MainWindow::initPlayer()
{
    mpPlayer = new AVPlayer(this);
    mIsReady = true;
    qDebug("player created");
    connect(mpStopBtn, SIGNAL(clicked()), mpPlayer, SLOT(stop()));
    connect(mpForwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekForward()));
    connect(mpBackwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekBackward()));
    connect(mpPlayer, SIGNAL(started()), this, SLOT(onStartPlay()));
    connect(mpPlayer, SIGNAL(stopped()), this, SLOT(onStopPlay()));
    connect(mpPlayer, SIGNAL(paused(bool)), this, SLOT(onPaused(bool)));
    emit ready(); //emit this signal after connection. otherwise the slots may not be called for the first time
}

void MainWindow::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setMargin(0);
    setLayout(mainLayout);

    mpPlayerLayout = new QVBoxLayout();
    mpControl = new QWidget(this);
    mpControl->setMaximumHeight(22);

    mpTimeSlider = new Slider(mpControl);
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

    mpCurrent = new QLabel(mpControl);
    mpCurrent->setMargin(2);
    mpCurrent->setText("00:00");
    mpDuration = new QLabel(mpControl);
    mpDuration->setMargin(2);
    mpDuration->setText("00:00");
    mpTitle = new QLabel(mpControl);
    mpTitle->setText("QPainter");
    mpTitle->setIndent(8);

    mPlayPixmap = QPixmap(":/theme/button-play-pause.png");
    int w = mPlayPixmap.width(), h = mPlayPixmap.height();
    mPausePixmap = mPlayPixmap.copy(QRect(w/2, 0, w/2, h));
    mPlayPixmap = mPlayPixmap.copy(QRect(0, 0, w/2, h));
    qDebug("%d x %d", mPlayPixmap.width(), mPlayPixmap.height());
    mpPlayPauseBtn = new Button(mpControl);
    int a = qMin(w/2, h);
    const int kMaxButtonIconWidth = 18;
    const int kMaxButtonIconMargin = kMaxButtonIconWidth/3;
    a = qMin(a, kMaxButtonIconWidth);
    mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    mpPlayPauseBtn->setIconSize(QSize(a, a));
    mpPlayPauseBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpStopBtn = new Button(mpControl);
    mpStopBtn->setIconWithSates(QPixmap(":/theme/button-stop.png"));
    mpStopBtn->setIconSize(QSize(a, a));
    mpStopBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpBackwardBtn = new Button(mpControl);
    mpBackwardBtn->setIconWithSates(QPixmap(":/theme/button-rewind.png"));
    mpBackwardBtn->setIconSize(QSize(a, a));
    mpBackwardBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpForwardBtn = new Button(mpControl);
    mpForwardBtn->setIconWithSates(QPixmap(":/theme/button-fastforward.png"));
    mpForwardBtn->setIconSize(QSize(a, a));
    mpForwardBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpOpenBtn = new Button(mpControl);
    mpOpenBtn->setIconWithSates(QPixmap(":/theme/open_folder.png"));
    mpOpenBtn->setIconSize(QSize(a, a));
    mpOpenBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);

    mpInfoBtn = new Button();
    mpInfoBtn->setIconWithSates(QPixmap(":/theme/info.png"));
    mpInfoBtn->setIconSize(QSize(a, a));
    mpInfoBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpCaptureBtn = new Button();
    mpCaptureBtn->setIconWithSates(QPixmap(":/theme/screenshot.png"));
    mpCaptureBtn->setIconSize(QSize(a, a));
    mpCaptureBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpMenuBtn = new Button();
    mpMenuBtn->setAutoRaise(true);
    mpMenuBtn->setPopupMode(QToolButton::InstantPopup);

/*
    mpMenuBtn->setIconWithSates(QPixmap(":/theme/search-arrow.png"));
    mpMenuBtn->setIconSize(QSize(a, a));
    mpMenuBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
*/
    mpMenu = new QMenu(mpMenuBtn);
    mpMenu->addAction(tr("Open Url"), this, SLOT(openUrl()));
    QMenu *subMenu = new QMenu(tr("Online channels"));
    connect(subMenu, SIGNAL(triggered(QAction*)), SLOT(playOnlineVideo(QAction*)));
    QSettings tv(qApp->applicationDirPath() + "/tv.ini", QSettings::IniFormat);
    tv.setIniCodec("UTF-8");
    foreach (QString key, tv.allKeys()) {
        subMenu->addAction(key)->setData(tv.value(key).toString());
    }
    mpMenu->addMenu(subMenu);
    mpMenu->addSeparator();
    mpMenu->addAction(tr("Setup"), this, SLOT(setup()))->setEnabled(false);
    mpMenu->addAction(tr("Report"))->setEnabled(false); //report bug, suggestions etc. using maillist?
    mpMenu->addAction(tr("About"), this, SLOT(about()));
    mpMenu->addAction(tr("Help"), this, SLOT(help()))->setEnabled(false);
    mpMenu->addSeparator();
    mpMenu->addAction(tr("About Qt"), qApp, SLOT(aboutQt()));
    mpMenuBtn->setMenu(mpMenu);
    mpMenu->addSeparator();
    subMenu = new QMenu(tr("Aspect ratio"), mpMenu);
    mpMenu->addMenu(subMenu);
    connect(subMenu, SIGNAL(triggered(QAction*)), SLOT(switchAspectRatio(QAction*)));
    mpARAction = subMenu->addAction(tr("Video"));
    mpARAction->setData(0);
    subMenu->addAction(tr("Window"))->setData(-1);
    subMenu->addAction("4:3")->setData(4.0/3.0);
    subMenu->addAction("16:9")->setData(16.0/9.0);
    subMenu->addAction(tr("Custom"))->setData(-2);
    foreach(QAction* action, subMenu->actions()) {
        action->setCheckable(true);
    }
    mpARAction->setChecked(true);

    subMenu = new QMenu(tr("Renderer"));
    mpMenu->addMenu(subMenu);
    connect(subMenu, SIGNAL(triggered(QAction*)), SLOT(changeVO(QAction*)));
    //TODO: AVOutput.name,detail(description). check whether it is available
    mpVOAction = subMenu->addAction("QPainter");
    mpVOAction->setData(VideoRendererId_Widget);
    subMenu->addAction("OpenGL")->setData(VideoRendererId_GLWidget);
    subMenu->addAction("GDI+")->setData(VideoRendererId_GDI);
    subMenu->addAction("Direct2D")->setData(VideoRendererId_Direct2D);
    subMenu->addAction("XV")->setData(VideoRendererId_XV);
    mVOActions = subMenu->actions();
    foreach(QAction* action, subMenu->actions()) {
        action->setCheckable(true);
    }
    mpVOAction->setChecked(true);


    mainLayout->addLayout(mpPlayerLayout);
    mainLayout->addWidget(mpTimeSlider);
    mainLayout->addWidget(mpControl);

    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(0);
    controlLayout->setMargin(1);
    mpControl->setLayout(controlLayout);
    controlLayout->addWidget(mpCurrent);
    controlLayout->addWidget(mpTitle);
    QSpacerItem *space = new QSpacerItem(mpPlayPauseBtn->width(), mpPlayPauseBtn->height(), QSizePolicy::MinimumExpanding);
    controlLayout->addSpacerItem(space);
    controlLayout->addWidget(mpCaptureBtn);
    controlLayout->addWidget(mpPlayPauseBtn);
    controlLayout->addWidget(mpStopBtn);
    controlLayout->addWidget(mpBackwardBtn);
    controlLayout->addWidget(mpForwardBtn);
    controlLayout->addWidget(mpOpenBtn);
    controlLayout->addWidget(mpInfoBtn);
    //controlLayout->addWidget(mpSetupBtn);
    controlLayout->addWidget(mpMenuBtn);

    controlLayout->addWidget(mpDuration);

    connect(mpOpenBtn, SIGNAL(clicked()), SLOT(openFile()));
    connect(mpPlayPauseBtn, SIGNAL(clicked()), SLOT(togglePlayPause()));
    connect(mpCaptureBtn, SIGNAL(clicked()), this, SLOT(capture()));
    //valueChanged can be triggered by non-mouse event
    //TODO: connect sliderMoved(int) to preview(int)
    //connect(mpTimeSlider, SIGNAL(sliderMoved(int)), this, SLOT(seekToMSec(int)));
    connect(mpTimeSlider, SIGNAL(sliderPressed()), SLOT(seek()));
    connect(mpTimeSlider, SIGNAL(sliderReleased()), SLOT(seek()));

    QTimer::singleShot(0, this, SLOT(initPlayer()));
}

void MainWindow::changeVO(QAction *action)
{
    if (action == mpVOAction) {
        action->toggle(); //check state changes if clicked
        return;
    }
    VideoRendererId vid = (VideoRendererId)action->data().toInt();
    VideoRenderer *vo = VideoRendererFactory::create(vid);
    if (vo) {
        if (vo->osdFilter()) {
            vo->osdFilter()->setShowType(OSD::ShowNone);
        }
        if (vo->widget()) {
            vo->widget()->resize(rect().size()); //TODO: why not mpPlayer->renderer()->rendererSize()?
            vo->resizeRenderer(mpPlayer->renderer()->rendererSize());
        }
        setRenderer(vo);
    } else {
        action->toggle(); //check state changes if clicked
        return;
    }
}

void MainWindow::processPendingActions()
{
    if (!mpTempRenderer)
        return;
    setRenderer(mpTempRenderer);
    mpTempRenderer = 0;
    if (mHasPendingPlay) {
        mHasPendingPlay = false;
        play(mFile);
    }
}

void MainWindow::setRenderer(QtAV::VideoRenderer *renderer)
{
    if (!mIsReady) {
        mpTempRenderer = renderer;
        return;
    }
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
    mpRenderer = renderer;
    mpPlayer->setRenderer(mpRenderer);
    //release old renderer and add new
    if (r) {
        mpPlayerLayout->removeWidget(r);
        r->QWidget::close();
        //delete r; //pure virtual called
        r = 0;
    }
#if SLIDER_ON_VO
    if (mpTimeSlider) {
        mpTimeSlider->setParent(mpRenderer->widget());
        mpTimeSlider->show();
    }
#endif //SLIDER_ON_VO
    qDebug("add renderer to layout");
    mpPlayerLayout->addWidget(mpRenderer->widget());
    AVDEBUG();
    resize(mpRenderer->widget()->size());
    if (mpVOAction) {
        mpVOAction->setChecked(false);
    }
    foreach (QAction *action, mVOActions) {
        if (action->data() == renderer->id()) {
            mpVOAction = action;
            break;
        }
    }
    mpVOAction->setChecked(true);
    mpTitle->setText(mpVOAction->text());
}

void MainWindow::play(const QString &name)
{
    mFile = name;
    if (!mIsReady) {
        mHasPendingPlay = true;
        return;
    }
    //QLabelSetElideText(mpTitle, QFileInfo(mFile).fileName());
    if (mFile.contains("://") && !mFile.startsWith("file://"))
        setWindowTitle(mFile);
    else
        setWindowTitle(QFileInfo(mFile).fileName());

    mpPlayer->play(name);
}

void MainWindow::openFile()
{
    QString file = QFileDialog::getOpenFileName(0, tr("Open a media file"));
    if (file.isEmpty())
        return;
    play(file);
}

void MainWindow::togglePlayPause()
{
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
    mFile = mpPlayer->file(); //open from EventFilter's menu
    if (mFile.contains("://") && !mFile.startsWith("file://"))
        setWindowTitle(mFile);
    else
        setWindowTitle(QFileInfo(mFile).fileName());

    mpPlayPauseBtn->setIconWithSates(mPausePixmap);
    mpTimeSlider->setMaximum(mpPlayer->duration()*1000);
    mpTimeSlider->setValue(0);
    qDebug(">>>>>>>>>>>>>>enable slider");
    mpTimeSlider->setEnabled(true);
    mpDuration->setText(QTime(0, 0, 0).addSecs(mpPlayer->duration()).toString("mm:ss"));
    mTimerId = startTimer(kSliderUpdateInterval);
}

void MainWindow::onStopPlay()
{
    mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    mpTimeSlider->setValue(0);
    qDebug(">>>>>>>>>>>>>>disable slider");
    mpTimeSlider->setDisabled(true);
    mpCurrent->setText("00:00");
    mpDuration->setText("00:00");
}

void MainWindow::seekToMSec(int msec)
{
    mpPlayer->seek(qreal(msec)/qreal(mpTimeSlider->maximum()));
}

void MainWindow::seek()
{
    mpPlayer->seek(qreal(mpTimeSlider->value())/qreal(mpTimeSlider->maximum()));
}

void MainWindow::capture()
{
    mpPlayer->captureVideo();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
    if (e->size() == qApp->desktop()->size()) {
        mpControl->hide();
        mpTimeSlider->hide();
    } else {
        if (mpControl->isHidden())
            mpControl->show();
        if (mpTimeSlider->isHidden())
            mpTimeSlider->show();
    }
    /*
    if (mpTitle)
        QLabelSetElideText(mpTitle, QFileInfo(mFile).fileName(), e->size().width());
    */
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
    int ms = mpPlayer->masterClock()->value()*1000.0;
    mpTimeSlider->setValue(ms);
    mpCurrent->setText(QTime(0, 0, 0).addMSecs(ms).toString("mm:ss"));
}


void MainWindow::about()
{
    QtAV::about();
}

void MainWindow::help()
{
}

void MainWindow::openUrl()
{
    QString url = QInputDialog::getText(0, tr("Open an url"), tr("Url"));
    if (url.isEmpty())
        return;
    play(url);
}

void MainWindow::switchAspectRatio(QAction *action)
{
    qreal r = action->data().toDouble();
    if (action == mpARAction && r != -2) {
        action->toggle(); //check state changes if clicked
        return;
    }
    if (r == 0) {
        mpPlayer->renderer()->setOutAspectRatioMode(VideoRenderer::VideoAspectRatio);
    } else if (r == -1) {
        mpPlayer->renderer()->setOutAspectRatioMode(VideoRenderer::RendererAspectRatio);
    } else {
        if (r == -2)
            r = QInputDialog::getDouble(0, tr("Aspect ratio"), "", 1.0);
        mpPlayer->renderer()->setOutAspectRatioMode(VideoRenderer::CustomAspectRation);
        mpPlayer->renderer()->setOutAspectRatio(r);
    }
    mpARAction->setChecked(false);
    mpARAction = action;
    mpARAction->setChecked(true);
}

void MainWindow::playOnlineVideo(QAction *action)
{
    play(action->data().toString());
}
