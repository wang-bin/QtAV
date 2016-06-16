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
    qDebug() << "help: ./subtitle [-engine engine] [-f file] [-fuzzy] [-t sec] [-t1 sec] [-count n]";
    qDebug() << "-fuzzy: fuzzy match subtitle name";
    qDebug() << "-t: set subtitle begin time";
    qDebug() << "-t1: set subtitle end time";
    qDebug() << "-count: set subtitle frame count from t to t1";
    qDebug() << "-engine: subtitle processing engine, can be 'ffmpeg' and 'libass'";
    qDebug() << "-dir: add subtitle search directories";
    QString file;
    bool fuzzy = false;
    int t = -1, t1 = -1, count = 1;
    int i = a.arguments().indexOf(QLatin1String("-f"));
    if (i > 0) {
        file = a.arguments().at(i+1);
    }
    i = a.arguments().indexOf(QLatin1String("-fuzzy"));
    if (i > 0)
        fuzzy = true;
    i = a.arguments().indexOf(QLatin1String("-t"));
    if (i > 0)
        t = a.arguments().at(i+1).toInt();
    i = a.arguments().indexOf(QLatin1String("-t1"));
    if (i > 0)
        t1 = a.arguments().at(i+1).toInt();
    i = a.arguments().indexOf(QLatin1String("-count"));
    if (i > 0)
        count = a.arguments().at(i+1).toInt();
    QString engine;
    i = a.arguments().indexOf(QLatin1String("-engine"));
    if (i > 0)
        engine = a.arguments().at(i+1);

    QStringList dirs;
    i = a.arguments().indexOf(QLatin1String("-dir"));
    while (i > 0) {
        dirs += a.arguments().at(i+1).split(QLatin1Char(';'));
        i = a.arguments().indexOf(QLatin1String("-dir"), i+2);
    }

    Subtitle sub;
    if (!engine.isEmpty())
        sub.setEngines(QStringList() << engine);
    qDebug() << "supported extensions: " << sub.supportedSuffixes();

    if (file.isEmpty())
        return 0;

    sub.setFileName(file);
    sub.setDirs(dirs);
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
    if (t < 0 && t1 >= 0) {
        t = 0;
        count = 1;
    }
    if (t >= 0) {
        if (t1 <= t) {
            sub.setTimestamp(qreal(t));
            qDebug() << sub.timestamp() << "s: " << sub.getText();
            QImage img(sub.getImage(720, 400));
            img.save(QString::fromLatin1("sub-%1.png").arg(sub.timestamp(), 0, 'f', 2));
        } else {
            if (count < 2)
                count = 2;
            const qreal kInterval = (t1-t)/qreal(count-1);
            for (int n = 0; n < count; ++n) {
                sub.setTimestamp(qreal(t) + qreal(n)*kInterval);
                qDebug() << sub.timestamp() << "s: " << sub.getText();
                QImage img(sub.getImage(720, 400));
                img.save(QString::fromLatin1("sub-%1.png").arg(sub.timestamp(), 0, 'f', 2));
            }
        }
    }
    qDebug() << "find subtitle content elapsed: " << timer.elapsed() << "ms";

    return 0;
}

#include "main.moc"
