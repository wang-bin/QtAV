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

#if defined(Q_DLL_LIBRARY)
#  undef Q_EXPORT
#  define Q_EXPORT Q_DECL_EXPORT
#else
#  undef Q_EXPORT
#  define Q_EXPORT Q_DECL_IMPORT //only for vc?
#endif

/* runtime version. used to compare with compile time version */
Q_EXPORT unsigned QtAV_Version();
Q_EXPORT QString QtAV_Version_String();
Q_EXPORT QString QtAV_Version_String_Long();
namespace QtAV {
Q_EXPORT void about(); //popup a dialog
Q_EXPORT void aboutFFmpeg();
Q_EXPORT QString aboutFFmpeg_PlainText();
Q_EXPORT QString aboutFFmpeg_HTML();
Q_EXPORT void aboutQtAV();
Q_EXPORT QString aboutQtAV_PlainText();
Q_EXPORT QString aboutQtAV_HTML();
}

/*
 * msvc sucks! can not deal with (defined QTAV_HAVE_##FEATURE && QTAV_HAVE_##FEATURE)
 */
#define QTAV_HAVE(FEATURE) (defined QTAV_HAVE_##FEATURE && QTAV_HAVE_##FEATURE)

//TODO: always inline
/* --gnu option of the RVCT compiler also defines __GNUC__ */
#if defined(Q_CC_GNU) && !defined(Q_CC_RVCT)
#define GCC_VERSION_AT_LEAST(major, minor, patch) \
    (__GNUC__ > major || (__GNUC__ == major && (__GNUC_MINOR__ > minor \
    || (__GNUC_MINOR__ == minor && __GNUC_PATCHLEVEL__ >= patch))))
#else
/* Define this for !GCC compilers, just so we can write things like GCC_VERSION_AT_LEAST(4, 1, 0). */
#define GCC_VERSION_AT_LEAST(major, minor, patch) 0
#endif

#endif // QTAV_GLOBAL_H

