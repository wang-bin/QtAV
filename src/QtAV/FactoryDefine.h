/******************************************************************************
    Some macros to create a factory and register functions
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

#ifndef FACTORYDEFINE_H
#define FACTORYDEFINE_H

#include <string>

/*!
 * A: Suppose we need a factory for class MyClass. We can query a derived class
 * of MyClass from factory by an id type MyClassId.
 * To create the factory, just 2 steps
 * 1. In MyClass.h:
 *      #include "FactoryDefine.h"
 *      FACTORY_DECLARE(MyClass)
 * 2. In MyClass.cpp:
 *      #include "factory.h"
 *      FACTORY_DEFINE(MyClass)
 *
 * To create and register a new subclass MyClassSubA with it's id
 * 0. In MyClassTypes.h (Usually just include this is enough to use the factory. And MyClassXXX.{h,cpp} can NOT include this file),
 *    MyClassSubA's ID:
 *      extern Q_EXPORT MyClassId MyClassId_SubA;
 *    In MyClassTypes.cpp, define the id value:
 *      MyClassId MyClassId_SubA = some_value;
 *  We define the id in MyClassTypes.cpp because MyClassSubA may not be compiled(e.g. platform dependent features), but the id must be defined.
 * 1. create a source file MyClassSubA.cpp and implement the required members
 * 2. In MyClassSubA.cpp, add the following lines
 *      #include "prepost.h" //for PRE_FUNC_ADD()
 *      //we don't want to depend on MyClassTypes.h, so extern
 *      extern MyClassId MyClassId_SubA;
 *      FACTORY_REGISTER_ID_AUTO(MyClass, SubA, "SubA's name")
 *      void RegisterMyClassSubA_Man()
 *      {
 *          FACTORY_REGISTER_ID_MAN(MyClass, SubA, "SubA's name")
 *      }
 *
 * 3. In MyClass.cpp, add register function into MyClass_RegisterAll();
 *      extern void RegisterMyClassSubA_Man();
 *      void MyClass_RegisterAll()
 *      {
 *          RegisterMyClassSubA_Man(); //add this line
 *      }
 *
 *******************************************************************************************************
 * B: If MyClass and it's factory already exist in the library and you want to add a new subclass without
 * changing the library,
 *  just create MyClassSubA.cpp with the content:
 *
 *      #include "MyClass.h"
 *      #include "prepost.h" //for PRE_FUNC_ADD()
 *      MyClassId MyClassId_SubA = some_value; //it can be used somewhere else as "extern"
 *      FACTORY_REGISTER_ID_AUTO(MyClass, SubA, "SubA's name")
 *      void RegisterMyClassSubA_Man() //call it when you need as "extern"
 *      {
 *          FACTORY_REGISTER_ID_MAN(MyClass, SubA, "SubA's name")
 *      }
 *
 *      class MyClassSubA : public MyClass
 *      {
 *          ...
 *      };
 */

/*
 * This should be in header
 */
#define FACTORY_REGISTER_ID_AUTO(BASE, _ID, NAME) \
    FACTORY_REGISTER_ID_TYPE_AUTO(BASE, BASE##Id_##_ID, BASE##_ID, NAME)

#define FACTORY_REGISTER_ID_MAN(BASE, _ID, NAME) \
    FACTORY_REGISTER_ID_TYPE_MAN(BASE, BASE##Id_##_ID, BASE##_ID, NAME)

#define FACTORY_REGISTER_ID_TYPE_MAN(BASE, ID, TYPE, NAME) \
    BASE##Factory::registerCreator(ID, __create_##TYPE); \
    BASE##Factory::registerIdName(ID, NAME);

#define FACTORY_REGISTER_ID_TYPE_AUTO(BASE, ID, TYPE, NAME) \
    BASE* __create_##TYPE() { return new TYPE();} \
    static void __init_##TYPE() { \
        printf("__init_"#TYPE" id=%d\n", ID); fflush(0); \
        FACTORY_REGISTER_ID_TYPE_MAN(BASE, ID, TYPE, NAME) \
    } \
    PRE_FUNC_ADD(__init_##TYPE)


/*
 * This should be in header
 */
#define FACTORY_DECLARE(T) FACTORY_DECLARE_ID(T, T##Id)
#define FACTORY_DECLARE_ID(T, ID) \
    class Q_EXPORT T##Factory \
    { \
    public: \
        typedef T* (*T##Creator)(); \
        static T* create(const ID& id); \
        static bool registerCreator(const ID&, const T##Creator&); \
        static bool registerIdName(const ID& id, const std::string& name); \
        static bool unregisterCreator(const ID& id); \
        static ID id(const std::string& name); \
        static std::string name(const ID &id); \
        static size_t count(); \
        static T* getRandom(); \
    };

/*
 * This should be in cpp
 */
#define FACTORY_DEFINE(T) FACTORY_DEFINE_ID(T, T##Id)
#define FACTORY_DEFINE_ID(T, ID) \
    class T##FactoryBridge : public Factory<ID, T, T##FactoryBridge> {}; \
    T* T##Factory::create(const ID& id) { return T##FactoryBridge::Instance().create(id); } \
    bool T##Factory::registerCreator(const ID& id, const T##Creator& callback) { return T##FactoryBridge::Instance().registerCreator(id, callback); } \
    bool T##Factory::registerIdName(const ID& id, const std::string& name) { return T##FactoryBridge::Instance().registerIdName(id, name); } \
    bool T##Factory::unregisterCreator(const ID& id) { return T##FactoryBridge::Instance().unregisterCreator(id); } \
    ID T##Factory::id(const std::string& name) { return T##FactoryBridge::Instance().id(name); } \
    std::string T##Factory::name(const ID &id) { return T##FactoryBridge::Instance().name(id); } \
    size_t T##Factory::count() { return T##FactoryBridge::Instance().count(); } \
    T* T##Factory::getRandom() { fflush(0);return T##FactoryBridge::Instance().getRandom(); }

#endif // FACTORYDEFINE_H
