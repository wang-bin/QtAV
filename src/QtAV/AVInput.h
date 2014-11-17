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

namespace QtAV {

class AVInputPrivate;
class Q_AV_EXPORT AVInput
{
    DPTR_DECLARE_PRIVATE(AVInput)
    Q_DISABLE_COPY(AVInput)
public:
    AVInput();
    virtual ~AVInput();
    virtual bool isSeekable() = 0;
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
    DPTR_DECLARE(AVInput)
};

} //namespace QtAV
#endif // QTAV_AVINPUT_H
