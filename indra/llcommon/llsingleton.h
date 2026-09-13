/** 
 * @file llsingleton.h
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#ifndef LLSINGLETON_H
#define LLSINGLETON_H
#include "llerror.h"
#include <map>
#include <typeinfo>
#include <boost/noncopyable.hpp>
class LL_COMMON_API LLSingletonRegistry
{
	typedef std::map<std::string, void *> TypeMap;
	static TypeMap* sSingletonMap;
public:
	template<typename T> static void * & get()
	{
		std::string name(typeid(T).name());
		if (!sSingletonMap) sSingletonMap = new TypeMap();
		TypeMap::iterator result =
			sSingletonMap->insert(std::make_pair(name, (void*)NULL)).first;
		return result->second;
	}
};
template <typename DERIVED_TYPE>
class LLSingleton : private boost::noncopyable
{
private:
	typedef enum e_init_state
	{
		UNINITIALIZED,
		CONSTRUCTING,
		INITIALIZING,
		INITIALIZED,
		DELETED
	} EInitState;
	static DERIVED_TYPE* constructSingleton()
	{
		return new DERIVED_TYPE();
	}
	struct SingletonData;
	struct SingletonLifetimeManager
	{
		static void construct()
		{
			SingletonData& sData(getData());
			sData.mInitState = CONSTRUCTING;
			sData.mInstance = constructSingleton();
			sData.mInitState = INITIALIZING;
		}
	};
public:
	virtual ~LLSingleton()
	{
		SingletonData& sData(getData());
		sData.mInstance = NULL;
		sData.mInitState = DELETED;
	}
	static void deleteSingleton()
	{
		SingletonData& sData(getData());
		delete sData.mInstance;
		sData.mInstance = NULL;
		sData.mInitState = DELETED;
	}
	static SingletonData& getData()
	{
		static void * & registry = LLSingletonRegistry::get<DERIVED_TYPE>();
		if (!registry)
		{
			static SingletonData data;
			registry = &data;
		}
		return *static_cast<SingletonData *>(registry);
	}
	static DERIVED_TYPE* getInstance()
	{
		SingletonData& sData(getData());
		switch (sData.mInitState)
		{
		case CONSTRUCTING:
			LL_ERRS() << "Tried to access singleton " << typeid(DERIVED_TYPE).name() << " from singleton constructor!" << LL_ENDL;
			return NULL;
		case INITIALIZING:
			LL_WARNS() << "Using singleton " << typeid(DERIVED_TYPE).name() << " during its own initialization, before its initialization completed!" << LL_ENDL;
			return sData.mInstance;
		case INITIALIZED:
			return sData.mInstance;
		case DELETED:
			LL_WARNS() << "Trying to access deleted singleton " << typeid(DERIVED_TYPE).name() << " creating new instance" << LL_ENDL;
		case UNINITIALIZED:
			SingletonLifetimeManager::construct();
			sData.mInstance->initSingleton();
			sData.mInitState = INITIALIZED;
			return sData.mInstance;
		}
		return NULL;
	}
	static DERIVED_TYPE* getIfExists()
	{
		SingletonData& sData(getData());
		return sData.mInstance;
	}
	static DERIVED_TYPE& instance()
	{
		return *getInstance();
	}
	static bool instanceExists()
	{
		SingletonData& sData(getData());
		return sData.mInitState == INITIALIZED;
	}
	static bool destroyed()
	{
		SingletonData& sData(getData());
		return sData.mInitState == DELETED;
	}
private:
	virtual void initSingleton() {}
	struct SingletonData
	{
		EInitState		mInitState;
		DERIVED_TYPE*	mInstance;
	};
};
#ifndef LLSINGLETON
#define LLSINGLETON(DERIVED_TYPE) \
	friend class LLSingleton<DERIVED_TYPE>; \
	DERIVED_TYPE()
#endif
#endif
