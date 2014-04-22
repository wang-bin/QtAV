/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

class QWidgetAction;
namespace QtAV {
class AudioOutput;
class AVError;
class AVPlayer;
class AVClock;
class VideoRenderer;
}
class QMenu;
class QTimeEdit;
class QVBoxLayout;
class QLabel;
class QPushButton;
class QSpinBox;
class QTimeEdit;
class Button;
class Slider;
class PlayList;
class DecoderConfigPage;
class VideoEQConfigPage;
class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void enableAudio(bool yes = true);
    void setAudioOutput(QtAV::AudioOutput* ao);
    void setRenderer(QtAV::VideoRenderer* renderer);
    void setVideoDecoderNames(const QStringList& vd);

public slots:
    void play(const QString& name);
    void openFile();
    void togglePlayPause();

signals:
    void ready();

private slots:
    void about();
    void help();
    void openUrl();
    void initAudioTrackMenu();
    void updateChannelMenu();
    void switchAspectRatio(QAction* action);
    void toggleRepeat(bool);
    void setRepeateMax(int);
    void changeVO(QAction* action);
    void changeChannel(QAction* action);
    void changeAudioTrack(QAction* action);
    void onTVMenuClick();
    void playOnlineVideo(QAction *action);
    void onPlayListClick(const QString& key, const QString& value);
    void processPendingActions();
    void initPlayer();
    void setupUi();
    void onSpinBoxChanged(double v);
    void onStartPlay();
    void onStopPlay();
    void onPaused(bool p);
    void onSpeedChange(qreal speed);
    void seekToMSec(int msec);
    void seek();
    void capture();
    void showHideVolumeBar();
    void setVolume();
    void tryHideControlBar();
    void tryShowControlBar();
    void showInfo();
    void onPositionChange(qint64 pos);
    void repeatAChanged(const QTime& t);
    void repeatBChanged(const QTime& t);

    void onTimeSliderHover(int pos, int value);
    void onTimeSliderLeave();
    void handleError(const QtAV::AVError& e);
    void onMediaStatusChanged();

    void onVideoEQEngineChanged();
    void onBrightnessChanged(int b);
    void onContrastChanged(int c);
    void onHueChanged(int h);
    void onSaturationChanged(int s);

    void onCaptureConfigChanged();

    void donate();
    void setup();

    void handleFullscreenChange();

protected:
    virtual void closeEvent(QCloseEvent *e);
    virtual void resizeEvent(QResizeEvent *);
    virtual void timerEvent(QTimerEvent *);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void wheelEvent(QWheelEvent *e);

private:
    void workaroundRendererSize();

private:
    bool mIsReady, mHasPendingPlay;
    bool mNullAO;
    bool mControlOn;
    int mCursorTimer;
    int mShowControl; //0: can hide, 1: show and playing, 2: always show(not playing)
    int mRepeateMax;
    QVBoxLayout *mpPlayerLayout;

    QWidget *mpControl;
    QLabel *mpCurrent, *mpDuration;
    QLabel *mpTitle;
    QLabel *mpSpeed;
    Slider *mpTimeSlider, *mpVolumeSlider;
    Button *mpVolumeBtn;
    Button *mpPlayPauseBtn, *mpStopBtn, *mpForwardBtn, *mpBackwardBtn;
    Button *mpOpenBtn;
    Button *mpInfoBtn, *mpMenuBtn, *mpSetupBtn, *mpCaptureBtn;
    QMenu *mpMenu;
    QAction *mpVOAction, *mpARAction; //remove mpVOAction if vo.id() is supported
    QAction *mpRepeatEnableAction;
    QWidgetAction *mpRepeatAction;
    QSpinBox *mpRepeatBox;
    QTimeEdit *mpRepeatA, *mpRepeatB;
    QAction *mpAudioTrackAction;
    QMenu *mpAudioTrackMenu;
    QMenu *mpChannelMenu;
    QAction *mpChannelAction;
    QList<QAction*> mVOActions;

    QtAV::AVClock *mpClock;
    QtAV::AVPlayer *mpPlayer;
    QtAV::VideoRenderer *mpRenderer, *mpTempRenderer;
    QString mFile;
    QString mTitle;
    QPixmap mPlayPixmap;
    QPixmap mPausePixmap;

    QLabel *mpPreview;

    DecoderConfigPage *mpDecoderConfigPage;
    VideoEQConfigPage *mpVideoEQ;

    PlayList *mpPlayList, *mpHistory;

    QPointF mGlobalMouse;
};


#endif // MAINWINDOW_H
