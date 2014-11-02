#ifndef COMMON_EXPORT_H
#define COMMON_EXPORT_H

#include <QtCore/qglobal.h>


#if defined(BUILD_COMMON_LIB)
#  undef COMMON_EXPORT
#  define COMMON_EXPORT Q_DECL_EXPORT
#else
#  undef COMMON_EXPORT
#  define COMMON_EXPORT Q_DECL_IMPORT //only for vc?
#endif

#endif // COMMON_EXPORT_H
