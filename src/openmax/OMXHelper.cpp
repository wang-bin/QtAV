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

#include "OMXHelper.h"
#include <QtCore/QLibrary>
#include "utils/Logger.h"

namespace QtAV {
namespace omx {

static QLibrary sLib;

typedef OMX_ERRORTYPE OMX_APIENTRY t_Init();
typedef OMX_ERRORTYPE OMX_APIENTRY t_Deinit();
typedef OMX_ERRORTYPE OMX_APIENTRY t_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex);
typedef OMX_ERRORTYPE OMX_APIENTRY t_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks);
typedef OMX_ERRORTYPE OMX_APIENTRY t_FreeHandle(
    OMX_IN  OMX_HANDLETYPE hComponent);
typedef OMX_ERRORTYPE OMX_APIENTRY t_RoleOfComponentEnum(
    OMX_OUT OMX_STRING role,
    OMX_IN  OMX_STRING compName,
    OMX_IN  OMX_U32 nIndex);
typedef OMX_ERRORTYPE t_GetRolesOfComponent (
    OMX_IN      OMX_STRING compName,
    OMX_INOUT   OMX_U32 *pNumRoles,
    OMX_OUT     OMX_U8 **roles);

static t_Init *p_Init = 0;
static t_Deinit *p_Deinit = 0;
static t_ComponentNameEnum *p_ComponentNameEnum = 0;
static t_GetHandle *p_GetHandle = 0;
static t_FreeHandle *p_FreeHandle = 0;
static t_RoleOfComponentEnum *p_RoleOfComponentEnum = 0;
static t_GetRolesOfComponent *p_GetRolesOfComponent = 0;
static const char *kOMXLib[] =
{
    "libiomx.so", /* Not used when using IOMX, the lib should already be loaded */
    "/opt/vc/lib/libopenmaxil.so",  /* Broadcom IL core */
    "libOMX_Core.so", /* TI OMAP IL core */
    "libOmxCore.so", /* Qualcomm IL core */
    "libnvomx.so", /* Tegra3 IL core */
    "libomxil-bellagio.so",  /* Bellagio IL core reference implementation */
    "libSEC_OMX_Core.so", //SAMSUNG
    0
};

core::core()
{
    if (sLib.isLoaded())
        return;
    //load "/opt/vc/lib/libbcm_host.so" and call bcm_host_init first?
    for (int i = 0; kOMXLib[i]; ++i) {
        sLib.setFileName(QString::fromUtf8(kOMXLib[i]));
        if (sLib.load())
            break;
    }
    if (!sLib.isLoaded())
        return;
    struct {
        void** pf;
        const char* names[8];
    } sym_table[] = {
    {(void**)&p_Init, {"OMX_Init", "SEC_OMX_Init", 0}},
    {(void**)&p_Deinit, {"OMX_Deinit", "SEC_OMX_Deinit", 0}},
    {(void**)&p_ComponentNameEnum, {"OMX_ComponentNameEnum", "SEC_OMX_ComponentNameEnum", 0}},
    {(void**)&p_RoleOfComponentEnum, {"OMX_RoleOfComponentEnum", "SEC_OMX_OMX_RoleOfComponentEnum", 0}},
    {(void**)&p_GetHandle, {"OMX_GetHandle", "SEC_OMX_GetHandle", 0}},
    {(void**)&p_FreeHandle, {"OMX_FreeHandle", "SEC_OMX_FreeHandle", 0}}
    };
    for (size_t i = 0; i < sizeof(sym_table)/sizeof(sym_table[0]); ++i) {
        void** pf = sym_table[i].pf;
        for (int j = 0; sym_table[i].names[j]; ++j) {
            *pf = (void*)sLib.resolve(sym_table[i].names[j]);
        }
        if (!(*pf)) {
            qWarning("OMX API not found: %s", sym_table[i].names[0]);
        }
    }
}

OMX_ERRORTYPE core::OMX_Init()
{
    return p_Init();
}

OMX_ERRORTYPE core::OMX_Deinit()
{
    return p_Deinit();
}

OMX_ERRORTYPE core::OMX_ComponentNameEnum(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex)
{
    return p_ComponentNameEnum(cComponentName, nNameLength, nIndex);
}

OMX_ERRORTYPE core::OMX_GetHandle(OMX_HANDLETYPE* pHandle, OMX_STRING cComponentName, OMX_PTR pAppData, OMX_CALLBACKTYPE* pCallBacks)
{
    return p_GetHandle(pHandle, cComponentName, pAppData, pCallBacks);
}

OMX_ERRORTYPE core::OMX_FreeHandle(OMX_HANDLETYPE hComponent)
{
    return p_FreeHandle(hComponent);
}

OMX_ERRORTYPE core::OMX_GetRolesOfComponent(OMX_STRING compName, OMX_U32 *pNumRoles, OMX_U8 **roles)
{
    if (!p_GetRolesOfComponent)
        return OMX_ErrorNotImplemented;
    return p_GetRolesOfComponent(compName, pNumRoles, roles);
}

OMX_ERRORTYPE core::OMX_RoleOfComponentEnum(OMX_STRING role, OMX_STRING compName, OMX_U32 nIndex)
{
    if (!p_RoleOfComponentEnum) //omx 1.2
        return OMX_ErrorNotImplemented;
    return p_RoleOfComponentEnum(role, compName, nIndex);
}

