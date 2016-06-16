#ifndef COMMON_EXPORT_H
#define COMMON_EXPORT_H

#include <QtCore/qglobal.h>

#ifdef BUILD_COMMON_STATIC
#define COMMON_EXPORT
#else
#if defined(BUILD_COMMON_LIB)
#  undef COMMON_EXPORT
#  define COMMON_EXPORT Q_DECL_EXPORT
#else
#  undef COMMON_EXPORT
#  define COMMON_EXPORT //Q_DECL_IMPORT //only for vc? link to static lib error
#endif
#endif //BUILD_COMMON_STATIC
#endif // COMMON_EXPORT_H
