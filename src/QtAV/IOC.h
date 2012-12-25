/******************************************************************************
	IOC.h: description
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


#ifndef IOC_H
#define IOC_H
#include <QtCore/QScopedPointer>

/*
 *Inter Object Communication. Why need this? because demux thread has 2 blocking queue
 *if A is empty and B is full, we put data to B, the thread blocked, and A will keep
 *empty forever. so we need to notify B not to block when A is empty.
 */

class IOCObject;
void ConnectIOCPair(IOCObject *A, IOCObject *B);

class IOC;
class IOCObject
{
public:
	virtual void wake() = 0;
	void setIOC(IOC* c);
	IOC* getIOC();
protected:
	friend class IOC;
	QScopedPointer<IOC> ioc; //get the owner ship!
};


class IOC
{
public:
	explicit IOC(IOCObject* object);
	void wake();
private:
	IOCObject *obj;
};


#endif // IOC_H
