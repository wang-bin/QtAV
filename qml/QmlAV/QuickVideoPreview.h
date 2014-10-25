#ifndef QTAV_QUICKVIDEOPREVIEW_H
#define QTAV_QUICKVIDEOPREVIEW_H

#include <QmlAV/QQuickItemRenderer.h>
#include <QtAV/VideoFrameExtractor.h>
#include <QtAV/VideoCapture.h>

namespace QtAV {
class QMLAV_EXPORT QuickVideoPreview : public QQuickItemRenderer
{
    Q_OBJECT
    // position conflicts with QQuickItem.position
    Q_PROPERTY(int timestamp READ timestamp WRITE setTimestamp NOTIFY timestampChanged)
    // source is already in VideoOutput
    Q_PROPERTY(QUrl file READ file WRITE setFile NOTIFY fileChanged)
public:
    explicit QuickVideoPreview(QQuickItem *parent = 0);
    void setTimestamp(int value);
    int timestamp() const;
    void setFile(const QUrl& value);
    QUrl file() const;

signals:
    void timestampChanged();
    void fileChanged();

private slots:
    void displayFrame(const QtAV::VideoFrame& frame); //parameter VideoFrame

private:
    QUrl m_file;
    VideoFrameExtractor m_extractor;
    VideoCapture m_capture;
};
} //namespace QtAV
#endif // QUICKVIDEOPREVIEW_H
