/**
 * @file aicurlperservice.h
 * @brief Definition of class AIPerService
 *
 * Copyright (c) 2012, 2013, Aleric Inglewood.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   04/11/2012
 *   Initial version, written by Aleric Inglewood @ SL
 *
 *   06/04/2013
 *   Renamed AIPrivate::PerHostRequestQueue[Ptr] to AIPerHostRequestQueue[Ptr]
 *   to allow public access.
 *
 *   09/04/2013
 *   Renamed everything "host" to "service" and use "hostname:port" as key
 *   instead of just "hostname".
 */
#ifndef AICURLPERSERVICE_H
#define AICURLPERSERVICE_H
#include "llerror.h"
#include <string>
#include <deque>
#include <map>
#include <iterator>
#include <algorithm>
#include <boost/intrusive_ptr.hpp>
#include "aithreadsafe.h"
#include "aiaverage.h"
class AICurlEasyRequest;
class AIPerService;
class AIServiceBar;
namespace AICurlPrivate {
namespace curlthread { class MultiHandle; }
class RefCountedThreadSafePerService;
class ThreadSafeBufferedCurlEasyRequest;
typedef boost::intrusive_ptr<ThreadSafeBufferedCurlEasyRequest> BufferedCurlEasyRequestPtr;
}
typedef AIThreadSafeSimpleDC<AIPerService> threadsafe_PerService;
typedef AIAccessConst<AIPerService> PerService_crat;
typedef AIAccess<AIPerService> PerService_rat;
typedef AIAccess<AIPerService> PerService_wat;
typedef boost::intrusive_ptr<AICurlPrivate::RefCountedThreadSafePerService> AIPerServicePtr;
enum AICapabilityType {
  cap_texture	= 0,
  cap_inventory	= 1,
  cap_mesh		= 2,
  cap_other		= 3,
  number_of_capability_types = 4
};
static U32 const approved_mask = 3;
class AIPerService {
  public:
	typedef std::map<std::string, AIPerServicePtr> instance_map_type;
	typedef AIThreadSafeSimpleDC<instance_map_type> threadsafe_instance_map_type;
	typedef AIAccess<instance_map_type> instance_map_rat;
	typedef AIAccess<instance_map_type> instance_map_wat;
  private:
	static threadsafe_instance_map_type sInstanceMap;
	friend class AIThreadSafeSimpleDC<AIPerService>;
	AIPerService(void);
  public:
	typedef instance_map_type::iterator iterator;
	typedef instance_map_type::const_iterator const_iterator;
	static std::string extract_canonical_servicename(std::string const& url);
	static AIPerServicePtr instance(std::string const& servicename);
	static void release(AIPerServicePtr& instance);
	static void purge(void);
	template<class Action>
	static void copy_forEach(Action const& action);
  private:
	static U16 const ctf_empty = 1;
	static U16 const ctf_full = 2;
	static U16 const ctf_starvation = 4;
	static U16 const ctf_success = 8;
	static U16 const ctf_progress_mask = 0x70;
	static U16 const ctf_progress_shift = 4;
	static U16 const ctf_grey = 0x80;
	struct CapabilityType {
	  typedef std::deque<AICurlPrivate::BufferedCurlEasyRequestPtr> queued_request_type;
	  queued_request_type mQueuedRequests;
	  U16 mApprovedRequests;
	  S16 mQueuedCommands;
	  U16 mAdded;
	  U16 mFlags;
	  U32 mDownloading;
	  U16 mMaxPipelinedRequests;
	  U16 mConcurrentConnections;
	  CapabilityType(void);
	  ~CapabilityType();
	  S32 pipelined_requests(void) const { return mApprovedRequests + mQueuedCommands + mQueuedRequests.size() + mAdded; }
	};
	friend class AIServiceBar;
	CapabilityType mCapabilityType[number_of_capability_types];
	AIAverage mHTTPBandwidth;
	int mConcurrentConnections;
	int mApprovedRequests;
	int mTotalAdded;
	int mEventPolls;
	int mEstablishedConnections;
	U32 mUsedCT;
	U32 mCTInUse;
	struct ResetUsed { void operator()(instance_map_type::value_type const& service) const; };
	void redivide_connections(void);
	void mark_inuse(AICapabilityType capability_type)
	{
	  U32 bit = CT2mask(capability_type);
	  if ((mCTInUse & bit) == 0)
	  {
		mCTInUse |= bit;
		mUsedCT |= bit;
		if (mUsedCT != bit)
		{
		  redivide_connections();
		}
	  }
	}
	void mark_unused(AICapabilityType capability_type)
	{
	  U32 bit = CT2mask(capability_type);
	  if ((mCTInUse & bit) != 0)
	  {
		mCTInUse &= ~bit;
		if (mCTInUse && mUsedCT != bit)
		{
		  redivide_connections();
		}
	  }
	}
  public:
	int connection_established(void) { mEstablishedConnections++; return mEstablishedConnections; }
	int connection_closed(void) { mEstablishedConnections--; return mEstablishedConnections; }
	static bool is_approved(AICapabilityType capability_type) { return (((U32)1 << capability_type) & approved_mask); }
	static U32 CT2mask(AICapabilityType capability_type) { return (U32)1 << capability_type; }
	void resetUsedCt(void) { mUsedCT = mCTInUse; }
	bool is_used(AICapabilityType capability_type) const { return (mUsedCT & CT2mask(capability_type)); }
	bool is_inuse(AICapabilityType capability_type) const { return (mCTInUse & CT2mask(capability_type)); }
	static void resetUsed(void) { copy_forEach(ResetUsed()); }
	U32 is_used(void) const { return mUsedCT; }
	U32 is_inuse(void) const { return mCTInUse; }
  private:
	struct TotalQueued {
		S32 approved;
		bool empty;
		bool full;
		bool starvation;
		TotalQueued(void) : approved(0), empty(false), full(false), starvation(false) { }
	};
	static AIThreadSafeSimpleDC<TotalQueued> sTotalQueued;
	typedef AIAccessConst<TotalQueued> TotalQueued_crat;
	typedef AIAccess<TotalQueued> TotalQueued_rat;
	typedef AIAccess<TotalQueued> TotalQueued_wat;
  public:
	static S32 total_approved_queue_size(void) { return TotalQueued_rat(sTotalQueued)->approved; }
  private:
	struct MaxPipelinedRequests {
		S32 threshold;
		U64 last_increment;
		U64 last_decrement;
		MaxPipelinedRequests(void) : threshold(32), last_increment(0), last_decrement(0) { }
	};
	static AIThreadSafeSimpleDC<MaxPipelinedRequests> sMaxPipelinedRequests;
	typedef AIAccessConst<MaxPipelinedRequests> MaxPipelinedRequests_crat;
	typedef AIAccess<MaxPipelinedRequests> MaxPipelinedRequests_rat;
	typedef AIAccess<MaxPipelinedRequests> MaxPipelinedRequests_wat;
  public:
	static void setMaxPipelinedRequests(S32 threshold) { MaxPipelinedRequests_wat(sMaxPipelinedRequests)->threshold = threshold; }
	static void incrementMaxPipelinedRequests(S32 increment) { MaxPipelinedRequests_wat(sMaxPipelinedRequests)->threshold += increment; }
  private:
	struct ThrottleFraction {
	  U32 fraction;
	  AIAverage average;
	  U64 last_add;
	  ThrottleFraction(void) : fraction(1024), average(25), last_add(0) { }
	};
	static AIThreadSafeSimpleDC<ThrottleFraction> sThrottleFraction;
	typedef AIAccessConst<ThrottleFraction> ThrottleFraction_crat;
	typedef AIAccess<ThrottleFraction> ThrottleFraction_rat;
	typedef AIAccess<ThrottleFraction> ThrottleFraction_wat;
	static LLAtomicU32 sHTTPThrottleBandwidth125;
	static bool sNoHTTPBandwidthThrottling;
  public:
	void added_to_command_queue(AICapabilityType capability_type) { ++mCapabilityType[capability_type].mQueuedCommands; mark_inuse(capability_type); }
	void removed_from_command_queue(AICapabilityType capability_type) { --mCapabilityType[capability_type].mQueuedCommands; }
	void added_to_multi_handle(AICapabilityType capability_type, bool event_poll);
	void removed_from_multi_handle(AICapabilityType capability_type, bool event_poll,
								   bool downloaded_something, bool success);
	void download_started(AICapabilityType capability_type) { ++mCapabilityType[capability_type].mDownloading; }
	bool throttled(AICapabilityType capability_type) const;
	bool nothing_added(AICapabilityType capability_type) const { return mCapabilityType[capability_type].mAdded == 0; }
	bool queue(AICurlEasyRequest const& easy_request, AICapabilityType capability_type, bool force_queuing = true);
	bool cancel(AICurlEasyRequest const& easy_request, AICapabilityType capability_type);
    void add_queued_to(AICurlPrivate::curlthread::MultiHandle* mh, bool only_this_service = false);
	S32 pipelined_requests(AICapabilityType capability_type) const { return mCapabilityType[capability_type].pipelined_requests(); }
	AIAverage& bandwidth(void) { return mHTTPBandwidth; }
	AIAverage const& bandwidth(void) const { return mHTTPBandwidth; }
	static void setNoHTTPBandwidthThrottling(bool nb) { sNoHTTPBandwidthThrottling = nb; }
	static void setHTTPThrottleBandwidth(F32 max_kbps) { sHTTPThrottleBandwidth125 = 125.f * max_kbps; }
	static size_t getHTTPThrottleBandwidth125(void) { return sHTTPThrottleBandwidth125; }
	static F32 throttleFraction(void) { return ThrottleFraction_wat(sThrottleFraction)->fraction / 1024.f; }
	static void adjust_concurrent_connections(int increment);
	class Approvement : public LLThreadSafeRefCount {
	  private:
		AIPerServicePtr mPerServicePtr;
		AICapabilityType mCapabilityType;
		bool mHonored;
	  public:
		Approvement(AIPerServicePtr const& per_service, AICapabilityType capability_type) : mPerServicePtr(per_service), mCapabilityType(capability_type), mHonored(false) { }
		~Approvement() { if (!mHonored) not_honored(); }
		void honored(void);
		void not_honored(void);
	};
	static Approvement* approveHTTPRequestFor(AIPerServicePtr const& per_service, AICapabilityType capability_type);
	static bool checkBandwidthUsage(AIPerServicePtr const& per_service, U64 sTime_40ms);
  private:
	AIPerService(AIPerService const&);
};
namespace AICurlPrivate {
class RefCountedThreadSafePerService : public threadsafe_PerService {
  public:
	RefCountedThreadSafePerService(void) : mReferenceCount(0) { }
	bool exactly_two_left(void) const { return mReferenceCount == 2; }
  private:
	LLAtomicU32 mReferenceCount;
	friend void intrusive_ptr_add_ref(RefCountedThreadSafePerService* p);
	friend void intrusive_ptr_release(RefCountedThreadSafePerService* p);
};
extern U16 CurlConcurrentConnectionsPerService;
}
template<class Action>
void AIPerService::copy_forEach(Action const& action)
{
  std::vector<std::pair<instance_map_type::key_type, instance_map_type::mapped_type> > current_services;
  {
	instance_map_rat instance_map_r(sInstanceMap);
	std::copy(instance_map_r->begin(), instance_map_r->end(), std::back_inserter(current_services));
  }
  std::for_each(current_services.begin(), current_services.end(), action);
}
#endif
