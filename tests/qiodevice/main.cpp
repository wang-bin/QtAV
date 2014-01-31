#include <QApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtAV/QtAV.h>
#include <QFile>

using namespace QtAV;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile vidfile(a.arguments().last());
    AVPlayer player;
    WidgetRenderer renderer;

    if (vidfile.open(QIODevice::ReadOnly))
    {
        renderer.show();
        player.addVideoRenderer(&renderer);
        player.setIODevice(&vidfile);
        player.play();
    }

    return a.exec();
}
