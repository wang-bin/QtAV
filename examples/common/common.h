#ifndef COMMON_H
#define COMMON_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include "qoptions.h"
#include "Config.h"
#include "ScreenSaver.h"

QOptions COMMON_EXPORT get_common_options();
void COMMON_EXPORT load_qm(const QStringList& names, const QString &lang = "system");
// if appname ends with 'desktop', 'es', 'angle', software', 'sw', set by appname. otherwise set by command line option glopt
void COMMON_EXPORT set_opengl_backend(const QString& glopt = "auto", const QString& appname = QString());

class COMMON_EXPORT AppEventFilter : public QObject
{
public:
    AppEventFilter(QObject *player = 0, QObject* parent = 0);
    QUrl url() const;
    virtual bool eventFilter(QObject *obj, QEvent *ev);
private:
    QObject *m_player;
};

#endif // COMMON_H
