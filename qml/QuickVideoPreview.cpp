#include "QuickVideoPreview.h"

namespace QtAV {

QuickVideoPreview::QuickVideoPreview(QQuickItem *parent) :
    QQuickItemRenderer(parent)
{
    connect(&m_extractor, SIGNAL(positionChanged()), this, SIGNAL(timestampChanged()));
    connect(&m_extractor, SIGNAL(frameExtracted(QtAV::VideoFrame)), SLOT(displayFrame(QtAV::VideoFrame)));
}

void QuickVideoPreview::setTimestamp(int value)
{
    m_extractor.setPosition((qint64)value);
}

int QuickVideoPreview::timestamp() const
{
    return (int)m_extractor.position();
}

void QuickVideoPreview::setFile(const QUrl &value)
{
    if (m_file == value)
        return;
    qDebug() << value;
    m_file = value;
    emit fileChanged();
    if (m_file.isLocalFile())
        m_extractor.setSource(m_file.toLocalFile());
    else
        m_extractor.setSource(m_file.toString());
}

QUrl QuickVideoPreview::file() const
{
    return m_file;
}

void QuickVideoPreview::displayFrame(const QtAV::VideoFrame &frame)
{
    receive(frame);
}

} //namespace QtAV
