/** 
 * @file llchainio.h
 * @author Phoenix
 * @date 2005-08-04
 * @brief This class declares the interface for constructing io chains.
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
#ifndef LL_LLCHAINIO_H
#define LL_LLCHAINIO_H
#include "llpumpio.h"
class LLDeferredChain
{
public:
	static bool addToPump(
		LLPumpIO* pump,
		F32 in_seconds,
		const LLPumpIO::chain_t& chain,
		F32 chain_timeout);
};
class LLChainIOFactory
{
public:
	LLChainIOFactory();
	virtual ~LLChainIOFactory();
	virtual bool build(LLPumpIO::chain_t& chain, LLSD context) const = 0;
protected:
};
template<class Pipe>
class LLSimpleIOFactory : public LLChainIOFactory
{
public:
	virtual bool build(LLPumpIO::chain_t& chain, LLSD context) const
	{
		chain.push_back(LLIOPipe::ptr_t(new Pipe));
		return true;
	}
};
template<class Pipe>
class LLCloneIOFactory : public LLChainIOFactory
{
public:
	LLCloneIOFactory(Pipe* original) :
		mHandle(original),
		mOriginal(original) {}
	virtual bool build(LLPumpIO::chain_t& chain, LLSD context) const
	{
		chain.push_back(LLIOPipe::ptr_t(new Pipe(*mOriginal)));
		return true;
	}
protected:
	LLIOPipe::ptr_t mHandle;
	Pipe* mOriginal;
};
#endif
