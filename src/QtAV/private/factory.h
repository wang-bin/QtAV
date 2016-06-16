/******************************************************************************
    Factory: factory template
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include <ctype.h> //tolower
#include <cstring>
#include <time.h>
#include <map>
#include <vector>
#include <algorithm> //std::remove
#include "singleton.h"
#if 0
#include <loki/Singleton.h>
template<class Class>
Class& Loki::Singleton<Class>::Instance()
{
    return Loki::SingletonHolder<Class>::Instance();
}
#endif

#define FACTORY_REGISTER(BASE, _ID, NAME) FACTORY_REGISTER_ID_TYPE(BASE, BASE##Id_##_ID, BASE##_ID, NAME)
#define FACTORY_REGISTER_ID_TYPE(BASE, ID, TYPE, NAME) \
    FACTORY_REGISTER_ID_TYPE_AUTO(BASE, ID, TYPE, NAME) \
    bool Register##TYPE##_Man() { \
        return BASE::Register<TYPE>(ID, NAME); \
    }

#define FACTORY_REGISTER_ID_TYPE_AUTO(BASE, ID, TYPE, NAME) \
    namespace { \
        static const struct factory_register_##TYPE { \
            inline factory_register_##TYPE() { \
                BASE::Register<TYPE>(ID, NAME); \
            } \
        } sInit_##TYPE; \
    }

#define FACTORY_DEFINE(T) \
    class T##Factory : public Factory<T##Id, T, T##Factory> {}; \
    bool T::Register(T##Id id, T##Creator c, const char *name) { \
        DBG(#T "::Register(..., %s)\n", name); \
        return T##Factory::Instance().registerCreator(id, c) && T##Factory::Instance().registerIdName(id, name); \
    } \
    T* T::create(T##Id id) {return T##Factory::Instance().create(id);} \
    T* T::create(const char* name) { return T::create(T::id(name));} \
    T##Id* T::next(T##Id *id) { \
        const std::vector<T##Id>& ids = T##Factory::Instance().registeredIds(); \
        if (!id) return (T##Id*)&ids[0];  DBG(#T "::next(%p)\n", id);\
        T##Id *id0 = (T##Id*)&ids[0], *id1 = (T##Id*)&ids[ids.size() - 1]; \
        if (id >= id0 && id < id1) return id + 1; \
        if (id == id1) return NULL; \
        std::vector<T##Id>::const_iterator it = std::find(ids.begin(), ids.end(), *id); \
        if (it == ids.end()) return NULL; \
    DBG(#T "::next(%p) found\n", id); \
        return (T##Id*)&*(it++); \
    } \
    T##Id T::id(const char* name) { DBG(#T "::id(\"%s\")\n", name); return T##Factory::Instance().id(name, false);} \
    const char* T::name(T##Id id) {return T##Factory::Instance().name(id);}
/*
 * Used in library, can not be used both in library and outside. so we don't need export it
 */

template <typename Id, typename T, class Class>
class Factory : public Singleton<Class>
{
    DISABLE_COPY(Factory)
    typedef Id ID;
    typedef T Type;
    typedef Type* (*Creator)();
public:
    Type* create(const ID& id);
    template<class C>
    bool register_(const ID& id) { // register_<C>(id, name)
        std::pair<typename CreatorMap::iterator, bool> result = creators.insert(std::make_pair(id, create<C>));
        return result.second;
    }

    //template <typename Func>
    bool registerCreator(const ID& id, const Creator& callback);
    bool registerIdName(const ID& id, const char* name);
    bool unregisterCreator(const ID& id);
    //bool unregisterAll();
    ID id(const char* name, bool caseSensitive = true) const;
    const char* name(const ID &id) const;
    size_t count() const;
    const std::vector<ID> &registeredIds() const;
    std::vector<const char*> registeredNames() const;
    Type* getRandom(); //remove
//    Type* at(int index);
//    ID idAt(int index);

protected:
    Factory() {}
    virtual ~Factory() {}

private:
    template<class C>
    static Type* create() {
        return new C();
    }
    typedef std::map<ID, Creator> CreatorMap;
    CreatorMap creators;
    std::vector<ID> ids;
    typedef std::map<ID, const char*> NameMap;
    NameMap name_map; //static?
};
#if 0
template<typename Id, typename T, class Class>
typename Factory<Id, T, Class>::CreatorMap Factory<Id, T, Class>::creators;
template<typename Id, typename T, class Class>
typename Factory<Id, T, Class>::NameMap Factory<Id, T, Class>::name_map;
#endif
template<typename Id, typename T, class Class>
typename Factory<Id, T, Class>::Type *Factory<Id, T, Class>::create(const ID& id)
{
    typename CreatorMap::const_iterator it = creators.find(id);
    if (it == creators.end()) {
        DBG("Unknown id ");
        return 0;
        //throw std::runtime_error(err_msg.arg(id).toStdString());
    }
    return (it->second)();
}

template<typename Id, typename T, class Class>
bool Factory<Id, T, Class>::registerCreator(const ID& id, const Creator& callback)
{
    //DBG("%p id [%d] registered. size=%d\n", &Factory<Id, T, Class>::Instance(), id, ids.size());
    ids.insert(ids.end(), id);
    return creators.insert(typename CreatorMap::value_type(id, callback)).second;
}

template<typename Id, typename T, class Class>
bool Factory<Id, T, Class>::registerIdName(const ID& id, const char* name)
{
    return name_map.insert(typename NameMap::value_type(id, name/*.toLower()*/)).second;
}

template<typename Id, typename T, class Class>
bool Factory<Id, T, Class>::unregisterCreator(const ID& id)
{
    //DBG("Id [%d] unregistered\n", id);
    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
    name_map.erase(id);
    return creators.erase(id) == 1;
}

template<typename Id, typename T, class Class>
typename Factory<Id, T, Class>::ID Factory<Id, T, Class>::id(const char* name, bool caseSensitive) const
{
#ifdef _MSC_VER
#define strcasecmp(s1, s2) _strcmpi(s1, s2)
#endif
    //need 'typename'  because 'Factory<Id, T, Class>::NameMap' is a dependent scope
    for (typename NameMap::const_iterator it = name_map.begin(); it!=name_map.end(); ++it) {
        if (caseSensitive) {
            if (it->second == name || !strcmp(it->second, name))
                return it->first;
        } else {
            if (!strcasecmp(it->second, name)) {
                return it->first;
            }
        }
    }
    DBG("Not found\n");
    return ID(); //can not return ref. TODO: Use a ID wrapper class
}

template<typename Id, typename T, class Class>
const char* Factory<Id, T, Class>::name(const ID &id) const
{
    typename NameMap::const_iterator it = name_map.find(id);
    if (it == name_map.end())
        return NULL;
    return it->second;
}

template<typename Id, typename T, class Class>
const std::vector<Id>& Factory<Id, T, Class>::registeredIds() const
{
    return ids;
}

template<typename Id, typename T, class Class>
std::vector<const char*> Factory<Id, T, Class>::registeredNames() const
{
    std::vector<const char*> names;
    for (typename NameMap::const_iterator it = name_map.begin(); it != name_map.end(); ++it) {
        names.push_back((*it).second);
    }
    return names;
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
    ID new_eid = ids.at(index);
    //DBG("id %d", new_eid);
    return create(new_eid);
}

#endif // FACTORY_H
