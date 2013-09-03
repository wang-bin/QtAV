#ifndef QML_AV_EXPORT_H
#define QML_AV_EXPORT_H

#include <qglobal.h>

#if defined(BUILD_QMLAV)
#  undef QMLAV_EXPORT
#  define QMLAV_EXPORT Q_DECL_EXPORT
#else
#  undef QMLAV_EXPORT
#  define QMLAV_EXPORT Q_DECL_IMPORT //only for vc?
#endif

#endif // QML_AV_EXPORT_H
