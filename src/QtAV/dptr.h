/******************************************************************************
    dptr.h: An improved d-pointer interface from Qxt
    Improved by Wang Bin <wbsecg1@gmail.com>, 2012
******************************************************************************/

/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/
/****************************************************************************
** This file is derived from code bearing the following notice:
** The sole author of this file, Adam Higerd, has explicitly disclaimed all
** copyright interest and protection for the content within. This file has
** been placed in the public domain according to United States copyright
** statute and case law. In jurisdictions where this public domain dedication
** is not legally recognized, anyone who receives a copy of this file is
** permitted to use, modify, duplicate, and redistribute this file, in whole
** or in part, with no restrictions or conditions. In these jurisdictions,
** this file shall be copyright (C) 2006-2008 by Adam Higerd.
****************************************************************************/
#ifndef DPTR_H
#define DPTR_H

/*!
 example:
    //Base.h
    class BasePrivate
    class Base
    {
        DPTR_DECLARE_PRIVATE(Base)
    public:
        Base();
        virtual ~Base();
    protected:
        Base(BasePrivate& d);
        DPTR_DECLARE(Base)
    };
    //Base.cpp:
    Base::Base(){}
    Base::Base(BasePrivate& d):DPTR_INIT(&d){}
    ...
    //Base_p.h:
    class Base;
    class BasePrivate : public DPtrPrivate<Base>
    {
    public:
        int data;
    };
    //Derived.h:
    class DerivedPrivate;
    class Derived : public Base
    {
        DPTR_DECLARE_PRIVATE(Derived)
    public:
        Derived();
        virtual ~Derived();
    protected:
        Derived(DerivedPrivate& d);
    };
    //Derived.cpp
    Derived::Derived():Base(*new DerivedPrivate()){}
    Derived::Derived(DerivedPrivate& d):Base(d){}
    //Derived_p.h
    class DerivedPrivate : public BasePrivate
    {
    public:
        int more_data;
    };
*/

/*
 * Initialize the dptr when calling Base(BasePrivate& d) ctor.
 * The derived class using this ctor will reduce memory allocation
 * p is a DerivedPrivate*
 */
#define DPTR_INIT(p) dptr(p)
//put in protected
#define DPTR_DECLARE(Class) DPtrInterface<Class, Class##Private> dptr;
//put in private
#define DPTR_DECLARE_PRIVATE(Class) \
    inline Class##Private& d_func() { return dptr.pri<Class##Private>(); } \
    inline const Class##Private& d_func() const { return dptr.pri<Class##Private>(); } \
    friend class Class##Private;

#define DPTR_DECLARE_PUBLIC(Class) \
    inline Class& q_func() { return *static_cast<Class*>(dptr_ptr()); } \
    inline const Class& q_func() const { return *static_cast<const Class*>(dptr_ptr()); } \
    friend class Class;

#define DPTR_INIT_PRIVATE(Class) dptr.setPublic(this);
#define DPTR_D(Class) Class##Private& d = dptr.pri<Class##Private>()
#define DPTR_P(Class) Class& p = *static_cast<Class*>(dptr_ptr())

//interface
template <typename PUB>
class DPtrPrivate
{
public:
    virtual ~DPtrPrivate() {}
    inline void DPTR_setPublic(PUB* pub) { dptr_p_ptr = pub; }
protected:
    inline PUB& dptr_p() { return *dptr_p_ptr; }
    inline const PUB& dptr_p() const { return *dptr_p_ptr; }
    inline PUB* dptr_ptr() { return dptr_p_ptr; }
    inline const PUB* dptr_ptr() const { return dptr_p_ptr; }
private:
    PUB* dptr_p_ptr;
};

//interface
template <typename PUB, typename PVT>
class DPtrInterface
{
    friend class DPtrPrivate<PUB>;
public:
    DPtrInterface(PVT* d):pvt(d) {}
    DPtrInterface():pvt(new PVT) {}
    ~DPtrInterface() {
        if (pvt) {
            delete pvt;
            pvt = 0;
        }
    }
    inline void setPublic(PUB* pub) { pvt->DPTR_setPublic(pub); }
    template <typename T>
    inline T& pri() { return *reinterpret_cast<T*>(pvt); }
    template <typename T>
    inline const T& pri() const { return *reinterpret_cast<T*>(pvt); } //static cast requires defination of T
    inline PVT& operator()() { return *static_cast<PVT*>(pvt); }
    inline const PVT& operator()() const { return *static_cast<PVT*>(pvt); }
    inline PVT * operator->() { return static_cast<PVT*>(pvt); }
    inline const PVT * operator->() const { return static_cast<PVT*>(pvt); }
private:
    DPtrInterface(const DPtrInterface&);
    DPtrInterface& operator=(const DPtrInterface&);
    DPtrPrivate<PUB>* pvt;
};

#endif // DPTR_H
