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
#include <QtCore/QByteArray>
#include <QtCore/QVector>

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
    // omx <=1.1 only. Return OMX_ErrorNotImplemented if symbol not found
    OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_STRING compName, OMX_U32 *pNumRoles, OMX_U8 **roles);
    // omx 1.2 only. Return OMX_ErrorNotImplemented if symbol not found
    OMX_ERRORTYPE OMX_RoleOfComponentEnum(OMX_STRING role, OMX_STRING compName, OMX_U32 nIndex);
};

class component : protected core
{
public:
    component();
    bool init();
    bool deinit();
    OMX_HANDLETYPE handle() const {return m_handle;}

    QVector<QByteArray> getComponentsByRole(const char* role);
    bool getPortDefinitionByComponentRole(const char* com, const char* role_name, OMX_PARAM_PORTDEFINITIONTYPE *def_in, OMX_PARAM_PORTDEFINITIONTYPE* def_out);
    bool getPortDefinitionByRole(const char* role_name, OMX_PARAM_PORTDEFINITIONTYPE *def_in, OMX_PARAM_PORTDEFINITIONTYPE* def_out);

    virtual OMX_ERRORTYPE onEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData) = 0;
    virtual OMX_ERRORTYPE onEmptyBufferDone(OMX_BUFFERHEADERTYPE* pBuffer) = 0;
    virtual OMX_ERRORTYPE onFillBufferDone(OMX_BUFFERHEADERTYPE* pBufferHeader) = 0;

private:

    OMX_HANDLETYPE m_handle;
    static QAtomicInt sRef;
};

template<typename T>
static void DefaultInit(T* t) {
    memset(t, 0, sizeof(*t));
    t->nSize = sizeof(*t);
    t->nVersion.s.nVersionMajor = 1;
    // from vlc
#ifdef __ANDROID__
    t->nVersion.s.nVersionMinor = 0;
    t->nVersion.s.nRevision = 0;
#else
    t->nVersion.s.nVersionMinor = 1;
#ifdef RPI
    t->nVersion.s.nRevision = 2;
#else
    t->nVersion.s.nRevision = 1;
#endif //RPI
#endif
    t->nVersion.s.nStep = 0;
}
} //namespace omx
} //namespace QtAV
