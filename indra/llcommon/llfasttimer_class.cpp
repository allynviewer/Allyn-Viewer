/** 
 * @file llfasttimer_class.cpp
 * @brief Implementation of the fast timer.
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
#include "linden_common.h"
#include "llfasttimer.h"
#include "llmemory.h"
#include "llprocessor.h"
#include "llsingleton.h"
#include "lltreeiterators.h"
#include "llsdserialize.h"
#include <boost/bind.hpp>
#if LL_WINDOWS
#include "lltimer.h"
#include <intrin.h>
#elif LL_SOLARIS
#include <sys/time.h>
#include <sched.h>
#include "lltimer.h"
#elif LL_DARWIN
#include <sys/time.h>
#include "lltimer.h"
#else
#error "architecture not supported"
#endif
S32 LLFastTimer::sCurFrameIndex = -1;
S32 LLFastTimer::sLastFrameIndex = -1;
U64 LLFastTimer::sLastFrameTime = LLFastTimer::getCPUClockCount64();
bool LLFastTimer::sPauseHistory = 0;
bool LLFastTimer::sResetHistory = 0;
LLFastTimer::CurTimerData LLFastTimer::sCurTimerData;
BOOL LLFastTimer::sLog = FALSE;
std::string LLFastTimer::sLogName = "";
BOOL LLFastTimer::sMetricLog = FALSE;
LLMutex* LLFastTimer::sLogLock = NULL;
std::queue<LLSD> LLFastTimer::sLogQueue;
const int LLFastTimer::NamedTimer::HISTORY_NUM = 300;
#if defined(LL_WINDOWS)
#define USE_RDTSC 1
#endif
std::vector<LLFastTimer::FrameState>* LLFastTimer::sTimerInfos = NULL;
U64				LLFastTimer::sTimerCycles = 0;
U32				LLFastTimer::sTimerCalls = 0;
typedef LLTreeDFSPostIter<LLFastTimer::NamedTimer, LLFastTimer::NamedTimer::child_const_iter> timer_tree_bottom_up_iterator_t;
static timer_tree_bottom_up_iterator_t begin_timer_tree_bottom_up(LLFastTimer::NamedTimer& id)
{
	return timer_tree_bottom_up_iterator_t(&id,
							boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::beginChildren), _1),
							boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::endChildren), _1));
}
static timer_tree_bottom_up_iterator_t end_timer_tree_bottom_up()
{
	return timer_tree_bottom_up_iterator_t();
}
typedef LLTreeDFSIter<LLFastTimer::NamedTimer, LLFastTimer::NamedTimer::child_const_iter> timer_tree_dfs_iterator_t;
static timer_tree_dfs_iterator_t begin_timer_tree(LLFastTimer::NamedTimer& id)
{
	return timer_tree_dfs_iterator_t(&id,
		boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::beginChildren), _1),
							boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::endChildren), _1));
}
static timer_tree_dfs_iterator_t end_timer_tree()
{
	return timer_tree_dfs_iterator_t();
}
class NamedTimerFactory : public LLSingleton<NamedTimerFactory>
{
public:
	NamedTimerFactory()
		: mActiveTimerRoot(NULL),
		  mTimerRoot(NULL),
		  mAppTimer(NULL),
		  mRootFrameState(NULL)
	{}
	void initSingleton()
	{
		mTimerRoot = new LLFastTimer::NamedTimer("root");
		mActiveTimerRoot = new LLFastTimer::NamedTimer("Frame");
		mActiveTimerRoot->setCollapsed(false);
		mRootFrameState = new LLFastTimer::FrameState(mActiveTimerRoot);
		mRootFrameState->mParent = &LLFastTimer::getFrameStateList()[0];
		mRootFrameState->mParent->mActiveCount = 1;
		llassert(!mActiveTimerRoot->mParent);
		mActiveTimerRoot->mParent = mTimerRoot;
		mTimerRoot->getChildren().push_back(mActiveTimerRoot);
		mTimerRoot->mNeedsSorting = true;
		mAppTimer = new LLFastTimer(mRootFrameState);
	}
	~NamedTimerFactory()
	{
		std::for_each(mTimers.begin(), mTimers.end(), DeletePairedPointer());
		delete mAppTimer;
		delete mActiveTimerRoot;
		delete mTimerRoot;
		delete mRootFrameState;
	}
	LLFastTimer::NamedTimer& createNamedTimer(const std::string& name)
	{
		timer_map_t::iterator found_it = mTimers.find(name);
		if (found_it != mTimers.end())
		{
			return *found_it->second;
		}
		LLFastTimer::NamedTimer* timer = new LLFastTimer::NamedTimer(name);
		timer->setParent(mTimerRoot);
		mTimers.insert(std::make_pair(name, timer));
		return *timer;
	}
	LLFastTimer::NamedTimer* getTimerByName(const std::string& name)
	{
		timer_map_t::iterator found_it = mTimers.find(name);
		if (found_it != mTimers.end())
		{
			return found_it->second;
		}
		return NULL;
	}
	LLFastTimer::NamedTimer* getActiveRootTimer() { return mActiveTimerRoot; }
	LLFastTimer::NamedTimer* getRootTimer() { return mTimerRoot; }
	const LLFastTimer* getAppTimer() { return mAppTimer; }
	LLFastTimer::FrameState& getRootFrameState() { return *mRootFrameState; }
	typedef std::map<std::string, LLFastTimer::NamedTimer*> timer_map_t;
	timer_map_t::iterator beginTimers() { return mTimers.begin(); }
	timer_map_t::iterator endTimers() { return mTimers.end(); }
	S32 timerCount() { return mTimers.size(); }
private:
	timer_map_t mTimers;
	LLFastTimer::NamedTimer*		mActiveTimerRoot;
	LLFastTimer::NamedTimer*		mTimerRoot;
	LLFastTimer*						mAppTimer;
	LLFastTimer::FrameState*		mRootFrameState;
};
void update_cached_pointers_if_changed()
{
	static LLFastTimer::FrameState* sFirstTimerAddress = NULL;
	if (&*(LLFastTimer::getFrameStateList().begin()) != sFirstTimerAddress)
	{
		LLFastTimer::updateCachedPointers();
		sFirstTimerAddress = &*(LLFastTimer::getFrameStateList().begin());
	}
}
LLFastTimer::DeclareTimer::DeclareTimer(const std::string& name, bool open )
:	mTimer(NamedTimerFactory::instance().createNamedTimer(name))
{
	mTimer.setCollapsed(!open);
	mFrameState = &mTimer.getFrameState();
	update_cached_pointers_if_changed();
}
LLFastTimer::DeclareTimer::DeclareTimer(const std::string& name)
:	mTimer(NamedTimerFactory::instance().createNamedTimer(name))
{
	mFrameState = &mTimer.getFrameState();
	update_cached_pointers_if_changed();
}
void LLFastTimer::updateCachedPointers()
{
	for (DeclareTimer::instance_iter it = DeclareTimer::beginInstances(), it_end = DeclareTimer::endInstances(); it != it_end; ++it)
	{
		it->mFrameState = &it->mTimer.getFrameState();
	}
	FrameState& root_frame_state(NamedTimerFactory::instance().getRootFrameState());
	CurTimerData* cur_timer_data = &LLFastTimer::sCurTimerData;
	while(cur_timer_data->mFrameState != &root_frame_state)
	{
		cur_timer_data->mFrameState = cur_timer_data->mCurTimer->mFrameState = &cur_timer_data->mNamedTimer->getFrameState();
		cur_timer_data = &cur_timer_data->mCurTimer->mLastTimerData;
	}
	info_list_t& frame_state_list(getFrameStateList());
	FrameState* const vector_start = &*frame_state_list.begin();
	int const vector_size = frame_state_list.size();
	FrameState const* const old_vector_start = root_frame_state.mParent;
	if (vector_start != old_vector_start)
	{
		root_frame_state.mParent = vector_start;
		ptrdiff_t offset = vector_start - old_vector_start;
		llassert(frame_state_list[vector_size - 1].mParent == vector_start);
		for (int i = 2; i < vector_size - 1; ++i)
		{
			FrameState*& parent = frame_state_list[i].mParent;
			if (parent != &root_frame_state)
			{
				parent += offset;
			}
		}
	}
}
#if USE_RDTSC
std::string LLFastTimer::sClockType = "rdtsc";
#elif LL_DARWIN || LL_SOLARIS
std::string LLFastTimer::sClockType = "gettimeofday";
#elif LL_WINDOWS
std::string LLFastTimer::sClockType = "QueryPerformanceCounter";
#else
#error "Platform not supported"
#endif
U64 LLFastTimer::countsPerSecond()
{
#if USE_RDTSC
	static U64 sCPUClockFrequency = U64(LLProcessorInfo().getCPUFrequency()*1000000.0);
#else
	static bool firstcall = true;
	static U64 sCPUClockFrequency;
	if (firstcall)
	{
		sCPUClockFrequency = calc_clock_frequency();
		firstcall = false;
	}
#endif
	return sCPUClockFrequency >> 8;
}
LLFastTimer::FrameState::FrameState(LLFastTimer::NamedTimer* timerp)
:	mActiveCount(0),
	mCalls(0),
	mSelfTimeCounter(0),
	mParent(NULL),
	mLastCaller(NULL),
	mMoveUpTree(false),
	mTimer(timerp)
{}
LLFastTimer::NamedTimer::NamedTimer(const std::string& name)
:	mName(name),
	mCollapsed(true),
	mParent(NULL),
	mTotalTimeCounter(0),
	mCountAverage(0),
	mCallAverage(0),
	mNeedsSorting(false)
{
	info_list_t& frame_state_list = getFrameStateList();
	mFrameStateIndex = frame_state_list.size();
	getFrameStateList().push_back(FrameState(this));
	mCountHistory.resize(HISTORY_NUM);
	mCallHistory.resize(HISTORY_NUM);
}
LLFastTimer::NamedTimer::~NamedTimer()
{
}
std::string LLFastTimer::NamedTimer::getToolTip(S32 history_idx)
{
	F64 ms_multiplier = 1000.0 / (F64)LLFastTimer::countsPerSecond();
	if (history_idx < 0)
	{
		return llformat("%s (%.2f ms, %d calls)", getName().c_str(), (F32)((F32)getCountAverage() * ms_multiplier), (S32)getCallAverage());
	}
	else
	{
		return llformat("%s (%.2f ms, %d calls)", getName().c_str(), (F32)((F32)getHistoricalCount(history_idx) * ms_multiplier), (S32)getHistoricalCalls(history_idx));
	}
}
void LLFastTimer::NamedTimer::setParent(NamedTimer* parent)
{
	llassert_always(parent != this);
	llassert_always(parent != NULL);
	if (mParent)
	{
		for (S32 i = 0; i < HISTORY_NUM; i++)
		{
			mParent->mCountHistory[i] -= mCountHistory[i];
		}
		mParent->mCountAverage -= mCountAverage;
		std::vector<NamedTimer*>& children = mParent->getChildren();
		std::vector<NamedTimer*>::iterator found_it = std::find(children.begin(), children.end(), this);
		if (found_it != children.end())
		{
			children.erase(found_it);
		}
	}
	mParent = parent;
	if (parent)
	{
		getFrameState().mParent = &parent->getFrameState();
		parent->getChildren().push_back(this);
		parent->mNeedsSorting = true;
	}
}
S32 LLFastTimer::NamedTimer::getDepth()
{
	S32 depth = 0;
	NamedTimer* timerp = mParent;
	while(timerp)
	{
		depth++;
		timerp = timerp->mParent;
	}
	return depth;
}
void LLFastTimer::NamedTimer::processTimes()
{
	if (sCurFrameIndex < 0) return;
	buildHierarchy();
	accumulateTimings();
}
struct SortTimersDFS
{
	bool operator()(const LLFastTimer::FrameState& i1, const LLFastTimer::FrameState& i2)
	{
		return i1.mTimer->getFrameStateIndex() < i2.mTimer->getFrameStateIndex();
	}
};
struct SortTimerByName
{
	bool operator()(const LLFastTimer::NamedTimer* i1, const LLFastTimer::NamedTimer* i2)
	{
		return i1->getName() < i2->getName();
	}
};
void LLFastTimer::NamedTimer::buildHierarchy()
{
	if (sCurFrameIndex < 0 ) return;
	{
		for (instance_iter it = beginInstances(), it_end = endInstances(); it != it_end; ++it)
		{
			NamedTimer& timer = *it;
			if (&timer == NamedTimerFactory::instance().getRootTimer()) continue;
			FrameState& frame_state(timer.getFrameState());
			if (frame_state.mLastCaller && timer.mParent == NamedTimerFactory::instance().getRootTimer())
			{
				timer.setParent(frame_state.mLastCaller);
				frame_state.mMoveUpTree = false;
			}
		}
	}
	for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree_bottom_up();
		++it)
	{
		NamedTimer* timerp = *it;
		if (timerp == NamedTimerFactory::instance().getRootTimer()) continue;
		if (timerp->getFrameState().mMoveUpTree)
		{
			timerp->setParent(timerp->getParent()->getParent());
			timerp->getFrameState().mMoveUpTree = false;
			it.skipAncestors();
		}
	}
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree();
		++it)
	{
		NamedTimer* timerp = (*it);
		if (timerp->mNeedsSorting)
		{
			std::sort(timerp->getChildren().begin(), timerp->getChildren().end(), SortTimerByName());
		}
		timerp->mNeedsSorting = false;
	}
}
void LLFastTimer::NamedTimer::accumulateTimings()
{
	U32 cur_time = getCPUClockCount32();
	LLFastTimer* cur_timer = sCurTimerData.mCurTimer;
	CurTimerData* cur_data = &sCurTimerData;
	while(cur_timer->mLastTimerData.mCurTimer != cur_timer)
	{
		U32 cumulative_time_delta = cur_time - cur_timer->mStartTime;
		U32 self_time_delta = cumulative_time_delta - cur_data->mChildTime;
		cur_data->mChildTime = 0;
		cur_timer->mFrameState->mSelfTimeCounter += self_time_delta;
		cur_timer->mStartTime = cur_time;
		cur_data = &cur_timer->mLastTimerData;
		cur_data->mChildTime += cumulative_time_delta;
		cur_timer = cur_timer->mLastTimerData.mCurTimer;
	}
	for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(*NamedTimerFactory::instance().getActiveRootTimer());
		it != end_timer_tree_bottom_up();
		++it)
	{
		NamedTimer* timerp = (*it);
		timerp->mTotalTimeCounter = timerp->getFrameState().mSelfTimeCounter;
		for (child_const_iter child_it = timerp->beginChildren(); child_it != timerp->endChildren(); ++child_it)
		{
			timerp->mTotalTimeCounter += (*child_it)->mTotalTimeCounter;
		}
		S32 cur_frame = sCurFrameIndex;
		if (cur_frame >= 0)
		{
			int hidx = cur_frame % HISTORY_NUM;
			int weight = llmin(100, cur_frame);
			timerp->mCountHistory[hidx] = timerp->mTotalTimeCounter;
			timerp->mCountAverage = ((F64)timerp->mCountAverage * weight + (F64)timerp->mTotalTimeCounter) / (weight+1);
			timerp->mCallHistory[hidx] = timerp->getFrameState().mCalls;
			timerp->mCallAverage = ((F64)timerp->mCallAverage * weight + (F64)timerp->getFrameState().mCalls) / (weight+1);
		}
	}
}
U32 LLFastTimer::NamedTimer::getCountAverage() const
{
	return mCountAverage;
}
U32 LLFastTimer::NamedTimer::getCallAverage() const
{
	return mCallAverage;
}
void LLFastTimer::NamedTimer::resetFrame()
{
	if (sLog)
	{
		static S32 call_count = 0;
		if (call_count % 100 == 0)
		{
			LL_INFOS() << "countsPerSecond (32 bit): " << countsPerSecond() << LL_ENDL;
			LL_INFOS() << "get_clock_count (64 bit): " << get_clock_count() << LL_ENDL;
			LL_INFOS() << "LLProcessorInfo().getCPUFrequency() " << LLProcessorInfo().getCPUFrequency() << LL_ENDL;
			LL_INFOS() << "getCPUClockCount32() " << getCPUClockCount32() << LL_ENDL;
			LL_INFOS() << "getCPUClockCount64() " << getCPUClockCount64() << LL_ENDL;
			LL_INFOS() << "elapsed sec " << ((F64)getCPUClockCount64())/((F64)LLProcessorInfo().getCPUFrequency()*1000000.0) << LL_ENDL;
		}
		call_count++;
		F64 iclock_freq = 1000.0 / countsPerSecond();
		F64 total_time = 0;
		LLSD sd;
		{
			for (instance_iter it = beginInstances(), it_end = endInstances(); it != it_end; ++it)
			{
				NamedTimer& timer = *it;
				FrameState& info = timer.getFrameState();
				sd[timer.getName()]["Time"] = (LLSD::Real) (info.mSelfTimeCounter*iclock_freq);
				sd[timer.getName()]["Calls"] = (LLSD::Integer) info.mCalls;
				total_time = total_time + info.mSelfTimeCounter * iclock_freq;
			}
		}
		sd["Total"]["Time"] = (LLSD::Real) total_time;
		sd["Total"]["Calls"] = (LLSD::Integer) 1;
		{
			LLMutexLock lock(sLogLock);
			sLogQueue.push(sd);
		}
	}
	S32 index = 0;
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree();
		++it)
	{
		NamedTimer* timerp = (*it);
		timerp->mFrameStateIndex = index;
		index++;
	}
	llassert(index == (S32)getFrameStateList().size());
	std::sort(getFrameStateList().begin(), getFrameStateList().end(), SortTimersDFS());
	updateCachedPointers();
	{
		for (instance_iter it = beginInstances(), it_end = endInstances(); it != it_end; ++it)
		{
			NamedTimer& timer = *it;
			FrameState& info = timer.getFrameState();
			info.mSelfTimeCounter = 0;
			info.mCalls = 0;
			info.mLastCaller = NULL;
			info.mMoveUpTree = false;
			if (timer.mParent)
			{
				info.mParent = &timer.mParent->getFrameState();
			}
		}
	}
}
void LLFastTimer::NamedTimer::reset()
{
	resetFrame();
	U32 cur_time = getCPUClockCount32();
	CurTimerData* cur_data = &sCurTimerData;
	LLFastTimer* cur_timer = cur_data->mCurTimer;
	while(cur_timer->mLastTimerData.mCurTimer != cur_timer)
	{
		cur_timer->mStartTime = cur_time;
		cur_data->mChildTime = 0;
		cur_data = &cur_timer->mLastTimerData;
		cur_timer = cur_data->mCurTimer;
	}
	{
		for (instance_iter it = beginInstances(), it_end = endInstances(); it != it_end; ++it)
		{
			NamedTimer& timer = *it;
			if (&timer != NamedTimerFactory::instance().getRootTimer())
			{
				timer.setParent(NamedTimerFactory::instance().getRootTimer());
			}
			timer.mCountAverage = 0;
			timer.mCallAverage = 0;
			timer.mCountHistory.clear();
			timer.mCountHistory.resize(HISTORY_NUM);
			timer.mCallHistory.clear();
			timer.mCallHistory.resize(HISTORY_NUM);
		}
	}
	sLastFrameIndex = 0;
	sCurFrameIndex = 0;
}
LLFastTimer::info_list_t& LLFastTimer::getFrameStateList()
{
	if (!sTimerInfos)
	{
		sTimerInfos = new info_list_t;
#if 0
		sTimerInfos->reserve(1024);
#endif
	}
	return *sTimerInfos;
}
U32 LLFastTimer::NamedTimer::getHistoricalCount(S32 history_index) const
{
	S32 history_idx = (getLastFrameIndex() + history_index) % LLFastTimer::NamedTimer::HISTORY_NUM;
	return mCountHistory[history_idx];
}
U32 LLFastTimer::NamedTimer::getHistoricalCalls(S32 history_index ) const
{
	S32 history_idx = (getLastFrameIndex() + history_index) % LLFastTimer::NamedTimer::HISTORY_NUM;
	return mCallHistory[history_idx];
}
LLFastTimer::FrameState& LLFastTimer::NamedTimer::getFrameState() const
{
	llassert_always(mFrameStateIndex >= 0);
	if (this == NamedTimerFactory::instance().getActiveRootTimer())
	{
		return NamedTimerFactory::instance().getRootFrameState();
	}
	return getFrameStateList()[mFrameStateIndex];
}
LLFastTimer::NamedTimer& LLFastTimer::NamedTimer::getRootNamedTimer()
{
	return *NamedTimerFactory::instance().getActiveRootTimer();
}
std::vector<LLFastTimer::NamedTimer*>::const_iterator LLFastTimer::NamedTimer::beginChildren()
{
	return mChildren.begin();
}
std::vector<LLFastTimer::NamedTimer*>::const_iterator LLFastTimer::NamedTimer::endChildren()
{
	return mChildren.end();
}
std::vector<LLFastTimer::NamedTimer*>& LLFastTimer::NamedTimer::getChildren()
{
	return mChildren;
}
void LLFastTimer::nextFrame()
{
	countsPerSecond();
	U64 frame_time = getCPUClockCount64();
	if ((frame_time - sLastFrameTime) >> 8 > 0xffffffff)
	{
		LL_INFOS() << "Slow frame, fast timers inaccurate" << LL_ENDL;
	}
	if (!sPauseHistory)
	{
		NamedTimer::processTimes();
		sLastFrameIndex = sCurFrameIndex;
		++sCurFrameIndex;
	}
	NamedTimer::resetFrame();
	sLastFrameTime = frame_time;
}
void LLFastTimer::dumpCurTimes()
{
	NamedTimer::processTimes();
	F64 clock_freq = (F64)countsPerSecond();
	F64 iclock_freq = 1000.0 / clock_freq;
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree();
		++it)
	{
		NamedTimer* timerp = (*it);
		F64 total_time_ms = ((F64)timerp->getHistoricalCount(0) * iclock_freq);
		if (total_time_ms < 0.1) continue;
		std::ostringstream out_str;
		for (S32 i = 0; i < timerp->getDepth(); i++)
		{
			out_str << "\t";
		}
		out_str << timerp->getName() << " "
			<< std::setprecision(3) << total_time_ms << " ms, "
			<< timerp->getHistoricalCalls(0) << " calls";
		LL_INFOS() << out_str.str() << LL_ENDL;
	}
}
void LLFastTimer::reset()
{
	NamedTimer::reset();
}
void LLFastTimer::writeLog(std::ostream& os)
{
	while (!sLogQueue.empty())
	{
		LLSD& sd = sLogQueue.front();
		LLSDSerialize::toXML(sd, os);
		LLMutexLock lock(sLogLock);
		sLogQueue.pop();
	}
}
const LLFastTimer::NamedTimer* LLFastTimer::getTimerByName(const std::string& name)
{
	return NamedTimerFactory::instance().getTimerByName(name);
}
LLFastTimer::LLFastTimer(LLFastTimer::FrameState* state)
:	mFrameState(state)
{
	llassert(state == &NamedTimerFactory::instance().getRootFrameState());
	U32 start_time = getCPUClockCount32();
	mStartTime = start_time;
	mFrameState->mActiveCount++;
	LLFastTimer::sCurTimerData.mCurTimer = this;
	LLFastTimer::sCurTimerData.mNamedTimer = mFrameState->mTimer;
	LLFastTimer::sCurTimerData.mFrameState = mFrameState;
	LLFastTimer::sCurTimerData.mChildTime = 0;
	mLastTimerData = LLFastTimer::sCurTimerData;
}
#if USE_RDTSC
U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)(__rdtsc()>>8);
}
U64 LLFastTimer::getCPUClockCount64()
{
	return (U64)__rdtsc();
}
#else
U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)(get_clock_count()>>8);
}
U64 LLFastTimer::getCPUClockCount64()
{
	return get_clock_count();
}
#endif
