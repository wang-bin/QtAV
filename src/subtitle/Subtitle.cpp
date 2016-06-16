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

#include "QtAV/Subtitle.h"
#include "QtAV/private/SubtitleProcessor.h"
#include <algorithm>
#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QIODevice>
#include <QtCore/QLinkedList>
#include <QtCore/QRegExp>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>
#include <QtCore/QMutexLocker>
#include "subtitle/CharsetDetector.h"
#include "utils/Logger.h"

namespace QtAV {

const int kMaxSubtitleSize = 10 * 1024 * 1024; // TODO: remove because we find the matched extenstions

class Subtitle::Private {
public:
    Private()
        : loaded(false)
        , fuzzy_match(true)
        , update_text(true)
        , update_image(true)
        , last_can_render(false)
        , processor(0)
        , codec("AutoDetect")
        , t(0)
        , delay(0)
        , current_count(0)
        , force_font_file(false)
    {}
    void reset() {
        QMutexLocker lock(&mutex);
        Q_UNUSED(lock);
        loaded = false;
        processor = 0;
        update_text = true;
        update_image = true;
        t = 0;
        frame = SubtitleFrame();
        frames.clear();
        itf = frames.begin();
        current_count = 0;
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
    bool update_text;
    bool update_image; //TODO: detect image change from engine
    bool last_can_render;
    SubtitleProcessor *processor;
    QList<SubtitleProcessor*> processors;
    QByteArray codec;
    QStringList engine_names;
    QLinkedList<SubtitleFrame> frames;
    QUrl url;
    QByteArray raw_data;
    QString file_name;
    QStringList dirs;
    QStringList suffixes;
    QStringList supported_suffixes;
    QIODevice *dev;
    // last time image
    qreal t;
    qreal delay;
    SubtitleFrame frame;
    QString current_text;
    QImage current_image;
    SubImageSet current_ass;
    QLinkedList<SubtitleFrame>::iterator itf;
    /* number of subtitle frames at current time.
     * <0 means itf is the last. >0 means itf is the 1st
     */
    int current_count;
    QMutex mutex;

    bool force_font_file;
    QString font_file;
    QString fonts_dir;
};

Subtitle::Subtitle(QObject *parent) :
    QObject(parent)
  , priv(new Private())
{
    // TODO: use factory.registedNames() and the order
    setEngines(QStringList() << QStringLiteral("LibASS") << QStringLiteral("FFmpeg"));
}

Subtitle::~Subtitle()
{
    if (priv) {
        delete priv;
        priv = 0;
    }
}

bool Subtitle::isLoaded() const
{
    return priv->loaded;
}

void Subtitle::setCodec(const QByteArray &value)
{
    if (priv->codec == value)
        return;
    priv->codec = value;
    Q_EMIT codecChanged();
}

QByteArray Subtitle::codec() const
{
    return priv->codec;
}

void Subtitle::setEngines(const QStringList &value)
{
    if (priv->engine_names == value)
        return;
    // can not reset processor here, not thread safe.
    priv->supported_suffixes.clear();
    priv->engine_names = value;
    if (priv->engine_names.isEmpty()) {
        Q_EMIT enginesChanged();
        Q_EMIT supportedSuffixesChanged();
        return;
    }
    QList<SubtitleProcessor*> sps;
    foreach (const QString& e, priv->engine_names) {
        qDebug() << "engine:" << e;
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
            SubtitleProcessor* sp = SubtitleProcessor::create(e.toLatin1().constData());
            if (sp)
                sps.append(sp);
        }
    }
    // release the processors not wanted
    qDeleteAll(priv->processors);
    priv->processors = sps;
    if (sps.isEmpty()) {
        Q_EMIT enginesChanged();
        Q_EMIT supportedSuffixesChanged();
        return;
    }
    foreach (SubtitleProcessor* sp, sps) {
        priv->supported_suffixes.append(sp->supportedTypes());
    }
    priv->supported_suffixes.removeDuplicates();
    // DO NOT set priv->suffixes
    Q_EMIT enginesChanged();
    Q_EMIT supportedSuffixesChanged();
    // it's safe to reload
}

QStringList Subtitle::engines() const
{
    return priv->engine_names;
}

QString Subtitle::engine() const
{
    if (!priv->processor)
        return QString();
    return priv->processor->name();
}

void Subtitle::setFuzzyMatch(bool value)
{
    if (priv->fuzzy_match == value)
        return;
    priv->fuzzy_match = value;
    Q_EMIT fuzzyMatchChanged();
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
    Q_EMIT rawDataChanged();

    priv->url.clear();
    priv->file_name.clear();
}

QByteArray Subtitle::rawData() const
{
    return priv->raw_data;
}

extern QString getLocalPath(const QString& fullPath);

void Subtitle::setFileName(const QString &name)
{
    if (priv->file_name == name)
        return;
    priv->url.clear();
    priv->raw_data.clear();
    priv->file_name = name;
    if (priv->file_name.startsWith(QLatin1String("file:")))
        priv->file_name = getLocalPath(priv->file_name);
    Q_EMIT fileNameChanged();
}

QString Subtitle::fileName() const
{
    return priv->file_name;
}

void Subtitle::setDirs(const QStringList &value)
{
    if (priv->dirs == value)
        return;
    priv->dirs = value;
    Q_EMIT dirsChanged();
}

QStringList Subtitle::dirs() const
{
    return priv->dirs;
}

QStringList Subtitle::supportedSuffixes() const
{
    return priv->supported_suffixes;
}

void Subtitle::setSuffixes(const QStringList &value)
{
    if (priv->suffixes == value)
        return;
    priv->suffixes = value;
    Q_EMIT suffixesChanged();
}

QStringList Subtitle::suffixes() const
{
    //if (priv->suffixes.isEmpty())
    //    return supportedSuffixes();
    return priv->suffixes;
}

void Subtitle::setTimestamp(qreal t)
{
    // TODO: detect image change?
    {
        QMutexLocker lock(&priv->mutex);
        Q_UNUSED(lock);
        priv->t = t;
        if (!isLoaded())
            return;
        if (!priv->prepareCurrentFrame())
            return;
        priv->update_text = true;
        priv->update_image = true;
    }
    Q_EMIT contentChanged();
}

qreal Subtitle::timestamp() const
{
    return priv->t;
}

void Subtitle::setDelay(qreal value)
{
    if (priv->delay == value)
        return;
    priv->delay = value;
    Q_EMIT delayChanged();
}

qreal Subtitle::delay() const
{
    return priv->delay;
}

QString Subtitle::fontFile() const
{
    return priv->font_file;
}

void Subtitle::setFontFile(const QString &value)
{
    if (priv->font_file == value)
        return;
    priv->font_file = value;
    Q_EMIT fontFileChanged();
    if (priv->processor) {
        priv->processor->setFontFile(value);
    }
}

QString Subtitle::fontsDir() const
{
    return priv->fonts_dir;
}

void Subtitle::setFontsDir(const QString &value)
{
    if (priv->fonts_dir == value)
        return;
    priv->fonts_dir = value;
    Q_EMIT fontsDirChanged();
    if (priv->processor) {
        priv->processor->setFontsDir(value);
    }
}

bool Subtitle::isFontFileForced() const
{
    return priv->force_font_file;
}

void Subtitle::setFontFileForced(bool value)
{
    if (priv->force_font_file == value)
        return;
    priv->force_font_file = value;
    Q_EMIT fontFileForcedChanged();
    if (priv->processor) {
        priv->processor->setFontFileForced(value);
    }
}

void Subtitle::load()
{
    SubtitleProcessor *old_processor = priv->processor;
    priv->reset();
    Q_EMIT contentChanged(); //notify user to update subtitle
    // lock is not needed because it's not loaded now
    if (!priv->url.isEmpty()) {
        // need qt network module network
        if (old_processor != priv->processor)
            Q_EMIT engineChanged();
        return;
    }
    // raw data is set, file name and url are empty
    QByteArray u8 = priv->raw_data;
    if (!u8.isEmpty()) {
        priv->loaded = priv->processRawData(u8);
        if (priv->loaded)
            Q_EMIT loaded();
        checkCapability();
        if (old_processor != priv->processor)
            Q_EMIT engineChanged();
        return;
    }
    // read from a url
    QFile f(QUrl::fromPercentEncoding(priv->url.toEncoded()));
    if (f.exists()) {
        u8 = priv->readFromFile(f.fileName());
        if (u8.isEmpty())
            return;
        priv->loaded = priv->processRawData(u8);
        if (priv->loaded)
            Q_EMIT loaded(QUrl::fromPercentEncoding(priv->url.toEncoded()));
        checkCapability();
        if (old_processor != priv->processor)
            Q_EMIT engineChanged();
        return;
    }
    // read from a file
    QStringList paths = priv->find();
    if (paths.isEmpty()) {
        checkCapability();
        if (old_processor != priv->processor)
            Q_EMIT engineChanged();
        return;
    }
    foreach (const QString& path, paths) {
        if (path.isEmpty())
            continue;
        u8 = priv->readFromFile(path);
        if (u8.isEmpty())
            continue;
        if (!priv->processRawData(u8))
            continue;
        priv->loaded = true;
        Q_EMIT loaded(path);
        break;
    }
    checkCapability();
    if (old_processor != priv->processor)
        Q_EMIT engineChanged();

    if (priv->processor) {
        priv->processor->setFontFile(priv->font_file);
        priv->processor->setFontsDir(priv->fonts_dir);
        priv->processor->setFontFileForced(priv->force_font_file);
    }
}

void Subtitle::checkCapability()
{
    if (priv->last_can_render == canRender())
        return;
    priv->last_can_render = canRender();
    Q_EMIT canRenderChanged();
}

void Subtitle::loadAsync()
{
    if (fileName().isEmpty())
        return;
    class Loader : public QRunnable {
    public:
        Loader(Subtitle *sub) : m_sub(sub) {}
        void run() {
            if (m_sub)
                m_sub->load();
        }
    private:
        Subtitle *m_sub;
    };
    QThreadPool::globalInstance()->start(new Loader(this));
}

bool Subtitle::canRender() const
{
    return priv->processor && priv->processor->canRender();
}

QString Subtitle::getText() const
{
    QMutexLocker lock(&priv->mutex);
    Q_UNUSED(lock);
    if (!isLoaded())
        return QString();
    if (!priv->current_count)
        return QString();
    if (!priv->update_text)
        return priv->current_text;
    priv->update_text = false;
    priv->current_text.clear();
    const int count = qAbs(priv->current_count);
    QLinkedList<SubtitleFrame>::iterator it = priv->current_count > 0 ? priv->itf : priv->itf + (priv->current_count+1);
    for (int i = 0; i < count; ++i) {
        priv->current_text.append(it->text).append(QStringLiteral("\n"));
        ++it;
    }
    priv->current_text = priv->current_text.trimmed();
    return priv->current_text;
}

QImage Subtitle::getImage(int width, int height, QRect* boundingRect)
{
    QMutexLocker lock(&priv->mutex);
    Q_UNUSED(lock);
    if (!isLoaded())
        return QImage();
    if (width == 0 || height == 0)
        return QImage();
#if 0
    if (!priv->current_count) //seems ok to use this code
        return QImage();
    // always render the image to support animations
    if (!priv->update_image
            && width == priv->current_image.width() && height == priv->current_image.height())
        return priv->current_image;
#endif
    priv->update_image = false;
    if (!canRender())
        return QImage();
    priv->processor->setFrameSize(width, height);
    // TODO: store bounding rect here and not in processor
    priv->current_image = priv->processor->getImage(priv->t - priv->delay, boundingRect);
    return priv->current_image;
}

SubImageSet Subtitle::getSubImages(int width, int height, QRect *boundingRect)
{
    QMutexLocker lock(&priv->mutex);
    Q_UNUSED(lock);
    if (!isLoaded())
        return SubImageSet();
    if (width == 0 || height == 0)
        return SubImageSet();
    priv->update_image = false;
    if (!canRender())
        return SubImageSet();
    priv->processor->setFrameSize(width, height);
    // TODO: store bounding rect here and not in processor
    priv->current_ass = priv->processor->getSubImages(priv->t - priv->delay, boundingRect);
    return priv->current_ass;
}

bool Subtitle::processHeader(const QByteArray& codec, const QByteArray &data)
{
    qDebug() << "codec: " << codec;
    qDebug() << "header: " << data;
    SubtitleProcessor *old_processor = priv->processor;
    priv->reset(); // reset for the new subtitle stream (internal)
    if (priv->processors.isEmpty())
        return false;
    foreach (SubtitleProcessor *sp, priv->processors) {
        if (sp->supportedTypes().contains(QLatin1String(codec))) {
            priv->processor = sp;
            qDebug() << "current subtitle processor: " << sp->name();
            break;
        }
    }
    if (old_processor != priv->processor)
        Q_EMIT engineChanged();
    if (!priv->processor) {
        qWarning("No subtitle processor supports the codec '%s'", codec.constData());
        return false;
    }
    if (!priv->processor->processHeader(codec, data))
        return false;
    priv->loaded = true;

    priv->processor->setFontFile(priv->font_file);
    priv->processor->setFontsDir(priv->fonts_dir);
    priv->processor->setFontFileForced(priv->force_font_file);
    return true;
}

bool Subtitle::processLine(const QByteArray &data, qreal pts, qreal duration)
{
    if (!priv->processor)
        return false;
    SubtitleFrame f = priv->processor->processLine(data, pts, duration);
    if (!f.isValid())
        return false; // TODO: if seek to previous position, an invalid frame is returned.
    if (priv->frames.isEmpty() || priv->frames.last() < f) {
        priv->frames.append(f);
        priv->itf = priv->frames.begin();
        return true;
    }
    // usually add to the end. TODO: test
    QLinkedList<SubtitleFrame>::iterator it = priv->frames.end();
    if (it != priv->frames.begin())
        --it;
    while (it != priv->frames.begin() && f < (*it)) {--it;}
    if (it != priv->frames.begin()) // found in middle, insert before next
        ++it;
    priv->frames.insert(it, f);
    priv->itf = it;
    return true;
}

// DO NOT set frame's image to reduce memory usage
// assume frame.text is already set
// check previous text if now no subtitle
bool Subtitle::Private::prepareCurrentFrame()
{
    if (frames.isEmpty())
        return false;
    QLinkedList<SubtitleFrame>::iterator it = itf;
    int found = 0;
    const int old_current_count = current_count;
    const qreal t = this->t - delay;
    if (t < it->begin) {
        while (it != frames.begin()) {
            --it;
            if (t > it->end) {
                if (found > 0)
                    break;
                // no subtitle at that time. check previous text
                if (old_current_count) {
                    current_count = 0;
                    return true;
                }
                return false;
            }
            if (t >= (*it).begin) {
                if (found == 0)
                    itf = it;
                found++;
            }
        }
        current_count = -found;
        if (found > 0) {
            frame = *it;
            return true;
        }
        // no subtitle at that time. it == begin(). check previous text
        if (old_current_count)
            return true;
        return false;
    }
    bool it_changed = false;
    while (it != frames.end()) {
        if (t > it->end) {
            ++it;
            it_changed = true;
            continue;
        }
        if (t < it->begin) {
            if (found > 0)
                break;
            // no subtitle at that time. check previous text
            if (old_current_count) {
                current_count = 0;
                return true;
            }
            return false;
        }
        if (found == 0)
            itf = it;
        ++found;
        ++it;
    }
    current_count = found;
    if (found > 0)
        return it_changed || current_count != old_current_count;
    // no subtitle at that time, it == end(). check previous text
    if (old_current_count)
        return true;
    return false;
}

QStringList Subtitle::Private::find()
{
    if (file_name.isEmpty())
        return QStringList();
    // !fuzzyMatch: return the file
    if (!fuzzy_match) {
        return QStringList() << file_name;
    }
    // found files will be sorted by extensions in sfx order
    QStringList sfx(suffixes);
    if (sfx.isEmpty())
        sfx = supported_suffixes;
    if (sfx.isEmpty())
        return QStringList() << file_name;
    QFileInfo fi(file_name);
    QString name = fi.fileName();
    QString base_name = fi.completeBaseName(); // a.mp4=>a, video suffix has only 1 dot
    QStringList filters, filters_base;
    foreach (const QString& suf, sfx) {
        filters.append(QStringLiteral("%1*.%2").arg(name).arg(suf));
        if (name != base_name)
            filters_base.append(QStringLiteral("%1*.%2").arg(base_name).arg(suf));
    }
    QStringList search_dirs(dirs);
    search_dirs.prepend(fi.absolutePath());
    QFileInfoList list;
    foreach (const QString& d, search_dirs) {
        QDir dir(d);
        //qDebug() << "dir: " << dir;
        QFileInfoList fis = dir.entryInfoList(filters, QDir::Files, QDir::Unsorted);
        if (fis.isEmpty()) {
            if (filters_base.isEmpty())
                continue;
            fis = dir.entryInfoList(filters_base, QDir::Files, QDir::Unsorted);
        }
        if (fis.isEmpty())
            continue;
        list.append(fis);
    }
    if (list.isEmpty())
        return QStringList();
    // TODO: sort. get entryList from nameFilters 1 by 1 is slower?
    QStringList sorted;
    // sfx is not empty, sort to the given order (sfx's order)
    foreach (const QString& suf, sfx) {
        if (list.isEmpty())
            break;
        QRegExp rx(QStringLiteral("*.") + suf);
        rx.setPatternSyntax(QRegExp::Wildcard);
        QFileInfoList::iterator it = list.begin();
        while (it != list.end()) {
            if (!it->fileName().startsWith(name) && !it->fileName().startsWith(base_name)) {// why it happens?
                it = list.erase(it);
                continue;
            }
            if (!rx.exactMatch(it->fileName())) {
                ++it;
                continue;
            }
            sorted.append(it->absoluteFilePath());
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
    if (!codec.isEmpty()) {
        if (codec.toLower() == "system") {
            ts.setCodec(QTextCodec::codecForLocale());
        } else if (codec.toLower() == "autodetect") {
            CharsetDetector det;
            if (det.isAvailable()) {
                QByteArray charset = det.detect(f.readAll());
                qDebug("charset>>>>>>>>: %s", charset.constData());
                f.seek(0);
                if (!charset.isEmpty())
                    ts.setCodec(QTextCodec::codecForName(charset));
            }
        } else {
            ts.setCodec(QTextCodec::codecForName(codec));
        }
    }
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
    if (fs.isEmpty())
        return false;
    std::sort(fs.begin(), fs.end());
    foreach (const SubtitleFrame& f, fs) {
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
    QString name = QUrl::fromPercentEncoding(url.toEncoded()).section(ushort('/'), -1);
    if (name.isEmpty())
        name = QFileInfo(file_name).fileName(); //priv->name.section('/', -1); // if no seperator?
    if (name.isEmpty())
        name = QStringLiteral("QtAV_u8_sub_cache");
    name.append(QStringLiteral("_%1").arg((quintptr)this));
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


SubtitleAPIProxy::SubtitleAPIProxy(QObject* obj)
    : m_obj(obj)
    , m_s(0)
{}

void SubtitleAPIProxy::setSubtitle(Subtitle *sub)
{
    m_s = sub;

    QObject::connect(m_s, SIGNAL(canRenderChanged()), m_obj, SIGNAL(canRenderChanged()));
    QObject::connect(m_s, SIGNAL(contentChanged()), m_obj, SIGNAL(contentChanged()));
    QObject::connect(m_s, SIGNAL(loaded(QString)), m_obj, SIGNAL(loaded(QString)));

    QObject::connect(m_s, SIGNAL(codecChanged()), m_obj, SIGNAL(codecChanged()));
    QObject::connect(m_s, SIGNAL(enginesChanged()), m_obj, SIGNAL(enginesChanged()));
    QObject::connect(m_s, SIGNAL(engineChanged()), m_obj, SIGNAL(engineChanged()));
//    QObject::connect(m_s, SIGNAL(fileNameChanged()), m_obj, SIGNAL(fileNameChanged()));
    QObject::connect(m_s, SIGNAL(dirsChanged()), m_obj, SIGNAL(dirsChanged()));
    QObject::connect(m_s, SIGNAL(fuzzyMatchChanged()), m_obj, SIGNAL(fuzzyMatchChanged()));
    QObject::connect(m_s, SIGNAL(suffixesChanged()), m_obj, SIGNAL(suffixesChanged()));
    QObject::connect(m_s, SIGNAL(supportedSuffixesChanged()), m_obj, SIGNAL(supportedSuffixesChanged()));
    QObject::connect(m_s, SIGNAL(delayChanged()), m_obj, SIGNAL(delayChanged()));
    QObject::connect(m_s, SIGNAL(fontFileChanged()), m_obj, SIGNAL(fontFileChanged()));
    QObject::connect(m_s, SIGNAL(fontsDirChanged()), m_obj, SIGNAL(fontsDirChanged()));
    QObject::connect(m_s, SIGNAL(fontFileForcedChanged()), m_obj, SIGNAL(fontFileForcedChanged()));
}

void SubtitleAPIProxy::setCodec(const QByteArray& value)
{
    if (!m_s)
        return;
    m_s->setCodec(value);
}

QByteArray SubtitleAPIProxy::codec() const
{
    if (!m_s)
        return QByteArray();
    return m_s->codec();
}
bool SubtitleAPIProxy::isLoaded() const
{
    return m_s && m_s->isLoaded();
}

void SubtitleAPIProxy::setEngines(const QStringList& value)
{
    if (!m_s)
        return;
    m_s->setEngines(value);
}

QStringList SubtitleAPIProxy::engines() const
{
    if (!m_s)
        return QStringList();
    return m_s->engines();
}
QString SubtitleAPIProxy::engine() const
{
    if (!m_s)
        return QString();
    return m_s->engine();
}

void SubtitleAPIProxy::setFuzzyMatch(bool value)
{
    if (!m_s)
        return;
    m_s->setFuzzyMatch(value);
}

bool SubtitleAPIProxy::fuzzyMatch() const
{
    return m_s && m_s->fuzzyMatch();
}
#if 0
void SubtitleAPIProxy::setFileName(const QString& name)
{
    if (!m_s)
        return;
    m_s->setFileName(name);
}

QString SubtitleAPIProxy::fileName() const
{
    if (!m_s)
        return QString();
    return m_s->fileName();
}
#endif

void SubtitleAPIProxy::setDirs(const QStringList &value)
{
    if (!m_s)
        return;
    m_s->setDirs(value);
}

QStringList SubtitleAPIProxy::dirs() const
{
    if (!m_s)
        return QStringList();
    return m_s->dirs();
}

QStringList SubtitleAPIProxy::supportedSuffixes() const
{
    if (!m_s)
        return QStringList();
    return m_s->supportedSuffixes();
}

void SubtitleAPIProxy::setSuffixes(const QStringList& value)
{
    if (!m_s)
        return;
    m_s->setSuffixes(value);
}

QStringList SubtitleAPIProxy::suffixes() const
{
    if (!m_s)
        return QStringList();
    return m_s->suffixes();
}

bool SubtitleAPIProxy::canRender() const
{
    return m_s && m_s->canRender();
}

void SubtitleAPIProxy::setDelay(qreal value)
{
    if (!m_s)
        return;
    m_s->setDelay(value);
}

qreal SubtitleAPIProxy::delay() const
{
    return m_s ? m_s->delay() : 0;
}

QString SubtitleAPIProxy::fontFile() const
{
    return m_s ? m_s->fontFile() : QString();
}

void SubtitleAPIProxy::setFontFile(const QString &value)
{
    if (!m_s)
        return;
    m_s->setFontFile(value);
}

QString SubtitleAPIProxy::fontsDir() const
{
    return m_s ? m_s->fontsDir() : QString();
}

void SubtitleAPIProxy::setFontsDir(const QString &value)
{
    if (!m_s)
        return;
    m_s->setFontsDir(value);
}

bool SubtitleAPIProxy::isFontFileForced() const
{
    return m_s && m_s->isFontFileForced();
}

void SubtitleAPIProxy::setFontFileForced(bool value)
{
    if (!m_s)
        return;
    m_s->setFontFileForced(value);
}

} //namespace QtAV
