/**
 * @file aiperservice.cpp
 * @brief Implementation of AIPerService
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
 *   Renamed AICurlPrivate::PerHostRequestQueue[Ptr] to AIPerHostRequestQueue[Ptr]
 *   to allow public access.
 *
 *   09/04/2013
 *   Renamed everything "host" to "service" and use "hostname:port" as key
 *   instead of just "hostname".
 */
#include "sys.h"
#include "aicurlperservice.h"
#include "aicurlthread.h"
#include "llcontrol.h"
AIPerService::threadsafe_instance_map_type AIPerService::sInstanceMap;
AIThreadSafeSimpleDC<AIPerService::TotalQueued> AIPerService::sTotalQueued;
#undef AICurlPrivate
namespace AICurlPrivate {
U16 CurlConcurrentConnectionsPerService;
void intrusive_ptr_add_ref(RefCountedThreadSafePerService* per_service)
{
  per_service->mReferenceCount++;
}
void intrusive_ptr_release(RefCountedThreadSafePerService* per_service)
{
  if (--per_service->mReferenceCount == 0)
  {
    delete per_service;
  }
}
}
using namespace AICurlPrivate;
AIPerService::AIPerService(void) :
		mHTTPBandwidth(25),
		mConcurrentConnections(CurlConcurrentConnectionsPerService),
		mApprovedRequests(0),
		mTotalAdded(0),
		mEventPolls(0),
		mEstablishedConnections(0),
		mUsedCT(0),
		mCTInUse(0)
{
}
AIPerService::CapabilityType::CapabilityType(void) :
  		mApprovedRequests(0),
		mQueuedCommands(0),
		mAdded(0),
		mFlags(0),
		mDownloading(0),
		mMaxPipelinedRequests(CurlConcurrentConnectionsPerService),
		mConcurrentConnections(CurlConcurrentConnectionsPerService)
{
}
AIPerService::CapabilityType::~CapabilityType()
{
}
AIPerService::AIPerService(AIPerService const&) : mHTTPBandwidth(0)
{
}
std::string AIPerService::extract_canonical_servicename(std::string const& url)
{
  char const* p = url.data();
  char const* const end = p + url.size();
  char const* sheme_colon = NULL;
  char const* sheme_slash = NULL;
  char const* first_ampersand = NULL;
  char const* port_colon = NULL;
  std::string servicename;
  char const* hostname = p;
  while (p < end)
  {
    int c = *p;
    if (c == ':')
    {
      if (!port_colon && LLStringOps::isDigit(p[1]))
      {
        port_colon = p;
      }
      else if (!sheme_colon && !sheme_slash && !first_ampersand && !port_colon)
      {
        sheme_colon = p;
      }
    }
    else if (c == '/')
    {
      if (!sheme_slash && sheme_colon && sheme_colon == p - 1 && !first_ampersand && p[1] == '/')
      {
        sheme_slash = p;
        hostname = ++p + 1;
        servicename.clear();
      }
      else
      {
        if (hostname < sheme_colon)
        {
          servicename.clear();
        }
        break;
      }
    }
    else if (c == '@')
    {
      if (!first_ampersand)
      {
        first_ampersand = p;
        hostname = p + 1;
        servicename.clear();
      }
    }
    else if (c == '\\')
    {
      servicename.clear();
      break;
    }
    if (p >= hostname)
    {
#if APR_CHARSET_EBCDIC
#error Not implemented
#else
      if (c >= 'A' && c <= 'Z')
        c += ('a' - 'A');
#endif
      servicename += c;
    }
    ++p;
  }
  if (p - 3 == port_colon && p[-1] == '0' && p[-2] == '8')
  {
    return servicename.substr(0, p - hostname - 3);
  }
  return servicename;
}
AIPerServicePtr AIPerService::instance(std::string const& servicename)
{
  llassert(!servicename.empty());
  instance_map_wat instance_map_w(sInstanceMap);
  AIPerService::iterator iter = instance_map_w->find(servicename);
  if (iter == instance_map_w->end())
  {
	iter = instance_map_w->insert(instance_map_type::value_type(servicename, new RefCountedThreadSafePerService)).first;
	Dout(dc::curlio, "Created new service \"" << servicename << "\" [" << (void*)&*PerService_rat(*iter->second) << "]");
  }
  return iter->second;
}
void AIPerService::release(AIPerServicePtr& instance)
{
  if (instance->exactly_two_left())
  {
	if (LLApp::isStopped())
	{
	  return;
	}
	instance_map_wat instance_map_w(sInstanceMap);
	if (!instance->exactly_two_left())
	{
	  return;
	}
#ifdef SHOW_ASSERT
	{
	  PerService_rat per_service_r(*instance);
	  for (int i = 0; i < number_of_capability_types; ++i)
	  {
	  	llassert(per_service_r->mCapabilityType[i].mQueuedRequests.empty());
	  }
	}
#endif
	iterator const end = instance_map_w->end();
	for(iterator iter = instance_map_w->begin(); iter != end; ++iter)
	{
	  if (instance == iter->second)
	  {
		instance_map_w->erase(iter);
		instance.reset();
		return;
	  }
	}
	llassert(false);
  }
  instance.reset();
}
void AIPerService::redivide_connections(void)
{
  static AICapabilityType order[number_of_capability_types] = { cap_inventory, cap_texture, cap_mesh, cap_other };
  AICapabilityType used_order[number_of_capability_types];
  int number_of_capability_types_in_use = 0;
  for (int i = 0; i < number_of_capability_types; ++i)
  {
	U32 const mask = CT2mask(order[i]);
	if ((mCTInUse & mask))
	{
	  used_order[number_of_capability_types_in_use++] = order[i];
	}
	else
	{
	  mCapabilityType[order[i]].mConcurrentConnections = 1;
	}
  }
  int reserve = (mUsedCT != mCTInUse) ? 1 : 0;
  U16 max_connections_per_CT = (mConcurrentConnections - reserve) / number_of_capability_types_in_use + 1;
  int count = (mConcurrentConnections - reserve) % number_of_capability_types_in_use;
  for(int i = 1, j = 0;; --i)
  {
	while (j < count)
	{
	  mCapabilityType[used_order[j++]].mConcurrentConnections = max_connections_per_CT;
	}
	if (i == 0)
	{
	  break;
	}
	count = number_of_capability_types_in_use;
	if (max_connections_per_CT > 1)
	{
	  --max_connections_per_CT;
	}
  }
}
bool AIPerService::throttled(AICapabilityType capability_type) const
{
  return mTotalAdded >= mConcurrentConnections ||
		 mCapabilityType[capability_type].mAdded >= mCapabilityType[capability_type].mConcurrentConnections;
}
void AIPerService::added_to_multi_handle(AICapabilityType capability_type, bool event_poll)
{
  if (event_poll)
  {
	llassert(capability_type == cap_other);
	U16 counted_event_polls = (mEventPolls == 0) ? 0 : 1;
	if (mCapabilityType[capability_type].mAdded == counted_event_polls &&
		mCapabilityType[capability_type].pipelined_requests() == counted_event_polls + 1)
	{
	  mark_unused(capability_type);
	}
	if (++mEventPolls > 1)
	{
	  return;
	}
  }
  ++mCapabilityType[capability_type].mAdded;
  ++mTotalAdded;
}
void AIPerService::removed_from_multi_handle(AICapabilityType capability_type, bool event_poll, bool downloaded_something, bool success)
{
  CapabilityType& ct(mCapabilityType[capability_type]);
  llassert(mTotalAdded > 0 && ct.mAdded > 0 && (!event_poll || mEventPolls));
  if (!event_poll || --mEventPolls == 0)
  {
	--ct.mAdded;
	--mTotalAdded;
  }
  if (downloaded_something)
  {
	llassert(ct.mDownloading > 0);
	--ct.mDownloading;
  }
  U16 counted_event_polls = (capability_type != cap_other || mEventPolls == 0) ? 0 : 1;
  if (ct.mAdded == counted_event_polls && ct.pipelined_requests() == counted_event_polls)
  {
	mark_unused(capability_type);
  }
  if (success)
  {
	ct.mFlags |= ctf_success;
  }
}
bool AIPerService::queue(AICurlEasyRequest const& easy_request, AICapabilityType capability_type, bool force_queuing)
{
  CapabilityType::queued_request_type& queued_requests(mCapabilityType[capability_type].mQueuedRequests);
  bool needs_queuing = force_queuing || !queued_requests.empty();
  if (needs_queuing)
  {
	queued_requests.push_back(easy_request.get_ptr());
	if (is_approved(capability_type))
	{
	  TotalQueued_wat(sTotalQueued)->approved++;
	}
  }
  return needs_queuing;
}
bool AIPerService::cancel(AICurlEasyRequest const& easy_request, AICapabilityType capability_type)
{
  CapabilityType::queued_request_type::iterator const end = mCapabilityType[capability_type].mQueuedRequests.end();
  CapabilityType::queued_request_type::iterator cur = std::find(mCapabilityType[capability_type].mQueuedRequests.begin(), end, easy_request.get_ptr());
  if (cur == end)
	return false;
  CapabilityType::queued_request_type::iterator prev = cur;
  while (++cur != end)
  {
	prev->swap(*cur);
	prev = cur;
  }
  mCapabilityType[capability_type].mQueuedRequests.pop_back();
  if (is_approved(capability_type))
  {
	TotalQueued_wat total_queued_w(sTotalQueued);
	llassert(total_queued_w->approved > 0);
	total_queued_w->approved--;
  }
  return true;
}
void AIPerService::add_queued_to(curlthread::MultiHandle* multi_handle, bool only_this_service)
{
  U32 success = 0;
  bool success_this_pass = false;
  int i = 0;
  for (int pass = 0;; ++i)
  {
	if (i == number_of_capability_types)
	{
	  i = 0;
	  if (pass++ && !success_this_pass)
	  {
		break;
	  }
	  success_this_pass = false;
	}
	CapabilityType& ct(mCapabilityType[i]);
	if (!pass != !ct.mAdded)
	{
	  continue;
	}
	if (multi_handle->added_maximum())
	{
	  only_this_service = true;
	  break;
	}
	if (mTotalAdded >= mConcurrentConnections)
	{
	  break;
	}
	if (ct.mAdded >= ct.mConcurrentConnections)
	{
	  continue;
	}
	U32 mask = CT2mask((AICapabilityType)i);
	if (ct.mQueuedRequests.empty())
	{
	  ct.mFlags |= ((success & mask) ? ctf_empty : ctf_starvation);
	}
	else
	{
	  if (!multi_handle->add_easy_request(ct.mQueuedRequests.front(), true))
	  {
		break;
	  }
	  ct.mQueuedRequests.pop_front();
	  success |= mask;
	  success_this_pass = true;
	  if (is_approved((AICapabilityType)i))
	  {
		TotalQueued_wat total_queued_w(sTotalQueued);
		llassert(total_queued_w->approved > 0);
		total_queued_w->approved--;
	  }
	}
  }
  size_t queuedapproved_size = 0;
  for (int i = 0; i < number_of_capability_types; ++i)
  {
	CapabilityType& ct(mCapabilityType[i]);
	U32 mask = CT2mask((AICapabilityType)i);
	if ((approved_mask & mask))
	{
	  queuedapproved_size += ct.mQueuedRequests.size();
	}
	if (!(success & mask))
	{
	  continue;
	}
	if (!ct.mQueuedRequests.empty())
	{
	  ct.mFlags |= ctf_full;
	}
  }
  {
	TotalQueued_wat total_queued_w(sTotalQueued);
	if (total_queued_w->approved == 0)
	{
	  if ((success & approved_mask))
	  {
		total_queued_w->empty = true;
	  }
	  else
	  {
		total_queued_w->starvation = true;
	  }
	}
	else if ((success & approved_mask))
	{
	  total_queued_w->full = true;
	}
  }
  if (success || only_this_service)
  {
	return;
  }
  instance_map_wat instance_map_w(sInstanceMap);
  for (iterator service = instance_map_w->begin(); service != instance_map_w->end(); ++service)
  {
	PerService_wat per_service_w(*service->second);
	if (&*per_service_w == this)
	{
	  continue;
	}
	per_service_w->add_queued_to(multi_handle, true);
  }
}
void AIPerService::purge(void)
{
  instance_map_wat instance_map_w(sInstanceMap);
  for (iterator service = instance_map_w->begin(); service != instance_map_w->end(); ++service)
  {
	Dout(dc::curl, "Purging queues of service \"" << service->first << "\".");
	PerService_wat per_service_w(*service->second);
	TotalQueued_wat total_queued_w(sTotalQueued);
	for (int i = 0; i < number_of_capability_types; ++i)
	{
	  size_t s = per_service_w->mCapabilityType[i].mQueuedRequests.size();
	  per_service_w->mCapabilityType[i].mQueuedRequests.clear();
	  if (is_approved((AICapabilityType)i))
	  {
		llassert(total_queued_w->approved >= (S32)s);
		total_queued_w->approved -= s;
	  }
	}
  }
}
void AIPerService::adjust_concurrent_connections(int increment)
{
  instance_map_wat instance_map_w(sInstanceMap);
  for (AIPerService::iterator iter = instance_map_w->begin(); iter != instance_map_w->end(); ++iter)
  {
	PerService_wat per_service_w(*iter->second);
	U16 old_concurrent_connections = per_service_w->mConcurrentConnections;
	int new_concurrent_connections = llclamp(old_concurrent_connections + increment, 1, (int)CurlConcurrentConnectionsPerService);
	per_service_w->mConcurrentConnections = (U16)new_concurrent_connections;
	increment = per_service_w->mConcurrentConnections - old_concurrent_connections;
	for (int i = 0; i < number_of_capability_types; ++i)
	{
	  per_service_w->mCapabilityType[i].mMaxPipelinedRequests = llmax(per_service_w->mCapabilityType[i].mMaxPipelinedRequests + increment, 0);
	  int new_concurrent_connections_per_capability_type =
		  llclamp((new_concurrent_connections * per_service_w->mCapabilityType[i].mConcurrentConnections + old_concurrent_connections / 2) / old_concurrent_connections, 1, new_concurrent_connections);
	  per_service_w->mCapabilityType[i].mConcurrentConnections = (U16)new_concurrent_connections_per_capability_type;
	}
  }
}
void AIPerService::ResetUsed::operator()(AIPerService::instance_map_type::value_type const& service) const
{
  PerService_wat(*service.second)->resetUsedCt();
}
void AIPerService::Approvement::honored(void)
{
  if (!mHonored)
  {
	mHonored = true;
	PerService_wat per_service_w(*mPerServicePtr);
	llassert(per_service_w->mCapabilityType[mCapabilityType].mApprovedRequests > 0 && per_service_w->mApprovedRequests > 0);
	per_service_w->mCapabilityType[mCapabilityType].mApprovedRequests--;
	per_service_w->mApprovedRequests--;
  }
}
void AIPerService::Approvement::not_honored(void)
{
  honored();
  LL_WARNS() << "Approvement for has not been honored." << LL_ENDL;
}
