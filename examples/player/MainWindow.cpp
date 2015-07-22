/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
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
#include "MainWindow.h"
#include "EventFilter.h"
#include <QtAV>
#include <QtAVWidgets>
#include <QtCore/QtDebug>
#include <QtCore/QLocale>
#include <QtCore/QTimer>
#include <QTimeEdit>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QtCore/QFileInfo>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QGraphicsOpacityEffect>
#include <QComboBox>
#include <QResizeEvent>
#include <QWidgetAction>
#include <QLayout>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>
#include <QToolTip>
#include <QKeyEvent>
#include <QWheelEvent>
#include "ClickableMenu.h"
#include "Slider.h"
#include "StatisticsView.h"
#include "TVView.h"
#include "config/DecoderConfigPage.h"
#include "config/VideoEQConfigPage.h"
#include "config/ConfigDialog.h"
#include "filters/OSDFilter.h"
//#include "filters/AVFilterSubtitle.h"
#include "playlist/PlayList.h"
#include "../common/common.h"

/*
 *TODO:
 * disable a/v actions if player is 0;
 * use action's value to set player's parameters when start to play a new file
 */

#define AVDEBUG() \
    qDebug("%s %s @%d", __FILE__, __FUNCTION__, __LINE__);

using namespace QtAV;
const qreal kVolumeInterval = 0.04;

extern QStringList idsToNames(QVector<VideoDecoderId> ids);
extern QVector<VideoDecoderId> idsFromNames(const QStringList& names);

void QLabelSetElideText(QLabel *label, QString text, int W = 0)
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
  , mControlOn(false)
  , mShowControl(2)
  , mRepeateMax(0)
  , mpPlayer(0)
  , mpRenderer(0)
  , mpTempRenderer(0)
  , mpVideoFilter(0)
  , mpAudioFilter(0)
  , mpStatisticsView(0)
  , mpOSD(0)
  , mpSubtitle(0)
  , m_preview(0)
{
    setWindowIcon(QIcon(":/QtAV.svg"));
    mpOSD = new OSDFilterQPainter(this);
    mpSubtitle = new SubtitleFilter(this);
    mpChannelAction = 0;
    mpChannelMenu = 0;
    mpAudioTrackAction = 0;
    setMouseTracking(true); //mouseMoveEvent without press.
    connect(this, SIGNAL(ready()), SLOT(processPendingActions()));
    //QTimer::singleShot(10, this, SLOT(setupUi()));
    setupUi();
    //setToolTip(tr("Click black area to use shortcut(see right click menu)"));
    WindowEventFilter *we = new WindowEventFilter(this);
    installEventFilter(we);
    connect(we, SIGNAL(fullscreenChanged()), SLOT(handleFullscreenChange()));
}

MainWindow::~MainWindow()
{
    if (m_preview) {
        m_preview->close();
        delete m_preview;
    }
    mpHistory->save();
    mpPlayList->save();
    if (mpVolumeSlider && !mpVolumeSlider->parentWidget()) {
        mpVolumeSlider->close();
        delete mpVolumeSlider;
        mpVolumeSlider = 0;
    }
    if (mpStatisticsView) {
        delete mpStatisticsView;
        mpStatisticsView = 0;
    }
}

void MainWindow::initPlayer()
{
    mpPlayer = new AVPlayer(this);
    mIsReady = true;
    //mpSubtitle->installTo(mpPlayer); //filter on frame
    mpSubtitle->setPlayer(mpPlayer);
    //mpPlayer->setAudioOutput(AudioOutputFactory::create(AudioOutputId_OpenAL));
    EventFilter *ef = new EventFilter(mpPlayer);
    qApp->installEventFilter(ef);
    connect(ef, SIGNAL(helpRequested()), SLOT(help()));
    connect(ef, SIGNAL(showNextOSD()), SLOT(showNextOSD()));
    onCaptureConfigChanged();
    onAVFilterVideoConfigChanged();
    onAVFilterAudioConfigChanged();
    connect(&Config::instance(), SIGNAL(captureDirChanged(QString)), SLOT(onCaptureConfigChanged()));
    connect(&Config::instance(), SIGNAL(captureFormatChanged(QString)), SLOT(onCaptureConfigChanged()));
    connect(&Config::instance(), SIGNAL(captureQualityChanged(int)), SLOT(onCaptureConfigChanged()));
    connect(&Config::instance(), SIGNAL(avfilterVideoChanged()), SLOT(onAVFilterVideoConfigChanged()));
    connect(&Config::instance(), SIGNAL(avfilterAudioChanged()), SLOT(onAVFilterAudioConfigChanged()));
    connect(&Config::instance(), SIGNAL(bufferValueChanged()), SLOT(onBufferValueChanged()));
    connect(&Config::instance(), SIGNAL(abortOnTimeoutChanged()), SLOT(onAbortOnTimeoutChanged()));
    connect(mpStopBtn, SIGNAL(clicked()), this, SLOT(stopUnload()));
    connect(mpForwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekForward()));
    connect(mpBackwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekBackward()));
    connect(mpVolumeBtn, SIGNAL(clicked()), SLOT(showHideVolumeBar()));
    connect(mpVolumeSlider, SIGNAL(sliderPressed()), SLOT(setVolume()));
    connect(mpVolumeSlider, SIGNAL(valueChanged(int)), SLOT(setVolume()));

    connect(mpPlayer, SIGNAL(seekFinished()), SLOT(onSeekFinished()));
    connect(mpPlayer, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)), SLOT(onMediaStatusChanged()));
    connect(mpPlayer, SIGNAL(bufferProgressChanged(qreal)), SLOT(onBufferProgress(qreal)));
    connect(mpPlayer, SIGNAL(error(QtAV::AVError)), this, SLOT(handleError(QtAV::AVError)));
    connect(mpPlayer, SIGNAL(started()), this, SLOT(onStartPlay()));
    connect(mpPlayer, SIGNAL(stopped()), this, SLOT(onStopPlay()));
    connect(mpPlayer, SIGNAL(paused(bool)), this, SLOT(onPaused(bool)));
    connect(mpPlayer, SIGNAL(speedChanged(qreal)), this, SLOT(onSpeedChange(qreal)));
    connect(mpPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(onPositionChange(qint64)));
    //connect(mpPlayer, SIGNAL(volumeChanged(qreal)), SLOT(syncVolumeUi(qreal)));
    connect(mpVideoEQ, SIGNAL(brightnessChanged(int)), this, SLOT(onBrightnessChanged(int)));
    connect(mpVideoEQ, SIGNAL(contrastChanged(int)), this, SLOT(onContrastChanged(int)));
    connect(mpVideoEQ, SIGNAL(hueChanegd(int)), this, SLOT(onHueChanged(int)));
    connect(mpVideoEQ, SIGNAL(saturationChanged(int)), this, SLOT(onSaturationChanged(int)));

    connect(mpCaptureBtn, SIGNAL(clicked()), mpPlayer->videoCapture(), SLOT(request()));

    emit ready(); //emit this signal after connection. otherwise the slots may not be called for the first time
}

