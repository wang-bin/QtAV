/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#ifndef QTAV_GLOBAL_H
#define QTAV_GLOBAL_H

#include <qglobal.h>
#include <dptr.h>


#define QTAV_MAJOR 1	//((QTAV_VERSION&0xff0000)>>16)
#define QTAV_MINOR 1	//((QTAV_VERSION&0xff00)>>8)
#define QTAV_PATCH 5	//(QTAV_VERSION&0xff)

#define QTAV_VERSION_CHK(major, minor, patch) \
    (((major&0xff)<<16) | ((minor&0xff)<<8) | (patch&0xff))

#define QTAV_VERSION VERSION_CHK(QTAV_MAJOR, QTAV_MINOR, QTAV_PATCH)

/*! Stringify \a x. */
#define _TOSTR(x)   #x
/*! Stringify \a x, perform macro expansion. */
#define TOSTR(x)  _TOSTR(x)

/* C++11 requires a space between literal and identifier */
static const char* const qtav_version_string = TOSTR(QTAV_MAJOR) "." TOSTR(QTAV_MINOR) "." TOSTR(QTAV_PATCH) "(" __DATE__ ", " __TIME__ ")";
#define QTAV_VERSION_STR		TOSTR(QTAV_MAJOR) "." TOSTR(QTAV_MINOR) "." TOSTR(QTAV_PATCH)
#define QTAV_VERSION_STR_LONG	QTAV_VERSION_STR "(" __DATE__ ", " __TIME__ ")"


/* --gnu option of the RVCT compiler also defines __GNUC__ */
#if defined(Q_CC_GNU) && !defined(Q_CC_RVCT)
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define GCC_VERSION_AT_LEAST(major, minor, patch) (GCC_VERSION >= (major * 10000 + minor * 100 + patch))
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

//TODO: always inline

#endif // QTAV_GLOBAL_H

