/** 
 * @file lliopipe.h
 * @author Phoenix
 * @date 2004-11-18
 * @brief Declaration of base IO class
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
#ifndef LL_LLIOPIPE_H
#define LL_LLIOPIPE_H
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "apr_poll.h"
#include "llsd.h"
class LLIOPipe;
class LLPumpIO;
class LLBufferArray;
class LLChannelDescriptors;
#ifdef LL_DEBUG_PUMPS
void pump_debug(const char *file, S32 line);
#define PUMP_DEBUG pump_debug(__FILE__, __LINE__);
#define END_PUMP_DEBUG pump_debug("none", 0);
#else
#define PUMP_DEBUG
#define END_PUMP_DEBUG
#endif
	void intrusive_ptr_add_ref(LLIOPipe* p);
	void intrusive_ptr_release(LLIOPipe* p);
class LLIOPipe
{
public:
	typedef boost::intrusive_ptr<LLIOPipe> ptr_t;
	typedef boost::shared_ptr<LLBufferArray> buffer_ptr_t;
	enum EStatus
	{
		STATUS_OK = 0,
		STATUS_STOP = 1,
		STATUS_DONE = 2,
		STATUS_BREAK = 3,
		STATUS_NEED_PROCESS = 4,
		STATUS_SUCCESS_COUNT = 5,
		STATUS_ERROR = -1,
		STATUS_NOT_IMPLEMENTED = -2,
		STATUS_PRECONDITION_NOT_MET = -3,
		STATUS_NO_CONNECTION = -4,
		STATUS_LOST_CONNECTION = -5,
		STATUS_EXPIRED = -6,
		STATUS_ERROR_COUNT = 6,
	};
	inline static bool isError(EStatus status)
	{
		return ((S32)status < 0);
	}
	inline static bool isSuccess(EStatus status)
	{
		return ((S32)status >= 0);
	}
	static std::string lookupStatusString(EStatus status);
	EStatus process(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	virtual EStatus handleError(EStatus status, LLPumpIO* pump);
	virtual ~LLIOPipe();
	virtual bool hasExpiration(void) const;
	virtual bool hasNotExpired(void) const;
protected:
	LLIOPipe();
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump) = 0;
private:
	friend void intrusive_ptr_add_ref(LLIOPipe* p);
	friend void intrusive_ptr_release(LLIOPipe* p);
	U32 mReferenceCount;
};
	inline void intrusive_ptr_add_ref(LLIOPipe* p)
	{
		++p->mReferenceCount;
	}
	inline void intrusive_ptr_release(LLIOPipe* p)
	{
		if(p && 0 == --p->mReferenceCount)
		{
			delete p;
		}
	}
#if 0
class LLIOBoiler : public LLIOPipe
{
public:
	LLIOBoiler();
	virtual ~LLIOBoiler();
protected:
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
};
LLIOPipe::EStatus process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	return STATUS_NOT_IMPLEMENTED;
}
#endif
#endif
