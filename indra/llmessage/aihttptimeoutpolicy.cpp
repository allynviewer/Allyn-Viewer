/**
 * @file aihttptimeoutpolicy.cpp
 * @brief Implementation of AIHTTPTimeoutPolicy
 *
 * Copyright (c) 2012, Aleric Inglewood.
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
 *   24/08/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */
#include "sys.h"
#include "aihttptimeoutpolicy.h"
#include "llerror.h"
#include "lldefs.h"
#include "v3math.h"
#include <vector>
namespace {
U16 const ABS_min_DNS_lookup = 0;
U16 const ABS_max_DNS_lookup = 300;
U16 const ABS_min_connect_time = 1;
U16 const ABS_max_connect_time = 30;
U16 const ABS_min_reply_delay = 1;
U16 const ABS_max_reply_delay = 120;
U16 const ABS_min_low_speed_time = 4;
U16 const ABS_max_low_speed_time = 120;
U32 const ABS_min_low_speed_limit = 1;
U32 const ABS_max_low_speed_limit = 1000000;
U16 const ABS_min_transaction = 60;
U16 const ABS_max_transaction = 1200;
U16 const ABS_min_total_delay = 60;
U16 const ABS_max_total_delay = 3000;
using namespace AIHTTPTimeoutPolicyOperators;
U16 const AITP_default_DNS_lookup_grace = 60;
U16 const AITP_default_maximum_connect_time = 10;
U16 const AITP_default_maximum_reply_delay = 60;
U16 const AITP_default_low_speed_time = 30;
U32 const AITP_default_low_speed_limit = 7000;
U16 const AITP_default_maximum_curl_transaction = 300;
U16 const AITP_default_maximum_total_delay = 600;
}
AIHTTPTimeoutPolicy& AIHTTPTimeoutPolicy::operator=(AIHTTPTimeoutPolicy const& rhs)
{
  llassert(!mBase);
  mDNSLookupGrace = rhs.mDNSLookupGrace;
  mMaximumConnectTime = rhs.mMaximumConnectTime;
  mMaximumReplyDelay = rhs.mMaximumReplyDelay;
  mLowSpeedTime = rhs.mLowSpeedTime;
  mLowSpeedLimit = rhs.mLowSpeedLimit;
  mMaximumCurlTransaction = rhs.mMaximumCurlTransaction;
  mMaximumTotalDelay = rhs.mMaximumTotalDelay;
  changed();
  return *this;
}
AIHTTPTimeoutPolicy::AIHTTPTimeoutPolicy(char const* name,
										 U16 dns_lookup_grace, U16 subsequent_connects, U16 reply_delay,
										 U16 low_speed_time, U32 low_speed_limit,
										 U16 curl_transaction, U16 total_delay) :
	mName(name),
	mBase(NULL),
	mDNSLookupGrace(dns_lookup_grace),
	mMaximumConnectTime(subsequent_connects),
	mMaximumReplyDelay(reply_delay),
	mLowSpeedTime(low_speed_time),
	mLowSpeedLimit(low_speed_limit),
	mMaximumCurlTransaction(curl_transaction),
	mMaximumTotalDelay(total_delay)
{
  sanity_checks();
}
struct PolicyOp {
  PolicyOp* mNext;
  PolicyOp(void) : mNext(NULL) { }
  PolicyOp(PolicyOp& op) : mNext(&op) { }
  virtual void perform(AIHTTPTimeoutPolicy* policy) const { }
  void nextOp(AIHTTPTimeoutPolicy* policy) const { if (mNext) mNext->perform(policy); }
};
class AIHTTPTimeoutPolicyBase : public AIHTTPTimeoutPolicy {
  private:
	std::vector<AIHTTPTimeoutPolicy*> mDerived;
	PolicyOp const* mOp;
  public:
	AIHTTPTimeoutPolicyBase(U16 dns_lookup_grace, U16 subsequent_connects, U16 reply_delay,
							U16 low_speed_time, U32 low_speed_limit,
							U16 curl_transaction, U16 total_delay) :
		AIHTTPTimeoutPolicy(NULL, dns_lookup_grace, subsequent_connects, reply_delay, low_speed_time, low_speed_limit, curl_transaction, total_delay),
		mOp(NULL) { }
	AIHTTPTimeoutPolicyBase(AIHTTPTimeoutPolicyBase& rhs, PolicyOp& op) : AIHTTPTimeoutPolicy(rhs), mOp(&op) { rhs.derived(this); mOp->perform(this); }
	void derived(AIHTTPTimeoutPolicy* derived) { mDerived.push_back(derived); }
	void base_changed(void);
	void changed(void);
	static AIHTTPTimeoutPolicyBase& getDebugSettingsCurlTimeout(void) { return sDebugSettingsCurlTimeout; }
  protected:
	friend void AIHTTPTimeoutPolicy::setDefaultCurlTimeout(AIHTTPTimeoutPolicy const& timeout);
	AIHTTPTimeoutPolicyBase& operator=(AIHTTPTimeoutPolicy const& rhs);
};
void AIHTTPTimeoutPolicyBase::base_changed(void)
{
  AIHTTPTimeoutPolicy::base_changed();
  if (mOp)
	mOp->perform(this);
  changed();
}
void AIHTTPTimeoutPolicyBase::changed(void)
{
  for (std::vector<AIHTTPTimeoutPolicy*>::iterator iter = mDerived.begin(); iter != mDerived.end(); ++iter)
	(*iter)->base_changed();
}
void AIHTTPTimeoutPolicy::changed(void)
{
  Dout(dc::notice, "Policy \"" << mName << "\" changed into: DNSLookup: " << mDNSLookupGrace << "; Connect: " << mMaximumConnectTime <<
	  "; ReplyDelay: " << mMaximumReplyDelay << "; LowSpeedTime: " << mLowSpeedTime << "; LowSpeedLimit: " << mLowSpeedLimit <<
	  "; MaxTransaction: " << mMaximumCurlTransaction << "; MaxTotalDelay:" << mMaximumTotalDelay);
}
AIHTTPTimeoutPolicy::AIHTTPTimeoutPolicy(AIHTTPTimeoutPolicy& base) :
	mName(NULL),
	mBase(static_cast<AIHTTPTimeoutPolicyBase*>(&base)),
	mDNSLookupGrace(base.mDNSLookupGrace),
	mMaximumConnectTime(base.mMaximumConnectTime),
	mMaximumReplyDelay(base.mMaximumReplyDelay),
	mLowSpeedTime(base.mLowSpeedTime),
	mLowSpeedLimit(base.mLowSpeedLimit),
	mMaximumCurlTransaction(base.mMaximumCurlTransaction),
	mMaximumTotalDelay(base.mMaximumTotalDelay)
{
}
AIHTTPTimeoutPolicyBase& AIHTTPTimeoutPolicyBase::operator=(AIHTTPTimeoutPolicy const& rhs)
{
  AIHTTPTimeoutPolicy::operator=(rhs);
  return *this;
}
AIHTTPTimeoutPolicy::AIHTTPTimeoutPolicy(char const* name, AIHTTPTimeoutPolicyBase& base) :
	mName(name),
	mBase(&base),
	mDNSLookupGrace(mBase->mDNSLookupGrace),
	mMaximumConnectTime(mBase->mMaximumConnectTime),
	mMaximumReplyDelay(mBase->mMaximumReplyDelay),
	mLowSpeedTime(mBase->mLowSpeedTime),
	mLowSpeedLimit(mBase->mLowSpeedLimit),
	mMaximumCurlTransaction(mBase->mMaximumCurlTransaction),
	mMaximumTotalDelay(mBase->mMaximumTotalDelay)
{
  sNameMap.insert(namemap_t::value_type(name, this));
  mBase->derived(this);
}
void AIHTTPTimeoutPolicy::base_changed(void)
{
  mDNSLookupGrace = mBase->mDNSLookupGrace;
  mMaximumConnectTime = mBase->mMaximumConnectTime;
  mMaximumReplyDelay = mBase->mMaximumReplyDelay;
  mLowSpeedTime = mBase->mLowSpeedTime;
  mLowSpeedLimit = mBase->mLowSpeedLimit;
  mMaximumCurlTransaction = mBase->mMaximumCurlTransaction;
  mMaximumTotalDelay = mBase->mMaximumTotalDelay;
  changed();
}
void AIHTTPTimeoutPolicy::setDefaultCurlTimeout(AIHTTPTimeoutPolicy const& timeout)
{
  sDebugSettingsCurlTimeout = timeout;
  LL_INFOS() << "CurlTimeout Debug Settings now"
	  ": DNSLookup: " << sDebugSettingsCurlTimeout.mDNSLookupGrace <<
	  "; Connect: " << sDebugSettingsCurlTimeout.mMaximumConnectTime <<
	  "; ReplyDelay: " << sDebugSettingsCurlTimeout.mMaximumReplyDelay <<
	  "; LowSpeedTime: " << sDebugSettingsCurlTimeout.mLowSpeedTime <<
	  "; LowSpeedLimit: " << sDebugSettingsCurlTimeout.mLowSpeedLimit <<
	  "; MaxTransaction: " << sDebugSettingsCurlTimeout.mMaximumCurlTransaction <<
	  "; MaxTotalDelay: " << sDebugSettingsCurlTimeout.mMaximumTotalDelay << LL_ENDL;
  if (sDebugSettingsCurlTimeout.mDNSLookupGrace < AITP_default_DNS_lookup_grace)
  {
	LL_WARNS() << "CurlTimeoutDNSLookup (" << sDebugSettingsCurlTimeout.mDNSLookupGrace << ") is lower than the built-in default value (" << AITP_default_DNS_lookup_grace << ")." << LL_ENDL;
  }
  if (sDebugSettingsCurlTimeout.mMaximumConnectTime < AITP_default_maximum_connect_time)
  {
	LL_WARNS() << "CurlTimeoutConnect (" << sDebugSettingsCurlTimeout.mMaximumConnectTime << ") is lower than the built-in default value (" << AITP_default_maximum_connect_time << ")." << LL_ENDL;
  }
  if (sDebugSettingsCurlTimeout.mMaximumReplyDelay < AITP_default_maximum_reply_delay)
  {
	LL_WARNS() << "CurlTimeoutReplyDelay (" << sDebugSettingsCurlTimeout.mMaximumReplyDelay << ") is lower than the built-in default value (" << AITP_default_maximum_reply_delay << ")." << LL_ENDL;
  }
  if (sDebugSettingsCurlTimeout.mLowSpeedTime < AITP_default_low_speed_time)
  {
	LL_WARNS() << "CurlTimeoutLowSpeedTime (" << sDebugSettingsCurlTimeout.mLowSpeedTime << ") is lower than the built-in default value (" << AITP_default_low_speed_time << ")." << LL_ENDL;
  }
  if (sDebugSettingsCurlTimeout.mLowSpeedLimit > AITP_default_low_speed_limit)
  {
	LL_WARNS() << "CurlTimeoutLowSpeedLimit (" << sDebugSettingsCurlTimeout.mLowSpeedLimit << ") is higher than the built-in default value (" << AITP_default_low_speed_limit << ")." << LL_ENDL;
  }
  if (sDebugSettingsCurlTimeout.mMaximumCurlTransaction < AITP_default_maximum_curl_transaction)
  {
	LL_WARNS() << "CurlTimeoutMaxTransaction (" << sDebugSettingsCurlTimeout.mMaximumCurlTransaction << ") is lower than the built-in default value (" << AITP_default_maximum_curl_transaction<< ")." << LL_ENDL;
  }
  if (sDebugSettingsCurlTimeout.mMaximumTotalDelay < AITP_default_maximum_total_delay)
  {
	LL_WARNS() << "CurlTimeoutMaxTotalDelay (" << sDebugSettingsCurlTimeout.mMaximumTotalDelay << ") is lower than the built-in default value (" << AITP_default_maximum_total_delay << ")." << LL_ENDL;
  }
}
AIHTTPTimeoutPolicy const& AIHTTPTimeoutPolicy::getDebugSettingsCurlTimeout(void)
{
  return sDebugSettingsCurlTimeout;
}
#ifdef SHOW_ASSERT
#include "aithreadid.h"
static AIThreadID curlthread(AIThreadID::none);
#endif
static std::set<std::string> gSeenHostnames;
U16 AIHTTPTimeoutPolicy::getConnectTimeout(std::string const& hostname) const
{
#ifdef SHOW_ASSERT
  if (curlthread.is_no_thread())
	curlthread.reset();
  llassert(curlthread.equals_current_thread());
#endif
  U16 connect_timeout = mMaximumConnectTime;
  if (gSeenHostnames.insert(hostname).second)
	connect_timeout += mDNSLookupGrace;
  return connect_timeout;
}
bool AIHTTPTimeoutPolicy::connect_timed_out(std::string const& hostname)
{
  llassert(curlthread.equals_current_thread());
  return gSeenHostnames.erase(hostname) > 0;
}
namespace AIHTTPTimeoutPolicyOperators {
struct DNS : PolicyOp {
  int mSeconds;
  DNS(int seconds) : mSeconds(seconds) { }
  DNS(int seconds, PolicyOp& op) : PolicyOp(op), mSeconds(seconds) { }
  static void fix(AIHTTPTimeoutPolicy* policy);
  static U16 min(void) { return ABS_min_DNS_lookup; }
  static U16 max(void) { return ABS_max_DNS_lookup; }
  virtual void perform(AIHTTPTimeoutPolicy* policy) const;
};
struct Connect : PolicyOp {
  int mSeconds;
  Connect(int seconds) : mSeconds(seconds) { }
  Connect(int seconds, PolicyOp& op) : PolicyOp(op), mSeconds(seconds) { }
  static void fix(AIHTTPTimeoutPolicy* policy);
  static U16 min(void) { return ABS_min_connect_time; }
  static U16 max(void) { return ABS_max_connect_time; }
  virtual void perform(AIHTTPTimeoutPolicy* policy) const;
};
struct Reply : PolicyOp {
  int mSeconds;
  Reply(int seconds) : mSeconds(seconds) { }
  Reply(int seconds, PolicyOp& op) : PolicyOp(op), mSeconds(seconds) { }
  static void fix(AIHTTPTimeoutPolicy* policy);
  static U16 min(void) { return ABS_min_reply_delay; }
  static U16 max(void) { return ABS_max_reply_delay; }
  virtual void perform(AIHTTPTimeoutPolicy* policy) const;
};
struct Speed : PolicyOp {
  int mSeconds;
  int mRate;
  Speed(int seconds, int rate) : mSeconds(seconds), mRate(rate) { }
  Speed(int seconds, int rate, PolicyOp& op) : PolicyOp(op), mSeconds(seconds), mRate(rate) { }
  static void fix(AIHTTPTimeoutPolicy* policy);
  static U16 min(void) { return ABS_min_low_speed_time; }
  static U16 max(AIHTTPTimeoutPolicy const* policy) { return llmin(ABS_max_low_speed_time, (U16)(policy->mMaximumCurlTransaction / 2)); }
  static U32 lmin(void) { return ABS_min_low_speed_limit; }
  static U32 lmax(void) { return ABS_max_low_speed_limit; }
  virtual void perform(AIHTTPTimeoutPolicy* policy) const;
};
struct Transaction : PolicyOp {
  int mSeconds;
  Transaction(int seconds) : mSeconds(seconds) { }
  Transaction(int seconds, PolicyOp& op) : PolicyOp(op), mSeconds(seconds) { }
  static void fix(AIHTTPTimeoutPolicy* policy);
  static U16 min(AIHTTPTimeoutPolicy const* policy) { return llmax(ABS_min_transaction, (U16)(policy->mMaximumConnectTime + policy->mMaximumReplyDelay + 4 * policy->mLowSpeedTime)); }
  static U16 max(void) { return ABS_max_transaction; }
  virtual void perform(AIHTTPTimeoutPolicy* policy) const;
};
struct Total : PolicyOp {
  int mSeconds;
  Total(int seconds) : mSeconds(seconds) { }
  Total(int seconds, PolicyOp& op) : PolicyOp(op), mSeconds(seconds) { }
  static void fix(AIHTTPTimeoutPolicy* policy);
  static U16 min(AIHTTPTimeoutPolicy const* policy) { return llmax(ABS_min_total_delay, (U16)(policy->mMaximumCurlTransaction + 1)); }
  static U16 max(void) { return ABS_max_total_delay; }
  virtual void perform(AIHTTPTimeoutPolicy* policy) const;
};
void DNS::perform(AIHTTPTimeoutPolicy* policy) const
{
  policy->mDNSLookupGrace = mSeconds;
  fix(policy);
  nextOp(policy);
}
void Connect::perform(AIHTTPTimeoutPolicy* policy) const
{
  policy->mMaximumConnectTime = mSeconds;
  fix(policy);
  nextOp(policy);
}
void Reply::perform(AIHTTPTimeoutPolicy* policy) const
{
  policy->mMaximumReplyDelay = mSeconds;
  fix(policy);
  nextOp(policy);
}
void Speed::perform(AIHTTPTimeoutPolicy* policy) const
{
  policy->mLowSpeedTime = mSeconds;
  policy->mLowSpeedLimit = mRate;
  fix(policy);
  nextOp(policy);
}
void Transaction::perform(AIHTTPTimeoutPolicy* policy) const
{
  policy->mMaximumCurlTransaction = mSeconds;
  fix(policy);
  nextOp(policy);
}
void Total::perform(AIHTTPTimeoutPolicy* policy) const
{
  policy->mMaximumTotalDelay = mSeconds;
  fix(policy);
  nextOp(policy);
}
void DNS::fix(AIHTTPTimeoutPolicy* policy)
{
  if (policy->mDNSLookupGrace > max())
  {
	policy->mDNSLookupGrace = max();
  }
  else if (policy->mDNSLookupGrace < min())
  {
	policy->mDNSLookupGrace = min();
  }
}
void Connect::fix(AIHTTPTimeoutPolicy* policy)
{
  bool changed = false;
  if (policy->mMaximumConnectTime > max())
  {
	policy->mMaximumConnectTime = max();
	changed = true;
  }
  else if (policy->mMaximumConnectTime < min())
  {
	policy->mMaximumConnectTime = min();
	changed = true;
  }
  if (changed)
  {
	Transaction::fix(policy);
  }
}
void Reply::fix(AIHTTPTimeoutPolicy* policy)
{
  bool changed = false;
  if (policy->mMaximumReplyDelay > max())
  {
	policy->mMaximumReplyDelay = max();
	changed = true;
  }
  else if (policy->mMaximumReplyDelay < min())
  {
	policy->mMaximumReplyDelay = min();
	changed = true;
  }
  if (changed)
  {
	Transaction::fix(policy);
  }
}
void Speed::fix(AIHTTPTimeoutPolicy* policy)
{
  bool changed = false;
  if (policy->mLowSpeedTime > ABS_max_low_speed_time)
  {
	policy->mLowSpeedTime = ABS_max_low_speed_time;
	changed = true;
  }
  else if (policy->mLowSpeedTime != 0 && policy->mLowSpeedTime < min())
  {
	policy->mLowSpeedTime = min();
	changed = true;
  }
  if (changed)
  {
	Transaction::fix(policy);
  }
  if (policy->mLowSpeedTime > max(policy))
  {
	policy->mLowSpeedTime = max(policy);
  }
  if (policy->mLowSpeedLimit > lmax())
  {
	policy->mLowSpeedLimit = lmax();
  }
  else if (policy->mLowSpeedLimit != 0 && policy->mLowSpeedLimit < lmin())
  {
	policy->mLowSpeedLimit = lmin();
  }
}
void Transaction::fix(AIHTTPTimeoutPolicy* policy)
{
  bool changed = false;
  if (policy->mMaximumCurlTransaction > max())
  {
	policy->mMaximumCurlTransaction = max();
	changed = true;
  }
  else if (policy->mMaximumCurlTransaction < ABS_min_transaction)
  {
	policy->mMaximumCurlTransaction = ABS_min_transaction;
	changed = true;
  }
  if (changed)
  {
	Total::fix(policy);
	if (policy->mMaximumCurlTransaction < min(policy))
	{
	  LLVector3 const ref(1, 1, 4);
	  LLVector3 const vec_min(ABS_min_connect_time, ABS_min_reply_delay, ABS_min_low_speed_time);
	  LLVector3 vec_res;
	  if (policy->mMaximumCurlTransaction > ref * vec_min)
	  {
		LLVector3 vec_cur(policy->mMaximumConnectTime, policy->mMaximumReplyDelay, policy->mLowSpeedTime);
		LLVector3 vec_def(AITP_default_maximum_connect_time, AITP_default_maximum_reply_delay, AITP_default_low_speed_time);
		vec_cur -= vec_min;
		vec_def -= vec_min;
		vec_def.normalize();
		LLVector3 vec_dp = vec_def * (vec_cur * vec_def);
		LLVector3 a;
		LLVector3 b = vec_cur;
		if (policy->mMaximumCurlTransaction > ref * (vec_dp + vec_min))
		{
		  a = vec_dp;
		}
		else
		{
		  b = vec_dp;
		}
		F32 lambda = (policy->mMaximumCurlTransaction - ref * (a + vec_min)) / (ref * (b - a));
		vec_res = a + lambda * (b - a);
	  }
	  vec_res += vec_min;
	  policy->mMaximumConnectTime = vec_res[VX];
	  policy->mMaximumReplyDelay = vec_res[VY];
	  policy->mLowSpeedTime = vec_res[VZ];
	}
  }
  if (policy->mMaximumCurlTransaction < min(policy))
  {
	policy->mMaximumCurlTransaction = min(policy);
  }
}
void Total::fix(AIHTTPTimeoutPolicy* policy)
{
  bool changed = false;
  if (policy->mMaximumTotalDelay > max())
  {
	policy->mMaximumTotalDelay = max();
	changed = true;
  }
  else if (policy->mMaximumTotalDelay < ABS_min_total_delay)
  {
	policy->mMaximumTotalDelay = ABS_min_total_delay;
	changed = true;
  }
  if (changed)
  {
	if (policy->mMaximumTotalDelay < policy->mMaximumCurlTransaction + 1)
	{
	  policy->mMaximumCurlTransaction = policy->mMaximumTotalDelay - 1;
	}
  }
  if (policy->mMaximumTotalDelay < min(policy))
  {
	policy->mMaximumTotalDelay = min(policy);
  }
}
}
void AIHTTPTimeoutPolicy::sanity_checks(void) const
{
  llassert(            DNS::min() <= mDNSLookupGrace         &&         mDNSLookupGrace <= DNS::max());
  llassert(        Connect::min() <= mMaximumConnectTime     &&     mMaximumConnectTime <= Connect::max());
  llassert(          Reply::min() <= mMaximumReplyDelay      &&      mMaximumReplyDelay <= Reply::max());
  llassert(mLowSpeedTime == 0   ||
	                (Speed::min() <= mLowSpeedTime           &&           mLowSpeedTime <= Speed::max(this)));
  llassert(mLowSpeedLimit == 0  ||
	               (Speed::lmin() <= mLowSpeedLimit          &&          mLowSpeedLimit <= Speed::lmax()));
  llassert(Transaction::min(this) <= mMaximumCurlTransaction && mMaximumCurlTransaction <= Transaction::max());
  llassert(      Total::min(this) <= mMaximumTotalDelay      &&      mMaximumTotalDelay <= Total::max());
}
AIHTTPTimeoutPolicyBase HTTPTimeoutPolicy_default(
		AITP_default_DNS_lookup_grace,
		AITP_default_maximum_connect_time,
		AITP_default_maximum_reply_delay,
		AITP_default_low_speed_time,
		AITP_default_low_speed_limit,
		AITP_default_maximum_curl_transaction,
		AITP_default_maximum_total_delay);
