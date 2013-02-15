/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/


#ifndef QTAV_GLOBAL_H
#define QTAV_GLOBAL_H

#include <qglobal.h>
#include <dptr.h>

#define QTAV_MAJOR 1	//((QTAV_VERSION&0xff0000)>>16)
#define QTAV_MINOR 1	//((QTAV_VERSION&0xff00)>>8)
#define QTAV_PATCH 8	//(QTAV_VERSION&0xff)


#define QTAV_VERSION_MAJOR(V) ((V & 0xff0000) >> 16)
#define QTAV_VERSION_MINOR(V) ((V & 0xff00) >> 8)
#define QTAV_VERSION_PATCH(V) (V & 0xff)

#define QTAV_VERSION_CHK(major, minor, patch) \
    (((major&0xff)<<16) | ((minor&0xff)<<8) | (patch&0xff))

#define QTAV_VERSION QTAV_VERSION_CHK(QTAV_MAJOR, QTAV_MINOR, QTAV_PATCH)

/*! Stringify \a x. */
#define _TOSTR(x)   #x
/*! Stringify \a x, perform macro expansion. */
#define TOSTR(x)  _TOSTR(x)


/* runtime version. used to compare with compile time version */
unsigned QtAV_Version();

/* the following are compile time version */
/* C++11 requires a space between literal and identifier */
static const char* const qtav_version_string = TOSTR(QTAV_MAJOR) "." TOSTR(QTAV_MINOR) "." TOSTR(QTAV_PATCH) "(" __DATE__ ", " __TIME__ ")";
#define QTAV_VERSION_STR		TOSTR(QTAV_MAJOR) "." TOSTR(QTAV_MINOR) "." TOSTR(QTAV_PATCH)
#define QTAV_VERSION_STR_LONG	QTAV_VERSION_STR "(" __DATE__ ", " __TIME__ ")"


/* --gnu option of the RVCT compiler also defines __GNUC__ */
#if defined(Q_CC_GNU) && !defined(Q_CC_RVCT)
#define GCC_VERSION_AT_LEAST(major, minor, patch) \
    (__GNUC__ > major || (__GNUC__ == major && (__GNUC_MINOR__ > minor \
    || (__GNUC_MINOR__ == minor && __GNUC_PATCHLEVEL__ >= patch))))
#else
/* Define this for !GCC compilers, just so we can write things like GCC_VERSION_AT_LEAST(4, 1, 0). */
#define GCC_VERSION_AT_LEAST(major, minor, patch) 0
#endif

#if defined(Q_DLL_LIBRARY)
#  undef Q_EXPORT
#  define Q_EXPORT Q_DECL_EXPORT
#else
#  undef Q_EXPORT
#  define Q_EXPORT //Q_DECL_IMPORT //only for vc?
#endif
// default network timeout in ms
#define QTAV_DEFAULT_NETWORK_TIMEOUT 30000

namespace QtAV {
Q_EXPORT QString aboutQtAV();
Q_EXPORT QString aboutQtAV_HTML();
}
//TODO: always inline

#endif // QTAV_GLOBAL_H

