#include <QtQml/QQmlExtensionPlugin>
#include <QtQml/qqml.h>
#include <QmlAV/QQuickItemRenderer.h>

namespace QtAV {

class QtAVQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri)
    {
        Q_ASSERT(uri == QLatin1String("QtAV"));
        qmlRegisterType<QQuickItemRenderer>(uri, 1, 0, "QQuickItemRenderer");
    }
};
} //namespace QtAV

#include "plugin.moc"
