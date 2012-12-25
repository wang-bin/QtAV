/******************************************************************************
	BlockingRing.h: description
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


#ifndef BLOCKINGRING_H
#define BLOCKINGRING_H

#include <vector>
#include <QtCore/QObject>
#include <QtCore/QWaitCondition>
#include <QtCore/QReadWriteLock>
#include <QtAV/IOC.h>

template <typename T>
class BlockingRing : public IOCObject
{
public:
	typedef enum {
		Forward, Backward
	} Direction;
	typedef T ValueType;//TODO:trait
	BlockingRing(size_t capacity = 0);
	~BlockingRing();

	void setBlocking(bool block) {} //just api compat with BlockingQueue
	inline void clear();
	inline size_t size() const;
	inline bool isFull() const;
	inline bool isEmpty() const;

	void setDirection(Direction d);
	//Direction direction() const;
	T& at(size_t n);
	const T& at(size_t n) const;
	T& take();
	void put(const T& value);

	void setCapacity(size_t c);//setmax
	size_t capacity() const;
	void setThreshold(size_t m);
	size_t threshold() const;

	void wake();
private:
	inline size_t _size() const;
	inline bool _isFull() const;
	inline bool _isEmpty() const;
	Direction dir;
	size_t thres, cap;
	bool zero_reached;
	size_t zero; //changed(to head) when direction changed and tail set as zero
	mutable size_t head, tail;
	mutable QReadWriteLock lock; //locker in const func
	QWaitCondition cond_full, cond_empty;
	std::vector<T> data;
};


template <typename T>
BlockingRing<T>::BlockingRing(size_t capacity)
	:cap(capacity)
{
	//if cap == 0, allocat 2 chunck
	dir = Forward;
	thres = 0;
	zero_reached = false;
	zero = head = tail = 0;
	if (cap > 0)
		data.reserve(cap);
}

template <typename T>
BlockingRing<T>::~BlockingRing()
{
	data.clear();
}

/*!
 * tail must setted to zero so that the data that head points to is continuous.
 * zero must record
 */
template <typename T>
void BlockingRing<T>::setDirection(Direction d)
{
	QWriteLocker locker(&lock);
	Q_UNUSED(locker);
	dir = d;
	size_t zero_tail = tail;
	tail = zero;
	if (zero_reached)
		zero = zero_tail;
	else
		zero = head;
	zero_reached = false;
}
/*
//why compile error
template <typename T>
BlockingRing<T>::Direction BlockingRing<T>::direction() const
{
	QReadLocker locker(&lock);
	Q_UNUSED(locker);
	return dir;
}
*/
//just reset
template <typename T>
void BlockingRing<T>::clear()
{
	cond_full.wakeAll();
	QWriteLocker locker(&lock);
	Q_UNUSED(locker);
	tail = head;
	zero = head;
}

//head,...,tail. allocat at least 2 chunck. tail and head never meets
template <typename T>
size_t BlockingRing<T>::_size() const
{
	//QReadLocker locker(&lock); //Read lock is important. it is used for isEmpty and is Full
	//Q_UNUSED(locker);
	if (dir == Forward) {
		if (tail > head)
			return tail - head + 1;
		else if (tail == head)
			return 0;
		else
			return cap - (head - tail + 1);
	} else {
		if (head > tail)
			return head - tail + 1;
		else if (head == tail)
			return 0;
		else
			return cap - (tail - head + 1);
	}
}

//tail,head: head - tail == 1 || (head == 0 && tail == cap - 1)
template <typename T>
bool BlockingRing<T>::_isFull() const
{
	return _size() == cap;
}

//tail == head
template <typename T>
bool BlockingRing<T>::_isEmpty() const
{
	return _size() == 0;
}

template <typename T>
size_t BlockingRing<T>::size() const
{
	QReadLocker locker(&lock);
	Q_UNUSED(locker);
	return _size();
}

template <typename T>
bool BlockingRing<T>::isEmpty() const
{
	QReadLocker locker(&lock);
	Q_UNUSED(locker);
	return _isEmpty();
}

template <typename T>
bool BlockingRing<T>::isFull() const
{
	QReadLocker locker(&lock);
	Q_UNUSED(locker);
	return _isFull();
}

template <typename T>
T& BlockingRing<T>::at(size_t n)
{
	//QReadLocker locker(&lock);
	//Q_UNUSED(locker);
	return data[n];
}

template <typename T>
const T &BlockingRing<T>::at(size_t n) const
{
	QReadLocker locker(&lock);
	Q_UNUSED(locker);
	return data[n];
}

template <typename T>
T& BlockingRing<T>::take()
{
	QWriteLocker locker(&lock);
	Q_UNUSED(locker);
	if (_size() < thres)
		cond_full.wakeAll();
	if (_isEmpty()) {
		qDebug("Take from empty queue: head=%d tail=%d zero=%d. waiting...", head, tail, zero);
		if (ioc) {
			qDebug("Empty! Call wake another");
			ioc->wake();
		}
		cond_empty.wait(&lock);
		qDebug("empty waked up");
	}
	if (_isEmpty()) {
		qWarning("Still empty.");
		return data[head];
	}
	size_t pos = head;
	if (dir == Forward) {
		//head = (head + 1)%cap
		if (head >= cap - 1)
			head = 0;
		else
			++head;
	} else {
		//can not --head first, head is unsigned
		if (head <= 0)
			head = cap - 1;
		else
			--head;
	}
	//qDebug("Take: head=>%d", head);
	return data[pos];
}

template <typename T>
void BlockingRing<T>::put(const T &value)
{
	QWriteLocker locker(&lock);
	Q_UNUSED(locker);
	if (_isFull()) {
		qDebug("Put to a full queue.  head=%d tail=%d zero=%d. Waiting...", head, tail, zero);
		cond_full.wait(&lock);
		qDebug("Full waked up");
	}
	if (_isFull()) {
		qDebug("Still full. Reserve more space");
		//another queue is empty. this queue must encrease capacity to put more
		data.reserve(cap + thres);
	}
	if (dir == Forward) {
		if (tail >= cap - 1)
			tail = 0;
		else
			++tail;
	} else {
		if (tail <= 0)
			tail = cap - 1;
		else
			--tail;
	}
	//qDebug("Put: tail=>%d %d", tail, data.size());
	if (tail == zero) {
		qDebug("zero %d reached", zero);
		zero_reached = true;
	}
	data[tail] = value;
	cond_empty.wakeAll();
}

template <typename T>
void BlockingRing<T>::setCapacity(size_t c)
{
	QWriteLocker locker(&lock);
	Q_UNUSED(locker);
	cap = c;
	data.reserve(cap);
	data.resize(cap);
	qDebug("cap=%d", data.size());
}

template <typename T>
size_t BlockingRing<T>::capacity() const
{
	QReadLocker locker(&lock);
	Q_UNUSED(locker);
	return cap;
}

template <typename T>
void BlockingRing<T>::setThreshold(size_t m)
{
	QWriteLocker locker(&lock);
	Q_UNUSED(locker);
	thres = m;
}

template <typename T>
size_t BlockingRing<T>::threshold() const
{
	QReadLocker locker(&lock);
	Q_UNUSED(locker);
	return thres;
}

template <typename T>
void BlockingRing<T>::wake()
{
	//cond_empty.wakeAll();
	cond_full.wakeAll();
}

#endif // BLOCKINGRING_H
