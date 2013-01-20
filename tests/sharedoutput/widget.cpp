#include "widget.h"
#include <QtAV/AVPlayer.h>
#include <QtAV/WidgetRenderer.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>

using namespace QtAV;

Widget::Widget(QWidget *parent) :
    QWidget(parent)
{
    setWindowTitle("A test for shared video renderer. QtAV" QTAV_VERSION_STR_LONG " wbsecg1@gmail.com");
    QVBoxLayout *main_layout = new QVBoxLayout;
    QHBoxLayout *btn_layout = new QHBoxLayout;
    renderer = new WidgetRenderer;
    renderer->setFocusPolicy(Qt::StrongFocus);
    renderer->resizeVideo(640, 480);
    for (int i = 0; i < 2; ++i) {
        player[i] = new AVPlayer(this);
        player[i]->setRenderer(renderer);
        QVBoxLayout *vb = new QVBoxLayout;
        play_btn[i] = new QPushButton(this);
        play_btn[i]->setText(QString("Play-%1").arg(i));
        file_btn[i] = new QPushButton(this);
        file_btn[i]->setText(QString("Choose video-%1").arg(i));
        connect(play_btn[i], SIGNAL(clicked()), SLOT(playVideo()));
        connect(file_btn[i], SIGNAL(clicked()), SLOT(setVideo()));
        vb->addWidget(play_btn[i]);
        vb->addWidget(file_btn[i]);
        btn_layout->addLayout(vb);
    }
    main_layout->addWidget(renderer);
    main_layout->addLayout(btn_layout);
    setLayout(main_layout);
    resize(720, 600);
}

Widget::~Widget()
{
}

void Widget::playVideo()
{
    for (int i = 0; i < 2; ++i)
        player[i]->pause(true);
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    int idx = btn->text().section('-', 1).toInt();
    player[idx]->pause(false);
}

void Widget::setVideo()
{
    QString v = QFileDialog::getOpenFileName(0, "Select a video");
    if (v.isEmpty())
        return;

    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    int idx = btn->text().section('-', 1).toInt();
    QString oldv = player[idx]->file();
    if (v == oldv)
        return;
    for (int i = 0; i < 2; ++i)
        player[i]->pause(true);
    player[idx]->stop();
    player[idx]->setFile(v);
    player[idx]->play();
}
