/******************************************************************************
    dptr.h: A simple d-pointer interface. Some code is from Qxt
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
    inline const Class& q_func() const { return *static_cast<Class*>(dptr_ptr()); } \
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
