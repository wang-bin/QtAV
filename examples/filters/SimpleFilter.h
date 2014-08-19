/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_EXAMPLE_SIMPLEFILTER_H
#define QTAV_EXAMPLE_SIMPLEFILTER_H

#include <QtAV/Filter.h>
#include <QtCore/QTime>
#include <QtGui/QMatrix4x4>
#include <QWidget>

namespace QtAV {

class SimpleFilter : public VideoFilter
{
    Q_OBJECT
public:
    SimpleFilter(QWidget *parent = 0);
    virtual ~SimpleFilter();
    void enableRotate(bool r);
    void enableWaveEffect(bool w);
    virtual VideoFilterContext::Type contextType() const { return VideoFilterContext::QtPainter; }
    void setText(const QString& text);
    QString text() const;
    //show image if text is null
    void setImage(const QImage& img);

    void prepare();

protected:
    virtual void process(Statistics* statistics, VideoFrame* frame);
    virtual void timerEvent(QTimerEvent *);
private:
    bool mCanRot;
    bool mWave;
    QTime mTime;
    qreal mStartValue;
    QString mText;
    QMatrix4x4 mMat;
    QImage mImage;
};

} //namespace QtAV

#endif // QTAV_EXAMPLE_SIMPLEFILTER_H
