/******************************************************************************
    Simple Player:  this file is part of QtAV examples
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QGuiApplication>
#include <QQuickItem>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtGui/QScreen>
#include <QTouchDevice>
#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#endif
#include "qtquick2applicationviewer.h"
#include "../common/ScreenSaver.h"
#include "../common/common.h"

int main(int argc, char *argv[])
{
    //call qtav api result in crash at startup
    QOptions options(get_common_options());
    options.add(QLatin1String("QMLPlayer options"))
            ("scale", 1.0, QLatin1String("scale of graphics context. 0: auto"))
            ;
    qputenv("QTAV_MEDIACODEC_KEY", ")A0aZ!WIgo2DdCsc#EnR?NAsFtWrnENONeeiiED^OM@6gI+Hew6s5)77p^>$K(Fe");
    options.parse(argc, argv);
    Config::setName(QString::fromLatin1("QMLPlayer"));
    do_common_options_before_qapp(options);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QMLPlayer"));
    app.setApplicationDisplayName(QStringLiteral("QtAV QMLPlayer"));
    QDir::setCurrent(qApp->applicationDirPath());
qDebug() <<  "event dispatcher:" << QCoreApplication::eventDispatcher();
;
    do_common_options(options);
    qDebug() << "arguments======= " << app.arguments();
    qDebug() << "current dir: " << QDir::currentPath();
    set_opengl_backend(options.option(QStringLiteral("gl")).value().toString(), app.arguments().first());
    load_qm(QStringList() << QStringLiteral("QMLPlayer"), options.value(QStringLiteral("language")).toString());
    QtQuick2ApplicationViewer viewer;
    QString binDir = qApp->applicationDirPath();
    if (binDir.endsWith(QLatin1String(".app/Contents/MacOS"))) {
        binDir.remove(QLatin1String(".app/Contents/MacOS"));
        binDir = binDir.left(binDir.lastIndexOf(QLatin1String("/")));
    }
    QQmlEngine *engine = viewer.engine();
    if (!engine->importPathList().contains(binDir))
        engine->addImportPath(binDir);
    qDebug() << engine->importPathList();
    engine->rootContext()->setContextProperty(QStringLiteral("PlayerConfig"), &Config::instance());
    qDebug(">>>>>>>>devicePixelRatio: %f", qApp->devicePixelRatio());
    QScreen *sc = app.primaryScreen();
    qDebug() << "dpi phy: " << sc->physicalDotsPerInch() << ", logical: " << sc->logicalDotsPerInch() << ", dpr: " << sc->devicePixelRatio()
                << "; vis rect:" << sc->virtualGeometry();
    // define a global var for js and qml
    engine->rootContext()->setContextProperty(QStringLiteral("screenPixelDensity"), sc->physicalDotsPerInch()*sc->devicePixelRatio());
    qreal r = sc->physicalDotsPerInch()/sc->logicalDotsPerInch();
    const qreal kR =
#if defined(Q_OS_ANDROID)
            2.0;
#elif defined(Q_OS_WINRT)
            1.2;
#else
            1.0;
#endif
    if (std::isinf(r) || std::isnan(r))
        r = kR;
    float sr = options.value(QStringLiteral("scale")).toFloat();
#if defined(Q_OS_ANDROID) || defined(Q_OS_WINRT)
    sr = r;
#if defined(Q_OS_WINPHONE)
    sr = kR;
#endif
    if (sr > 2.0)
        sr = 2.0; //FIXME
#endif
    if (qFuzzyIsNull(sr))
        sr = r;
    engine->rootContext()->setContextProperty(QStringLiteral("scaleRatio"), sr);
    qDebug() << "touch devices: " << QTouchDevice::devices();
    engine->rootContext()->setContextProperty(QStringLiteral("isTouchScreen"), false);
#ifdef Q_OS_WINPHONE
    engine->rootContext()->setContextProperty(QStringLiteral("isTouchScreen"), true);
#endif
    foreach (const QTouchDevice* dev, QTouchDevice::devices()) {
        if (dev->type() == QTouchDevice::TouchScreen) {
            engine->rootContext()->setContextProperty(QStringLiteral("isTouchScreen"), true);
            break;
        }
    }
    QString qml = QStringLiteral("qml/QMLPlayer/main.qml");
    if (QFile(qApp->applicationDirPath() + QLatin1String("/") + qml).exists())
        qml.prepend(qApp->applicationDirPath() + QLatin1String("/"));
    else
        qml.prepend(QLatin1String("qrc:///"));
    viewer.setMainQmlFile(qml);
    viewer.show();
    QOption op = options.option(QStringLiteral("width"));
    if (op.isSet())
        viewer.setWidth(op.value().toInt());
    op = options.option(QStringLiteral("height"));
    if (op.isSet())
        viewer.setHeight(op.value().toInt());
    op = options.option(QStringLiteral("x"));
    if (op.isSet())
        viewer.setX(op.value().toInt());
    op = options.option(QStringLiteral("y"));
    if (op.isSet())
        viewer.setY(op.value().toInt());
    if (options.value(QStringLiteral("fullscreen")).toBool())
        viewer.showFullScreen();
    viewer.setTitle(QStringLiteral("QMLPlayer based on QtAV. wbsecg1@gmail.com"));
    /*
     * find root item, then root.init(argv). so we can deal with argv in qml
     */
#if 1
    QString json = app.arguments().join(QStringLiteral("\",\""));
    json.prepend(QLatin1String("[\"")).append(QLatin1String("\"]"));
    json.replace(QLatin1String("\\"), QLatin1String("/")); //FIXME
    QMetaObject::invokeMethod(viewer.rootObject(), "init", Q_ARG(QVariant, json));
//#else
    QObject *player = viewer.rootObject()->findChild<QObject*>(QStringLiteral("player"));
    AppEventFilter *ae = new AppEventFilter(player, player);
    qApp->installEventFilter(ae);
    QString file;
#ifdef Q_OS_ANDROID
    file = QAndroidJniObject::callStaticObjectMethod("org.qtav.qmlplayer.QMLPlayerActivity"
                                                                              , "getUrl"
                                                                              , "()Ljava/lang/String;")
            .toString();
#endif
    if (app.arguments().size() > 1) {
        file = options.value(QStringLiteral("file")).toString();
        if (file.isEmpty()) {
            if (argc > 1 && !app.arguments().last().startsWith(QLatin1Char('-')) && !app.arguments().at(argc-2).startsWith(QLatin1Char('-')))
                file = app.arguments().last();
        }
    }
    qDebug() << "file: " << file;
    if (player && !file.isEmpty()) {
        if (!file.startsWith(QLatin1String("file:")) && QFile(file).exists())
            file.prepend(QLatin1String("file:")); //qml use url and will add qrc: if no scheme
#ifdef Q_OS_WIN
        file.replace(QLatin1String("\\"), QLatin1String("/")); //qurl
#endif
        //QMetaObject::invokeMethod(player, "play", Q_ARG(QUrl, QUrl(file)));
        QUrl url;
        player->setProperty("source", QUrl(file.toUtf8().toPercentEncoding()));
    }
#endif
    QObject::connect(&Config::instance(), SIGNAL(changed()), &Config::instance(), SLOT(save()));
    QObject::connect(viewer.rootObject(), SIGNAL(requestFullScreen()), &viewer, SLOT(showFullScreen()));
    QObject::connect(viewer.rootObject(), SIGNAL(requestNormalSize()), &viewer, SLOT(showNormal()));
    ScreenSaver::instance().disable(); //restore in dtor
    return app.exec();
}
