/******************************************************************************
    VideoCapture.cpp: description
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>
    
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
#include "QtAV/VideoCapture.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QDesktopServices>
#else
#include <QtCore/QStandardPaths>
#endif
#include "utils/Logger.h"

namespace QtAV {

Q_GLOBAL_STATIC(QThreadPool, videoCaptureThreadPool)
static bool app_is_dieing = false;
// TODO: cancel if qapp is quit
class CaptureTask : public QRunnable
{
public:
    CaptureTask(VideoCapture* c)
        : cap(c)
        , save(true)
        , original_fmt(false)
        , quality(-1)
        , format(QStringLiteral("PNG"))
        , qfmt(QImage::Format_ARGB32)
    {
        setAutoDelete(true);
    }
    virtual void run() {
        if (app_is_dieing) {
            qDebug("app is dieing. cancel capture task %p", this);
            return;
        }
        QImage image(frame.toImage());
        if (image.isNull()) {
            qWarning("Failed to convert to QImage");
            return;
        }
        QMetaObject::invokeMethod(cap, "imageCaptured", Q_ARG(QImage, image));
        if (!save)
            return;
        bool main_thread = QThread::currentThread() == qApp->thread();
        qDebug("capture task running in thread %p [main thread=%d]", QThread::currentThreadId(), main_thread);
        if (!QDir(dir).exists()) {
            if (!QDir().mkpath(dir)) {
                qWarning("Failed to create capture dir [%s]", qPrintable(dir));
                QMetaObject::invokeMethod(cap, "failed");
                return;
            }
        }
        name += QString::number(frame.timestamp(), 'f', 3);
        QString path(dir + QStringLiteral("/") + name + QStringLiteral("."));
        if (original_fmt) {
            if (!frame.constBits(0)) {
                frame = frame.to(frame.format());
            }
            path.append(frame.format().name());
            qDebug("Saving capture to %s", qPrintable(path));
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) {
                qWarning("VideoCapture is failed to open file %s", qPrintable(path));
                QMetaObject::invokeMethod(cap, "failed");
                return;
            }
            int sz = 0;
            const char* data = (const char*)frame.frameDataPtr(&sz);
            if (file.write(data, sz) <= 0) {
                qWarning("VideoCapture is failed to write captured frame with original format");
                QMetaObject::invokeMethod(cap, "failed");
                file.close();
                return;
            }
            file.close();
            QMetaObject::invokeMethod(cap, "saved", Q_ARG(QString, path));
            return;
        }
        if (image.isNull())
            return;
        path.append(format.toLower());
        qDebug("Saving capture to %s", qPrintable(path));
        bool ok = image.save(path, format.toLatin1().constData(), quality);
        if (!ok) {
            qWarning("Failed to save capture");
            QMetaObject::invokeMethod(cap, "failed");
        }
        QMetaObject::invokeMethod(cap, "saved", Q_ARG(QString, path));
    }

    VideoCapture *cap;
    bool save;
    bool original_fmt;
    int quality;
    QString format, dir, name;
    QImage::Format qfmt;
    VideoFrame frame;
};

VideoCapture::VideoCapture(QObject *parent) :
    QObject(parent)
  , async(true)
  , auto_save(true)
  , original_fmt(false)
  , qfmt(QImage::Format_ARGB32)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    dir = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
#else
    dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
#endif
    if (dir.isEmpty())
        dir = qApp->applicationDirPath() + QStringLiteral("/capture");
    fmt = QStringLiteral("PNG");
    qual = -1;
    // seems no direct connection is fine too
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(handleAppQuit()), Qt::DirectConnection);
}

void VideoCapture::setAsync(bool value)
{
    if (async == value)
        return;
    async = value;
    Q_EMIT asyncChanged();
}

bool VideoCapture::isAsync() const
{
    return async;
}

void VideoCapture::setAutoSave(bool value)
{
    if (auto_save == value)
        return;
    auto_save = value;
    Q_EMIT autoSaveChanged();
}

bool VideoCapture::autoSave() const
{
    return auto_save;
}

void VideoCapture::setOriginalFormat(bool value)
{
    if (original_fmt == value)
        return;
    original_fmt = value;
    Q_EMIT originalFormatChanged();
}

bool VideoCapture::isOriginalFormat() const
{
    return original_fmt;
}

void VideoCapture::handleAppQuit()
{
    app_is_dieing = true;
    // TODO: how to cancel? since qt5.2, we can use clear()
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    videoCaptureThreadPool()->clear();
#endif
    videoCaptureThreadPool()->setExpiryTimeout(0);
    videoCaptureThreadPool()->waitForDone();
}

void  VideoCapture::capture()
{
    Q_EMIT requested();
}

void VideoCapture::start()
{
    Q_EMIT frameAvailable(frame);
    if (!frame.isValid() || !frame.constBits(0)) { // if frame is always cloned, then size is at least width*height
        qDebug("Captured frame from hardware decoder surface.");
    }
    CaptureTask *task = new CaptureTask(this);
    // copy properties so the task will not be affect even if VideoCapture properties changed
    task->save = autoSave();
    task->original_fmt = original_fmt;
    task->quality = qual;
    task->dir = dir;
    task->name = name;
    task->format = fmt;
    task->qfmt = qfmt;
    task->frame = frame; //copy here and it's safe in capture thread because start() is called immediatly after setVideoFrame
    if (isAsync()) {
        videoCaptureThreadPool()->start(task);
    } else {
        task->run();
        delete task;
    }
}

void VideoCapture::setSaveFormat(const QString &format)
{
    if (format.toLower() == fmt.toLower())
        return;
    fmt = format;
    Q_EMIT saveFormatChanged();
}

QString VideoCapture::saveFormat() const
{
    return fmt;
}

void VideoCapture::setQuality(int value)
{
    if (qual == value)
        return;
    qual = value;
    Q_EMIT qualityChanged();
}

int VideoCapture::quality() const
{
    return qual;
}

void VideoCapture::setCaptureName(const QString &value)
{
    if (name == value)
        return;
    name = value;
    Q_EMIT captureNameChanged();
}

QString VideoCapture::captureName() const
{
    return name;
}

void VideoCapture::setCaptureDir(const QString &value)
{
    if (dir == value)
        return;
    dir = value;
    Q_EMIT captureDirChanged();
}

QString VideoCapture::captureDir() const
{
    return dir;
}

/*
 * If the frame is not created for direct rendering, then the frame data is already deep copied, so detach is enough.
 * TODO: map frame from texture etc.
 */
void VideoCapture::setVideoFrame(const VideoFrame &frame)
{
    // parameter in ready(QtAV::VideoFrame) ensure we can access the frame without lock
    /*
     * clone here may block VideoThread. But if not clone here, the frame may be
     * modified outside and is not safe.
     */
    this->frame = frame.clone(); // TODO: no clone, use detach()
}

} //namespace QtAV

