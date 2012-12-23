/******************************************************************************
    VideoCapture.cpp: description
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

    virtual void run() {
        qDebug("capture task running in thread %p", QThread::currentThreadId());
    //TODO: ensure dir exists
        if (!QDir(dir).exists()) {
            if (!QDir().mkpath(dir)) {
                qWarning("Failed to create capture dir [%s]", qPrintable(dir));
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
        if (!ok)
            qWarning("Failed to save capture");
        QMetaObject::invokeMethod(cap, "finished");
    }

    VideoCapture *cap;
    int width, height, quality;
    QString format, dir, name;
    QByteArray data;
};

VideoCapture::VideoCapture(QObject *parent) :
    QObject(parent)
{
#if CAPTURE_USE_EVENT
    capture_thread = 0;
#endif //CAPTURE_USE_EVENT
    fmt = "PNG";
    qual = -1;
}

VideoCapture::~VideoCapture()
{
    qDebug("%p %s %s", QThread::currentThreadId(), __FILE__, __FUNCTION__);
#if CAPTURE_USE_EVENT
    if (capture_thread) {
        capture_thread->quit();
        delete capture_thread;
        capture_thread = 0;
    }
#endif //CAPTURE_USE_EVENT
}

void VideoCapture::request()
{
#if CAPTURE_USE_EVENT
    if (!capture_thread) {
        qDebug("Creating capture thread %p", QThread::currentThreadId());
        capture_thread = new QThread;
        capture_thread->start();
        moveToThread(capture_thread);
    }
    qDebug("Posting capture event in thread %p", QThread::currentThreadId());
    qApp->postEvent(this, new QEvent(QEvent::User));
#else
    CaptureTask *task = new CaptureTask(this);
    task->width = width;
    task->height = height;
    task->quality = qual;
    task->dir = dir;
    task->name = name;
    task->format = fmt;
    task->data = data;
    QThreadPool::globalInstance()->start(task);
#endif //CAPTURE_USE_EVENT
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

void VideoCapture::setRawImageSize(int width, int height)
{
    this->width = width;
    this->height = height;
}

void VideoCapture::setRawImageSize(const QSize &size)
{
    setRawImageSize(size.width(), size.height());
}

void VideoCapture::setRawImageData(const QByteArray &raw)
{
    data = raw;
}
#if CAPTURE_USE_EVENT
bool VideoCapture::event(QEvent *event)
{
    if (event->type() != QEvent::User)
        return false;
    qDebug("Receive capture event in thread %p", QThread::currentThreadId());
//TODO: ensure dir exists
#if QT_VERSION >= QT_VERSION_CHECK(4, 0, 0)
    QImage image((uchar*)data.data(), width, height, QImage::Format_RGB32);
#else
    QImage image((uchar*)data.data(), width, height, 16, NULL, 0, QImage::IgnoreEndian);
#endif
    QString path(dir + "/" + name + "." + fmt.toLower());
    qDebug("Saving capture to %s", qPrintable(path));
    bool ok = image.save(path, fmt.toAscii().constData(), qual);
    if (!ok)
        qWarning("Failed to save capture");
    emit finished();
}
#endif
} //namespace QtAV

