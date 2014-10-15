/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#include "CharsetDetector.h"
#include "QtAV/QtAV_Global.h"
#include <cassert>
#include <QtCore/QLibrary>
#include "utils/Logger.h"

#if QTAV_HAVE(CHARDET)
#include <chardet.h>
class CharDet {
public:
    bool isLoaded() { return true;}
};
#else
//from https://github.com/cnangel/libchardet, chardet.h
#define CHARDET_RESULT_OK    0
#define CHARDET_RESULT_NOMEMORY    (-1)
#define CHARDET_RESULT_INVALID_DETECTOR    (-2)

#define CHARDET_MAX_ENCODING_NAME    64

typedef void* chardet_t;

class dll_helper {
public:
    dll_helper(const QString& soname) {
        m_lib.setFileName(soname);
        if (m_lib.load())
            qDebug("%s loaded", m_lib.fileName().toUtf8().constData());
        else
            qDebug("can not load %s: %s", m_lib.fileName().toUtf8().constData(), m_lib.errorString().toUtf8().constData());

    }
    virtual ~dll_helper() { m_lib.unload();}
    bool isLoaded() const { return m_lib.isLoaded(); }
    void* resolve(const char *symbol) { return (void*)m_lib.resolve(symbol);}
private:
    QLibrary m_lib;
};

class CharDet : public dll_helper {
public:
    typedef int chardet_create_t(chardet_t* pdet);
    typedef void chardet_destroy_t(chardet_t det);
    typedef int chardet_handle_data_t(chardet_t det, const char* data, unsigned int len);
    typedef int chardet_data_end_t(chardet_t det);
    typedef int chardet_reset_t(chardet_t det);
    typedef int chardet_get_charset_t(chardet_t det, char* namebuf, unsigned int buflen);

    CharDet() : dll_helper("chardet") {
        fp_chardet_create = (chardet_create_t*)resolve("chardet_create");
        fp_chardet_destroy = (chardet_destroy_t*)resolve("chardet_destroy");
        fp_chardet_handle_data = (chardet_handle_data_t*)resolve("chardet_handle_data");
        fp_chardet_data_end = (chardet_data_end_t*)resolve("chardet_data_end");
        fp_chardet_reset = (chardet_reset_t*)resolve("chardet_reset");
        fp_chardet_get_charset = (chardet_get_charset_t*)resolve("chardet_get_charset");
    }
    int chardet_create(chardet_t* pdet) {
        assert(fp_chardet_create);
        return fp_chardet_create(pdet);
    }
    void chardet_destroy(chardet_t det) {
        assert(fp_chardet_destroy);
        fp_chardet_destroy(det);
    }
    int chardet_handle_data(chardet_t det, const char* data, unsigned int len) {
        assert(fp_chardet_handle_data);
        return fp_chardet_handle_data(det, data, len);
    }
    int chardet_data_end(chardet_t det) {
        assert(fp_chardet_data_end);
        return fp_chardet_data_end(det);
    }
    int chardet_reset(chardet_t det) {
        assert(fp_chardet_reset);
        return fp_chardet_reset(det);
    }
    int chardet_get_charset(chardet_t det, char* namebuf, unsigned int buflen) {
        assert(fp_chardet_get_charset);
        return fp_chardet_get_charset(det, namebuf, buflen);
    }
private:
    chardet_create_t* fp_chardet_create;
    chardet_destroy_t* fp_chardet_destroy;
    chardet_handle_data_t* fp_chardet_handle_data;
    chardet_data_end_t* fp_chardet_data_end;
    chardet_reset_t* fp_chardet_reset;
    chardet_get_charset_t* fp_chardet_get_charset;
};
#endif //QTAV_HAVE(CHARDET)
class CharsetDetector::Private : public CharDet
{
public:
    Private()
        : CharDet()
        , m_det(NULL)
    {
        if (!isLoaded())
            return;
        int ret = chardet_create(&m_det);
        if (ret != CHARDET_RESULT_OK)
            m_det = NULL;
    }
    ~Private() {
        if (!m_det)
            return;
        chardet_destroy(m_det);
        m_det = NULL;
    }

    QByteArray detect(const QByteArray& data) {
        if (!m_det)
            return QByteArray();
        int ret = chardet_handle_data(m_det, data.constData(), data.size());
        if (ret != CHARDET_RESULT_OK)
            return QByteArray();
        chardet_data_end(m_det);
        QByteArray cs(256, ' ');
        chardet_get_charset(m_det, cs.data(), cs.size());
        chardet_reset(m_det);
        return cs.trimmed();
    }

    chardet_t m_det;
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
