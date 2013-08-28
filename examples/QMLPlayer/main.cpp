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
    viewer.setMainQmlFile(QStringLiteral("qml/QMLPlayer/main.qml"));
    viewer.showExpanded();

    AVPlayer player;
    QQuickItemRenderer* renderer = viewer.rootObject()->findChild<QQuickItemRenderer*>("quickItemRenderer");

    player.setRenderer(renderer);

    return app.exec();
}