void MainWindow::onSeekFinished()
{
    qDebug("seek finished at %lld", mpPlayer->position());
}

void MainWindow::stopUnload()
{
    mpPlayer->stop();
    mpPlayer->unload();
}

void MainWindow::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setMargin(0);
    setLayout(mainLayout);

    mpPlayerLayout = new QVBoxLayout();
    mpControl = new QWidget(this);
    mpControl->setMaximumHeight(30);

    //mpPreview = new QLable(this);

    mpTimeSlider = new Slider(mpControl);
    mpTimeSlider->setDisabled(true);
    mpTimeSlider->setTracking(true);
    mpTimeSlider->setOrientation(Qt::Horizontal);
    mpTimeSlider->setMinimum(0);
    mpCurrent = new QLabel(mpControl);
    mpCurrent->setToolTip(tr("Current time"));
    mpCurrent->setMargin(2);
    mpCurrent->setText("00:00:00");
    mpEnd = new QLabel(mpControl);
    mpEnd->setToolTip(tr("Duration"));
    mpEnd->setMargin(2);
    mpEnd->setText("00:00:00");
    mpTitle = new QLabel(mpControl);
    mpTitle->setToolTip(tr("Render engine"));
    mpTitle->setText("QPainter");
    mpTitle->setIndent(8);
    mpSpeed = new QLabel("1.00");
    mpSpeed->setMargin(1);
    mpSpeed->setToolTip(tr("Speed. Ctrl+Up/Down"));

    mpPlayPauseBtn = new QToolButton(mpControl);
    mpPlayPauseBtn->setIcon(QIcon(":/theme/dark/play.svg"));
    mpStopBtn = new QToolButton(mpControl);
    mpStopBtn->setIcon(QIcon(":/theme/dark/stop.svg"));
    mpBackwardBtn = new QToolButton(mpControl);
    mpBackwardBtn->setIcon(QIcon(":/theme/dark/backward.svg"));
    mpForwardBtn = new QToolButton(mpControl);
    mpForwardBtn->setIcon(QIcon(":/theme/dark/forward.svg"));
    mpOpenBtn = new QToolButton(mpControl);
    mpOpenBtn->setToolTip(tr("Open"));
    mpOpenBtn->setIcon(QIcon(":/theme/dark/open.svg"));

    mpInfoBtn = new QToolButton();
    mpInfoBtn->setToolTip(QString("Media information"));
    mpInfoBtn->setIcon(QIcon(":/theme/dark/info.svg"));
    mpCaptureBtn = new QToolButton();
    mpCaptureBtn->setToolTip(QString("Capture"));
    mpCaptureBtn->setIcon(QIcon(":/theme/dark/capture.svg"));
    mpVolumeBtn = new QToolButton();
    mpVolumeBtn->setIcon(QIcon(":/theme/dark/sound.svg"));

    mpVolumeSlider = new Slider();
    mpVolumeSlider->hide();
    mpVolumeSlider->setOrientation(Qt::Horizontal);
    mpVolumeSlider->setMinimum(0);
    const int kVolumeSliderMax = 100;
    mpVolumeSlider->setMaximum(kVolumeSliderMax);
    //mpVolumeSlider->setMaximumHeight(12);
    mpVolumeSlider->setMaximumWidth(88);
    mpVolumeSlider->setValue(int(1.0/kVolumeInterval*qreal(kVolumeSliderMax)/100.0));
    setVolume();

    mpMenuBtn = new QToolButton();
    mpMenuBtn->setIcon(QIcon(":/theme/dark/menu.svg"));
    mpMenuBtn->setAutoRaise(true);
    mpMenuBtn->setPopupMode(QToolButton::InstantPopup);

    QMenu *subMenu = 0;
    QWidgetAction *pWA = 0;
    mpMenu = new QMenu(mpMenuBtn);
    mpMenu->addAction(tr("Open Url"), this, SLOT(openUrl()));
    //mpMenu->addAction(tr("Online channels"), this, SLOT(onTVMenuClick()));
    mpMenu->addSeparator();

    subMenu = new QMenu(tr("Play list"));
    mpMenu->addMenu(subMenu);
    mpPlayList = new PlayList(this);
    mpPlayList->setSaveFile(Config::instance().defaultDir() + "/playlist.qds");
    mpPlayList->load();
    connect(mpPlayList, SIGNAL(aboutToPlay(QString)), SLOT(play(QString)));
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(mpPlayList);
    subMenu->addAction(pWA); //must add action after the widget action is ready. is it a Qt bug?

    subMenu = new QMenu(tr("History"));
    mpMenu->addMenu(subMenu);
    mpHistory = new PlayList(this);
    mpHistory->setMaxRows(20);
    mpHistory->setSaveFile(Config::instance().defaultDir() + "/history.qds");
    mpHistory->load();
    connect(mpHistory, SIGNAL(aboutToPlay(QString)), SLOT(play(QString)));
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(mpHistory);
    subMenu->addAction(pWA); //must add action after the widget action is ready. is it a Qt bug?

    mpMenu->addSeparator();

    //mpMenu->addAction(tr("Report"))->setEnabled(false); //report bug, suggestions etc. using maillist?
    mpMenu->addAction(tr("About"), this, SLOT(about()));
    mpMenu->addAction(tr("Help"), this, SLOT(help()));
    mpMenu->addAction(tr("Donate"), this, SLOT(donate()));
    mpMenu->addAction(tr("Setup"), this, SLOT(setup()));
    mpMenu->addSeparator();
    mpMenuBtn->setMenu(mpMenu);
    mpMenu->addSeparator();

    subMenu = new QMenu(tr("Speed"));
    mpMenu->addMenu(subMenu);
    QDoubleSpinBox *pSpeedBox = new QDoubleSpinBox(0);
    pSpeedBox->setRange(0.01, 20);
    pSpeedBox->setValue(1.0);
    pSpeedBox->setSingleStep(0.01);
    pSpeedBox->setCorrectionMode(QAbstractSpinBox::CorrectToPreviousValue);
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(pSpeedBox);
    subMenu->addAction(pWA); //must add action after the widget action is ready. is it a Qt bug?

    subMenu = new ClickableMenu(tr("Repeat"));
    mpMenu->addMenu(subMenu);
    //subMenu->setEnabled(false);
    mpRepeatEnableAction = subMenu->addAction(tr("Enable"));
    mpRepeatEnableAction->setCheckable(true);
    connect(mpRepeatEnableAction, SIGNAL(toggled(bool)), SLOT(toggleRepeat(bool)));
    // TODO: move to a func or class
    mpRepeatBox = new QSpinBox(0);
    mpRepeatBox->setMinimum(-1);
    mpRepeatBox->setValue(-1);
    mpRepeatBox->setToolTip("-1: " + tr("infinity"));
    connect(mpRepeatBox, SIGNAL(valueChanged(int)), SLOT(setRepeateMax(int)));
    QLabel *pRepeatLabel = new QLabel(tr("Times"));
    QHBoxLayout *hb = new QHBoxLayout;
    hb->addWidget(pRepeatLabel);
    hb->addWidget(mpRepeatBox);
    QVBoxLayout *vb = new QVBoxLayout;
    vb->addLayout(hb);
    pRepeatLabel = new QLabel(tr("From"));
    mpRepeatA = new QTimeEdit();
    mpRepeatA->setDisplayFormat("HH:mm:ss");
    mpRepeatA->setToolTip(tr("negative value means from the end"));
    connect(mpRepeatA, SIGNAL(timeChanged(QTime)), SLOT(repeatAChanged(QTime)));
    hb = new QHBoxLayout;
    hb->addWidget(pRepeatLabel);
    hb->addWidget(mpRepeatA);
    vb->addLayout(hb);
    pRepeatLabel = new QLabel(tr("To"));
    mpRepeatB = new QTimeEdit();
    mpRepeatB->setDisplayFormat("HH:mm:ss");
    mpRepeatB->setToolTip(tr("negative value means from the end"));
    connect(mpRepeatB, SIGNAL(timeChanged(QTime)), SLOT(repeatBChanged(QTime)));
    hb = new QHBoxLayout;
    hb->addWidget(pRepeatLabel);
    hb->addWidget(mpRepeatB);
    vb->addLayout(hb);
    QWidget *wgt = new QWidget;
    wgt->setLayout(vb);

    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(wgt);
    pWA->defaultWidget()->setEnabled(false);
    subMenu->addAction(pWA); //must add action after the widget action is ready. is it a Qt bug?
    mpRepeatAction = pWA;

    mpMenu->addSeparator();

    subMenu = new ClickableMenu(tr("Clock"));
    mpMenu->addMenu(subMenu);
    QActionGroup *ag = new QActionGroup(subMenu);
    ag->setExclusive(true);
    connect(subMenu, SIGNAL(triggered(QAction*)), SLOT(changeClockType(QAction*)));
    subMenu->addAction(tr("Auto"))->setData(-1);
    subMenu->addAction(tr("Audio"))->setData(AVClock::AudioClock);
    subMenu->addAction(tr("Video"))->setData(AVClock::VideoClock);
    foreach(QAction* action, subMenu->actions()) {
        action->setActionGroup(ag);
        action->setCheckable(true);
    }
    QAction *autoClockAction = subMenu->actions().at(0);
    autoClockAction->setChecked(true);
    autoClockAction->setToolTip(tr("Take effect in next playback"));

    subMenu = new ClickableMenu(tr("Subtitle"));
    mpMenu->addMenu(subMenu);
    QAction *act = subMenu->addAction(tr("Enable"));
    act->setCheckable(true);
    act->setChecked(mpSubtitle->isEnabled());
    connect(act, SIGNAL(toggled(bool)), SLOT(toggoleSubtitleEnabled(bool)));
    act = subMenu->addAction(tr("Auto load"));
    act->setCheckable(true);
    act->setChecked(mpSubtitle->autoLoad());
    connect(act, SIGNAL(toggled(bool)), SLOT(toggleSubtitleAutoLoad(bool)));
    subMenu->addAction(tr("Open"), this, SLOT(openSubtitle()));

    wgt = new QWidget();
    hb = new QHBoxLayout();
    wgt->setLayout(hb);
    hb->addWidget(new QLabel(tr("Engine")));
    QComboBox *box = new QComboBox();
    hb->addWidget(box);
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(wgt);
    subMenu->addAction(pWA); //must add action after the widget action is ready. is it a Qt bug?
    box->addItem("FFmpeg", "FFmpeg");
    box->addItem("LibASS", "LibASS");
    connect(box, SIGNAL(activated(QString)), SLOT(setSubtitleEngine(QString)));
    mpSubtitle->setEngines(QStringList() << box->itemData(box->currentIndex()).toString());
    box->setToolTip(tr("FFmpeg supports more subtitles but only render plain text") + "\n" + tr("LibASS supports ass styles"));

    wgt = new QWidget();
    hb = new QHBoxLayout();
    wgt->setLayout(hb);
    hb->addWidget(new QLabel(tr("Charset")));
    box = new QComboBox();
    hb->addWidget(box);
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(wgt);
    subMenu->addAction(pWA); //must add action after the widget action is ready. is it a Qt bug?
    box->addItem(tr("Auto detect"), "AutoDetect");
    box->addItem(tr("System"), "System");
    foreach (const QByteArray& cs, QTextCodec::availableCodecs()) {
        box->addItem(cs, cs);
    }
    connect(box, SIGNAL(activated(QString)), SLOT(setSubtitleCharset(QString)));
    mpSubtitle->setCodec(box->itemData(box->currentIndex()).toByteArray());
    box->setToolTip(tr("Auto detect requires libchardet"));

    subMenu = new ClickableMenu(tr("Audio track"));
    mpMenu->addMenu(subMenu);
    mpAudioTrackMenu = subMenu;
    connect(subMenu, SIGNAL(triggered(QAction*)), SLOT(changeAudioTrack(QAction*)));
    initAudioTrackMenu();

    subMenu = new ClickableMenu(tr("Channel"));
    mpMenu->addMenu(subMenu);
    mpChannelMenu = subMenu;
    connect(subMenu, SIGNAL(triggered(QAction*)), SLOT(changeChannel(QAction*)));
    subMenu->addAction(tr("As input"))->setData(AudioFormat::ChannelLayout_Unsupported); //will set to input in resampler if not supported.
    subMenu->addAction(tr("Stero"))->setData(AudioFormat::ChannelLayout_Stero);
    subMenu->addAction(tr("Mono (center)"))->setData(AudioFormat::ChannelLayout_Center);
    subMenu->addAction(tr("Left"))->setData(AudioFormat::ChannelLayout_Left);
    subMenu->addAction(tr("Right"))->setData(AudioFormat::ChannelLayout_Right);
    ag = new QActionGroup(subMenu);
    ag->setExclusive(true);
    foreach(QAction* action, subMenu->actions()) {
        ag->addAction(action);
        action->setCheckable(true);
    }

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

    subMenu = new ClickableMenu(tr("Color space"));
    mpMenu->addMenu(subMenu);
    mpVideoEQ = new VideoEQConfigPage();
    connect(mpVideoEQ, SIGNAL(engineChanged()), SLOT(onVideoEQEngineChanged()));
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(mpVideoEQ);
    subMenu->addAction(pWA);

    subMenu = new ClickableMenu(tr("Decoder"));
    mpMenu->addMenu(subMenu);
    mpDecoderConfigPage = new DecoderConfigPage();
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(mpDecoderConfigPage);
    subMenu->addAction(pWA);

    subMenu = new ClickableMenu(tr("Renderer"));
    mpMenu->addMenu(subMenu);
    connect(subMenu, SIGNAL(triggered(QAction*)), SLOT(changeVO(QAction*)));
    //TODO: AVOutput.name,detail(description). check whether it is available
    mpVOAction = subMenu->addAction("QPainter");
    mpVOAction->setData(VideoRendererId_Widget);
    subMenu->addAction("OpenGLWidget")->setData(VideoRendererId_OpenGLWidget);
    subMenu->addAction("GLWidget 2")->setData(VideoRendererId_GLWidget2);
    subMenu->addAction("GLWidget")->setData(VideoRendererId_GLWidget);
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
    controlLayout->addWidget(mpVolumeSlider);
    controlLayout->addWidget(mpVolumeBtn);
    controlLayout->addWidget(mpCaptureBtn);
    controlLayout->addWidget(mpPlayPauseBtn);
    controlLayout->addWidget(mpStopBtn);
    controlLayout->addWidget(mpBackwardBtn);
    controlLayout->addWidget(mpForwardBtn);
    controlLayout->addWidget(mpOpenBtn);
    controlLayout->addWidget(mpInfoBtn);
    controlLayout->addWidget(mpSpeed);
    //controlLayout->addWidget(mpSetupBtn);
    controlLayout->addWidget(mpMenuBtn);
    controlLayout->addWidget(mpEnd);

    connect(pSpeedBox, SIGNAL(valueChanged(double)), SLOT(onSpinBoxChanged(double)));
    connect(mpOpenBtn, SIGNAL(clicked()), SLOT(openFile()));
    connect(mpPlayPauseBtn, SIGNAL(clicked()), SLOT(togglePlayPause()));
    connect(mpInfoBtn, SIGNAL(clicked()), SLOT(showInfo()));
    //valueChanged can be triggered by non-mouse event
    //TODO: connect sliderMoved(int) to preview(int)
    connect(mpTimeSlider, SIGNAL(sliderPressed()), SLOT(seek()));
    connect(mpTimeSlider, SIGNAL(sliderReleased()), SLOT(seek()));
    connect(mpTimeSlider, SIGNAL(onLeave()), SLOT(onTimeSliderLeave()));
    connect(mpTimeSlider, SIGNAL(onHover(int,int)), SLOT(onTimeSliderHover(int,int)));
    QTimer::singleShot(0, this, SLOT(initPlayer()));
}

