#include "videoplayer.h"
#include <QApplication>

#include <QtCore/QLocale>
#include <QtCore/QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator ts;
    if (ts.load(qApp->applicationDirPath() + "/i18n/QtAV_" + QLocale::system().name()))
        a.installTranslator(&ts);
    QTranslator qtts;
    if (qtts.load("qt_" + QLocale::system().name()))
        a.installTranslator(&qtts);

    VideoPlayer w;
    w.show();
    if (a.arguments().size() > 1)
        w.play(a.arguments().last());
    
    return a.exec();
}
