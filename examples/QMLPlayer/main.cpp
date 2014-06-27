#include <cmath>
#include <QtGui/QGuiApplication>
#include <QQuickItem>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtGui/QScreen>
#include "qtquick2applicationviewer.h"
#include "../common/ScreenSaver.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QtQuick2ApplicationViewer viewer;
    qDebug(">>>>>>>>devicePixelRatio: %f", qApp->devicePixelRatio());
    QScreen *sc = app.primaryScreen();
    qDebug() << "dpi phy: " << sc->physicalDotsPerInch() << ", logical: " << sc->logicalDotsPerInch() << ", dpr: " << sc->devicePixelRatio()
                << "; vis rect:" << sc->virtualGeometry();
    // define a global var for js and qml
    viewer.engine()->rootContext()->setContextProperty("screenPixelDensity", qApp->primaryScreen()->physicalDotsPerInch()*qApp->primaryScreen()->devicePixelRatio());
    qreal r = sc->physicalDotsPerInch()/sc->logicalDotsPerInch();
    if (std::isinf(r) || std::isnan(r))
#if defined(Q_OS_ANDROID)
        r = 2.0;
#else
        r = 1.0;
#endif
    viewer.engine()->rootContext()->setContextProperty("scaleRatio", r);
    QString qml = "qml/QMLPlayer/main.qml";
    if (QFile(qApp->applicationDirPath() + "/" + qml).exists())
        qml.prepend(qApp->applicationDirPath() + "/");
    else
        qml.prepend("qrc:///");
    viewer.setMainQmlFile(qml);
    viewer.showExpanded();
    viewer.setTitle("QMLPlayer based on QtAV. wbsecg1@gmail.com");
    /*
     * find root item, then root.init(argv). so we can deal with argv in qml
     */
#if 1
    QString json = app.arguments().join("\",\"");
    json.prepend("[\"").append("\"]");
    json.replace("\\", "/"); //FIXME
    QMetaObject::invokeMethod(viewer.rootObject(), "init", Q_ARG(QVariant, json));
//#else
    if (app.arguments().size() > 1) {
        qDebug("arguments > 1");
        QObject *player = viewer.rootObject()->findChild<QObject*>("player");
        if (player) {
            QString file = app.arguments().last();
            file.replace("\\", "/"); //qurl
            qDebug("arguments > 1, file: %s", qPrintable(file));
            QMetaObject::invokeMethod(player, "play", Q_ARG(QUrl, QUrl(file)));
        }
    }
#endif
    QObject::connect(viewer.rootObject(), SIGNAL(requestFullScreen()), &viewer, SLOT(showFullScreen()));
    QObject::connect(viewer.rootObject(), SIGNAL(requestNormalSize()), &viewer, SLOT(showNormal()));
    ScreenSaver::instance().disable(); //restore in dtor
    return app.exec();
}
