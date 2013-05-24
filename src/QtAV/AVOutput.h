/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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

#ifndef QAV_WRITER_H
#define QAV_WRITER_H

#include <QtCore/QByteArray>
#include <QtAV/QtAV_Global.h>

/*!
 * TODO: add api id(), name(), detail()
 */

namespace QtAV {

class AVDecoder;
class AVOutputPrivate;
class Filter;
class FilterContext;
class Statistics;
class Q_EXPORT AVOutput
{
    DPTR_DECLARE_PRIVATE(AVOutput)
public:
    AVOutput();
    virtual ~AVOutput() = 0;
    /* store the data ref, then call convertData() and write(). tryPause() will be called*/
    bool writeData(const QByteArray& data);
    bool isAvailable() const;
    virtual bool open() = 0;
    virtual bool close() = 0;
    //Demuxer thread automatically paused because packets will be full
    void pause(bool p); //processEvents when waiting?
    bool isPaused() const;

    /* check context.type, if not compatible(e.g. type is QtPainter but vo is d2d)
     * but type is not same is also ok if we just use render engine in vo but not in context.
     * TODO:
     * what if multiple vo(different render engines) share 1 player?
     * private?: set in AVThread, context is used by this class internally
     */
    virtual int filterContextType() const;
    //No filters() api, they are used internally?
    //for add/remove/clear on list. avo.add/remove/clear?
    QList<Filter*>& filters();
protected:
    AVOutput(AVOutputPrivate& d);
    /*
     * Reimplement this. You should convert and save the decoded data, e.g. QImage,
     * which will be used in write() or some other functions. Do nothing by default.
     */
    virtual void convertData(const QByteArray& data);
    virtual bool write() = 0; //TODO: why pure may case "pure virtual method called"
    /*
     * If the pause state is true setted by pause(true), then block the thread and wait for pause state changed, i.e. pause(false)
     * and return true. Otherwise, return false immediatly.
     */
    bool tryPause();

    DPTR_DECLARE(AVOutput)

private:
    void setStatistics(Statistics* statistics);
    friend class AVPlayer;
};

} //namespace QtAV
#endif //QAV_WRITER_H
