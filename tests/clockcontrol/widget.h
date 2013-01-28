#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QSlider>

namespace QtAV {
class WidgetRenderer;
class AVPlayer;
class AVClock;
}
class Widget : public QWidget
{
    Q_OBJECT
    
public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    
public slots:
    void chooseVideo();

    void sendClock();
    void seekVideo(int);
protected slots:
    void slotStarted();
protected:
    virtual void timerEvent(QTimerEvent *);

private:
    class
    QtAV::WidgetRenderer *renderer;
    QtAV::AVPlayer *player;
    QtAV::AVClock *clock;
    QPushButton *send_btn, *file_btn, *play_btn, *pause_btn, *stop_btn;
    QSlider *slider;
    QSpinBox *time_box;
    QString vfile;
};

#endif // WIDGET_H
