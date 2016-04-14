/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

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

#include <OMX_Core.h>
#include <OMX_Component.h>

#define OMX_ENSURE(f, ...) do { \
    OMX_ERRORTYPE err = f; \
    if (err != OMX_ErrorNone) { \
        qWarning("OMX error (%d) - " #f , err); \
        return __VA_ARGS__; \
    } \
} while(0)

#define OMX_WARN(f) do { \
    OMX_ERRORTYPE err = f; \
    if (err != OMX_ErrorNone) { \
        qWarning("OMX error (%d) - " #f , err); \
    } \
} while(0)

namespace QtAV {
namespace omx {

class core
{
public:
    core();
    OMX_ERRORTYPE OMX_Init();
    OMX_ERRORTYPE OMX_Deinit();
    OMX_ERRORTYPE OMX_ComponentNameEnum(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex);
    OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE* pHandle, OMX_STRING cComponentName, OMX_PTR pAppData, OMX_CALLBACKTYPE* pCallBacks);
    OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent);
};

class componet : protected core
{
public:
};
} //namespace omx
} //namespace QtAV
