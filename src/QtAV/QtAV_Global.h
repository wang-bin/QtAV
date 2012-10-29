/******************************************************************************
    qtav_global.h: description
    Copyright (C) 2011-2012 Wang Bin <wbsecg1@gmail.com>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
******************************************************************************/

#ifndef QTAV_GLOBAL_H
#define QTAV_GLOBAL_H

#include <qglobal.h>

#undef LIB_VERSION

#define MAJOR 1	//((LIB_VERSION&0xff0000)>>16)
#define MINOR 0	//((LIB_VERSION&0xff00)>>8)
#define PATCH 0	//(LIB_VERSION&0xff)

#define VERSION_CHK(major, minor, patch) \
    (((major&0xff)<<16) | ((minor&0xff)<<8) | (patch&0xff))

#define LIB_VERSION VERSION_CHK(MAJOR, MINOR, PATCH)

/*! Stringify \a x. */
#define _TOSTR(x)   #x
/*! Stringify \a x, perform macro expansion. */
#define TOSTR(x)  _TOSTR(x)

/* C++11 requires a space between literal and identifier */
static const char* const version_string = TOSTR(MAJOR) "." TOSTR(MINOR) "." TOSTR(PATCH) "(" __DATE__ ", " __TIME__ ")";
#define LIB_VERSION_STR		TOSTR(MAJOR) "." TOSTR(MINOR) "." TOSTR(PATCH)
#define LIB_VERSION_STR_LONG	LIB_VERSION_STR "(" __DATE__ ", " __TIME__ ")"




#if defined(Q_DLL_LIBRARY)
#  undef Q_EXPORT
#  define Q_EXPORT Q_DECL_EXPORT
#else
#  undef Q_EXPORT
#  define Q_EXPORT //Q_DECL_IMPORT //only for vc?
#endif

#endif // QTAV_GLOBAL_H

