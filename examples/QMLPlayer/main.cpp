#include <QtGui/QGuiApplication>

#include "qtquick2applicationviewer.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QtQuick2ApplicationViewer viewer;
    QString qml = "qml/QMLPlayer/main.qml";
    if (!QFile(qml).exists())
        qml.prepend("qrc:///");
    viewer.setMainQmlFile(qml);
    viewer.showExpanded();
    viewer.setTitle("QMLPlayer based on QtAV. wbsecg1@gmail.com");
    return app.exec();
}
