#include "VUMeterFilter.h"

#include <QtGui/QGuiApplication>
#include <QQuickItem>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlApplicationEngine>
int main(int argc, char** argv)
{
    qmlRegisterType<VUMeterFilter>("com.qtav.vumeter", 1, 0, "VUMeterFilter");
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