AIHTTPTimeoutPolicyBase AIHTTPTimeoutPolicy::sDebugSettingsCurlTimeout(
		AITP_default_DNS_lookup_grace,
		AITP_default_maximum_connect_time,
		AITP_default_maximum_reply_delay,
		AITP_default_low_speed_time,
		AITP_default_low_speed_limit,
		AITP_default_maximum_curl_transaction,
		AITP_default_maximum_total_delay);
Connect connectOp5s(5);
AIHTTPTimeoutPolicyBase connect_5s(AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout(),
	connectOp5s
	);
Connect connectOp10s(10);
AIHTTPTimeoutPolicyBase connect_10s(AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout(),
	connectOp10s
	);
Connect connectOp30s(30);
AIHTTPTimeoutPolicyBase connect_30s(AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout(),
	connectOp30s
	);
Connect connectOp60s(60);
AIHTTPTimeoutPolicyBase connect_60s(AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout(),
	connectOp60s
	);
Transaction transactionOp18s(18, connectOp5s);
AIHTTPTimeoutPolicyBase transfer_18s_connect_5s(AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout(),
	transactionOp18s
	);
Transaction transactionOp22s(22, connectOp10s);
AIHTTPTimeoutPolicyBase transfer_22s_connect_10s(AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout(),
	transactionOp22s
	);
