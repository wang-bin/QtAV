#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

namespace QtAV {
class AVPlayer;
class AVClock;
class VideoRenderer;
}

class QVBoxLayout;
class QLabel;
class QPushButton;
class Slider;
class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    void setRenderer(QtAV::VideoRenderer* renderer);
    void play(const QString& name);
    
public slots:
    void openFile();
    void togglePlayStop();

private slots:
    void onStartPlay();
    void onStopPlay();
    void onPaused(bool p);
    void seekToMSec(int msec);
    void seek();

protected:
    virtual void resizeEvent(QResizeEvent *);
    virtual void timerEvent(QTimerEvent *);

private:
    int mTimerId;
    QVBoxLayout *mpPlayerLayout;
    //QLabel *mpTitle
    Slider *mpTimeSlider;//, *mpVolumeSlider;
    QPushButton *mpPlayStopBtn, *mpPauseBtn, *mpForwardBtn, *mpBackwardBtn;
    QPushButton *mpOpenBtn;
    QtAV::AVClock *mpClock;
    QtAV::AVPlayer *mpPlayer;
    QtAV::VideoRenderer *mpRenderer;
    QString mFile;
};

#endif // MAINWINDOW_H
