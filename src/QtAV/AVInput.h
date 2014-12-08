/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_AVINPUT_H
#define QTAV_AVINPUT_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/FactoryDefine.h>
#include <QtCore/QStringList>

namespace QtAV {

typedef int AVInputId;
class AVInput;
FACTORY_DECLARE(AVInput)

class AVInputPrivate;
class Q_AV_EXPORT AVInput
{
    DPTR_DECLARE_PRIVATE(AVInput)
    Q_DISABLE_COPY(AVInput)
public:
    /// Registered AVInput::name(): "QIODevice", "QFile"
    static QStringList builtInNames();
    static AVInput* create(const QString& name);
    /*!
     * \brief createForProtocol
     * If an AVInput subclass SomeInput.protocols() contains the protocol, return it's instance.
     * "QFile" input has protocols "qrc"(and empty "" means "qrc")
     * \return Null if none of registered AVInput supports the protocol
     */
    static AVInput* createForProtocol(const QString& protocol);

    AVInput();
    virtual ~AVInput();
    virtual QString name() const = 0;
    virtual void setUrl(const QString& url);
    QString url() const;
    /// supported protocols. default is empty
    virtual const QStringList& protocols() const;
    virtual bool isSeekable() const = 0;
    virtual qint64 read(char *data, qint64 maxSize) = 0;
    /*!
     * \brief seek
     * \param from
     * 0: seek to offset from beginning position
     * 1: from current position
     * 2: from end position
     * \return true if success
     */
    virtual bool seek(qint64 offset, int from) = 0;
    /*!
     * \brief position
     * MUST implement this. Used in seek
     */
    virtual qint64 position() const = 0;
    /*!
     * \brief size
     * \return <=0 if not support
     */
    virtual qint64 size() const = 0;
    //struct AVIOContext; //anonymous struct in FFmpeg1.0.x
    void* avioContext(); //const?
    void release(); //TODO: how to remove it?
protected:
    AVInput(AVInputPrivate& d);
    virtual void onUrlChanged();
    DPTR_DECLARE(AVInput)
};

} //namespace QtAV
#endif // QTAV_AVINPUT_H
