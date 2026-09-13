/** 
 * @file llinstancetracker.h
 * @brief LLInstanceTracker is a mixin class that automatically tracks object
 *        instances with or without an associated key
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#ifndef LL_LLINSTANCETRACKER_H
#define LL_LLINSTANCETRACKER_H
#include <boost/unordered_map.hpp>
#include <typeinfo>
#include "llstringtable.h"
#include "llerror.h"
#include <boost/utility.hpp>
#ifndef BOOST_FUNCTION_HPP_INCLUDED
#include <boost/function.hpp>
#define BOOST_FUNCTION_HPP_INCLUDED
#endif
#include <boost/bind.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>
class LL_COMMON_API LLInstanceTrackerBase : public boost::noncopyable
{
protected:
	static void * & getInstances(std::type_info const & info);
	template<typename STATICDATA, class TRACKED>
	static STATICDATA& getStatic()
	{
		static void *& instances = getInstances(typeid(TRACKED));
		if (! instances)
		{
			instances = new STATICDATA;
		}
		return *static_cast<STATICDATA*>(instances);
	}
    struct StaticBase
    {
        StaticBase():
            sIterationNestDepth(0)
        {}
        S32 sIterationNestDepth;
    };
};
template<typename T, typename KEY = T*>
class LLInstanceTracker : public LLInstanceTrackerBase
{
	typedef LLInstanceTracker<T, KEY> MyT;
	typedef typename boost::unordered_map<KEY, T*> InstanceMap;
	struct StaticData: public StaticBase
	{
		InstanceMap sMap;
	};
	static StaticData& getStatic() { return LLInstanceTrackerBase::getStatic<StaticData, MyT>(); }
	static InstanceMap& getMap_() { return getStatic().sMap; }
public:
	class instance_iter : public boost::iterator_facade<instance_iter, T, boost::forward_traversal_tag>
	{
	public:
		typedef boost::iterator_facade<instance_iter, T, boost::forward_traversal_tag> super_t;
		instance_iter(const typename InstanceMap::iterator& it)
		:	mIterator(it)
		{
			++getStatic().sIterationNestDepth;
		}
		~instance_iter()
		{
			--getStatic().sIterationNestDepth;
		}
	private:
		friend class boost::iterator_core_access;
		void increment() { mIterator++; }
		void decrement() { --mIterator; }
		bool equal(instance_iter const& other) const
		{
			return mIterator == other.mIterator;
		}
		T& dereference() const
		{
			return *(mIterator->second);
		}
		typename InstanceMap::iterator mIterator;
	};
	class key_iter : public boost::iterator_facade<key_iter, KEY, boost::forward_traversal_tag>
	{
	public:
		typedef boost::iterator_facade<key_iter, KEY, boost::forward_traversal_tag> super_t;
		key_iter(typename InstanceMap::iterator it)
			:	mIterator(it)
		{
			++getStatic().sIterationNestDepth;
		}
		key_iter(const key_iter& other)
			:	mIterator(other.mIterator)
		{
			++getStatic().sIterationNestDepth;
		}
		~key_iter()
		{
			--getStatic().sIterationNestDepth;
		}
	private:
		friend class boost::iterator_core_access;
		void increment() { mIterator++; }
		void decrement() { --mIterator; }
		bool equal(key_iter const& other) const
		{
			return mIterator == other.mIterator;
		}
		KEY& dereference() const
		{
			return const_cast<KEY&>(mIterator->first);
		}
		typename InstanceMap::iterator mIterator;
	};
	static T* getInstance(const KEY& k)
	{
		const InstanceMap& map(getMap_());
		typename InstanceMap::const_iterator found = map.find(k);
		return (found == map.end()) ? NULL : found->second;
	}
	static instance_iter beginInstances()
	{
		return instance_iter(getMap_().begin());
	}
	static instance_iter endInstances()
	{
		return instance_iter(getMap_().end());
	}
	static S32 instanceCount() { return getMap_().size(); }
	static key_iter beginKeys()
	{
		return key_iter(getMap_().begin());
	}
	static key_iter endKeys()
	{
		return key_iter(getMap_().end());
	}
protected:
	LLInstanceTracker(KEY key)
	{
		getStatic();
		add_(key);
	}
	virtual ~LLInstanceTracker()
	{
		llassert_always(getStatic().sIterationNestDepth == 0);
		remove_();
	}
	virtual void setKey(KEY key) { remove_(); add_(key); }
	virtual const KEY& getKey() const { return mInstanceKey; }
private:
	void add_(KEY key)
	{
		mInstanceKey = key;
		getMap_()[key] = static_cast<T*>(this);
	}
	void remove_()
	{
		getMap_().erase(mInstanceKey);
	}
private:
	KEY mInstanceKey;
};
template<typename T>
class LLInstanceTracker<T, T*> : public LLInstanceTrackerBase
{
	typedef LLInstanceTracker<T, T*> MyT;
	typedef typename std::set<T*> InstanceSet;
	struct StaticData: public StaticBase
	{
		InstanceSet sSet;
	};
	static StaticData& getStatic() { return LLInstanceTrackerBase::getStatic<StaticData, MyT>(); }
	static InstanceSet& getSet_() { return getStatic().sSet; }
public:
	static T* getInstance(T* k)
	{
		const InstanceSet& set(getSet_());
		typename InstanceSet::const_iterator found = set.find(k);
		return (found == set.end())? NULL : *found;
	}
	static S32 instanceCount() { return getSet_().size(); }
	class instance_iter : public boost::iterator_facade<instance_iter, T, boost::forward_traversal_tag>
	{
	public:
		instance_iter(const typename InstanceSet::iterator& it)
		:	mIterator(it)
		{
			++getStatic().sIterationNestDepth;
		}
		instance_iter(const instance_iter& other)
		:	mIterator(other.mIterator)
		{
			++getStatic().sIterationNestDepth;
		}
		~instance_iter()
		{
			--getStatic().sIterationNestDepth;
		}
	private:
		friend class boost::iterator_core_access;
		void increment() { mIterator++; }
		bool equal(instance_iter const& other) const
		{
			return mIterator == other.mIterator;
		}
		T& dereference() const
		{
			return **mIterator;
		}
		typename InstanceSet::iterator mIterator;
	};
	static instance_iter beginInstances() {	return instance_iter(getSet_().begin()); }
	static instance_iter endInstances() { return instance_iter(getSet_().end()); }
protected:
	LLInstanceTracker()
	{
		getStatic();
		getSet_().insert(static_cast<T*>(this));
	}
	virtual ~LLInstanceTracker()
	{
		llassert_always(getStatic().sIterationNestDepth == 0);
		getSet_().erase(static_cast<T*>(this));
	}
	LLInstanceTracker(const LLInstanceTracker& other)
	{
		getSet_().insert(static_cast<T*>(this));
	}
};
#endif
