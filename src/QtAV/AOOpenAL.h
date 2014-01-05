/******************************************************************************
    AOOpenAL.h: description
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>
    
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


#ifndef QTAV_AOOPENAL_H
#define QTAV_AOOPENAL_H

#include <QtAV/AudioOutput.h>

namespace QtAV {

class AOOpenALPrivate;
class Q_AV_EXPORT AOOpenAL : public AudioOutput
{
    DPTR_DECLARE_PRIVATE(AOOpenAL)
public:
    AOOpenAL();
    ~AOOpenAL();

    virtual bool open();
    virtual bool close();

    QString name() const;

protected:
    virtual bool write();
};

} //namespace QtAV
#endif // QTAV_AOOPENAL_H