void MainWindow::changeChannel(QAction *action)
{
    if (action == mpChannelAction) {
        action->toggle();
        return;
    }
    AudioFormat::ChannelLayout cl = (AudioFormat::ChannelLayout)action->data().toInt();
    AudioOutput *ao = mpPlayer ? mpPlayer->audio() : 0; //getAO()?
    if (!ao) {
        qWarning("No audio output!");
        return;
    }
    mpChannelAction->setChecked(false);
    mpChannelAction = action;
    mpChannelAction->setChecked(true);
    if (!ao->close()) {
        qWarning("close audio failed");
        return;
    }
    ao->audioFormat().setChannelLayout(cl);
    if (!ao->open()) {
        qWarning("open audio failed");
        return;
    }
}

void MainWindow::changeAudioTrack(QAction *action)
{
    int track = action->data().toInt();
    if (mpAudioTrackAction == action && track >= 0) { // external action is always clickable
        action->toggle();
        return;
    }
    if (track < 0) {
        QString f = QFileDialog::getOpenFileName(0, tr("Open an external audio track"));
        if (f.isEmpty()) {
            action->toggle();
            return;
        }
        mpPlayer->setExternalAudio(f);
    } else {
        mpPlayer->setExternalAudio(QString());
        if (!mpPlayer->setAudioStream(track)) {
            action->toggle();
            return;
        }
    }

    if (mpAudioTrackAction)
        mpAudioTrackAction->setChecked(false);
    mpAudioTrackAction = action;
    mpAudioTrackAction->setChecked(true);
    if (mpStatisticsView && mpStatisticsView->isVisible())
        mpStatisticsView->setStatistics(mpPlayer->statistics());
}

