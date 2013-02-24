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

#include <QtAV/AVOutput.h>
#include <private/AVOutput_p.h>

namespace QtAV {

AVOutput::AVOutput()
{
}

AVOutput::AVOutput(AVOutputPrivate &d)
    :DPTR_INIT(&d)
{
}

//TODO: why need this?
AVOutput::~AVOutput()
{
    pause(false); //Does not work. cond may still waiting when destroyed
}

bool AVOutput::writeData(const QByteArray &data)
{
    Q_UNUSED(data);
    //DPTR_D(AVOutput);
    //locker(&mutex)
    //TODO: make sure d.data thread safe. lock around here? for audio and video(main thread problem)?
    /* you can use d.data directly in AVThread. In other thread, it's not safe, you must do something
     * to make sure the data is not be modified in AVThread when using it*/
    //d_func().data = data;
	convertData(data);
	bool result = write();
	//write then pause: if capture when pausing, the displayed picture is captured
    tryPause();
	return result;
}

bool AVOutput::isAvailable() const
{
    return d_func().available;
}

void AVOutput::pause(bool p)
{
    DPTR_D(AVOutput);
    if (d.paused == p)
        return;
    d.paused = p;
    if (!d.paused)
        d.cond.wakeAll();
}

bool AVOutput::isPaused() const
{
    return d_func().paused;
}

//TODO: how to call this automatically before write()?
bool AVOutput::tryPause()
{
    DPTR_D(AVOutput);
    if (!d.paused)
        return false;
    QMutexLocker lock(&d.mutex);
    Q_UNUSED(lock);
    d.cond.wait(&d.mutex); //TODO: qApp->processEvents?
    return true;
}

void AVOutput::convertData(const QByteArray &data)
{
    //TODO: make sure d.data thread safe. lock here?
    DPTR_D(AVOutput);
    d.data = data;
}

} //namespace QtAV
