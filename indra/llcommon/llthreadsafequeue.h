/** 
 * @file llthreadsafequeue.h
 * @brief Base classes for thread, mutex and condition handling.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#ifndef LL_LLTHREADSAFEQUEUE_H
#define LL_LLTHREADSAFEQUEUE_H
#include <string>
#include <stdexcept>
#include "llaprpool.h"
class LLThreadSafeQueueImplementation;
class LL_COMMON_API LLThreadSafeQueueError:
public std::runtime_error
{
public:
	LLThreadSafeQueueError(std::string const & message):
	std::runtime_error(message)
	{
		;
	}
};
class LL_COMMON_API LLThreadSafeQueueInterrupt:
	public LLThreadSafeQueueError
{
public:
	LLThreadSafeQueueInterrupt(void):
		LLThreadSafeQueueError("queue operation interrupted")
	{
		;
	}
};
struct apr_queue_t;
class LL_COMMON_API LLThreadSafeQueueImplementation
{
public:
	LLThreadSafeQueueImplementation(unsigned int capacity);
	~LLThreadSafeQueueImplementation();
	void pushFront(void * element);
	bool tryPushFront(void * element);
	void * popBack(void);
	bool tryPopBack(void *& element);
	size_t size();
private:
	LLAPRPool mPool;
	apr_queue_t * mQueue;
};
template<typename ElementT>
class LLThreadSafeQueue
{
public:
	typedef ElementT value_type;
	LLThreadSafeQueue(unsigned int capacity = 1024);
	void pushFront(ElementT const & element);
	bool tryPushFront(ElementT const & element);
	ElementT popBack(void);
	bool tryPopBack(ElementT & element);
	size_t size();
private:
	LLThreadSafeQueueImplementation mImplementation;
};
template<typename ElementT>
LLThreadSafeQueue<ElementT>::LLThreadSafeQueue(unsigned int capacity) :
	mImplementation(capacity)
{
	;
}
template<typename ElementT>
void LLThreadSafeQueue<ElementT>::pushFront(ElementT const & element)
{
	ElementT * elementCopy = new ElementT(element);
	try {
		mImplementation.pushFront(elementCopy);
	} catch (LLThreadSafeQueueInterrupt) {
		delete elementCopy;
		throw;
	}
}
template<typename ElementT>
bool LLThreadSafeQueue<ElementT>::tryPushFront(ElementT const & element)
{
	ElementT * elementCopy = new ElementT(element);
	bool result = mImplementation.tryPushFront(elementCopy);
	if(!result) delete elementCopy;
	return result;
}
template<typename ElementT>
ElementT LLThreadSafeQueue<ElementT>::popBack(void)
{
	ElementT * element = reinterpret_cast<ElementT *> (mImplementation.popBack());
	ElementT result(*element);
	delete element;
	return result;
}
template<typename ElementT>
bool LLThreadSafeQueue<ElementT>::tryPopBack(ElementT & element)
{
	void * storedElement;
	bool result = mImplementation.tryPopBack(storedElement);
	if(result) {
		ElementT * elementPtr = reinterpret_cast<ElementT *>(storedElement);
		element = *elementPtr;
		delete elementPtr;
	} else {
		;
	}
	return result;
}
template<typename ElementT>
size_t LLThreadSafeQueue<ElementT>::size(void)
{
	return mImplementation.size();
}
#endif