void MainWindow::changeVO(QAction *action)
{
    if (action == mpVOAction) {
        action->toggle(); //check state changes if clicked
        return;
    }
    VideoRendererId vid = (VideoRendererId)action->data().toInt();
    VideoRenderer *vo = VideoRendererFactory::create(vid);
    if (vo && vo->isAvailable()) {

        setRenderer(vo);
    } else {
        action->toggle(); //check state changes if clicked
        QMessageBox::critical(0, "QtAV", tr("not availabe on your platform!"));
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

void MainWindow::setAudioBackends(const QStringList& backends)
{
    mAudioBackends = backends;
}

void MainWindow::setRenderer(QtAV::VideoRenderer *renderer)
{
    if (!mIsReady) {
        mpTempRenderer = renderer;
        return;
    }
    if (!renderer)
        return;
    mpOSD->uninstall();
    mpSubtitle->uninstall();
    renderer->widget()->setMouseTracking(true); //mouseMoveEvent without press.
    mpPlayer->setRenderer(renderer);
    QWidget *r = 0;
    if (mpRenderer)
        r = mpRenderer->widget();
    //release old renderer and add new
    if (r) {
        mpPlayerLayout->removeWidget(r);
        if (r->testAttribute(Qt::WA_DeleteOnClose)) {
            r->close();
        } else {
            r->close();
            delete r;
        }
        r = 0;
    }
    mpRenderer = renderer;
    //setInSize?
    mpPlayerLayout->addWidget(renderer->widget());
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
    const VideoRendererId vid = mpPlayer->renderer()->id();
    if (vid == VideoRendererId_GLWidget
            || vid == VideoRendererId_GLWidget2
            || vid == VideoRendererId_OpenGLWidget
            ) {
        mpVideoEQ->setEngines(QVector<VideoEQConfigPage::Engine>() << VideoEQConfigPage::SWScale << VideoEQConfigPage::GLSL);
        mpVideoEQ->setEngine(VideoEQConfigPage::GLSL);
        mpPlayer->renderer()->forcePreferredPixelFormat(true);
    } else if (vid == VideoRendererId_XV) {
        mpVideoEQ->setEngines(QVector<VideoEQConfigPage::Engine>() << VideoEQConfigPage::XV);
        mpVideoEQ->setEngine(VideoEQConfigPage::XV);
        mpPlayer->renderer()->forcePreferredPixelFormat(true);
    } else {
        mpVideoEQ->setEngines(QVector<VideoEQConfigPage::Engine>() << VideoEQConfigPage::SWScale);
        mpVideoEQ->setEngine(VideoEQConfigPage::SWScale);
        mpPlayer->renderer()->forcePreferredPixelFormat(false);
    }
    onVideoEQEngineChanged();
    mpOSD->installTo(mpRenderer);
    mpSubtitle->installTo(mpRenderer);
}

void MainWindow::play(const QString &name)
{
    mFile = name;
    if (!mIsReady) {
        mHasPendingPlay = true;
        return;
    }
    mTitle = mFile;
    if (!mFile.contains("://") || mFile.startsWith("file://")) {
        mTitle = QFileInfo(mFile).fileName();
    }
    setWindowTitle(mTitle);
    mpPlayer->setFrameRate(Config::instance().forceFrameRate());
    if (!mAudioBackends.isEmpty())
        mpPlayer->audio()->setBackends(mAudioBackends);
    if (!mpRepeatEnableAction->isChecked())
        mRepeateMax = 0;
    mpPlayer->setInterruptOnTimeout(Config::instance().abortOnTimeout());
    mpPlayer->setInterruptTimeout(Config::instance().timeout()*1000.0);
    mpPlayer->setBufferMode(QtAV::BufferPackets);
    mpPlayer->setBufferValue(Config::instance().bufferValue());
    mpPlayer->setRepeat(mRepeateMax);
    mpPlayer->setPriority(idsFromNames(Config::instance().decoderPriorityNames()));
    mpPlayer->setOptionsForAudioCodec(mpDecoderConfigPage->audioDecoderOptions());
    mpPlayer->setOptionsForVideoCodec(mpDecoderConfigPage->videoDecoderOptions());
    if (Config::instance().avformatOptionsEnabled())
        mpPlayer->setOptionsForFormat(Config::instance().avformatOptions());
    PlayListItem item;
    item.setUrl(mFile);
    item.setTitle(mTitle);
    item.setLastTime(0);
    mpHistory->remove(mFile);
    mpHistory->insertItemAt(item, 0);
    mpPlayer->play(name);
}

void MainWindow::play(const QUrl &url)
{
    play(QUrl::fromPercentEncoding(url.toEncoded()));
}

void MainWindow::setVideoDecoderNames(const QStringList &vd)
{
    QStringList vdnames;
    foreach (const QString& v, vd) {
        vdnames << v.toLower();
    }
    QStringList vidp;
    QStringList vids = idsToNames(GetRegistedVideoDecoderIds());
    foreach (const QString& v, vids) {
        if (vdnames.contains(v.toLower())) {
            vidp.append(v);
        }
    }
    Config::instance().setDecoderPriorityNames(vidp);
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
        if (!mpPlayer->isPlaying())
            play(mFile);
        else
            mpPlayer->play();
        mpPlayPauseBtn->setIcon(QIcon(":/theme/dark/pause.svg"));
    }
}

void MainWindow::showNextOSD()
{
    if (!mpOSD)
        return;
    mpOSD->useNextShowType();
}

void MainWindow::onSpinBoxChanged(double v)
{
    if (!mpPlayer)
        return;
    mpPlayer->setSpeed(v);
}

void MainWindow::onPaused(bool p)
{
    if (p) {
        qDebug("start pausing...");
        mpPlayPauseBtn->setIcon(QIcon(":/theme/dark/play.svg"));
    } else {
        qDebug("stop pausing...");
        mpPlayPauseBtn->setIcon(QIcon(":/theme/dark/pause.svg"));
    }
}

void MainWindow::onStartPlay()
{
    mpRenderer->setRegionOfInterest(QRectF());
    mFile = mpPlayer->file(); //open from EventFilter's menu
    mTitle = mFile;
    if (!mFile.contains("://") || mFile.startsWith("file://"))
        mTitle = QFileInfo(mFile).fileName();
    setWindowTitle(mTitle);

    mpPlayPauseBtn->setIcon(QIcon(":/theme/dark/pause.svg"));
    mpTimeSlider->setMinimum(mpPlayer->mediaStartPosition());
    mpTimeSlider->setMaximum(mpPlayer->mediaStopPosition());
    mpTimeSlider->setValue(0);
    mpTimeSlider->setEnabled(mpPlayer->isSeekable());
    mpEnd->setText(QTime(0, 0, 0).addMSecs(mpPlayer->mediaStopPosition()).toString("HH:mm:ss"));
    setVolume();
    mShowControl = 0;
    QTimer::singleShot(3000, this, SLOT(tryHideControlBar()));
    ScreenSaver::instance().disable();
    initAudioTrackMenu();
    mpRepeatA->setMinimumTime(QTime(0, 0, 0).addMSecs(mpPlayer->mediaStartPosition()));
    mpRepeatA->setMaximumTime(QTime(0, 0, 0).addMSecs(mpPlayer->mediaStopPosition()));
    mpRepeatB->setMinimumTime(QTime(0, 0, 0).addMSecs(mpPlayer->mediaStartPosition()));
    mpRepeatB->setMaximumTime(QTime(0, 0, 0).addMSecs(mpPlayer->mediaStopPosition()));
    mpRepeatA->setTime(QTime(0, 0, 0).addMSecs(mpPlayer->startPosition()));
    mpRepeatB->setTime(QTime(0, 0, 0).addMSecs(mpPlayer->stopPosition()));
    mCursorTimer = startTimer(3000);
    PlayListItem item = mpHistory->itemAt(0);
    item.setUrl(mFile);
    item.setTitle(mTitle);
    item.setDuration(mpPlayer->duration());
    mpHistory->setItemAt(item, 0);
    updateChannelMenu();

    if (mpStatisticsView && mpStatisticsView->isVisible())
        mpStatisticsView->setStatistics(mpPlayer->statistics());
}

void MainWindow::onStopPlay()
{
    mpPlayer->setPriority(idsFromNames(Config::instance().decoderPriorityNames()));
    if (mpPlayer->currentRepeat() < mpPlayer->repeat())
        return;
    // use shortcut to replay in EventFilter, the options will not be set, so set here
    mpPlayer->setFrameRate(Config::instance().forceFrameRate());
    mpPlayer->setOptionsForAudioCodec(mpDecoderConfigPage->audioDecoderOptions());
    mpPlayer->setOptionsForVideoCodec(mpDecoderConfigPage->videoDecoderOptions());
    if (Config::instance().avformatOptionsEnabled())
        mpPlayer->setOptionsForFormat(Config::instance().avformatOptions());

    mpPlayPauseBtn->setIcon(QIcon(":/theme/dark/play.svg"));
    mpTimeSlider->setValue(0);
    qDebug(">>>>>>>>>>>>>>disable slider");
    mpTimeSlider->setDisabled(true);
    mpTimeSlider->setMinimum(0);
    mpTimeSlider->setMaximum(0);
    mpCurrent->setText("00:00:00");
    mpEnd->setText("00:00:00");
    tryShowControlBar();
    ScreenSaver::instance().enable();
    toggleRepeat(false);
    //mRepeateMax = 0;
    killTimer(mCursorTimer);
    unsetCursor();
    if (m_preview)
        m_preview->setFile(QString());
}

void MainWindow::onSpeedChange(qreal speed)
{
    mpSpeed->setText(QString("%1").arg(speed, 4, 'f', 2, '0'));
}

void MainWindow::seek()
{
    mpPlayer->setSeekType(AccurateSeek);
    mpPlayer->seek((qint64)mpTimeSlider->value());
    if (!m_preview || !Config::instance().previewEnabled())
        return;
    m_preview->setTimestamp(mpTimeSlider->value());
    m_preview->preview();
    m_preview->setWindowFlags(m_preview->windowFlags() |Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
    m_preview->resize(Config::instance().previewWidth(), Config::instance().previewHeight());
    m_preview->show();
}

void MainWindow::showHideVolumeBar()
{
    if (mpVolumeSlider->isHidden()) {
        mpVolumeSlider->show();
    } else {
        mpVolumeSlider->hide();
    }
}

void MainWindow::setVolume()
{
    AudioOutput *ao = mpPlayer ? mpPlayer->audio() : 0;
    qreal v = qreal(mpVolumeSlider->value())*kVolumeInterval;
    if (ao) {
        if (qAbs(int(ao->volume()/kVolumeInterval) - mpVolumeSlider->value()) >= int(0.1/kVolumeInterval)) {
            ao->setVolume(v);
        }
    }
    mpVolumeSlider->setToolTip(QString::number(v));
    mpVolumeBtn->setToolTip(QString::number(v));
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e);
    if (mpPlayer)
        mpPlayer->stop();
    qApp->quit();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
    QWidget::resizeEvent(e);
    /*
    if (mpTitle)
        QLabelSetElideText(mpTitle, QFileInfo(mFile).fileName(), e->size().width());
    */
}

void MainWindow::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == mCursorTimer) {
        if (mpControl->isVisible())
            return;
        setCursor(Qt::BlankCursor);
    }
}

