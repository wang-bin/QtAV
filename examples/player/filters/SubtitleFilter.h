#ifndef QTAV_SUBTITLEFILTER_H
#define QTAV_SUBTITLEFILTER_H

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtAV/LibAVFilter.h>
#include <QtAV/AVPlayer.h>
using namespace QtAV;
class SubtitleFilter : public QObject, public LibAVFilter
{
    Q_OBJECT
public:
    explicit SubtitleFilter(QObject *parent = 0);
    void setPlayer(AVPlayer* player);
    QString setContent(const QString& doc); // return utf8 subtitle path
    bool setFile(const QString& filePath);
    QString file() const;
    bool autoLoad() const;

public slots:
    // TODO: enable changed & autoload=> load
    void setAutoLoad(bool value);
    void findAndSetFile(const QString& path);
    void onPlayerStart();

private:
    bool m_auto;
    AVPlayer *m_player;
    QString m_file;
    // convert to utf8 to ensure ffmpeg can open it.
    QHash<QString,QString> m_u8_files;
};

#endif // QTAV_SUBTITLEFILTER_H
