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

#include "QtAV/Subtitle.h"
#include "QtAV/private/SubtitleProcesser.h"
#include <algorithm>
#include <QtDebug>
#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QIODevice>
#include <QtCore/QLinkedList>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>

namespace QtAV {

const int kMaxSubtitleSize = 10 * 1024 * 1024;

class Subtitle::Private {
public:
    Private()
        : loaded(false)
        , fuzzy_match(true)
        , update_image(true)
        , processer(0)
        , t(0)
    {}
    void reset() {
        loaded = false;
        update_image = true;
        t = 0;
        frame = SubtitleFrame();
        frames.clear();
        itf = frames.begin();
    }
    static QStringList defaultNameFilters() {
        return QStringList() << "*.ass" << "*.ssa" << "*.srt";
    }

    // width/height == 0: do not create image
    // return true if both frame time and content(currently is text) changed
    bool prepareCurrentFrame();
    QStringList find();
    /*!
     * \brief readFromFile
     * read subtilte content from path
     * \param path
     * \return utf8 encoded content
     */
    QByteArray readFromFile(const QString& path);
    /*!
     * \brief processRawData
     * process utf8 encoded subtitle content. maybe a temp file is created if subtitle processer does not
     * support raw data
     * \param data utf8 subtitle content
     */
    bool processRawData(const QByteArray& data);
    bool processRawData(SubtitleProcesser* sp, const QByteArray& data);

