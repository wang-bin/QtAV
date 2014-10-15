#include "vaapi_helper.h"
#include "utils/Logger.h"

namespace QtAV {
namespace vaapi {

dll_helper::dll_helper(const QString &soname)
{
    m_lib.setFileName(soname);
    if (m_lib.load())
        qDebug("%s loaded", m_lib.fileName().toUtf8().constData());
    else
        qDebug("can not load %s: %s", m_lib.fileName().toUtf8().constData(), m_lib.errorString().toUtf8().constData());
}

} //namespace QtAV
} //namespace vaapi