QVector<QByteArray> component::getComponentsByRole(const char *role)
{
    QVector<QByteArray> coms;
    // TODO: vlc rpi case
    char com[OMX_MAX_STRINGNAME_SIZE];
    for (int i = 0;; ++i) {
        OMX_ComponentNameEnum(com, sizeof(com), i);
        OMX_U32 nb_roles = 0;
        OMX_ERRORTYPE err = OMX_GetRolesOfComponent(com, &nb_roles, NULL);
        if (err != OMX_ErrorNotImplemented) {
            if (nb_roles == 0)
                continue;
            QVector<QByteArray> role_array(nb_roles);
            QVector<OMX_U8*> roles(nb_roles);
            for (OMX_U32 k = 0; k < nb_roles; ++k) {
                role_array[k] = QByteArray(OMX_MAX_STRINGNAME_SIZE, 0);
                roles[k] = (OMX_U8*)role_array[k].constData();
            }
            err = OMX_GetRolesOfComponent(com, &nb_roles, roles.data());
            if (err != OMX_ErrorNone)
                continue;
            qDebug() << "Roles of " << com << ": " << role_array;
            for (OMX_U32 k = 0; k < nb_roles; ++k) {
                if (role_array[k].startsWith(role)) {
                    coms.append(QByteArray(com, sizeof(com)));
                    break;
                }
            }
        } else {
            for (int j = 0; ; ++j) {
                char r[OMX_MAX_STRINGNAME_SIZE];
                OMX_ERRORTYPE err = OMX_RoleOfComponentEnum(r, com, j);
                if (err == OMX_ErrorNotImplemented) {
                    qWarning("OMX_GetRolesOfComponent or OMX_RoleOfComponentEnum is required");
                    return coms;
                }
                if (err == OMX_ErrorNoMore)
                    break;
                if (qstrcmp(r, role) == 0) {
                    coms.append(QByteArray(com, sizeof(com)));
                    break;
                }
            }
        }
    }
    return coms;
}

QAtomicInt component::sRef;



static OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
    Q_UNUSED(hComponent);
    component *c = static_cast<component*>(pAppData);
    return c->onEvent(eEvent, nData1, nData2, pEventData);
}

static OMX_ERRORTYPE EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
{
    Q_UNUSED(hComponent);
    component *c = static_cast<component*>(pAppData);
    return c->onEmptyBufferDone(pBuffer);
}

static OMX_ERRORTYPE FillBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader)
{
    Q_UNUSED(hComponent);
    component *c = static_cast<component*>(pAppData);
    return c->onFillBufferDone(pBufferHeader);
}

component::component() : core()
  , m_handle(NULL)
{}

bool component::init()
{
    if (sRef.fetchAndAddOrdered(1) > 0)
        return true;
    OMX_ENSURE(OMX_Init(), false);
    return true;
}

bool component::deinit()
{
    //if (sRef.fetchAndAddOrdered(-1) != 1)
    if (!sRef.deref()) {
        OMX_ENSURE(OMX_Deinit(), false);
    }
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0) || QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
    if (sRef < 0)
#else
    if (sRef.load() < 0)
#endif
        sRef.ref();
    return true;
}

bool component::getPortDefinitionByComponentRole(const char *com, const char *role_name, OMX_PARAM_PORTDEFINITIONTYPE *def_in, OMX_PARAM_PORTDEFINITIONTYPE *def_out)
{
    static OMX_CALLBACKTYPE cb = {
        EventHandler,
        EmptyBufferDone,
        FillBufferDone
    };
    OMX_ERRORTYPE err = OMX_GetHandle(&m_handle, (OMX_STRING)com, this, &cb);
    if (err != OMX_ErrorNone) {
        m_handle = NULL;
        return false;
    }
    OMX_PARAM_COMPONENTROLETYPE role;
    omx::DefaultInit(&role);
    strcpy((char*)role.cRole, role_name);
    // ??
    err = OMX_SetParameter(m_handle, OMX_IndexParamStandardComponentRole, &role);
    err = OMX_GetParameter(m_handle, OMX_IndexParamStandardComponentRole, &role);
    if (err == OMX_ErrorNone)
        qDebug("Component standard role set to: %s", role.cRole);
    OMX_PORT_PARAM_TYPE param;
    omx::DefaultInit(&param);
    err = OMX_GetParameter(m_handle, OMX_IndexParamVideoInit, &param);

    if (err != OMX_ErrorNone)  // TODO: VLC android case
        return false;

    for (OMX_U32 i = 0; i < param.nPorts; ++i) {
        OMX_PARAM_PORTDEFINITIONTYPE def;
        omx::DefaultInit(&def);
        def.nPortIndex = param.nStartPortNumber + i;
        err = OMX_GetParameter(m_handle, OMX_IndexParamPortDefinition, &def);
        if (err != OMX_ErrorNone)
            continue;
        if (def.eDir == OMX_DirInput) {
            if (def_in->nPortIndex == OMX_U32(-1))
                *def_in = def;
        } else {
            if (def_out->nPortIndex == OMX_U32(-1))
                *def_out = def;
        }
        if (def_in->nPortIndex != OMX_U32(-1) && def_out->nPortIndex != OMX_U32(-1))
            return true;
    }
    return false;
}

bool component::getPortDefinitionByRole(const char *role_name, OMX_PARAM_PORTDEFINITIONTYPE *def_in, OMX_PARAM_PORTDEFINITIONTYPE *def_out)
{
    const QVector<QByteArray> coms = omx::component::getComponentsByRole(role_name);
    def_in->nPortIndex = -1;
    def_out->nPortIndex = -1;
    foreach (const QByteArray& com, coms) {
        if (getPortDefinitionByComponentRole(com.constData(), role_name, def_in, def_out))
            return true;
        def_in->nPortIndex = -1;
        def_out->nPortIndex = -1;
    }
    return false;
}
} //namespace omx
} //namespace QtAV
