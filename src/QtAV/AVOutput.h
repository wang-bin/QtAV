/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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

#ifndef QAV_WRITER_H
#define QAV_WRITER_H

#include <QtCore/QByteArray>
#include <QtAV/QtAV_Global.h>

namespace QtAV {

class AVDecoder;
class AVOutputPrivate;
class Q_EXPORT AVOutput
{
    DPTR_DECLARE_PRIVATE(AVOutput)
public:
    AVOutput();
    virtual ~AVOutput() = 0;
    //Call tryPause() first to try to pause
	bool writeData(const QByteArray& data);
    virtual bool open() = 0;
    virtual bool close() = 0;
    //Demuxer thread automatically paused because packets will be full
    void pause(bool p); //processEvents when waiting?
    bool isPaused() const;
protected:
    AVOutput(AVOutputPrivate& d);
	/*
	 * Reimplement this. You should convert and save the decoded data, e.g. QImage,
	 * which will be used in write() or some other functions. Do nothing by default.
	 */
	virtual void convertData(const QByteArray& data) = 0;// = 0; //TODO: why pure may case "pure virtual method called"
	virtual bool write() = 0; //TODO: why pure may case "pure virtual method called"
	/* If paused is true, block the thread(block write()) and wait
     * for paused be false, else do nothing. */
	void tryPause();

    DPTR_DECLARE(AVOutput)
};

} //namespace QtAV
#endif //QAV_WRITER_H
