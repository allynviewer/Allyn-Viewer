/**
 * @file aihttptimeoutpolicy.h
 * @brief Store the policy on timing out a HTTP curl transaction.
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
 *   24/09/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AIHTTPTIMEOUTPOLICY_H
#define AIHTTPTIMEOUTPOLICY_H
#include "stdtypes.h"
#include <string>
#include <map>
class AIHTTPTimeoutPolicyBase;
namespace AIHTTPTimeoutPolicyOperators {
struct DNS;
struct Connect;
struct Reply;
struct Speed;
struct Transaction;
struct Total;
}
class AIHTTPTimeoutPolicy {
  protected:
	char const* const mName;
	AIHTTPTimeoutPolicyBase* const mBase;
	static AIHTTPTimeoutPolicyBase sDebugSettingsCurlTimeout;
	typedef std::map<std::string, AIHTTPTimeoutPolicy*> namemap_t;
	static namemap_t sNameMap;
  private:
	U16 mDNSLookupGrace;
	U16 mMaximumConnectTime;
	U16 mMaximumReplyDelay;
	U16 mLowSpeedTime;
	U32 mLowSpeedLimit;
	U16 mMaximumCurlTransaction;
	U16 mMaximumTotalDelay;
	friend struct AIHTTPTimeoutPolicyOperators::DNS;
	friend struct AIHTTPTimeoutPolicyOperators::Connect;
	friend struct AIHTTPTimeoutPolicyOperators::Reply;
	friend struct AIHTTPTimeoutPolicyOperators::Speed;
	friend struct AIHTTPTimeoutPolicyOperators::Transaction;
	friend struct AIHTTPTimeoutPolicyOperators::Total;
 public:
	AIHTTPTimeoutPolicy(
		char const* name,
		AIHTTPTimeoutPolicyBase& base = sDebugSettingsCurlTimeout);
	AIHTTPTimeoutPolicy(
		char const* name,
		U16 dns_lookup_grace,
		U16 subsequent_connects,
		U16 reply_delay,
		U16 low_speed_time,
		U32 low_speed_limit,
		U16 curl_transaction,
		U16 total_delay);
	virtual ~AIHTTPTimeoutPolicy() { }
	void sanity_checks(void) const;
	char const* name(void) const { return mName ? mName : "AIHTTPTimeoutPolicyBase"; }
	U16 getConnectTimeout(std::string const& hostname) const;
	U16 getDNSLookup(void) const { return mDNSLookupGrace; }
	U16 getConnect(void) const { return mMaximumConnectTime; }
	U16 getReplyDelay(void) const { return mMaximumReplyDelay; }
	U16 getLowSpeedTime(void) const { return mLowSpeedTime; }
	U32 getLowSpeedLimit(void) const { return mLowSpeedLimit; }
	U16 getCurlTransaction(void) const { return mMaximumCurlTransaction; }
	U16 getTotalDelay(void) const { return mMaximumTotalDelay; }
	static AIHTTPTimeoutPolicy const& getDebugSettingsCurlTimeout(void);
	static AIHTTPTimeoutPolicy const* getTimeoutPolicyByName(std::string const& name);
	static void setDefaultCurlTimeout(AIHTTPTimeoutPolicy const& defaultCurlTimeout);
	static bool connect_timed_out(std::string const& hostname);
	virtual void base_changed(void);
	virtual void changed(void);
  protected:
	AIHTTPTimeoutPolicy(AIHTTPTimeoutPolicy&);
	AIHTTPTimeoutPolicy& operator=(AIHTTPTimeoutPolicy const&);
};
class LLSD;
bool validateCurlTimeoutDNSLookup(LLSD const& newvalue);
bool handleCurlTimeoutDNSLookup(LLSD const& newvalue);
bool validateCurlTimeoutConnect(LLSD const& newvalue);
bool handleCurlTimeoutConnect(LLSD const& newvalue);
bool validateCurlTimeoutReplyDelay(LLSD const& newvalue);
bool handleCurlTimeoutReplyDelay(LLSD const& newvalue);
bool validateCurlTimeoutLowSpeedLimit(LLSD const& newvalue);
bool handleCurlTimeoutLowSpeedLimit(LLSD const& newvalue);
bool validateCurlTimeoutLowSpeedTime(LLSD const& newvalue);
bool handleCurlTimeoutLowSpeedTime(LLSD const& newvalue);
bool validateCurlTimeoutMaxTransaction(LLSD const& newvalue);
bool handleCurlTimeoutMaxTransaction(LLSD const& newvalue);
bool validateCurlTimeoutMaxTotalDelay(LLSD const& newvalue);
bool handleCurlTimeoutMaxTotalDelay(LLSD const& newvalue);
#endif
