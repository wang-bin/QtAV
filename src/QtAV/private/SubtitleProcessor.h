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

#ifndef QTAV_SUBTITLEPROCESSOR_H
#define QTAV_SUBTITLEPROCESSOR_H

#include <QtCore/QList>
#include <QtGui/QImage>
#include <QtAV/QtAV_Global.h>
#include <QtAV/Subtitle.h>

namespace QtAV {

typedef QString SubtitleProcessorId;
class Q_AV_PRIVATE_EXPORT SubtitleProcessor
{
public:
    SubtitleProcessor();
    virtual ~SubtitleProcessor() {}
    virtual SubtitleProcessorId id() const = 0;
    virtual QString name() const = 0;
    /*!
     * \brief supportedTypes
     * \return a list of supported suffixes. e.g. [ "ass", "ssa", "srt" ]
     * used to find subtitle files with given suffixes
     */
    virtual QStringList supportedTypes() const = 0;
    /*!
     * \brief process
     * process subtitle from QIODevice.
     * \param dev dev is open and you don't have to close it
     * \return false if failed or does not supports iodevice, e.g. does not support sequential device
     */
    virtual bool process(QIODevice* dev) = 0;
    /*!
     * \brief process
     * default behavior is calling process(QFile*)
     * \param path
     * \return false if failed or does not support file
     */
    virtual bool process(const QString& path);
    /*!
     * \brief timestamps
     * call this after process(). SubtitleFrame.text must be set
     * \return
     */
    virtual QList<SubtitleFrame> frames() const = 0;
    virtual bool canRender() const { return false;}
    // return false if not supported
    virtual bool processHeader(const QByteArray& codec, const QByteArray& data) {
        Q_UNUSED(codec);
        Q_UNUSED(data);
        return false;
    }
    // return timestamp, insert it to Subtitle's internal linkedlist. can be invalid if only support renderering
    virtual SubtitleFrame processLine(const QByteArray& data, qreal pts = -1, qreal duration = 0) = 0;
    virtual QString getText(qreal pts) const = 0;
    // default null image
    virtual QImage getImage(qreal pts, QRect* boundingRect = 0);
    virtual SubImageSet getSubImages(qreal pts, QRect* boundingRect = 0);
    void setFrameSize(int width, int height);
    QSize frameSize() const;
    int frameWidth() const;
    int frameHeight() const;

    // font properties: libass only now
    virtual void setFontFile(const QString& file) {Q_UNUSED(file);}
    virtual void setFontsDir(const QString& dir) {Q_UNUSED(dir);}
    virtual void setFontFileForced(bool force) {Q_UNUSED(force);}
public:
    static void registerAll();
    template<class C> static bool Register(SubtitleProcessorId id, const char* name) { return Register(id, create<C>, name);}
    static SubtitleProcessor* create(SubtitleProcessorId id);
    static SubtitleProcessor* create(const char* name = "FFmpeg");
    /*!
     * \brief next
     * \param id NULL to get the first id address
     * \return address of id or NULL if not found/end
     */
    static SubtitleProcessorId* next(SubtitleProcessorId* id = 0);
    static const char* name(SubtitleProcessorId id);
    static SubtitleProcessorId id(const char* name);
private:
    template<class C> static SubtitleProcessor* create() { return new C();}
    typedef SubtitleProcessor* (*SubtitleProcessorCreator)();
    static bool Register(SubtitleProcessorId id, SubtitleProcessorCreator, const char *name);
protected:
    // default do nothing
    virtual void onFrameSizeChanged(int width, int height);
private:
    int m_width, m_height;
};

} //namespace QtAV
#endif // QTAV_SUBTITLEPROCESSOR_H
