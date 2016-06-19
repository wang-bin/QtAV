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

#include "QtAV/MediaIO.h"
#include "QtAV/private/MediaIO_p.h"
#include "QtAV/private/mkid.h"
#include "QtAV/private/factory.h"
#include <QtCore/QFile>
#ifndef TEST_QTAV_QIODeviceIO
#include "utils/Logger.h"
#else
#include <QtDebug>
#endif

namespace QtAV {
class QIODeviceIOPrivate;
class QIODeviceIO : public MediaIO
{
    Q_OBJECT
    Q_PROPERTY(QIODevice* device READ device WRITE setDevice NOTIFY deviceChanged)
    DPTR_DECLARE_PRIVATE(QIODeviceIO)
public:
    QIODeviceIO();
    virtual QString name() const Q_DECL_OVERRIDE;
    // MUST open/close outside
    void setDevice(QIODevice *dev); // set private in QFileIO etc
    QIODevice* device() const;

    virtual bool isSeekable() const Q_DECL_OVERRIDE;
    virtual bool isWritable() const Q_DECL_OVERRIDE;
    virtual qint64 read(char *data, qint64 maxSize) Q_DECL_OVERRIDE;
    virtual qint64 write(const char *data, qint64 maxSize) Q_DECL_OVERRIDE;
    virtual bool seek(qint64 offset, int from) Q_DECL_OVERRIDE;
    virtual qint64 position() const Q_DECL_OVERRIDE;
    /*!
     * \brief size
     * \return <=0 if not support
     */
    virtual qint64 size() const Q_DECL_OVERRIDE;
Q_SIGNALS:
    void deviceChanged();
protected:
    QIODeviceIO(QIODeviceIOPrivate &d);
};
typedef QIODeviceIO MediaIOQIODevice;
static const MediaIOId MediaIOId_QIODevice = mkid::id32base36_6<'Q','I','O','D','e','v'>::value;
static const char kQIODevName[] = "QIODevice";
FACTORY_REGISTER(MediaIO, QIODevice, kQIODevName)

class QIODeviceIOPrivate : public MediaIOPrivate
{
public:
    QIODeviceIOPrivate()
        : MediaIOPrivate()
        , dev(0)
    {}
    QIODevice *dev;
};

QIODeviceIO::QIODeviceIO() : MediaIO(*new QIODeviceIOPrivate()) {}
QIODeviceIO::QIODeviceIO(QIODeviceIOPrivate &d) : MediaIO(d) {}
QString QIODeviceIO::name() const { return QLatin1String(kQIODevName);}

void QIODeviceIO::setDevice(QIODevice *dev)
{
    DPTR_D(QIODeviceIO);
    if (d.dev == dev)
        return;
    d.dev = dev;
    emit deviceChanged();
}

QIODevice* QIODeviceIO::device() const
{
    return d_func().dev;
}

bool QIODeviceIO::isSeekable() const
{
    DPTR_D(const QIODeviceIO);
    return d.dev && !d.dev->isSequential();
}

bool QIODeviceIO::isWritable() const
{
    DPTR_D(const QIODeviceIO);
    return d.dev && d.dev->isWritable();
}

qint64 QIODeviceIO::read(char *data, qint64 maxSize)
{
    DPTR_D(QIODeviceIO);
    if (!d.dev)
        return 0;
    return d.dev->read(data, maxSize);
}

qint64 QIODeviceIO::write(const char *data, qint64 maxSize)
{
    DPTR_D(QIODeviceIO);
    if (!d.dev)
        return 0;
    return d.dev->write(data, maxSize);
}

bool QIODeviceIO::seek(qint64 offset, int from)
{
    DPTR_D(QIODeviceIO);
    if (!d.dev)
        return false;
    if (from == SEEK_END) {
        offset = d.dev->size() - offset;
    } else if (from == SEEK_CUR) {
        offset = d.dev->pos() + offset;
    }
    return d.dev->seek(offset);
}

qint64 QIODeviceIO::position() const
{
    DPTR_D(const QIODeviceIO);
    if (!d.dev)
        return 0;
    return d.dev->pos();
}

qint64 QIODeviceIO::size() const
{
    DPTR_D(const QIODeviceIO);
    if (!d.dev)
        return 0;
    return d.dev->size(); // sequential device returns bytesAvailable()
}
// qrc support
static const char kQFileName[] = "QFile";
class QFileIOPrivate;
class QFileIO Q_DECL_FINAL: public QIODeviceIO
{
    DPTR_DECLARE_PRIVATE(QFileIO)
public:
    QFileIO();
    QString name() const Q_DECL_OVERRIDE { return QLatin1String(kQFileName);}
    const QStringList& protocols() const Q_DECL_OVERRIDE
    {
        static QStringList p = QStringList() << QStringLiteral("") << QStringLiteral("qrc") << QStringLiteral("qfile")
#ifdef Q_OS_ANDROID
                                             << QStringLiteral("assets")
#endif
#ifdef Q_OS_IOS
                                             << QStringLiteral("assets-library")
#endif
                                                ;
        return p;
    }
protected:
    void onUrlChanged() Q_DECL_OVERRIDE;
private:
    using QIODeviceIO::setDevice;
};
typedef QFileIO MediaIOQFile;
static const MediaIOId MediaIOId_QFile = mkid::id32base36_5<'Q','F','i','l','e'>::value;
FACTORY_REGISTER(MediaIO, QFile, kQFileName)

class QFileIOPrivate Q_DECL_FINAL: public QIODeviceIOPrivate
{
public:
    QFileIOPrivate() : QIODeviceIOPrivate() {}
    ~QFileIOPrivate() {
        if (file.isOpen())
            file.close();
    }
    QFile file;
};

QFileIO::QFileIO()
    : QIODeviceIO(*new QFileIOPrivate())
{
    setDevice(&d_func().file);
}

void QFileIO::onUrlChanged()
{
    DPTR_D(QFileIO);
    if (d.file.isOpen())
        d.file.close();
    QString path(url());
    if (path.startsWith(QLatin1String("qrc:"))) {
        path = path.mid(3);
    } else if (path.startsWith(QLatin1String("qfile:"))) {
        path = path.mid(6);
#ifdef Q_OS_WIN
        int p = path.indexOf(QLatin1Char(':'));
        if (p < 1) {
            qWarning("invalid path. ':' wrong position");
            return;
        }
        p -= 1;
        QChar c = path.at(p).toUpper();
        if (c < QLatin1Char('A') || c > QLatin1Char('Z')) {
            qWarning("invalid path. wrong driver");
            return;
        }
        const QString path_maybe = path.mid(p);
        qDebug() << path_maybe;
        --p;
        while (p > 0) {
            c = path.at(p);
            if (c != QLatin1Char('\\') && c != QLatin1Char('/')) {
                qWarning("invalid path. wrong dir seperator");
                return;
            }
            --p;
        }
        path = path_maybe;
#endif
    }
    d.file.setFileName(path);
    if (path.isEmpty())
        return;
    if (!d.file.open(QIODevice::ReadOnly))
        qWarning() << "Failed to open [" << d.file.fileName() << "]: " << d.file.errorString();
}

} //namespace QtAV
#include "QIODeviceIO.moc"
#ifdef TEST_QTAV_QIODeviceIO
int main(int, char**)
{
    QtAV::QFileIO fi;
    qDebug() << "protocols: " << fi.protocols();
    fi.setUrl("qrc:/QtAV.svg");
    QByteArray data(1024, 0);
    fi.read(data.data(), data.size());
    qDebug("QFileIO url: %s, seekable: %d, size: %lld", fi.url().toUtf8().constData(), fi.isSeekable(), fi.size());
    qDebug() << data;
    return 0;
}
#endif
