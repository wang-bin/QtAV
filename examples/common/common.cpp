#include "common.h"

#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtCore/QCoreApplication>
#include <QtDebug>

void _link_hack()
{

}

QOptions get_common_options()
{
    static QOptions ops = QOptions().addDescription("Options for QtAV players")
            .add("common options")
            ("help,h", "print this")
            ("x", 0, "")
            ("y", 0, "y")
            ("-width", 800, "width of player")
            ("height", 450, "height of player")
            ("fullscreen", "fullscreen")
            ("deocder", "FFmpeg", "use a given decoder")
            ("decoders,-vd", "cuda;vaapi;vda;dxva;cedarv;ffmpeg", "decoder name list in priority order seperated by ';'")
            ("file,f", "", "file or url to play")
            ("language", "system", "language on UI. can be 'system', 'none' and locale name e.g. zh_CN")
            ;
    return ops;
}

void load_qm(const QStringList &names, const QString& lang)
{
    if (lang.isEmpty() || lang.toLower() == "none")
        return;
    QString l(lang);
    if (l.toLower() == "system")
        l = QLocale::system().name();
    QStringList qms(names);
    qms << "QtAV" << "qt";
    foreach(QString qm, qms) {
        QTranslator *ts = new QTranslator(qApp);
        QString path = qApp->applicationDirPath() + "/i18n/" + qm + "_" + l;
        //qDebug() << "loading qm: " << path;
        if (ts->load(path)) {
            qApp->installTranslator(ts);
        } else {
            path = ":/i18n/" + qm + "_" + l;
            //qDebug() << "loading qm: " << path;
            if (ts->load(path))
                qApp->installTranslator(ts);
            else
                delete ts;
        }
    }
    QTranslator qtts;
    if (qtts.load("qt_" + QLocale::system().name()))
        qApp->installTranslator(&qtts);
}
