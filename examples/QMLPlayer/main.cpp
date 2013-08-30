#include <QtCore/QFile>
#include <QtGui/QGuiApplication>
#include <QtAV/QQuickItemRenderer.h>
#include <QtAV/AVPlayer.h>

#include "qtquick2applicationviewer.h"

using namespace QtAV;
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QtQuick2ApplicationViewer viewer;
    qmlRegisterType<QQuickItemRenderer>("QtAV", 1, 0, "QQuickItemRenderer");
    QString qml = "qml/QMLPlayer/main.qml";
    if (!QFile(qml).exists())
        qml.prepend("qrc:///");
    viewer.setMainQmlFile(qml);
    viewer.showExpanded();

    AVPlayer player;
    QQuickItemRenderer* renderer = viewer.rootObject()->findChild<QQuickItemRenderer*>("quickItemRenderer");

    player.setRenderer(renderer);
    if (argc > 1)
        player.play(app.arguments().last());
    return app.exec();
}