    bool loaded;
    bool fuzzy_match;
    bool update_image;
    SubtitleProcesser *processer;
    QList<SubtitleProcesser*> processers;
    QStringList engine_names;
    QLinkedList<SubtitleFrame> frames;
    QUrl url;
    QByteArray raw_data;
    QString file_name;
    QStringList name_filters;
    QIODevice *dev;
    // last time image
    qreal t;
    SubtitleFrame frame;
    QLinkedList<SubtitleFrame>::iterator itf;
};

Subtitle::Subtitle(QObject *parent) :
    QObject(parent)
  , priv(new Private())
{
    // TODO: use factory.registedNames() and the order
    setEngines(QStringList() << "FFmpeg" << "LibAss");
}

Subtitle::~Subtitle()
{}

bool Subtitle::isLoaded() const
{
    return priv->loaded;
}

void Subtitle::setEngines(const QStringList &value)
{
    if (priv->engine_names == value)
        return;
    priv->engine_names = value;
    emit enginesChanged();
    QList<SubtitleProcesser*> sps;
    if (priv->engine_names.isEmpty()) {
        return;
    }
    foreach (QString e, priv->engine_names) {
        QList<SubtitleProcesser*>::iterator it = priv->processers.begin();
        for (; it != priv->processers.end(); ++it) {
            if (!(*it)) {
                priv->processers.erase(it);
                continue;
            }
            if ((*it)->name() != e) {
                continue;
            }
            sps.append(*it);
            priv->processers.erase(it);
            break;
        }
        if (it == priv->processers.end()) {
            SubtitleProcesser* sp = SubtitleProcesserFactory::create(SubtitleProcesserFactory::id(e.toStdString(), false));
            if (sp)
                sps.append(sp);
        }
    }
    priv->processers = sps;
}

QStringList Subtitle::engines() const
{
    return priv->engine_names;
}

void Subtitle::setFuzzyMatch(bool value)
{
    if (priv->fuzzy_match == value)
        return;
    priv->fuzzy_match = value;
    emit fuzzyMatchChanged();
}

bool Subtitle::fuzzyMatch() const
{
    return priv->fuzzy_match;
}

void Subtitle::setSource(const QUrl &url)
{
    if (priv->url == url)
        return;
    priv->url = url;
    emit sourceChanged();
    priv->file_name.clear();
    priv->raw_data.clear();
}

QUrl Subtitle::source() const
{
    return priv->url;
}

void Subtitle::setRawData(const QByteArray &data)
{
    // compare the whole content is not a good idea
    if (priv->raw_data.size() == data.size())
        return;
    priv->raw_data = data;
    emit rawDataChanged();

    priv->url.clear();
    priv->file_name.clear();
}

QByteArray Subtitle::rawData() const
{
    return priv->raw_data;
}

void Subtitle::setFileName(const QString &name)
{
    if (priv->file_name == name)
        return;
    priv->file_name = name;
    priv->url.clear();
    priv->raw_data.clear();
    emit fileNameChanged();
}

QString Subtitle::fileName() const
{
    return priv->file_name;
}

void Subtitle::setNameFilters(const QStringList &filters)
{
    if (priv->name_filters == filters)
        return;
    priv->name_filters = filters;
    emit nameFiltersChanged();
}

QStringList Subtitle::nameFilters() const
{
    return priv->name_filters;
}

void Subtitle::setTimestamp(qreal t)
{
    priv->t = t;
    if (!isLoaded())
        return;
    if (!priv->prepareCurrentFrame())
        return;
    priv->update_image = true;
    emit contentChanged();
}

qreal Subtitle::timestamp() const
{
    return priv->t;
}

bool Subtitle::load()
{
    priv->reset();
    if (!priv->url.isEmpty()) {
        // need qt network module network
        return false;
    }
    // raw data is set, file name and url are empty
    QByteArray u8 = priv->raw_data;
    if (!u8.isEmpty()) {
        priv->loaded = priv->processRawData(u8);
        return priv->loaded;
    }
    // read from a url
    QFile f(priv->url.toString());//QUrl::FullyDecoded));
    if (f.exists()) {
        u8 = priv->readFromFile(f.fileName());
        if (u8.isEmpty())
            return false;
        priv->loaded = priv->processRawData(u8);
        return priv->loaded;
    }
    // read from a file
    QStringList paths = priv->find();
    foreach (QString path, paths) {
        u8 = priv->readFromFile(path);
        if (u8.isEmpty())
            continue;
        if (!priv->processRawData(u8))
            continue;
        priv->loaded = true;
        return true;
    }
    return false;
}

QString Subtitle::getText() const
{
    if (!isLoaded())
        return QString();
    return priv->frame.text;
}

QImage Subtitle::getImage(int width, int height)
{
    if (!isLoaded())
        return QImage();
    if (!priv->frame || width == 0 || height == 0)
        return QImage();
    if (!priv->update_image
            && width == priv->frame.image.width() && height == priv->frame.image.height())
        return priv->frame.image;
    priv->update_image = false;
    if (!priv->processer)
        return QImage();
    priv->frame.image = priv->processer->getImage(priv->t, width, height);
    return priv->frame.image;
}

// DO NOT set frame's image to reduce memory usage
// assume frame.text is already set
// check previous text if now no subtitle
bool Subtitle::Private::prepareCurrentFrame()
{
    QLinkedList<SubtitleFrame>::iterator it = itf;
    bool found = false;
    if (t < it->begin) {
        while (it != frames.begin()) {
            --it;
            if (t > it->end) {
                // no subtitle at that time. check previous text
                if (frame.isValid()) {
                    frame = SubtitleFrame();
                    return true;
                }
                return false;
            }
            if (t >= (*it).begin) {
                found = true;
                itf = it;
                break;
            }
        }
        if (found) {
            frame = *it;
            return true;
        }
        // no subtitle at that time. it == begin(). check previous text
        if (frame.isValid()) {
            frame = SubtitleFrame();
            return true;
        }
        return false;
    }
    // t >= (*it)->begin, time does not change, frame does not change
    if (t <= (*it).end) {
        found = true;
        if (frame.isValid()) //previous has no subtitle, itf was not changed
            return false;
        frame = *it;
        return true;
    }
    // t >= (*it)->end, already t >= (*it)->begin
    while (it != frames.end()) {
        ++it;
        if (t < it->begin) {
            // no subtitle at that time. check previous text
            if (frame.isValid()) {
                frame = SubtitleFrame();
                return true;
            }
            return false;
        }
        if (t <= (*it).end) {
            found = true;
            itf = it;
            break;
        }
    }
    if (found) {
        frame = *it;
        return true;
    }
    // no subtitle at that time, it == end(). check previous text
    if (frame.isValid()) {
        frame = SubtitleFrame();
        return true;
    }
    return false;
}

QStringList Subtitle::Private::find()
{
    // !fuzzyMatch: return the file
    if (!fuzzy_match)
        return QStringList() << file_name;
    QFileInfo fi(file_name);
    QDir dir(fi.dir());
    QString name = fi.completeBaseName(); // video suffix has only 1 dot
    QStringList list = dir.entryList(name_filters, QDir::Files, QDir::Unsorted);
    // TODO: sort. get entryList from nameFilters 1 by 1 is slower?
    QStringList sorted;
    if (name_filters.isEmpty())
        name_filters = defaultNameFilters();
    if (name_filters.isEmpty()) {
        foreach (QString f, list) {
            if (!f.startsWith(name)) // why it happens?
                continue;
            sorted.append(dir.absoluteFilePath(f));
        }
        return sorted;
    }
    foreach (QString filter, name_filters) {
        QRegExp rx(filter);
        rx.setPatternSyntax(QRegExp::Wildcard);
        foreach (QString f, list) {
            if (!f.startsWith(name)) // why it happens?
                continue;
            if (!rx.exactMatch(f))
                continue;
            sorted.append(dir.absoluteFilePath(f));
        }
    }
    return sorted;
}

QByteArray Subtitle::Private::readFromFile(const QString &path)
{
    qDebug() << "read subtitle from: " << path;
    QFile f(path);
    if (f.size() > kMaxSubtitleSize)
        return QByteArray();
    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open subtitle [" << path << "]: " << f.errorString();
        return QByteArray();
    }
    QTextStream ts(&f);
    ts.setAutoDetectUnicode(true);
    return ts.readAll().toUtf8();
}

