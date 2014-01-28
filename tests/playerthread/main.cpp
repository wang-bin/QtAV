#include <QApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtAV/QtAV.h>

using namespace QtAV;
class Thread : public QThread
{
public:
    Thread(AVPlayer *player):
        QThread(0)
      , mpPlayer(player)
    {}
protected:
    virtual void run() {
        //mpPlayer->play();
        exec();
    }
    AVPlayer *mpPlayer;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    AVPlayer player;
    WidgetRenderer renderer;
    renderer.show();
    player.addVideoRenderer(&renderer);
    player.setFile(a.arguments().last());
    Thread thread(&player);
    player.moveToThread(&thread);
    thread.start();
    player.play();

    return a.exec();
}