void MainWindow::onPositionChange(qint64 pos)
{
    if (mpPlayer->isSeekable())
        mpTimeSlider->setValue(pos);
    mpCurrent->setText(QTime(0, 0, 0).addMSecs(pos).toString("HH:mm:ss"));
}

void MainWindow::repeatAChanged(const QTime& t)
{
    if (!mpPlayer)
        return;
    mpPlayer->setStartPosition(QTime(0, 0, 0).msecsTo(t));
}

void MainWindow::repeatBChanged(const QTime& t)
{
    if (!mpPlayer)
        return;
    // when this slot is called? even if only range is set?
    if (t <= mpRepeatA->time())
        return;
    mpPlayer->setStopPosition(QTime(0, 0, 0).msecsTo(t));
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    mControlOn = e->key() == Qt::Key_Control;
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    Q_UNUSED(e);
    mControlOn = false;
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if (!mControlOn)
        return;
    mGlobalMouse = e->globalPos();
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    unsetCursor();
    if (e->pos().y() > height() - mpTimeSlider->height() - mpControl->height()) {
        if (mShowControl == 0) {
            mShowControl = 1;
            tryShowControlBar();
        }
    } else {
        if (mShowControl == 1) {
            mShowControl = 0;
            QTimer::singleShot(3000, this, SLOT(tryHideControlBar()));
        }
    }
    if (mControlOn && e->button() == Qt::LeftButton) {
        if (!mpRenderer || !mpRenderer->widget())
            return;
        QRectF roi = mpRenderer->realROI();
        QPointF delta = e->pos() - mGlobalMouse;
        roi.moveLeft(-delta.x());
        roi.moveTop(-delta.y());
        mpRenderer->setRegionOfInterest(roi);
    }
}

