/******************************************************************************
	IOC.cpp: description
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


#include "IOC.h"

void ConnectIOCPair(IOCObject *A, IOCObject *B)
{
	A->setIOC(new IOC(B));
	B->setIOC(new IOC(A));
}

void IOCObject::setIOC(IOC *c)
{
	ioc.reset(c);
}

IOC* IOCObject::getIOC()
{
	return ioc.data();
}

IOC::IOC(IOCObject *object)
	:obj(object)
{
}

void IOC::wake()
{
	obj->wake();
}