bool Subtitle::Private::processRawData(const QByteArray &data)
{
    processer = 0;
    frames.clear();
    if (data.size() > kMaxSubtitleSize)
        return false;
    foreach (SubtitleProcesser* sp, processers) {
        if (processRawData(sp, data)) {
            processer = sp;
            break;
        }
    }
    if (!processer)
        return false;
    QList<SubtitleFrame> fs(processer->frames());
    std::sort(fs.begin(), fs.end());
    foreach (SubtitleFrame f, fs) {
       frames.push_back(f);
    }
    itf = frames.begin();
    frame = *itf;
    return true;
}

bool Subtitle::Private::processRawData(SubtitleProcesser *sp, const QByteArray &data)
{
    if (sp->isSupported(SubtitleProcesser::RawData)) {
        qDebug("processing subtitle from raw data...");
        QByteArray u8(data);
        QBuffer buf(&u8);
        if (buf.open(QIODevice::ReadOnly)) {
            const bool ok = sp->process(&buf);
            if (buf.isOpen())
                buf.close();
            if (ok)
                return true;
        } else {
            qWarning() << "open subtitle qbuffer error: " << buf.errorString();
        }
    }
    if (sp->isSupported(SubtitleProcesser::File)) {
        qDebug("processing subtitle from a tmp file...");
        QString name = url.toLocalFile().section('/', -1);
        if (name.isEmpty())
            name = QFileInfo(file_name).fileName(); //priv->name.section('/', -1); // if no seperator?
        if (name.isEmpty())
            name = "QtAV_u8_sub_cache";
        name.append(QString("_%1").arg((quintptr)this));
        QFile w(QDir::temp().absoluteFilePath(name));
        if (w.open(QIODevice::WriteOnly)) {
            w.write(data);
            w.close();
        } else {
            if (!w.exists())
                return false;
        }
        return sp->process(w.fileName());
    }
    return false;
}

} //namespace QtAV
