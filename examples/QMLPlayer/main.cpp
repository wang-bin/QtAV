#include <cmath>
#include <QtCore/QFile>
#include <QtGui/QGuiApplication>
#include <QQuickItem>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtGui/QScreen>
#include "qtquick2applicationviewer.h"
#include "../common/ScreenSaver.h"
#include "../common/common.h"

int main(int argc, char *argv[])
{
    QOptions options(get_common_options());
    options.add("QMLPlayer options")
            ("scale", 1.0, "scale of graphics context. 0: auto")
            ;
    options.parse(argc, argv);
    if (options.value("help").toBool()) {
        options.print();
        return 0;
    }

    QGuiApplication app(argc, argv);
    load_qm(QStringList() << "QMLPlayer", options.value("language").toString());
    QtQuick2ApplicationViewer viewer;
    viewer.engine()->rootContext()->setContextProperty("PlayerConfig", &Config::instance());
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
    float sr = options.value("scale").toFloat();
    if (qFuzzyIsNull(sr))
        sr = r;
    viewer.engine()->rootContext()->setContextProperty("scaleRatio", sr);
    QString qml = "qml/QMLPlayer/main.qml";
    if (QFile(qApp->applicationDirPath() + "/" + qml).exists())
        qml.prepend(qApp->applicationDirPath() + "/");
    else
        qml.prepend("qrc:///");
    viewer.setMainQmlFile(qml);
    viewer.showExpanded();
    QOption op = options.option("width");
    if (op.isSet())
        viewer.setWidth(op.value().toInt());
    op = options.option("height");
    if (op.isSet())
        viewer.setHeight(op.value().toInt());
    op = options.option("x");
    if (op.isSet())
        viewer.setX(op.value().toInt());
    op = options.option("y");
    if (op.isSet())
        viewer.setY(op.value().toInt());
    if (options.value("fullscreen").toBool())
        viewer.showFullScreen();

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
        QString file = options.value("file").toString();
        if (file.isEmpty()) {
            if (argc > 1 && !app.arguments().last().startsWith('-') && !app.arguments().at(argc-2).startsWith('-'))
                file = app.arguments().last();
        }
        if (player && !file.isEmpty()) {
            if (QFile(file).exists())
                file.prepend("file:"); //qml use url and will add qrc: if no scheme
            file.replace("\\", "/"); //qurl
            QMetaObject::invokeMethod(player, "play", Q_ARG(QUrl, QUrl(file)));
        }
    }
#endif
    QObject::connect(viewer.rootObject(), SIGNAL(requestFullScreen()), &viewer, SLOT(showFullScreen()));
    QObject::connect(viewer.rootObject(), SIGNAL(requestNormalSize()), &viewer, SLOT(showNormal()));
    ScreenSaver::instance().disable(); //restore in dtor
    return app.exec();
}
