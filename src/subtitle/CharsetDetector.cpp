/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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
#include "CharsetDetector.h"
#ifdef LINK_UCHARDET
#include <uchardet/uchardet.h>
#define HAVE_UCHARDET
#else
#ifdef BUILD_UCHARDET
#include "uchardet.h"
#define HAVE_UCHARDET
#endif
#endif //LINK_UCHARDET
#ifndef HAVE_UCHARDET
typedef struct uchardet* uchardet_t;
#endif

class CharsetDetector::Private
{
public:
    Private()
        : m_det(NULL)
    {
#ifdef HAVE_UCHARDET
        m_det = uchardet_new();
#endif
    }
    ~Private() {
        if (!m_det)
            return;
#ifdef HAVE_UCHARDET
        uchardet_delete(m_det);
#endif
        m_det = NULL;
    }
    QByteArray detect(const QByteArray& data) {
#ifdef HAVE_UCHARDET
        if (!m_det)
            return QByteArray();
        if (uchardet_handle_data(m_det, data.constData(), data.size()) != 0)
            return QByteArray();
        uchardet_data_end(m_det);
        QByteArray cs(uchardet_get_charset(m_det));
        uchardet_reset(m_det);
        return cs.trimmed();
#else
        return QByteArray();
#endif
    }

    uchardet_t m_det;
};

CharsetDetector::CharsetDetector()
    : priv(new Private())
{
}

CharsetDetector::~CharsetDetector()
{
    if (priv) {
        delete priv;
        priv = 0;
    }
}

bool CharsetDetector::isAvailable() const
{
    return !!priv->m_det;
}

QByteArray CharsetDetector::detect(const QByteArray &data)
{
    return priv->detect(data);
}
