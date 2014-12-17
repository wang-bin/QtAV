#ifndef COMMON_H
#define COMMON_H

#include <QtCore/QStringList>
#include "qoptions.h"
#include "Config.h"
#include "ScreenSaver.h"

extern "C" {
COMMON_EXPORT void _link_hack();
}

QOptions COMMON_EXPORT get_common_options();
void COMMON_EXPORT load_qm(const QStringList& names, const QString &lang = "system");
// if appname ends with 'desktop', 'es', 'angle', software', 'sw', set by appname. otherwise set by command line option glopt
void COMMON_EXPORT set_opengl_backend(const QString& glopt = "auto", const QString& appname = QString());

#endif // COMMON_H
