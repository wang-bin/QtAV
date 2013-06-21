/******************************************************************************
    Factory: factory template
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


#ifndef FACTORY_H
#define FACTORY_H

/*
 * NOTE: this file can not be included in public headers! It must be used
 * inside the library, i.e., only be included in cpp or internal header.
 * Using it outside results in initializing static singleton member twice.
 */

#include <time.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <algorithm> //std::remove
//#include <qglobal.h> //TODO: no qt

#include "singleton.h"
#if 0
#include <loki/Singleton.h>
template<class Class>
Class& Loki::Singleton<Class>::Instance()
{
    return Loki::SingletonHolder<Class>::Instance();
}
#endif

/*
 * Used in library, can not be used both in library and outside. so we don't need export it
 */

template <typename Id, typename T, class Class>
class Factory : public Singleton<Class>
{
    DISABLE_COPY(Factory)
    typedef Id ID;
    typedef T Type;
#if defined(Q_COMPILER_LAMBDA) && !defined(Q_CC_MSVC)
#include <functional>
typedef std::function<Type*()> Creator; //vc does not have std::function?
#else
typedef Type* (*Creator)();
#endif //defined(Q_COMPILER_LAMBDA)
public:
    virtual void init() {}
    Type* create(const ID& id);
    //template <typename Func>
    bool registerCreator(const ID& id, const Creator& callback);
    bool registerIdName(const ID& id, const std::string& name);
    bool unregisterCreator(const ID& id);
    //bool unregisterAll();
    ID id(const std::string& name) const;
    std::string name(const ID &id) const;
    size_t count() const;
    Type* getRandom(); //remove
//    Type* at(int index);
//    ID idAt(int index);

protected:
    Factory() {}
    //virtual ~Factory() {}

    typedef std::map<ID, Creator> CreatorMap;
    CreatorMap creators;
    std::vector<ID> ids;
    typedef std::map<ID, std::string> NameMap;
    NameMap name_map;
};


template<typename Id, typename T, class Class>
typename Factory<Id, T, Class>::Type *Factory<Id, T, Class>::create(const ID& id)
{
    typename CreatorMap::const_iterator it = creators.find(id);
    if (it == creators.end()) {
        std::cerr << "Unknown id: " << id << std::endl;
        return 0;
        //throw std::runtime_error(err_msg.arg(id).toStdString());
    }
    return (it->second)();
}

template<typename Id, typename T, class Class>
bool Factory<Id, T, Class>::registerCreator(const ID& id, const Creator& callback)
{
    //DBG("%p id [%d] registered. size=%d", &Factory<Id, T, Class>::Instance(), id, ids.size());
    ids.insert(ids.end(), id);
    return creators.insert(typename CreatorMap::value_type(id, callback)).second;
}

template<typename Id, typename T, class Class>
bool Factory<Id, T, Class>::registerIdName(const ID& id, const std::string& name)
{
    //DBG("Id with name [%s] registered", qPrintable(name));
    return name_map.insert(typename NameMap::value_type(id, name/*.toLower()*/)).second;
}

template<typename Id, typename T, class Class>
bool Factory<Id, T, Class>::unregisterCreator(const ID& id)
{
    //DBG("Id [%d] unregistered", id);
    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
    name_map.erase(id);
    return creators.erase(id) == 1;
}

template<typename Id, typename T, class Class>
typename Factory<Id, T, Class>::ID Factory<Id, T, Class>::id(const std::string &name) const
{
    DBG("get id of '%s'", name.c_str());
    //need 'typename'  because 'Factory<Id, T, Class>::NameMap' is a dependent scope
    for (typename NameMap::const_iterator it = name_map.begin(); it!=name_map.end(); ++it) {
        if (it->second == name)
            return it->first;
    }
    DBG("Not found");
    return ID(); //can not return ref. TODO: Use a ID wrapper class
}

template<typename Id, typename T, class Class>
std::string Factory<Id, T, Class>::name(const ID &id) const
{
    return name_map.find(id)->second;
}

template<typename Id, typename T, class Class>
size_t Factory<Id, T, Class>::count() const
{
    //DBG("%p size = %d", &Factory<Id, T, Class>::Instance(), ids.size());
    return ids.size();
}

template<typename Id, typename T, class Class>
typename Factory<Id, T, Class>::Type* Factory<Id, T, Class>::getRandom()
{
    srand(time(0));
    int index = rand() % ids.size();
    //DBG("random %d/%d", index, ids.size());
    int new_eid = ids.at(index);
    //DBG("id %d", new_eid);
    return create(new_eid);
}

#endif // FACTORY_H
