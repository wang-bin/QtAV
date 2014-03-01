#include <QApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtAV/QtAV.h>
#include <QFile>
#include <QBuffer>

using namespace QtAV;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile vidfile(a.arguments().last());

    if (!vidfile.open(QIODevice::ReadOnly))
        return 1;

    AVPlayer player[2];
    WidgetRenderer renderer[2];
    renderer[0].show();
    renderer[0].widget()->setWindowTitle("Test QFile");
    renderer[1].show();
    renderer[1].widget()->setWindowTitle("Test QBuffer. Play <=1M video from memory");
    player[0].addVideoRenderer(&renderer[0]);
    player[1].addVideoRenderer(&renderer[1]);

    QByteArray data = vidfile.read(1024*1024);
    vidfile.seek(0);
    QBuffer buf(&data);
    if (buf.open(QIODevice::ReadOnly)) {
        player[1].setIODevice(&buf);
    }
    player[0].setIODevice(&vidfile);
    player[0].play();
    player[1].play();

    return a.exec();
}
