/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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

#include "internal.h"
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QDesktopServices>
#else
#include <QtCore/QStandardPaths>
#endif
#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

namespace QtAV {
static const char kFileScheme[] = "file:";
#define CHAR_COUNT(s) (sizeof(s) - 1) // tail '\0'

#ifdef Q_OS_MAC
static QString fromCFString(CFStringRef string)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    return QString().fromCFString(string);
#else
    if (!string)
        return QString();
    CFIndex length = CFStringGetLength(string);
    // Fast path: CFStringGetCharactersPtr does not copy but may return null for any and no reason.
    const UniChar *chars = CFStringGetCharactersPtr(string);
    if (chars)
        return QString(reinterpret_cast<const QChar *>(chars), length);
    QString ret(length, Qt::Uninitialized);
    CFStringGetCharacters(string, CFRangeMake(0, length), reinterpret_cast<UniChar *>(ret.data()));
    return ret;
#endif
}

QString absolutePathFromOSX(const QString& s)
{
    QString result;
#if !(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 1060)
    CFStringRef cfStr = CFStringCreateWithCString(kCFAllocatorDefault, s.toUtf8().constData(),  kCFStringEncodingUTF8);
    if (cfStr) {
        CFURLRef cfUrl = CFURLCreateWithString(kCFAllocatorDefault, cfStr, NULL);
        if (cfUrl) {
            CFErrorRef error = 0;
            CFURLRef cfUrlAbs = CFURLCreateFilePathURL(kCFAllocatorDefault, cfUrl, &error);
            if (cfUrlAbs) {
                CFStringRef cfStrAbsUrl = CFURLGetString(cfUrlAbs);
                result = fromCFString(cfStrAbsUrl);
                CFRelease(cfUrlAbs);
            }
            CFRelease(cfUrl);
        }
        CFRelease(cfStr);
    }
#else
    Q_UNUSED(s);
#endif
    return result;
}
#endif //Q_OS_MAC
/*!
 * \brief getLocalPath
 * get path that works for both ffmpeg and QFile
 * Windows: ffmpeg does not supports file:///C:/xx.mov, only supports file:C:/xx.mov or C:/xx.mov
 * QFile: does not support file: scheme
 * fullPath can be file:///path from QUrl. QUrl.toLocalFile will remove file://
 */
QString getLocalPath(const QString& fullPath)
{
#ifdef Q_OS_MAC
    if (fullPath.startsWith(QLatin1String("file:///.file/id=")) || fullPath.startsWith(QLatin1String("/.file/id=")))
        return absolutePathFromOSX(fullPath);
#endif
    int pos = fullPath.indexOf(QLatin1String(kFileScheme));
    if (pos >= 0) {
        pos += CHAR_COUNT(kFileScheme);
        bool has_slash = false;
        while (fullPath.at(pos) == QLatin1Char('/')) {
            has_slash = true;
            ++pos;
        }
        // win: ffmpeg does not supports file:///C:/xx.mov, only supports file:C:/xx.mov or C:/xx.mov
#ifndef Q_OS_WIN // for QUrl
        if (has_slash)
            --pos;
#endif
    }
    // always remove "file:" even thought it works for ffmpeg.but fileName() may be used for QFile which does not file:
    if (pos > 0)
        return fullPath.mid(pos);
    return fullPath;
}
#undef CHAR_COUNT

namespace Internal {
namespace Path {

QString toLocal(const QString &fullPath)
{
    return getLocalPath(fullPath);
}

QString appDataDir()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    return QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#else
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#endif //5.4.0
#endif // 5.0.0
}

QString appFontsDir()
{
#if 0 //qt may return an read only path, for example OSX /System/Library/Fonts
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    const QString dir(QStandardPaths::writableLocation(QStandardPaths::FontsLocation));
    if (!dir.isEmpty())
        return dir;
#endif
#endif
    return appDataDir() + QStringLiteral("/fonts");
}

QString fontsDir()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    return QDesktopServices::storageLocation(QDesktopServices::FontsLocation);
#else
    return QStandardPaths::standardLocations(QStandardPaths::FontsLocation).first();
#endif
}
} //namespace Path

QString options2StringHelper(void* obj, const char* unit)
{
    qDebug("obj: %p", obj);
    QString s;
    const AVOption* opt = NULL;
    while ((opt = av_opt_next(obj, opt))) {
        if (opt->type == AV_OPT_TYPE_CONST) {
            if (!unit)
                continue;
            if (!qstrcmp(unit, opt->unit))
                s.append(QStringLiteral(" %1=%2").arg(QLatin1String(opt->name)).arg(opt->default_val.i64));
            continue;
        } else {
            if (unit)
                continue;
        }
        s.append(QStringLiteral("\n%1: ").arg(QLatin1String(opt->name)));
        switch (opt->type) {
        case AV_OPT_TYPE_FLAGS:
        case AV_OPT_TYPE_INT:
        case AV_OPT_TYPE_INT64:
            s.append(QStringLiteral("(%1)").arg(opt->default_val.i64));
            break;
        case AV_OPT_TYPE_DOUBLE:
        case AV_OPT_TYPE_FLOAT:
            s.append(QStringLiteral("(%1)").arg(opt->default_val.dbl, 0, 'f'));
            break;
        case AV_OPT_TYPE_STRING:
            if (opt->default_val.str)
                s.append(QStringLiteral("(%1)").arg(QString::fromUtf8(opt->default_val.str)));
            break;
        case AV_OPT_TYPE_RATIONAL:
            s.append(QStringLiteral("(%1/%2)").arg(opt->default_val.q.num).arg(opt->default_val.q.den));
            break;
        default:
            break;
        }
        if (opt->help)
            s.append(QLatin1String(" ")).append(QString::fromUtf8(opt->help));
        if (opt->unit && opt->type != AV_OPT_TYPE_CONST)
            s.append(QLatin1String("\n ")).append(options2StringHelper(obj, opt->unit));
    }
    return s;
}

