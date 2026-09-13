/** 
* @file llhandle.h
* @brief "Handle" to an object (usually a floater) whose lifetime you don't
* control.
*
* $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#ifndef LLHANDLE_H
#define LLHANDLE_H
#include "llpointer.h"
#include <boost/throw_exception.hpp>
class LLTombStone : public LLRefCount
{
public:
	LLTombStone(void* target = NULL) : mTarget(target) {}
	void setTarget(void* target) { mTarget = target; }
	void* getTarget() const { return mTarget; }
private:
	mutable void* mTarget;
};
template <typename T>
class LLHandle
{
	template <typename U> friend class LLHandle;
	template <typename U> friend class LLHandleProvider;
public:
	LLHandle() : mTombStone(getDefaultTombStone()) {}
	template<typename U>
	LLHandle(const LLHandle<U>& other, typename std::enable_if<std::is_convertible<U*, T*>::value>::type* dummy = nullptr)
	: mTombStone(other.mTombStone)
	{}
	bool isDead() const
	{
		return mTombStone->getTarget() == NULL;
	}
	void markDead()
	{
		mTombStone = getDefaultTombStone();
	}
	T* get() const
	{
		return reinterpret_cast<T*>(mTombStone->getTarget());
	}
	friend bool operator== (const LLHandle<T>& lhs, const LLHandle<T>& rhs)
	{
		return lhs.mTombStone == rhs.mTombStone;
	}
	friend bool operator!= (const LLHandle<T>& lhs, const LLHandle<T>& rhs)
	{
		return !(lhs == rhs);
	}
	friend bool	operator< (const LLHandle<T>& lhs, const LLHandle<T>& rhs)
	{
		return lhs.mTombStone < rhs.mTombStone;
	}
	friend bool	operator> (const LLHandle<T>& lhs, const LLHandle<T>& rhs)
	{
		return lhs.mTombStone > rhs.mTombStone;
	}
protected:
	LLPointer<LLTombStone> mTombStone;
private:
	typedef T* pointer_t;
	static LLPointer<LLTombStone>& getDefaultTombStone()
	{
		static LLPointer<LLTombStone> sDefaultTombStone = new LLTombStone;
		return sDefaultTombStone;
	}
};
template <typename T>
class LLRootHandle : public LLHandle<T>
{
public:
	typedef LLRootHandle<T> self_t;
	typedef LLHandle<T> base_t;
	LLRootHandle(T* object) { bind(object); }
	LLRootHandle() {};
	~LLRootHandle() { unbind(); }
	void bind(T* object)
	{
		if (LLHandle<T>::mTombStone.notNull())
		{
			if (LLHandle<T>::mTombStone->getTarget() == (void*)object) return;
			LLHandle<T>::mTombStone->setTarget(NULL);
		}
		LLHandle<T>::mTombStone = new LLTombStone((void*)object);
	}
	void unbind()
	{
		LLHandle<T>::mTombStone->setTarget(NULL);
	}
private:
	LLRootHandle(const LLRootHandle& other) {};
};
template <typename T>
class LLHandleProvider
{
public:
	LLHandle<T> getHandle() const
	{
		mHandle.bind(static_cast<T*>(const_cast<LLHandleProvider<T>* >(this)));
		return mHandle;
	}
	template <typename U>
	LLHandle<U> getDerivedHandle(typename std::enable_if<std::is_convertible<U*, T*>::value>::type* dummy = nullptr) const
	{
		LLHandle<U> downcast_handle;
		downcast_handle.mTombStone = getHandle().mTombStone;
		return downcast_handle;
	}
protected:
	typedef LLHandle<T> handle_type_t;
	LLHandleProvider()
	{
	}
private:
	mutable LLRootHandle<T> mHandle;
};
#endif
