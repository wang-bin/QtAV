#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QPushButton>

namespace QtAV {
class WidgetRenderer;
class AVPlayer;
}
class Widget : public QWidget
{
    Q_OBJECT
    
public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    
public slots:
    void setVideo();
    void playVideo();
    void testRTSP();

private:
    class
    QtAV::WidgetRenderer *renderer;
    QtAV::AVPlayer *player[2];
    QPushButton *play_btn[2];
    QPushButton *file_btn[2];
};

#endif // WIDGET_H
