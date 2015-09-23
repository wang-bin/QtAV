/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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
#include "vaapi_helper.h"
#include "utils/Logger.h"

namespace QtAV {
namespace vaapi {

dll_helper::dll_helper(const QString &soname, int version)
{
    if (version >= 0)
        m_lib.setFileNameAndVersion(soname, version);
    else
        m_lib.setFileName(soname);
    if (m_lib.load()) {
        qDebug("%s loaded", m_lib.fileName().toUtf8().constData());
    } else {
        if (version >= 0) {
            m_lib.setFileName(soname);
            m_lib.load();
        }
    }
    if (!m_lib.isLoaded())
        qDebug("can not load %s: %s", m_lib.fileName().toUtf8().constData(), m_lib.errorString().toUtf8().constData());
}

#define CONCAT(a, b)    CONCAT_(a, b)
#define CONCAT_(a, b)   a##b
#define STRINGIFY(x)    STRINGIFY_(x)
#define STRINGIFY_(x)   #x
#define STRCASEP(p, x)  STRCASE(CONCAT(p, x))
#define STRCASE(x)      case x: return STRINGIFY(x)

/* Return a string representation of a VAProfile */
const char *profileName(VAProfile profile)
{
    switch (profile) {
#define MAP(profile) \
        STRCASEP(VAProfile, profile)
        MAP(MPEG2Simple);
        MAP(MPEG2Main);
        MAP(MPEG4Simple);
        MAP(MPEG4AdvancedSimple);
        MAP(MPEG4Main);
#if VA_CHECK_VERSION(0,32,0)
        MAP(JPEGBaseline);
        MAP(H263Baseline);
        MAP(H264ConstrainedBaseline);
#endif
        MAP(H264Baseline);
        MAP(H264Main);
        MAP(H264High);
        MAP(VC1Simple);
        MAP(VC1Main);
        MAP(VC1Advanced);
#if VA_CHECK_VERSION(0, 38, 0)
        MAP(HEVCMain);
        MAP(HEVCMain10);
        MAP(VP9Profile0);
        MAP(VP8Version0_3); //defined in 0.37
#endif //VA_CHECK_VERSION(0, 38, 0)
#undef MAP
    default:
        break;
    }
}
} //namespace QtAV
} //namespace vaapi