QString optionsToString(void* obj) {
    return options2StringHelper(obj, NULL);
}

void setOptionsToFFmpegObj(const QVariant& opt, void* obj)
{
    if (!opt.isValid())
        return;
    AVClass *c = obj ? *(AVClass**)obj : 0;
    if (c)
        qDebug() << QStringLiteral("%1.%2 options:").arg(QLatin1String(c->class_name)).arg(QLatin1String(c->item_name(obj)));
    else
        qDebug() << "options:";
    if (opt.type() == QVariant::Map) {
        QVariantMap options(opt.toMap());
        if (options.isEmpty())
            return;
        QMapIterator<QString, QVariant> i(options);
        while (i.hasNext()) {
            i.next();
            const QVariant::Type vt = i.value().type();
            if (vt == QVariant::Map)
                continue;
            const QByteArray key(i.key().toUtf8());
            qDebug("%s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
            if (vt == QVariant::Int || vt == QVariant::UInt || vt == QVariant::Bool) {
                // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
                av_opt_set_int(obj, key.constData(), i.value().toInt(), AV_OPT_SEARCH_CHILDREN);
            } else if (vt == QVariant::LongLong || vt == QVariant::ULongLong) {
                av_opt_set_int(obj, key.constData(), i.value().toLongLong(), AV_OPT_SEARCH_CHILDREN);
            } else if (vt == QVariant::Double) {
                av_opt_set_double(obj, key.constData(), i.value().toDouble(), AV_OPT_SEARCH_CHILDREN);
            }
        }
        return;
    }
    QVariantHash options(opt.toHash());
    if (options.isEmpty())
        return;
    QHashIterator<QString, QVariant> i(options);
    while (i.hasNext()) {
        i.next();
        const QVariant::Type vt = i.value().type();
        if (vt == QVariant::Hash)
            continue;
        const QByteArray key(i.key().toUtf8());
        qDebug("%s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        if (vt == QVariant::Int || vt == QVariant::UInt || vt == QVariant::Bool) {
            av_opt_set_int(obj, key.constData(), i.value().toInt(), AV_OPT_SEARCH_CHILDREN);
        } else if (vt == QVariant::LongLong || vt == QVariant::ULongLong) {
            av_opt_set_int(obj, key.constData(), i.value().toLongLong(), AV_OPT_SEARCH_CHILDREN);
        }
    }
}
//FIXME: why to lower case?
void setOptionsToDict(const QVariant& opt, AVDictionary** dict)
{
    if (!opt.isValid())
        return;
    if (opt.type() == QVariant::Map) {
        QVariantMap options(opt.toMap());
        if (options.isEmpty())
            return;
        QMapIterator<QString, QVariant> i(options);
        while (i.hasNext()) {
            i.next();
            const QVariant::Type vt = i.value().type();
            if (vt == QVariant::Map)
                continue;
            const QByteArray key(i.key().toUtf8());
            switch (vt) {
            case QVariant::Bool: {
                // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
                av_dict_set(dict, key.constData(), QByteArray::number(i.value().toInt()), 0);
            }
                break;
            default:
                // key and value are in lower case
                av_dict_set(dict, i.key().toUtf8().constData(), i.value().toByteArray().constData(), 0);
                break;
            }
            qDebug("dict: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        }
        return;
    }
    QVariantHash options(opt.toHash());
    if (options.isEmpty())
        return;
    QHashIterator<QString, QVariant> i(options);
    while (i.hasNext()) {
        i.next();
        const QVariant::Type vt = i.value().type();
        if (vt == QVariant::Hash)
            continue;
        const QByteArray key(i.key().toUtf8());
        switch (vt) {
        case QVariant::Bool: {
            // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
            av_dict_set(dict, key.constData(), QByteArray::number(i.value().toInt()), 0);
        }
            break;
        default:
            // key and value are in lower case
            av_dict_set(dict, i.key().toUtf8().constData(), i.value().toByteArray().constData(), 0);
            break;
        }
        qDebug("dict: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
    }
}

void setOptionsForQObject(const QVariant& opt, QObject *obj)
{
    if (!opt.isValid())
        return;
    qDebug() << QStringLiteral("set %1(%2) meta properties:").arg(QLatin1String(obj->metaObject()->className())).arg(obj->objectName());
    if (opt.type() == QVariant::Hash) {
        QVariantHash options(opt.toHash());
        if (options.isEmpty())
            return;
        QHashIterator<QString, QVariant> i(options);
        while (i.hasNext()) {
            i.next();
            if (i.value().type() == QVariant::Hash) // for example "vaapi": {...}
                continue;
            obj->setProperty(i.key().toUtf8().constData(), i.value());
            qDebug("%s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        }
    }
    if (opt.type() != QVariant::Map)
        return;
    QVariantMap options(opt.toMap());
    if (options.isEmpty())
        return;
    QMapIterator<QString, QVariant> i(options);
    while (i.hasNext()) {
        i.next();
        if (i.value().type() == QVariant::Map) // for example "vaapi": {...}
            continue;
        obj->setProperty(i.key().toUtf8().constData(), i.value());
        qDebug("%s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
    }
}
} //namespace Internal
} //namespace QtAV
