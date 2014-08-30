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
#include "QtAV/private/SubtitleProcessor.h"
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
        , processor(0)
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
     * process utf8 encoded subtitle content. maybe a temp file is created if subtitle processor does not
     * support raw data
     * \param data utf8 subtitle content
     */
    bool processRawData(const QByteArray& data);
    bool processRawData(SubtitleProcessor* sp, const QByteArray& data);

    bool loaded;
    bool fuzzy_match;
    bool update_image;
    SubtitleProcessor *processor;
    QList<SubtitleProcessor*> processors;
    QStringList engine_names;
    QLinkedList<SubtitleFrame> frames;
    QUrl url;
    QByteArray raw_data;
    QString file_name;
    QStringList suffixes;
    QStringList supported_suffixes;
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
    priv->supported_suffixes.clear();
    priv->engine_names = value;
    emit enginesChanged();
    QList<SubtitleProcessor*> sps;
    if (priv->engine_names.isEmpty()) {
        return;
    }
    foreach (QString e, priv->engine_names) {
        QList<SubtitleProcessor*>::iterator it = priv->processors.begin();
        while (it != priv->processors.end()) {
            if (!(*it)) {
                it = priv->processors.erase(it);
                continue;
            }
            if ((*it)->name() != e) {
                ++it;
                continue;
            }
            sps.append(*it);
            it = priv->processors.erase(it);
            break;
        }
        if (it == priv->processors.end()) {
            SubtitleProcessor* sp = SubtitleProcessorFactory::create(SubtitleProcessorFactory::id(e.toStdString(), false));
            if (sp)
                sps.append(sp);
        }
    }
    // release the processors not wanted
    qDeleteAll(priv->processors);
    priv->processors = sps;
    if (sps.isEmpty())
        return;
    foreach (SubtitleProcessor* sp, sps) {
        priv->supported_suffixes.append(sp->supportedTypes());
    }
    // TODO: remove duplicates
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
    priv->url.clear();
    priv->raw_data.clear();
    priv->file_name = name;
    // FIXME: "/C:/movies/xxx" => "C:/movies/xxx"
    if (priv->file_name.startsWith("/") && priv->file_name.contains(":")) {
        int slash = priv->file_name.indexOf(":");
        slash = priv->file_name.lastIndexOf("/", slash);
        priv->file_name = priv->file_name.mid(slash+1);
    }
    emit fileNameChanged();
}

QString Subtitle::fileName() const
{
    return priv->file_name;
}

void Subtitle::setSuffixes(const QStringList &value)
{
    if (priv->suffixes == value)
        return;
    priv->suffixes = value;
    emit suffixesChanged();
}

QStringList Subtitle::suffixes() const
{
    return priv->suffixes;
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
    emit contentChanged(); //notify user to update subtitle
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
    if (!priv->processor)
        return QImage();
    priv->frame.image = priv->processor->getImage(priv->t, width, height);
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

    // found files will be sorted by extensions in sfx order
    QStringList sfx(suffixes);
    if (sfx.isEmpty())
        sfx = supported_suffixes;
    if (sfx.isEmpty())
        return QStringList() << file_name;

    QFileInfo fi(file_name);
    QString name = fi.fileName();
    QStringList filters;
    foreach (QString suf, sfx) {
        filters.append(QString("%1*.%2").arg(name).arg(suf));
    }
    QDir dir(fi.dir());
    QStringList list = dir.entryList(filters, QDir::Files, QDir::Unsorted);
    if (list.isEmpty()) {
        filters.clear();
        // a.mp4 => a
        name = fi.completeBaseName(); // video suffix has only 1 dot
        foreach (QString suf, sfx) {
            filters.append(QString("%1*.%2").arg(name).arg(suf));
        }
        list = dir.entryList(filters, QDir::Files, QDir::Unsorted);
    }
    if (list.isEmpty())
        return QStringList();
    // TODO: sort. get entryList from nameFilters 1 by 1 is slower?
    QStringList sorted;
    // sfx is not empty, sort to the given order (sfx's order)
    foreach (QString suf, sfx) {
        if (list.isEmpty())
            break;
        QRegExp rx("*." + suf);
        rx.setPatternSyntax(QRegExp::Wildcard);
        QStringList::iterator it = list.begin();
        while (it != list.end()) {
            if (!it->startsWith(name)) {// why it happens?
                it = list.erase(it);
                continue;
            }
            if (!rx.exactMatch(*it)) {
                ++it;
                continue;
            }
            sorted.append(dir.absoluteFilePath(*it));
            it = list.erase(it);
        }
    }
    // the given name is at the highest priority.
    if (QFile(file_name).exists()) {
        sorted.removeAll(file_name);
        sorted.prepend(file_name);
    }
    qDebug() << "subtitles found: " << sorted;
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
    processor = 0;
    frames.clear();
    if (data.size() > kMaxSubtitleSize)
        return false;
    foreach (SubtitleProcessor* sp, processors) {
        if (processRawData(sp, data)) {
            processor = sp;
            break;
        }
    }
    if (!processor)
        return false;
    QList<SubtitleFrame> fs(processor->frames());
    std::sort(fs.begin(), fs.end());
    foreach (SubtitleFrame f, fs) {
       frames.push_back(f);
    }
    itf = frames.begin();
    frame = *itf;
    return true;
}

bool Subtitle::Private::processRawData(SubtitleProcessor *sp, const QByteArray &data)
{
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
    qDebug("processing subtitle from a tmp utf8 file...");
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

} //namespace QtAV
