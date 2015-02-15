/******************************************************************************
    VideoRendererTypes: type id and manually id register function
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAVWidgets/VideoPreviewWidget.h"
#include "QtAV/VideoFrameExtractor.h"
#include "QtAV/VideoOutput.h"
#include <QtGui/QResizeEvent>

namespace QtAV {

VideoPreviewWidget::VideoPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_keep_ar(false)
    , m_extractor(new VideoFrameExtractor(this))
    , m_out(new VideoOutput(VideoRendererId_Widget, this))
    // FIXME: opengl may crash, so use software renderer here
{
    setWindowFlags(Qt::FramelessWindowHint);
    Q_ASSERT_X(m_out->widget(), "VideoPreviewWidget()", "widget based renderer is not found");
    m_out->widget()->setParent(this);
    connect(m_extractor, SIGNAL(positionChanged()), this, SIGNAL(timestampChanged()));
    connect(m_extractor, SIGNAL(frameExtracted(QtAV::VideoFrame)), SLOT(displayFrame(QtAV::VideoFrame)));
    connect(m_extractor, SIGNAL(error()), SLOT(displayNoFrame()));
    connect(this, SIGNAL(fileChanged()), SLOT(displayNoFrame()));
    m_extractor->setAutoExtract(false);
}

void VideoPreviewWidget::resizeEvent(QResizeEvent *e)
{
    m_out->widget()->resize(e->size());
}

void VideoPreviewWidget::setTimestamp(qint64 value)
{
    m_extractor->setPosition(value);
}

qint64 VideoPreviewWidget::timestamp() const
{
    return m_extractor->position();
}

void VideoPreviewWidget::preview()
{
    m_extractor->extract();
}

void VideoPreviewWidget::setFile(const QString &value)
{
    if (m_file == value)
        return;
    m_file = value;
    m_extractor->setSource(m_file);
    emit fileChanged();
}

QString VideoPreviewWidget::file() const
{
    return m_file;
}

void VideoPreviewWidget::displayFrame(const QtAV::VideoFrame &frame)
{
    int diff = qAbs(qint64(frame.timestamp()*1000.0) - m_extractor->position());
    if (diff > m_extractor->precision()) {
        //qWarning("timestamp difference (%d/%lld) is too large! ignore", diff);
    }
    if (m_out->isSupported(frame.format().pixelFormat())) {
        m_out->receive(frame);
        return;
    }
    QSize s = m_out->widget()->rect().size();
    if (m_keep_ar) {
        QSize fs(frame.size());
        fs.scale(s, Qt::KeepAspectRatio);
        s = fs;
    }
    VideoFrame f(frame.to(VideoFormat::Format_RGB32, s));
    if (!f.isValid())
        return;
    m_out->receive(f);
}

void VideoPreviewWidget::displayNoFrame()
{
    m_out->receive(VideoFrame());
}

} //namespace QtAV
