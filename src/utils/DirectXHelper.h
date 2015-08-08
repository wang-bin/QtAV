/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_DIRECTXHELPER_H
#define QTAV_DIRECTXHELPER_H

namespace QtAV {

#ifndef DX_LOG_COMPONENT
#define DX_LOG_COMPONENT "DirectX"
#endif //DX_LOG_COMPONENT
#define DX_ENSURE_OK(f, ...) \
    do { \
        HRESULT hr = f; \
        if (FAILED(hr)) { \
            qWarning() << QString::fromLatin1(DX_LOG_COMPONENT " error@%1. " #f ": (0x%2) %3").arg(__LINE__).arg(hr, 0, 16).arg(qt_error_string(hr)); \
            return __VA_ARGS__; \
        } \
    } while (0)


template <class T> void SafeRelease(T **ppT) {
  if (*ppT) {
    (*ppT)->Release();
    *ppT = NULL;
  }
}

} //namespace QtAV
#endif //QTAV_DIRECTXHELPER_H