Transaction transactionOp30s(30, connectOp10s);
AIHTTPTimeoutPolicyBase transfer_30s_connect_10s(AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout(),
	transactionOp30s
	);
Transaction transactionOp300s(300);
AIHTTPTimeoutPolicyBase transfer_300s(HTTPTimeoutPolicy_default,
	transactionOp300s
	);
Connect connectOp40s(40);
AIHTTPTimeoutPolicyBase connect_40s(AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout(),
	connectOp40s
	);
Reply replyOp15s(15);
AIHTTPTimeoutPolicyBase reply_15s(AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout(),
	replyOp15s
	);
Reply replyOp60s(60);
AIHTTPTimeoutPolicyBase reply_60s(AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout(),
	replyOp60s
	);
bool validateCurlTimeoutDNSLookup(LLSD const& newvalue)
{
  U32 new_value = newvalue.asInteger();
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::DNS op(new_value);
  op.perform(&timeout);
  return timeout.getDNSLookup() == new_value;
}
bool handleCurlTimeoutDNSLookup(LLSD const& newvalue)
{
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::DNS op(newvalue.asInteger());
  op.perform(&timeout);
  AIHTTPTimeoutPolicy::setDefaultCurlTimeout(timeout);
  return true;
}
bool validateCurlTimeoutConnect(LLSD const& newvalue)
{
  U32 new_value = newvalue.asInteger();
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Connect op(new_value);
  op.perform(&timeout);
  return timeout.getConnect() == new_value;
}
bool handleCurlTimeoutConnect(LLSD const& newvalue)
{
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Connect op(newvalue.asInteger());
  op.perform(&timeout);
  AIHTTPTimeoutPolicy::setDefaultCurlTimeout(timeout);
  return true;
}
bool validateCurlTimeoutReplyDelay(LLSD const& newvalue)
{
  U32 new_value = newvalue.asInteger();
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Reply op(new_value);
  op.perform(&timeout);
  return timeout.getReplyDelay() == new_value;
}
bool handleCurlTimeoutReplyDelay(LLSD const& newvalue)
{
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Reply op(newvalue.asInteger());
  op.perform(&timeout);
  AIHTTPTimeoutPolicy::setDefaultCurlTimeout(timeout);
  return true;
}
bool validateCurlTimeoutLowSpeedLimit(LLSD const& newvalue)
{
  U32 new_value = newvalue.asInteger();
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Speed op(timeout.getLowSpeedTime(), new_value);
  op.perform(&timeout);
  return timeout.getLowSpeedLimit() == new_value;
}
bool handleCurlTimeoutLowSpeedLimit(LLSD const& newvalue)
{
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Speed op(timeout.getLowSpeedTime(), newvalue.asInteger());
  op.perform(&timeout);
  AIHTTPTimeoutPolicy::setDefaultCurlTimeout(timeout);
  return true;
}
bool validateCurlTimeoutLowSpeedTime(LLSD const& newvalue)
{
  U32 new_value = newvalue.asInteger();
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Speed op(new_value, timeout.getLowSpeedLimit());
  op.perform(&timeout);
  return timeout.getLowSpeedTime() == new_value;
}
bool handleCurlTimeoutLowSpeedTime(LLSD const& newvalue)
{
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Speed op(newvalue.asInteger(), timeout.getLowSpeedLimit());
  op.perform(&timeout);
  AIHTTPTimeoutPolicy::setDefaultCurlTimeout(timeout);
  return true;
}
bool validateCurlTimeoutMaxTransaction(LLSD const& newvalue)
{
  U32 new_value = newvalue.asInteger();
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Transaction op(new_value);
  op.perform(&timeout);
  return timeout.getCurlTransaction() == new_value;
}
bool handleCurlTimeoutMaxTransaction(LLSD const& newvalue)
{
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Transaction op(newvalue.asInteger());
  op.perform(&timeout);
  AIHTTPTimeoutPolicy::setDefaultCurlTimeout(timeout);
  return true;
}
bool validateCurlTimeoutMaxTotalDelay(LLSD const& newvalue)
{
  U32 new_value = newvalue.asInteger();
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Total op(new_value);
  op.perform(&timeout);
  return timeout.getTotalDelay() == new_value;
}
bool handleCurlTimeoutMaxTotalDelay(LLSD const& newvalue)
{
  AIHTTPTimeoutPolicyBase timeout = AIHTTPTimeoutPolicyBase::getDebugSettingsCurlTimeout();
  AIHTTPTimeoutPolicyOperators::Total op(newvalue.asInteger());
  op.perform(&timeout);
  AIHTTPTimeoutPolicy::setDefaultCurlTimeout(timeout);
  return true;
}
AIHTTPTimeoutPolicy::namemap_t AIHTTPTimeoutPolicy::sNameMap;
AIHTTPTimeoutPolicy const* AIHTTPTimeoutPolicy::getTimeoutPolicyByName(std::string const& name)
{
  namemap_t::iterator iter = sNameMap.find(name);
  if (iter == sNameMap.end())
  {
	if (!name.empty())
	{
	  LL_WARNS() << "Cannot find AIHTTPTimeoutPolicy with name \"" << name << "\"." << LL_ENDL;
	}
	return &sDebugSettingsCurlTimeout;
  }
  return iter->second;
}
#undef P
#define P(n)		AIHTTPTimeoutPolicy n##_timeout(#n)
#define P2(n, b)	AIHTTPTimeoutPolicy n##_timeout(#n, b)
P(assetReportHandler);
P(authHandler);
P2(baseCapabilitiesComplete,					transfer_18s_connect_5s);
P2(baseCapabilitiesCompleteTracker,				transfer_18s_connect_5s);
P(blockingLLSDPost);
P(blockingLLSDGet);
P(blockingRawGet);
P(classifiedStatsResponder);
P(emeraldDicDownloader);
P(environmentApplyResponder);
P(environmentRequestResponder);
P2(eventPollResponder,							reply_60s);
P(FetchItemHttpHandler);
P(fetchScriptLimitsAttachmentInfoResponder);
P(fetchScriptLimitsRegionDetailsResponder);
P(fetchScriptLimitsRegionInfoResponder);
P(fetchScriptLimitsRegionSummaryResponder);
P(fnPtrResponder);
P(floaterPermsResponder);
P2(groupProposalBallotResponder,				transfer_300s);
P(homeLocationResponder);
P2(HTTPGetResponder,							reply_15s);
P(iamHere);
P2(BGFolderHttpHandler, transfer_300s);
P(BGItemHttpHandler);
P(lcl_responder);
P(mapLayerResponder);
P(materialsResponder);
P2(maturityPreferences,							transfer_30s_connect_10s);
P(mediaDataClientResponder);
P(mediaTypeResponder);
P2(meshDecompositionResponder,					connect_30s);
P2(meshHeaderResponder,							connect_30s);
P2(meshLODResponder,							connect_30s);
P2(meshPhysicsShapeResponder,					connect_30s);
P2(meshSkinInfoResponder,						connect_30s);
P(objectCostResponder);
P(physicsFlagsResponder);
P(productInfoRequestResponder);
P(regionResponder);
P(remoteParcelRequestResponder);
P(requestAgentUpdateAppearance);
P(responderIgnore);
P(setDisplayNameResponder);
P2(baseFeaturesReceived,						transfer_22s_connect_10s);
P2(startGroupVoteResponder,						transfer_300s);
P(translationReceiver);
P(uploadModelPremissionsResponder);
P(verifiedDestinationResponder);
P(viewerChatterBoxInvitationAcceptResponder);
P(viewerStatsResponder);
P(voiceCallCapResponder);
P(webProfileResponders);
P(wholeModelFeeResponder);
P(wholeModelUploadResponder);
P2(XMLRPCResponder,								connect_40s);
P(getUpdateInfoResponder);
P2(AISAPIResponder, connect_60s);