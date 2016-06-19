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

#ifndef SHARED_PTR
#define SHARED_PTR

#include <algorithm>
#include <QtCore/QAtomicInt>
/*
 * a simple thread safe shared ptr. QSharedPointer does not provide a way to get how many the ref count is.
 */

namespace impl {
template <typename T>
class SharedPtrImpl {
public:
    explicit SharedPtrImpl(T *ptr = 0) : m_ptr(ptr), m_counter(1) { }
    ~SharedPtrImpl() { delete m_ptr;}
    T * operator->() const { return m_ptr;}
    T & operator*() const { return *m_ptr;}
    T * get() const { return m_ptr;}
    void ref() { m_counter.ref(); }
    // return false if becomes 0
    bool deref() { return m_counter.deref();}
    int count() const {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0) || QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
        return m_counter;
#else
        return m_counter.load();
#endif
    }
    bool isNull() const { return !m_ptr;}
private:
    T *m_ptr;
    QAtomicInt m_counter;
};
} // namespace impl

template <typename T>
class SharedPtr {
public:
    explicit SharedPtr(T *ptr = 0) : m_impl(new impl::SharedPtrImpl<T>(ptr)) { }
    template <typename U>
    explicit SharedPtr(U *ptr = 0) : m_impl(new impl::SharedPtrImpl<T>(ptr)) { }
    ~SharedPtr() {
        if (!m_impl->deref())
            delete m_impl;
    }
    SharedPtr(const SharedPtr &other) {
        m_impl = other.m_impl;
        m_impl->ref();
    }
    template <typename U>
    SharedPtr(const SharedPtr<U> &other) {
        m_impl = other.m_impl;
        m_impl->ref();
    }
    SharedPtr & operator=(const SharedPtr &other) {
        SharedPtr tmp(other);
        swap(tmp);
        return *this;
    }
    template <typename U>
    SharedPtr<T> & operator=(const SharedPtr<U> &other) {
        SharedPtr<T> tmp(other);
        swap(tmp);
        return *this;
    }
    T * operator->() const { return m_impl->operator->();}
    T & operator*() const { return **m_impl;}
    bool operator !() const { return isNull();}
    template <typename U>
    bool operator<(const SharedPtr<U> &other) {  return m_impl->get() < other.get();}
    bool isNull() const { return m_impl->isNull();}
    T * get() const {  return m_impl->get();}
    int count() const { return m_impl->count();}
    void swap(SharedPtr<T> &other) { std::swap(m_impl, other.m_impl);}
private:
    impl::SharedPtrImpl<T> *m_impl;
};

template <typename T>
void swap(SharedPtr<T> &left, SharedPtr<T> &right) {
    left.swap(right);
}

#endif //SHARED_PTR
