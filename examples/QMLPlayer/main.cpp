/******************************************************************************
    Simple Player:  this file is part of QtAV examples
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include <cmath>
#include <QtCore/QtDebug>
#include <QtCore/QFile>
#include <QtGui/QGuiApplication>
#include <QQuickItem>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtGui/QScreen>
#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#endif
#include "qtquick2applicationviewer.h"
#include "../common/ScreenSaver.h"
#include "../common/common.h"

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN32)
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif
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
    qDebug() << "arguments======= " << app.arguments();
    set_opengl_backend(options.option("gl").value().toString(), app.arguments().first());
    load_qm(QStringList() << "QMLPlayer", options.value("language").toString());
    QtQuick2ApplicationViewer viewer;
    QString binDir = qApp->applicationDirPath();
    if (binDir.endsWith(".app/Contents/MacOS")) {
        binDir.remove(".app/Contents/MacOS");
        binDir = binDir.left(binDir.lastIndexOf("/"));
    }
    QQmlEngine *engine = viewer.engine();
    if (!engine->importPathList().contains(binDir))
        engine->addImportPath(binDir);
    qDebug() << engine->importPathList();
    engine->rootContext()->setContextProperty("PlayerConfig", &Config::instance());
    qDebug(">>>>>>>>devicePixelRatio: %f", qApp->devicePixelRatio());
    QScreen *sc = app.primaryScreen();
    qDebug() << "dpi phy: " << sc->physicalDotsPerInch() << ", logical: " << sc->logicalDotsPerInch() << ", dpr: " << sc->devicePixelRatio()
                << "; vis rect:" << sc->virtualGeometry();
    // define a global var for js and qml
    engine->rootContext()->setContextProperty("screenPixelDensity", qApp->primaryScreen()->physicalDotsPerInch()*qApp->primaryScreen()->devicePixelRatio());
    qreal r = sc->physicalDotsPerInch()/sc->logicalDotsPerInch();
    if (std::isinf(r) || std::isnan(r))
#if defined(Q_OS_ANDROID)
        r = 2.0;
#else
        r = 1.0;
#endif
    float sr = options.value("scale").toFloat();
#if defined(Q_OS_ANDROID)
    sr = r;
    if (sr > 2.0)
        sr = 2.0; //FIXME
#endif
    if (qFuzzyIsNull(sr))
        sr = r;
    engine->rootContext()->setContextProperty("scaleRatio", sr);
    QString qml = "qml/QMLPlayer/main.qml";
    if (QFile(qApp->applicationDirPath() + "/" + qml).exists())
        qml.prepend(qApp->applicationDirPath() + "/");
    else
        qml.prepend("qrc:///");
    viewer.setMainQmlFile(qml);
    viewer.show();
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
    QObject *player = viewer.rootObject()->findChild<QObject*>("player");
    if (player) {
        AppEventFilter *ae = new AppEventFilter(player, player);
        qApp->installEventFilter(ae);
    }
    QString file;
#ifdef Q_OS_ANDROID
    file = QAndroidJniObject::callStaticObjectMethod("org.qtav.qmlplayer.QMLPlayerActivity"
                                                                              , "getUrl"
                                                                              , "()Ljava/lang/String;")
            .toString();
#endif
    if (app.arguments().size() > 1) {
        file = options.value("file").toString();
        if (file.isEmpty()) {
            if (argc > 1 && !app.arguments().last().startsWith('-') && !app.arguments().at(argc-2).startsWith('-'))
                file = app.arguments().last();
        }
    }
    qDebug() << "file: " << file;
    if (player && !file.isEmpty()) {
        if (!file.startsWith("file:") && QFile(file).exists())
            file.prepend("file:"); //qml use url and will add qrc: if no scheme
        file.replace("\\", "/"); //qurl
        QMetaObject::invokeMethod(player, "play", Q_ARG(QUrl, QUrl(file)));
    }
#endif
    QObject::connect(viewer.rootObject(), SIGNAL(requestFullScreen()), &viewer, SLOT(showFullScreen()));
    QObject::connect(viewer.rootObject(), SIGNAL(requestNormalSize()), &viewer, SLOT(showNormal()));
    ScreenSaver::instance().disable(); //restore in dtor
    return app.exec();
}
