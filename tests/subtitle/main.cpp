#include <QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include <QtDebug>
#include <QtCore/QTime>
#include <QtAV/Subtitle.h>

using namespace QtAV;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qDebug() << "help: ./subtitle [-f file] [-fuzzy] [-t msec]";
    QString file = "test.srt";
    bool fuzzy = false;
    int t = -1;
    int i = a.arguments().indexOf("-f");
    if (i > 0) {
        file = a.arguments().at(i+1);
    }
    i = a.arguments().indexOf("-fuzzy");
    if (i > 0)
        fuzzy = true;
    i = a.arguments().indexOf("-t");
    if (i > 0)
        t = a.arguments().at(i+1).toInt();

    Subtitle sub;
    sub.setFileName(file);
    sub.setFuzzyMatch(fuzzy);
    QElapsedTimer timer;
    timer.start();
    if (!sub.start())
        return -1;
    qDebug() << "process subtitle file elapsed: " << timer.elapsed() << "ms";
    timer.restart();
    if (t >= 0) {
        for (int n = 0; n < 4; ++n) {
            sub.setTimestamp(qreal(t+n*1000)/1000.0);
            qDebug() << "sub at time " << sub.timestamp() << "s: " << sub.getText();
        }
        for (int n = 0; n < 4; ++n) {
            sub.setTimestamp(qreal(t-n*1000)/1000.0);
            qDebug() << "sub at time " << sub.timestamp() << ": " << sub.getText();
        }
        //QImage img(sub.getImage(800, 600));
        //img.save("sub.png");
    }
    qDebug() << "find subtitle content elapsed: " << timer.elapsed() << "ms";

    return 0;
}
