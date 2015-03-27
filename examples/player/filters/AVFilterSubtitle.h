#ifndef QTAV_AVFilterSubtitle_H
#define QTAV_AVFilterSubtitle_H

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtAV/LibAVFilter.h>
#include <QtAV/AVPlayer.h>
using namespace QtAV;
class AVFilterSubtitle : public LibAVFilterVideo
{
    Q_OBJECT
    Q_PROPERTY(bool autoLoad READ autoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
public:
    explicit AVFilterSubtitle(QObject *parent = 0);
    void setPlayer(AVPlayer* player);
    QString setContent(const QString& doc); // return utf8 subtitle path
    bool setFile(const QString& filePath);
    QString file() const;
    bool autoLoad() const;

signals:
    void loaded();
    void loadError();
    void fileChanged(const QString& path);
    void autoLoadChanged(bool value);
public slots:
    // TODO: enable changed & autoload=> load
    void setAutoLoad(bool value);
    void findAndSetFile(const QString& path);
    void onPlayerStart();
private slots:
    void onStatusChanged();
private:
    bool m_auto;
    AVPlayer *m_player;
    QString m_file;
    // convert to utf8 to ensure ffmpeg can open it.
    QHash<QString,QString> m_u8_files;
};

#endif // QTAV_AVFilterSubtitle_H