void MainWindow::wheelEvent(QWheelEvent *e)
{
    if (!mpRenderer || !mpRenderer->widget()) {
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    qreal deg = e->angleDelta().y()/8;
#else
    qreal deg = e->delta()/8;
#endif //QT_VERSION
#if WHEEL_SPEED
    if (!mControlOn) {
        qreal speed = mpPlayer->speed();
        mpPlayer->setSpeed(qMax(0.01, speed + deg/15.0*0.02));
        return;
    }
#endif //WHEEL_SPEED
    QPointF p = mpRenderer->widget()->mapFrom(this, e->pos());
    QPointF fp = mpRenderer->mapToFrame(p);
    if (fp.x() < 0)
        fp.setX(0);
    if (fp.y() < 0)
        fp.setY(0);
    if (fp.x() > mpRenderer->frameSize().width())
        fp.setX(mpRenderer->frameSize().width());
    if (fp.y() > mpRenderer->frameSize().height())
        fp.setY(mpRenderer->frameSize().height());

    QRectF viewport = QRectF(mpRenderer->mapToFrame(QPointF(0, 0)), mpRenderer->mapToFrame(QPointF(mpRenderer->rendererWidth(), mpRenderer->rendererHeight())));
    //qDebug("vo: (%.1f, %.1f)=> frame: (%.1f, %.1f)", p.x(), p.y(), fp.x(), fp.y());

    qreal zoom = 1.0 + deg*3.14/180.0;
    //qDebug("deg: %d, %d zoom: %.2f", e->angleDelta().x(), e->angleDelta().y(), zoom);
    QTransform m;
    m.translate(fp.x(), fp.y());
    m.scale(1.0/zoom, 1.0/zoom);
    m.translate(-fp.x(), -fp.y());
    QRectF r = m.mapRect(mpRenderer->realROI());
    mpRenderer->setRegionOfInterest((r | m.mapRect(viewport))&QRectF(QPointF(0,0), mpRenderer->frameSize()));
}

void MainWindow::about()
{
    QtAV::about();
}

void MainWindow::help()
{
    QString name = QString("help-%1.html").arg(QLocale::system().name());
    QFile f(qApp->applicationDirPath() + "/" + name);
    if (!f.exists()) {
        f.setFileName(":/" + name);
    }
    if (!f.exists()) {
        f.setFileName(qApp->applicationDirPath() + "/help.html");
    }
    if (!f.exists()) {
        f.setFileName(":/help.html");
    }
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open help-%s.html and help.html: %s", qPrintable(QLocale::system().name()), qPrintable(f.errorString()));
        return;
    }
    QTextStream ts(&f);
    ts.setCodec("UTF-8");
    QString text = ts.readAll();
    QMessageBox::information(0, "Help", text);
}

void MainWindow::openUrl()
{
    QString url = QInputDialog::getText(0, tr("Open an url"), tr("Url"));
    if (url.isEmpty())
        return;
    play(url);
}

