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

#include "QIODeviceInput.h"
#include "QtAV/private/AVInput_p.h"
#include "QtAV/private/mkid.h"
#include "QtAV/private/prepost.h"
#include <QtCore/QIODevice>
#include <QtCore/QFile>
#ifndef TEST_QTAV_QIODEVICEINPUT
#include "utils/Logger.h"
#else
#include <QtDebug>
#endif
namespace QtAV {

static const AVInputId AVInputId_QIODevice = mkid::id32base36_6<'Q','I','O','D','e','v'>::value;
static const char kQIODevName[] = "QIODevice";
FACTORY_REGISTER_ID_TYPE(AVInput, AVInputId_QIODevice, QIODeviceInput, kQIODevName)

class QIODeviceInputPrivate : public AVInputPrivate
{
public:
    QIODeviceInputPrivate()
        : AVInputPrivate()
        , dev(0)
    {}
    QIODevice *dev;
};

QIODeviceInput::QIODeviceInput() : AVInput(*new QIODeviceInputPrivate()) {}
QIODeviceInput::QIODeviceInput(QIODeviceInputPrivate &d) : AVInput(d) {}
QString QIODeviceInput::name() const { return kQIODevName;}

void QIODeviceInput::setIODevice(QIODevice *dev)
{
    d_func().dev = dev;
}

QIODevice* QIODeviceInput::device() const
{
    return d_func().dev;
}

bool QIODeviceInput::isSeekable() const
{
    DPTR_D(const QIODeviceInput);
    return d.dev && !d.dev->isSequential();
}

qint64 QIODeviceInput::read(char *data, qint64 maxSize)
{
    DPTR_D(QIODeviceInput);
    if (!d.dev)
        return 0;
    return d.dev->read(data, maxSize);
}

bool QIODeviceInput::seek(qint64 offset, int from)
{
    DPTR_D(QIODeviceInput);
    if (!d.dev)
        return false;
    if (from == 2) {
        offset = d.dev->size() - offset;
    } else if (from == 1) {
        offset = d.dev->pos() + offset;
    }
    return d.dev->seek(offset);
}

qint64 QIODeviceInput::position() const
{
    DPTR_D(const QIODeviceInput);
    if (!d.dev)
        return 0;
    return d.dev->pos();
}

qint64 QIODeviceInput::size() const
{
    DPTR_D(const QIODeviceInput);
    if (!d.dev)
        return 0;
    return d.dev->size(); // sequential device returns bytesAvailable()
}
// qrc support
static const char kQFileName[] = "QFile";
class QFileInputPrivate;
class QFileInput : public QIODeviceInput
{
    DPTR_DECLARE_PRIVATE(QFileInput)
public:
    QFileInput();
    QString name() const Q_DECL_OVERRIDE { return kQFileName;}
    const QStringList& protocols() const Q_DECL_OVERRIDE
    {
        static QStringList p = QStringList() << "" << "qrc";
        return p;
    }
protected:
    void onUrlChanged() Q_DECL_OVERRIDE;
private:
    using QIODeviceInput::setIODevice;
};

static const AVInputId AVInputId_QFile = mkid::id32base36_5<'Q','F','i','l','e'>::value;
FACTORY_REGISTER_ID_TYPE(AVInput, AVInputId_QFile, QFileInput, kQFileName)

class QFileInputPrivate : public QIODeviceInputPrivate
{
public:
    QFileInputPrivate() : QIODeviceInputPrivate() {}
    ~QFileInputPrivate() {
        if (file.isOpen())
            file.close();
    }
    QFile file;
};

QFileInput::QFileInput()
    : QIODeviceInput(*new QFileInputPrivate())
{
    setIODevice(&d_func().file);
}

void QFileInput::onUrlChanged()
{
    DPTR_D(QFileInput);
    if (d.file.isOpen())
        d.file.close();
    QString path(url());
    if (path.startsWith("qrc:"))
        path = path.mid(3);
    d.file.setFileName(path);
    if (path.isEmpty())
        return;
    if (!d.file.open(QIODevice::ReadOnly))
        qWarning() << "Failed to open [" << d.file.fileName() << "]: " << d.file.errorString();
}

} //namespace QtAV

#ifdef TEST_QTAV_QIODEVICEINPUT
int main(int, char**)
{
    QtAV::QFileInput fi;
    qDebug() << "protocols: " << fi.protocols();
    fi.setUrl("qrc:/QtAV.svg");
    QByteArray data(1024, 0);
    fi.read(data.data(), data.size());
    qDebug("QFileInput url: %s, seekable: %d, size: %lld", fi.url().toUtf8().constData(), fi.isSeekable(), fi.size());
    qDebug() << data;
    return 0;
}
#endif
