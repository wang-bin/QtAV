/******************************************************************************
    VideoCapture.cpp: description
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
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


#include "VideoCapture.h"
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtGui/QImage>

namespace QtAV {

class CaptureTask : public QRunnable
{
public:
    CaptureTask(VideoCapture* c):cap(c){
        format = "PNG";
        setAutoDelete(true);
    }
    //TODO: abort when app is exiting or wait for capturing finished
    virtual void run() {
        bool main_thread = QThread::currentThread() == qApp->thread();
        qDebug("capture task running in thread %p [main thread=%d]", QThread::currentThreadId(), main_thread);
    //TODO: ensure dir exists
        if (!QDir(dir).exists()) {
            if (!QDir().mkpath(dir)) {
                cap->error = VideoCapture::DirCreateError;
                qWarning("Failed to create capture dir [%s]", qPrintable(dir));
                QMetaObject::invokeMethod(cap, "failed");
                return;
            }
        }
#if QT_VERSION >= QT_VERSION_CHECK(4, 0, 0)
        QImage image((uchar*)data.data(), width, height, QImage::Format_RGB32);
#else
        QImage image((uchar*)data.data(), width, height, 16, NULL, 0, QImage::IgnoreEndian);
#endif
        QString path(dir + "/" + name + "." + format.toLower());
        qDebug("Saving capture to %s", qPrintable(path));
		bool ok = image.save(path, format.toLatin1().constData(), quality);
        if (!ok) {
            cap->error = VideoCapture::SaveError;
            qWarning("Failed to save capture");
            QMetaObject::invokeMethod(cap, "failed");
        }
        QMetaObject::invokeMethod(cap, "finished");
    }

    VideoCapture *cap;
    int width, height, quality;
    QString format, dir, name;
    QByteArray data;
};

VideoCapture::VideoCapture(QObject *parent) :
    QObject(parent),async(true),error(NoError)
{
    fmt = "PNG";
    qual = -1;
}

VideoCapture::~VideoCapture()
{
    qDebug("%p %s %s", QThread::currentThreadId(), __FILE__, __FUNCTION__);
}

void VideoCapture::setAsync(bool async)
{
    this->async = async;
}

bool VideoCapture::isAsync() const
{
    return async;
}

void VideoCapture::request()
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    error = NoError;
    emit ready();
    CaptureTask *task = new CaptureTask(this);
    task->width = width;
    task->height = height;
    task->quality = qual;
    task->dir = dir;
    task->name = name;
    task->format = fmt;
    task->data = data;
    if (isAsync()) {
        QThreadPool::globalInstance()->start(task);
    } else {
       task->run();
    }
}

void VideoCapture::setFormat(const QString &format)
{
    fmt = format;
}

QString VideoCapture::format() const
{
    return fmt;
}

void VideoCapture::setQuality(int quality)
{
    qual = quality;
}

int VideoCapture::quality() const
{
    return qual;
}

void VideoCapture::setCaptureName(const QString &name)
{
    this->name = name;
}

QString VideoCapture::captureName() const
{
    return name;
}

void VideoCapture::setCaptureDir(const QString &dir)
{
    this->dir = dir;
}

QString VideoCapture::captureDir() const
{
    return dir;
}

void VideoCapture::setRawImage(const QByteArray &raw, const QSize &size)
{
    setRawImage(raw, size.width(), size.height());
}

void VideoCapture::setRawImage(const QByteArray &raw, int w, int h)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    width = w;
    height = h;
    data = raw;
}

void VideoCapture::getRawImage(QByteArray *raw, int *w, int *h)
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    *raw = data;
    *w = width;
    *h = height;
}

} //namespace QtAV