void MainWindow::updateChannelMenu()
{
    if (mpChannelAction)
        mpChannelAction->setChecked(false);
    AudioOutput *ao = mpPlayer ? mpPlayer->audio() : 0; //getAO()?
    if (!ao) {
        return;
    }
    AudioFormat::ChannelLayout cl = ao->audioFormat().channelLayout();
    QList<QAction*> as = mpChannelMenu->actions();
    foreach (QAction *action, as) {
        if (action->data().toInt() != (int)cl)
            continue;
        action->setChecked(true);
        mpChannelAction = action;
        break;
    }
}

void MainWindow::initAudioTrackMenu()
{
    int track = -2;
    QAction *a = 0;
    QList<QAction*> as;
    int tracks = 0;
    if (!mpPlayer) {
        a = mpAudioTrackMenu->addAction(tr("External"));
        a->setData(-1);
        a->setCheckable(true);
        a->setChecked(false);
        as.push_back(a);
        mpAudioTrackAction = 0;
        goto end;
    }
    track = mpPlayer->currentAudioStream();
    as = mpAudioTrackMenu->actions();
    tracks = mpPlayer->audioStreamCount();
    if (mpAudioTrackAction && tracks == as.size()-1 && mpAudioTrackAction->data().toInt() == track)
        return;
    while (tracks + 1 < as.size()) {
        a = as.takeLast();
        mpAudioTrackMenu->removeAction(a);
        delete a;
    }
    if (as.isEmpty()) {
        a = mpAudioTrackMenu->addAction(tr("External"));
        a->setData(-1);
        a->setCheckable(true);
        a->setChecked(false);
        as.push_back(a);
        mpAudioTrackAction = 0;
    }
    while (tracks + 1 > as.size()) {
        a = mpAudioTrackMenu->addAction(QString::number(as.size()-1));
        a->setData(as.size()-1);
        a->setCheckable(true);
        a->setChecked(false);
        as.push_back(a);
    }
end:
    foreach(QAction *ac, as) {
        if (ac->data().toInt() == track && track >= 0) {
            if (mpPlayer && mpPlayer->externalAudio().isEmpty()) {
                qDebug("track found!!!!!");
                mpAudioTrackAction = ac;
                ac->setChecked(true);
            }
        } else {
            ac->setChecked(false);
        }
    }
    if (mpPlayer && !mpPlayer->externalAudio().isEmpty()) {
        mpAudioTrackAction = as.first();
    }
    if (mpAudioTrackAction)
        mpAudioTrackAction->setChecked(true);
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

void MainWindow::toggleRepeat(bool r)
{
    mpRepeatEnableAction->setChecked(r);
    mpRepeatAction->defaultWidget()->setEnabled(r); //why need defaultWidget?
    if (r) {
        mRepeateMax = mpRepeatBox->value();
    } else {
        mRepeateMax = 0;
    }
    if (mpPlayer) {
        mpPlayer->setRepeat(mRepeateMax);
    }
}

void MainWindow::setRepeateMax(int m)
{
    mRepeateMax = m;
    if (mpPlayer) {
        mpPlayer->setRepeat(m);
    }
}

void MainWindow::playOnlineVideo(QAction *action)
{
    mTitle = action->text();
    play(action->data().toString());
}

void MainWindow::onTVMenuClick()
{
    static TVView *tvv = new TVView;
    tvv->show();
    connect(tvv, SIGNAL(clicked(QString,QString)), SLOT(onPlayListClick(QString,QString)));
    tvv->setMaximumHeight(qApp->desktop()->height());
    tvv->setMinimumHeight(tvv->width()*2);
}

void MainWindow::onPlayListClick(const QString &key, const QString &value)
{
    mTitle = key;
    play(value);
}

void MainWindow::tryHideControlBar()
{
    if (mShowControl > 0) {
        return;
    }
    if (mpControl->isHidden() && mpTimeSlider->isHidden())
        return;
    mpControl->hide();
    mpTimeSlider->hide();
    workaroundRendererSize();
}

void MainWindow::tryShowControlBar()
{
    unsetCursor();
    if (mpTimeSlider && mpTimeSlider->isHidden())
        mpTimeSlider->show();
    if (mpControl && mpControl->isHidden())
        mpControl->show();
}

void MainWindow::showInfo()
{
    if (!mpStatisticsView)
        mpStatisticsView = new StatisticsView();
    if (mpPlayer)
        mpStatisticsView->setStatistics(mpPlayer->statistics());
    mpStatisticsView->show();
}

void MainWindow::onTimeSliderHover(int pos, int value)
{
    QPoint gpos = mapToGlobal(mpTimeSlider->pos() + QPoint(pos, 0));
    QToolTip::showText(gpos, QTime(0, 0, 0).addMSecs(value).toString("HH:mm:ss"));
    if (!Config::instance().previewEnabled())
        return;
    if (!m_preview)
        m_preview = new VideoPreviewWidget();
    m_preview->setFile(mpPlayer->file());
    m_preview->setTimestamp(value);
    m_preview->preview();
    const int w = Config::instance().previewWidth();
    const int h = Config::instance().previewHeight();
    m_preview->setWindowFlags(m_preview->windowFlags() |Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
    m_preview->resize(w, h);
    m_preview->move(gpos - QPoint(w/2, h));
    m_preview->show();
}

void MainWindow::onTimeSliderLeave()
{
    if (m_preview && m_preview->isVisible())
        m_preview->hide();
}

void MainWindow::handleError(const AVError &e)
{
    QMessageBox::warning(0, "Player error", e.string());
}

void MainWindow::onMediaStatusChanged()
{
    QString status;
    AVPlayer *player = qobject_cast<AVPlayer*>(sender());
    switch (player->mediaStatus()) {
    case NoMedia:
        status = "No media";
        break;
    case InvalidMedia:
        status = "Invalid meida";
        break;
    case BufferingMedia:
        status = "Buffering...";
        break;
    case BufferedMedia:
        status = "Buffered";
        break;
    case LoadingMedia:
        status = "Loading...";
        break;
    case LoadedMedia:
        status = "Loaded";
        break;
    case StalledMedia:
        status = "Stalled";
        break;
    default:
        status = "";
        onStopPlay();
        break;
    }
    qDebug() << "status changed " << status;
    setWindowTitle(status + " " + mTitle);
}

void MainWindow::onBufferProgress(qreal percent)
{
    setWindowTitle(QString("Buffering... %1% ").arg(percent*100.0, 0, 'f', 1) + mTitle);
}

void MainWindow::onVideoEQEngineChanged()
{
    VideoRenderer *vo = mpPlayer->renderer();
    VideoEQConfigPage::Engine e = mpVideoEQ->engine();
    if (e == VideoEQConfigPage::SWScale) {
        vo->forcePreferredPixelFormat(true);
        vo->setPreferredPixelFormat(VideoFormat::Format_RGB32);
    } else {
        vo->forcePreferredPixelFormat(false);
    }
    onBrightnessChanged(mpVideoEQ->brightness()*100.0);
    onContrastChanged(mpVideoEQ->contrast()*100.0);
    onHueChanged(mpVideoEQ->hue()*100.0);
    onSaturationChanged(mpVideoEQ->saturation()*100.0);
}

void MainWindow::onBrightnessChanged(int b)
{
    VideoRenderer *vo = mpPlayer->renderer();
    if (mpVideoEQ->engine() != VideoEQConfigPage::SWScale
            && vo->setBrightness(mpVideoEQ->brightness())) {
        mpPlayer->setBrightness(0);
    } else {
        vo->setBrightness(0);
        mpPlayer->setBrightness(b);
    }
}

void MainWindow::onContrastChanged(int c)
{
    VideoRenderer *vo = mpPlayer->renderer();
    if (mpVideoEQ->engine() != VideoEQConfigPage::SWScale
            && vo->setContrast(mpVideoEQ->contrast())) {
        mpPlayer->setContrast(0);
    } else {
        vo->setContrast(0);
        mpPlayer->setContrast(c);
    }
}

void MainWindow::onHueChanged(int h)
{
    VideoRenderer *vo = mpPlayer->renderer();
    if (mpVideoEQ->engine() != VideoEQConfigPage::SWScale
            && vo->setHue(mpVideoEQ->hue())) {
        mpPlayer->setHue(0);
    } else {
        vo->setHue(0);
        mpPlayer->setHue(h);
    }
}

void MainWindow::onSaturationChanged(int s)
{
    VideoRenderer *vo = mpPlayer->renderer();
    if (mpVideoEQ->engine() != VideoEQConfigPage::SWScale
            && vo->setSaturation(mpVideoEQ->saturation())) {
        mpPlayer->setSaturation(0);
    } else {
        vo->setSaturation(0);
        mpPlayer->setSaturation(s);
    }
}

void MainWindow::onCaptureConfigChanged()
{
    mpPlayer->videoCapture()->setCaptureDir(Config::instance().captureDir());
    mpPlayer->videoCapture()->setQuality(Config::instance().captureQuality());
    if (Config::instance().captureFormat().toLower() == "original") {
        mpPlayer->videoCapture()->setOriginalFormat(true);
    } else {
        mpPlayer->videoCapture()->setOriginalFormat(false);
        mpPlayer->videoCapture()->setSaveFormat(Config::instance().captureFormat());
    }
    mpCaptureBtn->setToolTip(tr("Capture video frame") + "\n" + tr("Save to") + ": " + mpPlayer->videoCapture()->captureDir()
                             + "\n" + tr("Format") + ": " + Config::instance().captureFormat());

}

void MainWindow::onAVFilterVideoConfigChanged()
{
    if (Config::instance().avfilterVideoEnable()) {
        if (!mpVideoFilter) {
            mpVideoFilter = new LibAVFilterVideo(this);
        }
        mpVideoFilter->setEnabled(true);
        mpPlayer->installVideoFilter(mpVideoFilter);
        mpVideoFilter->setOptions(Config::instance().avfilterVideoOptions());
    } else {
        if (mpVideoFilter) {
            mpVideoFilter->setEnabled(false);
        }
        mpPlayer->uninstallFilter(mpVideoFilter);
    }
}

void MainWindow::onAVFilterAudioConfigChanged()
{
    if (Config::instance().avfilterAudioEnable()) {
        if (!mpAudioFilter) {
            mpAudioFilter = new LibAVFilterAudio(this);
        }
        mpAudioFilter->setEnabled(true);
        mpPlayer->installAudioFilter(mpAudioFilter);
        mpAudioFilter->setOptions(Config::instance().avfilterAudioOptions());
    } else {
        if (mpAudioFilter) {
            mpAudioFilter->setEnabled(false);
        }
        mpPlayer->uninstallFilter(mpAudioFilter);
    }
}

void MainWindow::donate()
{
    //QDesktopServices::openUrl(QUrl("https://sourceforge.net/p/qtav/wiki/Donate%20%E6%8D%90%E8%B5%A0/"));
    QDesktopServices::openUrl(QUrl("http://www.qtav.org/donate.html"));
}

void MainWindow::onBufferValueChanged()
{
    if (!mpPlayer)
        return;
    mpPlayer->setBufferValue(Config::instance().bufferValue());
}

void MainWindow::onAbortOnTimeoutChanged()
{
    if (!mpPlayer)
        return;
    mpPlayer->setInterruptOnTimeout(Config::instance().abortOnTimeout());
}

void MainWindow::setup()
{
    ConfigDialog::display();
}

void MainWindow::handleFullscreenChange()
{
    workaroundRendererSize();

    // workaround renderer display size for ubuntu
    tryShowControlBar();
    QTimer::singleShot(3000, this, SLOT(tryHideControlBar()));
}

void MainWindow::toggoleSubtitleEnabled(bool value)
{
    mpSubtitle->setEnabled(value);
}

void MainWindow::toggleSubtitleAutoLoad(bool value)
{
    mpSubtitle->setAutoLoad(value);
}

void MainWindow::openSubtitle()
{
    QString file = QFileDialog::getOpenFileName(0, tr("Open a subtitle file"));
    if (file.isEmpty())
        return;
    mpSubtitle->setFile(file);
}

void MainWindow::setSubtitleCharset(const QString &charSet)
{
    Q_UNUSED(charSet);
    QComboBox *box = qobject_cast<QComboBox*>(sender());
    if (!box)
        return;
    mpSubtitle->setCodec(box->itemData(box->currentIndex()).toByteArray());
}

void MainWindow::setSubtitleEngine(const QString &value)
{
    Q_UNUSED(value)
    QComboBox *box = qobject_cast<QComboBox*>(sender());
    if (!box)
        return;
    mpSubtitle->setEngines(QStringList() << box->itemData(box->currentIndex()).toString());
}

void MainWindow::changeClockType(QAction *action)
{
    action->setChecked(true);
    int value = action->data().toInt();
    if (value < 0) {
        mpPlayer->masterClock()->setClockAuto(true);
        // TODO: guess clock type
        return;
    }
    mpPlayer->masterClock()->setClockAuto(false);
    mpPlayer->masterClock()->setClockType(AVClock::ClockType(value));
}

void MainWindow::syncVolumeUi(qreal value)
{
    const int v(value/kVolumeInterval);
    if (mpVolumeSlider->value() == v)
        return;
    mpVolumeSlider->setValue(v);
}

void MainWindow::workaroundRendererSize()
{
    if (!mpRenderer)
        return;
    QSize s = rect().size();
    //resize(QSize(s.width()-1, s.height()-1));
    //resize(s); //window resize to fullscreen size will create another fullScreenChange event
    mpRenderer->widget()->resize(QSize(s.width()+1, s.height()+1));
    mpRenderer->widget()->resize(s);
}
