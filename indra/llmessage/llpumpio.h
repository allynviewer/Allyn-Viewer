/** 
 * @file llpumpio.h
 * @author Phoenix
 * @date 2004-11-19
 * @brief Declaration of pump class which manages io chains.
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
#ifndef LL_LLPUMPIO_H
#define LL_LLPUMPIO_H
#include <set>
#include <boost/shared_ptr.hpp>
#include "llaprpool.h"
#include "llbuffer.h"
#include "llframetimer.h"
#include "lliopipe.h"
#include "llrun.h"
extern const F32 DEFAULT_CHAIN_EXPIRY_SECS;
extern const F32 SHORT_CHAIN_EXPIRY_SECS;
extern const F32 NEVER_CHAIN_EXPIRY_SECS;
class LLPumpIO
{
public:
	LLPumpIO(void);
	~LLPumpIO();
	typedef std::vector<LLIOPipe::ptr_t> chain_t;
	bool addChain(const chain_t& chain, F32 timeout);
	struct LLLinkInfo
	{
		LLIOPipe::ptr_t mPipe;
		LLChannelDescriptors mChannels;
	};
	typedef std::vector<LLLinkInfo> links_t;
	bool addChain(
		const links_t& links,
		LLIOPipe::buffer_ptr_t data,
		LLSD context,
		F32 timeout);
	bool setTimeoutSeconds(F32 timeout);
	void adjustTimeoutSeconds(F32 delta);
	bool setConditional(LLIOPipe* pipe, const apr_pollfd_t* poll);
	S32 setLock();
	void clearLock(S32 key);
	bool sleepChain(F64 seconds);
	bool copyCurrentLinkInfo(links_t& links) const;
	void pump(const S32& poll_timeout);
	void pump();
	bool respond(LLIOPipe* pipe);
	bool respond(
		const links_t& links,
		LLIOPipe::buffer_ptr_t data,
		LLSD context);
	void callback();
	enum EControl
	{
		PAUSE,
		RESUME,
	};
	void control(EControl op);
protected:
	enum EState
	{
		NORMAL,
		PAUSING,
		PAUSED
	};
	EState mState;
	bool mRebuildPollset;
	apr_pollset_t* mPollset;
	S32 mPollsetClientID;
	S32 mNextLock;
	std::set<S32> mClearLocks;
	LLRunner mRunner;
	struct LLChainInfo
	{
		LLChainInfo();
		void setTimeoutSeconds(F32 timeout);
		void adjustTimeoutSeconds(F32 delta);
		bool mInit;
		bool mEOS;
		bool mHasExpiration;
		S32 mLock;
		LLFrameTimer mTimer;
		links_t::iterator mHead;
		links_t mChainLinks;
		LLIOPipe::buffer_ptr_t mData;
		LLSD mContext;
		typedef std::pair<LLIOPipe::ptr_t, apr_pollfd_t> pipe_conditional_t;
		typedef std::vector<pipe_conditional_t> conditionals_t;
		conditionals_t mDescriptors;
		boost::shared_ptr<LLAPRPool> mDescriptorsPool;
	};
 	typedef std::vector<LLChainInfo> pending_chains_t;
	pending_chains_t mPendingChains;
	typedef std::list<LLChainInfo> running_chains_t;
	running_chains_t mRunningChains;
	typedef running_chains_t::iterator current_chain_t;
	current_chain_t mCurrentChain;
	typedef std::vector<LLChainInfo> callbacks_t;
	callbacks_t mPendingCallbacks;
	callbacks_t mCallbacks;
	LLAPRPool mPool;
	LLAPRPool mCurrentPool;
	S32 mCurrentPoolReallocCount;
#if LL_THREADS_PUMPIO
	LLMutex mChainsMutex;
	LLMutex mCallbackMutex;
#endif
protected:
	LLAPRPool& initPool();
	current_chain_t removeRunningChain(current_chain_t& chain) ;
	void rebuildPollset();
	void processChain(LLChainInfo& chain);
	bool handleChainError(LLChainInfo& chain, LLIOPipe::EStatus error);
	bool isChainExpired(LLChainInfo& chain) ;
public:
	running_chains_t::size_type runningChains() const
	{
		return mRunningChains.size();
	}
};
#endif
