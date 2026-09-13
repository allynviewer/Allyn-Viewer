/** 
 * @file llioutil.h
 * @author Phoenix
 * @date 2005-10-05
 * @brief Helper classes for dealing with IOPipes
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#ifndef LL_LLIOUTIL_H
#define LL_LLIOUTIL_H
#include "llbuffer.h"
#include "lliopipe.h"
#include "llpumpio.h"
class LLIOFlush : public LLIOPipe
{
public:
	LLIOFlush() {}
	virtual ~LLIOFlush() {}
protected:
	EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
protected:
};
class LLIOSleep : public LLIOPipe
{
public:
	LLIOSleep(F64 sleep_seconds) : mSeconds(sleep_seconds) {}
	virtual ~LLIOSleep() {}
protected:
	EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
protected:
	F64 mSeconds;
};
class LLIOAddChain : public LLIOPipe
{
public:
	LLIOAddChain(const LLPumpIO::chain_t& chain, F32 timeout) :
		mChain(chain),
		mTimeout(timeout)
	{}
	virtual ~LLIOAddChain() {}
protected:
	EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
protected:
	LLPumpIO::chain_t mChain;
	F32 mTimeout;
};
class LLChangeChannel
{
public:
	LLChangeChannel(S32 is, S32 becomes);
	void operator()(LLSegment& segment);
protected:
	S32 mIs;
	S32 mBecomes;
};
#endif
