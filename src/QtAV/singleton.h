/******************************************************************************
    singleton.h: singleton template
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
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


#ifndef SINGLETON_H
#define SINGLETON_H

#include <cstdlib> //harmattan: atexit
#include <iostream>
#include <cassert>
//#include <qglobal.h> //TODO: no qt
#ifndef QT_NO_EXCEPTIONS
#include <stdexcept>
#endif

#define DBG(fmt, ...) \
    printf(fmt, ##__VA_ARGS__); \
    fflush(0);

#define DISABLE_COPY(Class) \
    Class(const Class &); \
    Class &operator=(const Class &);

/*
 * Used in library, can not be used both in library and outside. so we don't need export it
 */

template <typename T>
class Singleton
{
    DISABLE_COPY(Singleton)
public:
    typedef T ObjectType;
    static T& Instance();
protected:
    Singleton() {}
    virtual ~Singleton() {}
private:
    static void MakeInstance();
    static void  DestroySingleton();

    static T* pInstance_;
    static bool destroyed_;
};

/*if it is used as dll, template will instanced in dll and exe
 *, and pInstance_ are 0 for both*/
//TODO: use static Singleton<T> inst; return inst;
template<typename T>
T* Singleton<T>::pInstance_ = 0; //Why it will be initialized twice? The order?
template<typename T>
bool Singleton<T>::destroyed_ = false;

template<typename T>
T &Singleton<T>::Instance()
{
    DBG("instance = %p\n", pInstance_);
    if (!pInstance_) {
        MakeInstance();
    }
    return *pInstance_;
}


template<typename T>
void Singleton<T>::MakeInstance()
{
    if (!pInstance_) {
        if (destroyed_) {
            destroyed_ = false;
#ifndef QT_NO_EXCEPTIONS
            throw std::logic_error("Dead Reference Detected");
#else
            std::cerr << "Dead Reference Detected" << std::endl;
            exit(1);
#endif //QT_NO_EXCEPTIONS
        }
        pInstance_ = new T();
        DBG("Singleton %p created...\n", pInstance_);
        std::atexit(&DestroySingleton);
    }
}

template<typename T>
void Singleton<T>::DestroySingleton()
{
    DBG("DestroySingleton...\n");
    assert(!destroyed_);
    delete pInstance_;
    pInstance_ = 0;
    destroyed_ = true;
}

#endif // SINGLETON_H
