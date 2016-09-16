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

#ifndef QTAV_QUICKVIDEOPREVIEW_H
#define QTAV_QUICKVIDEOPREVIEW_H

#include <QtAV/VideoFrameExtractor.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QmlAV/QuickFBORenderer.h>
typedef QuickFBORenderer BaseQuickRenderer;
#else
#include <QmlAV/QQuickItemRenderer.h>
typedef QQuickItemRenderer BaseQuickRenderer;
#endif
namespace QtAV {

class QuickVideoPreview : public BaseQuickRenderer
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
    void displayNoFrame();

private:
    QUrl m_file;
    VideoFrameExtractor m_extractor;
};
} //namespace QtAV
#endif // QUICKVIDEOPREVIEW_H
