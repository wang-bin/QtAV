#include "videoplayer.h"
#include <QApplication>

#include <QtCore/QLocale>
#include <QtCore/QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator ts;
    if (ts.load(qApp->applicationDirPath().append(QLatin1String("/i18n/QtAV_")).append(QLocale::system().name())))
        a.installTranslator(&ts);
    QTranslator qtts;
    if (qtts.load(QString::fromLatin1("qt_") + QLocale::system().name()))
        a.installTranslator(&qtts);

    VideoPlayer w;
    w.show();
    if (a.arguments().size() > 1)
        w.play(a.arguments().last());
    
    return a.exec();
}
