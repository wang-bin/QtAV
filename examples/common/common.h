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

#endif // COMMON_H
