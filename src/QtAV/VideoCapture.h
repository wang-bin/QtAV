/******************************************************************************
    VideoCapture.h: description
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
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


#ifndef VIDEOCAPTURE_H
#define VIDEOCAPTURE_H

#include <QtCore/QObject>

class QSize;
namespace QtAV {

//on capture per thread or all in one thread?
class VideoCapture : public QObject
{
    Q_OBJECT
public:
    explicit VideoCapture(QObject *parent = 0);
    ~VideoCapture();
    //void setAsync(bool async);
    void request();
    void setFormat(const QString& format);
    QString format() const;
    void setQuality(int quality);
    int quality() const;
    void setCaptureName(const QString& name);
    QString captureName() const;
    void setCaptureDir(const QString& dir);
    QString captureDir() const;
    void setRawImageSize(int width, int height);
    void setRawImageSize(const QSize& size);
    void setRawImageData(const QByteArray& raw);
#if CAPTURE_USE_EVENT
    virtual bool event(QEvent *event);
#endif
signals:
    void finished(); //TODO: with error code
private:
    //TODO: use blocking queue? If not, the parameters will change when thre previous is not finished
    //or use a capture event that wrapper all these parameters
    int width, height;
    int qual;
    QString fmt;
    QString name, dir;
    QByteArray data;
#if CAPTURE_USE_EVENT
    QThread *capture_thread;
#endif //CAPTURE_USE_EVENT
    //QImage image;
};

} //namespace QtAV
#endif // VIDEOCAPTURE_H
