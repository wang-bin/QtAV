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
static t_Init *p_Init = 0;
static t_Deinit *p_Deinit = 0;
static t_ComponentNameEnum *p_ComponentNameEnum = 0;
static t_GetHandle *p_GetHandle = 0;
static t_FreeHandle *p_FreeHandle = 0;
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

} //namespace omx
} //namespace QtAV
