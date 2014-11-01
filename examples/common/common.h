#ifndef COMMON_H
#define COMMON_H

#include <QtCore/QStringList>
#include "qoptions.h"

#if defined(BUILD_COMMON_LIB)
#  undef COMMON_EXPORT
#  define COMMON_EXPORT Q_DECL_EXPORT
#else
#  undef COMMON_EXPORT
#  define COMMON_EXPORT Q_DECL_IMPORT //only for vc?
#endif

extern "C" {
COMMON_EXPORT void _link_hack();
}

QOptions COMMON_EXPORT get_common_options();
void COMMON_EXPORT load_qm(const QStringList& names);

#endif // COMMON_H
