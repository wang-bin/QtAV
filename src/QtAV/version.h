/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_VERSION_H
#define QTAV_VERSION_H

#define QTAV_MAJOR 1    //((QTAV_VERSION&0xff0000)>>16)
#define QTAV_MINOR 11    //((QTAV_VERSION&0xff00)>>8)
#define QTAV_PATCH 0    //(QTAV_VERSION&0xff)


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


/* the following are compile time version */
/* C++11 requires a space between literal and identifier */
#define QTAV_VERSION_STR        TOSTR(QTAV_MAJOR) "." TOSTR(QTAV_MINOR) "." TOSTR(QTAV_PATCH)

#endif // QTAV_VERSION_H
