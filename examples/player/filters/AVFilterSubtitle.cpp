#include "AVFilterSubtitle.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

AVFilterSubtitle::AVFilterSubtitle(QObject *parent)
    :  LibAVFilterVideo(parent)
    , m_auto(true)
    , m_player(0)
{
    //connect(this, SIGNAL(statusChanged()), SLOT(onStatusChanged()));
}

void AVFilterSubtitle::setPlayer(AVPlayer *player)
{
    if (m_player == player)
        return;
    uninstall();
    if (m_player) {
        disconnect(this);
    }
    m_player = player;
    if (!player)
        return;
    player->installFilter(this);
    if (m_auto) {
//        connect(player, SIGNAL(fileChanged(QString)), SLOT(findAndSetFile(QString)));
        connect(player, SIGNAL(started()), SLOT(onPlayerStart()));
    }
}

bool AVFilterSubtitle::setFile(const QString &filePath)
{
    setOptions(QString());
    if (m_file != filePath) {
        emit fileChanged(filePath);
        // DO NOT return here because option is null now
    }
    m_file = filePath;
    QString u = m_u8_files[filePath];
    if (!u.isEmpty() && !QFile(u).exists())
        u = QString();
    if (u.isEmpty()) {
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning("open '%s' error: %s", filePath.toUtf8().constData(), f.errorString().toUtf8().constData());
            return false;
        }
        QTextStream ts(&f);
        ts.setAutoDetectUnicode(true);
        QString s = ts.readAll();
        QString tmp = setContent(s);
        if (!tmp.isEmpty()) {
            u = tmp;
            m_u8_files[filePath] = u;
        } else {
            // read the origin file
            qWarning("open cache file '%s' error, originanl subtitle file will be used", filePath.toUtf8().constData());
        }
    }
    if (u.isEmpty())
        u = filePath;
    // filter_name=argument. use ' to quote the argument, use \ to escaping chars within quoted text. on windows, path can be C:/a/b/c, ":" must be escaped
    u.replace(QLatin1String(":"), QLatin1String("\\:"));
    setOptions(QString::fromLatin1("subtitles='%1'").arg(u));
    qDebug("subtitle loaded: %s", filePath.toUtf8().constData());
    return true;
}

QString AVFilterSubtitle::file() const
{
    return m_file;
}

QString AVFilterSubtitle::setContent(const QString &doc)
{
    QString name = QFileInfo(m_file).fileName();
    if (name.isEmpty())
        name = QString::fromLatin1("QtAV_u8_sub_cache");
    QFile w(QDir::temp().absoluteFilePath(name));
    if (w.open(QIODevice::WriteOnly)) {
        w.write(doc.toUtf8());
        w.close();
    } else {
        return QString();
    }
    return w.fileName();
}

void AVFilterSubtitle::setAutoLoad(bool value)
{
    if (m_auto == value)
        return;
    m_auto = value;
    emit autoLoadChanged(value);
    if (!m_player || !m_auto)
        return;
    connect(m_player, SIGNAL(started()), SLOT(onPlayerStart()));
}

bool AVFilterSubtitle::autoLoad() const
{
    return m_auto;
}

void AVFilterSubtitle::findAndSetFile(const QString &path)
{
    QFileInfo fi(path);
    QDir dir(fi.dir());
    QString name = fi.completeBaseName(); // video suffix has only 1 dot
    QStringList list = dir.entryList(QStringList() << name + QString::fromLatin1("*.ass") << name + QString::fromLatin1("*.ssa"), QDir::Files);
    list.append(dir.entryList(QStringList() << QString::fromLatin1("*.srt"), QDir::Files));
    foreach (const QString& f, list) {
        // why it happens?
        if (!f.startsWith(name))
            continue;
        if (setFile(dir.absoluteFilePath(f)))
            break;
    }
}

void AVFilterSubtitle::onPlayerStart()
{
    setOptions(QString());
    if (!autoLoad())
        return;
    findAndSetFile(m_player->file());
}

void AVFilterSubtitle::onStatusChanged()
{
    if (status() == ConfigureOk) {
        emit loaded();
    } else if (status() == ConfigureFailed) {
        if (options().isEmpty())
            return;
        emit loadError();
    }
}
