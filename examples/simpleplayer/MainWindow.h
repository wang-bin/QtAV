#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

namespace QtAV {
class AVPlayer;
class AVClock;
class VideoRenderer;
}

class QLabel;
class QSlider;
class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    void setRenderer(QtAV::VideoRenderer* renderer);
    void play(const QString& name);
    
private slots:
    void onStartPlay();
    void onPaused(bool p);
    void seekToMSec(int msec);
    void seek();

protected:
    virtual void resizeEvent(QResizeEvent *);
    virtual void timerEvent(QTimerEvent *);

private:
    int mTimerId;
    //QLabel *mpTitle
    QSlider *mpTimeSlider;//, *mpVolumeSlider;
    QtAV::AVClock *mpClock;
    QtAV::AVPlayer *mpPlayer;
    QtAV::VideoRenderer *mpRenderer;
    QString mFile;
};

#endif // MAINWINDOW_H
