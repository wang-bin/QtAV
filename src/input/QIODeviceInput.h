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

#ifndef QTAV_QIODEVICEINPUT
#define QTAV_QIODEVICEINPUT

#include <QtAV/AVInput.h>
#include <QtCore/QIODevice>

namespace QtAV {

class QIODeviceInputPrivate;
class QIODeviceInput : public AVInput
{
    DPTR_DECLARE_PRIVATE(QIODeviceInput)
public:
    QIODeviceInput();
    virtual QString name() const Q_DECL_OVERRIDE;
    // MUST open/close outside
    void setIODevice(QIODevice *dev); // set private in QFileInput etc
    QIODevice* device() const;

    virtual bool isSeekable() const Q_DECL_OVERRIDE;
    virtual qint64 read(char *data, qint64 maxSize) Q_DECL_OVERRIDE;
    virtual bool seek(qint64 offset, int from) Q_DECL_OVERRIDE;
    virtual qint64 position() const Q_DECL_OVERRIDE;
    /*!
     * \brief size
     * \return <=0 if not support
     */
    virtual qint64 size() const Q_DECL_OVERRIDE;
protected:
    QIODeviceInput(QIODeviceInputPrivate &d);
};

} //namespace QtAV
#include "QtAV/AVInput.h"

#endif // QTAV_QIODEVICEINPUT

