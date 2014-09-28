#include <QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include <QtDebug>
#include <QtCore/QTime>
#include <QtAV/Subtitle.h>

using namespace QtAV;

class SubtitleObserver : public QObject
{
    Q_OBJECT
public:
    SubtitleObserver(QObject* parent = 0) : QObject(parent) {}
    void observe(Subtitle* sub) { connect(sub, SIGNAL(contentChanged()), this, SLOT(onSubtitleChanged()));}
private slots:
    void onSubtitleChanged() {
        Subtitle *sub = qobject_cast<Subtitle*>(sender());
        qDebug() << "subtitle changed at " << sub->timestamp() << "s\n" << sub->getText();
    }
};

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
    QString engine;
    i = a.arguments().indexOf("-engine");
    if (i > 0)
        engine = a.arguments().at(i+1);

    Subtitle sub;
    if (!engine.isEmpty())
        sub.setEngines(QStringList() << engine);
    sub.setFileName(file);
    sub.setFuzzyMatch(fuzzy);
    SubtitleObserver sob;
    sob.observe(&sub);
    QElapsedTimer timer;
    timer.start();
    sub.load();
    if (!sub.isLoaded())
        return -1;
    qDebug() << "process subtitle file elapsed: " << timer.elapsed() << "ms";
    timer.restart();
    if (t >= 0) {
        for (int n = 0; n < 4; ++n) {
            sub.setTimestamp(qreal(t+n*1000)/1000.0);
            qDebug() << sub.timestamp() << "s: " << sub.getText();
        }
        for (int n = 0; n < 4; ++n) {
            sub.setTimestamp(qreal(t-n*1000)/1000.0);
            qDebug() << sub.timestamp() << "s: " << sub.getText();
        }
        //QImage img(sub.getImage(800, 600));
        //img.save("sub.png");
    }
    qDebug() << "find subtitle content elapsed: " << timer.elapsed() << "ms";

    return 0;
}

#include "main.moc"
