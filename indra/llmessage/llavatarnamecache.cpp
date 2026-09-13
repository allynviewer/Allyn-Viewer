/** 
 * @file llavatarnamecache.cpp
 * @brief Provides lookup of avatar SLIDs ("bobsmith123") and display names
 * ("James Cook") from avatar UUIDs.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#include "llavatarnamecache.h"
#include "llcachename.h"
#include "llcontrol.h"
#include "llframetimer.h"
#include "llhttpclient.h"
#include "llsd.h"
#include "llsdserialize.h"
#include <boost/tokenizer.hpp>
#include <map>
#include <set>
namespace LLAvatarNameCache
{
	use_display_name_signal_t mUseDisplayNamesSignal;
	bool sForceDisplayNames = false;
	bool sRunning = false;
	bool sUsePeopleAPI = true;
	std::string sNameLookupURL;
	typedef uuid_set_t ask_queue_t;
	ask_queue_t sAskQueue;
	typedef std::map<LLUUID, F64> pending_queue_t;
	pending_queue_t sPendingQueue;
	typedef std::map<LLUUID, callback_signal_t*> signal_map_t;
	signal_map_t sSignalMap;
	typedef std::map<LLUUID, LLAvatarName> cache_t;
	cache_t sCache;
	LLFrameTimer sRequestTimer;
    const F64 MAX_UNREFRESHED_TIME = 20.0 * 60.0;
    static F64 sLastExpireCheck;
	const F64 TEMP_CACHE_ENTRY_LIFETIME = 60.0;
	void processName(const LLUUID& agent_id,
					 const LLAvatarName& av_name);
	void requestNamesViaCapability();
	void legacyNameCallback(const LLUUID& agent_id,
							const std::string& full_name,
							bool is_group);
	void legacyNameFetch(const LLUUID& agent_id,
						 const std::string& full_name,
						 bool is_group);
	void requestNamesViaLegacy();
	void fireSignal(const LLUUID& agent_id,
					const callback_slot_t& slot,
					const LLAvatarName& av_name);
	bool isRequestPending(const LLUUID& agent_id);
	void eraseUnrefreshed();
	bool expirationFromCacheControl(AIHTTPReceivedHeaders const& headers, F64* expires);
}
class LLAvatarNameResponder : public LLHTTPClient::ResponderWithResult
{
	LOG_CLASS(LLAvatarNameResponder);
private:
	uuid_vec_t mAgentIDs;
	bool needsHeaders() const { return true; }
	char const* getName() const { return "LLAvatarNameResponder"; }
public:
	LLAvatarNameResponder(const uuid_vec_t& agent_ids)
	:	mAgentIDs(agent_ids)
	{ }
protected:
	void httpSuccess()
	{
		const LLSD& content = getContent();
		if (!content.isMap())
		{
			failureResult(HTTP_INTERNAL_ERROR_OTHER, "Malformed response contents", content);
			return;
		}
		F64 expires = LLAvatarNameCache::nameExpirationFromHeaders(getResponseHeaders());
		F64 now = LLFrameTimer::getTotalSeconds();
		const LLSD& agents = content["agents"];
		LLSD::array_const_iterator it = agents.beginArray();
		for ( ; it != agents.endArray(); ++it)
		{
			const LLSD& row = *it;
			LLUUID agent_id = row["id"].asUUID();
			LLAvatarName av_name;
			av_name.fromLLSD(row);
			av_name.mExpires = expires;
			LL_DEBUGS("AvNameCache") << "LLAvatarNameResponder::result for " << agent_id << LL_ENDL;
			av_name.dump();
			LLAvatarNameCache::processName(agent_id, av_name);
		}
		const LLSD& unresolved_agents = content["bad_ids"];
		S32  num_unresolved = unresolved_agents.size();
		if (num_unresolved > 0)
		{
            LL_WARNS("AvNameCache") << "LLAvatarNameResponder::result " << num_unresolved << " unresolved ids; "
                                    << "expires in " << expires - now << " seconds"
                                    << LL_ENDL;
			it = unresolved_agents.beginArray();
			for ( ; it != unresolved_agents.endArray(); ++it)
			{
				const LLUUID& agent_id = *it;
				LL_WARNS("AvNameCache") << "LLAvatarNameResponder::result "
                                        << "failed id " << agent_id
                                        << LL_ENDL;
                LLAvatarNameCache::handleAgentError(agent_id);
			}
		}
        LL_DEBUGS("AvNameCache") << "LLAvatarNameResponder::result "
                                 << LLAvatarNameCache::sCache.size() << " cached names"
                                 << LL_ENDL;
    }
	void httpFailure()
	{
		LL_WARNS("AvNameCache") << dumpResponse() << LL_ENDL;
		auto it = mAgentIDs.begin();
		for ( ; it != mAgentIDs.end(); ++it)
		{
			const LLUUID& agent_id = *it;
			LLAvatarNameCache::handleAgentError(agent_id);
		}
	}
};
void LLAvatarNameCache::handleAgentError(const LLUUID& agent_id)
{
	std::map<LLUUID,LLAvatarName>::iterator existing = sCache.find(agent_id);
	if (existing == sCache.end())
    {
        LL_WARNS("AvNameCache") << "LLAvatarNameCache get legacy for agent "
                                << agent_id << LL_ENDL;
        gCacheName->get(agent_id, false,
                        boost::bind(&LLAvatarNameCache::legacyNameFetch, _1, _2, _3));
    }
	else
    {
        LLAvatarNameCache::sPendingQueue.erase(agent_id);
        LLAvatarName& av_name = existing->second;
        LL_DEBUGS("AvNameCache") << "LLAvatarNameCache use cache for agent " << agent_id << LL_ENDL;
		av_name.dump();
		av_name.setExpires(TEMP_CACHE_ENTRY_LIFETIME);
    }
}
void LLAvatarNameCache::processName(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	sCache[agent_id] = av_name;
	sPendingQueue.erase(agent_id);
	signal_map_t::iterator sig_it =	sSignalMap.find(agent_id);
	if (sig_it != sSignalMap.end())
	{
		callback_signal_t* signal = sig_it->second;
		(*signal)(agent_id, av_name);
		sSignalMap.erase(agent_id);
		delete signal;
		signal = NULL;
	}
}
void LLAvatarNameCache::requestNamesViaCapability()
{
	F64 now = LLFrameTimer::getTotalSeconds();
	static const U32 NAME_URL_MAX = 4096;
	static const U32 NAME_URL_SEND_THRESHOLD = 3500;
	std::string url;
	url.reserve(NAME_URL_MAX);
	uuid_vec_t agent_ids;
	agent_ids.reserve(128);
	U32 ids = 0;
	ask_queue_t::const_iterator it;
	while(!sAskQueue.empty())
	{
		it = sAskQueue.begin();
		const LLUUID agent_id = *it;
		sAskQueue.erase(it);
		if (url.empty())
		{
			url += sNameLookupURL;
			url += "?ids=";
			ids = 1;
		}
		else
		{
			url += "&ids=";
			ids++;
		}
		url += agent_id.asString();
		agent_ids.push_back(agent_id);
		sPendingQueue[agent_id] = now;
		if (url.size() > NAME_URL_SEND_THRESHOLD)
		{
			break;
		}
	}
	if (!url.empty())
	{
		LL_INFOS("AvNameCache") << "LLAvatarNameCache::requestNamesViaCapability getting " << ids << " ids" << LL_ENDL;
		LLHTTPClient::get(url, new LLAvatarNameResponder(agent_ids));
	}
}
void LLAvatarNameCache::legacyNameCallback(const LLUUID& agent_id,
										   const std::string& full_name,
										   bool is_group)
{
	legacyNameFetch(agent_id, full_name, is_group);
	std::map<LLUUID,LLAvatarName>::iterator av_record = sCache.find(agent_id);
	LLAvatarName& av_name = av_record->second;
	av_name.setExpires(MAX_UNREFRESHED_TIME);
}
void LLAvatarNameCache::legacyNameFetch(const LLUUID& agent_id,
										const std::string& full_name,
										bool is_group)
{
	LL_DEBUGS("AvNameCache") << "LLAvatarNameCache agent " << agent_id << " "
							 << "full name '" << full_name << "'"
							 << ( is_group ? " [group]" : "" )
							 << LL_ENDL;
	LLAvatarName av_name;
	av_name.fromString(full_name);
	processName(agent_id, av_name);
}
void LLAvatarNameCache::requestNamesViaLegacy()
{
	static const S32 MAX_REQUESTS = 100;
	F64 now = LLFrameTimer::getTotalSeconds();
	std::string full_name;
	ask_queue_t::const_iterator it;
	for (S32 requests = 0; !sAskQueue.empty() && requests < MAX_REQUESTS; ++requests)
	{
		it = sAskQueue.begin();
		const LLUUID& agent_id = *it;
		sAskQueue.erase(it);
		sPendingQueue[agent_id] = now;
		LL_DEBUGS("AvNameCache") << "LLAvatarNameCache::requestNamesViaLegacy agent " << agent_id << LL_ENDL;
		gCacheName->get(agent_id, false,
			boost::bind(&LLAvatarNameCache::legacyNameCallback, _1, _2, _3));
	}
}
void LLAvatarNameCache::initClass(bool running, bool usePeopleAPI)
{
	sRunning = running;
	sUsePeopleAPI = usePeopleAPI;
}
void LLAvatarNameCache::cleanupClass()
{
	sCache.clear();
}
bool LLAvatarNameCache::importFile(std::istream& istr)
{
	LLSD data;
	if (LLSDParser::PARSE_FAILURE == LLSDSerialize::fromXMLDocument(data, istr))
	{
        LL_WARNS("AvNameCache") << "avatar name cache data xml parse failed" << LL_ENDL;
		return false;
	}
	LLSD agents = data["agents"];
	LLUUID agent_id;
	LLAvatarName av_name;
	LLSD::map_const_iterator it = agents.beginMap();
	for ( ; it != agents.endMap(); ++it)
	{
		agent_id.set(it->first);
		av_name.fromLLSD( it->second );
		sCache[agent_id] = av_name;
	}
    LL_INFOS("AvNameCache") << "LLAvatarNameCache loaded " << sCache.size() << LL_ENDL;
    return true;
}
void LLAvatarNameCache::exportFile(std::ostream& ostr)
{
	LLSD agents;
	F64 max_unrefreshed = LLFrameTimer::getTotalSeconds() - MAX_UNREFRESHED_TIME;
    LL_INFOS("AvNameCache") << "LLAvatarNameCache at exit cache has " << sCache.size() << LL_ENDL;
	cache_t::const_iterator it = sCache.begin();
	for ( ; it != sCache.end(); ++it)
	{
		const LLUUID& agent_id = it->first;
		const LLAvatarName& av_name = it->second;
		if (av_name.isValidName(max_unrefreshed))
		{
			agents[agent_id.asString()] = av_name.asLLSD();
		}
	}
    LL_INFOS("AvNameCache") << "LLAvatarNameCache returning " << agents.size() << LL_ENDL;
	LLSD data;
	data["agents"] = agents;
	LLSDSerialize::toPrettyXML(data, ostr);
}
void LLAvatarNameCache::setNameLookupURL(const std::string& name_lookup_url)
{
	sNameLookupURL = name_lookup_url;
}
bool LLAvatarNameCache::hasNameLookupURL()
{
	return !sNameLookupURL.empty();
}
bool LLAvatarNameCache::usePeopleAPI()
{
	return hasNameLookupURL() && sUsePeopleAPI;
}
void LLAvatarNameCache::idle()
{
	sRunning = true;
	const F32 SECS_BETWEEN_REQUESTS = 0.1f;
	if (!sRequestTimer.hasExpired())
	{
		return;
	}
	if (!sAskQueue.empty())
	{
        if (usePeopleAPI())
        {
            requestNamesViaCapability();
        }
        else
        {
            LL_WARNS_ONCE("AvNameCache") << "LLAvatarNameCache still using legacy api" << LL_ENDL;
            requestNamesViaLegacy();
        }
	}
	if (sAskQueue.empty())
	{
		sRequestTimer.resetWithExpiry(SECS_BETWEEN_REQUESTS);
	}
    eraseUnrefreshed();
}
bool LLAvatarNameCache::isRequestPending(const LLUUID& agent_id)
{
	bool isPending = false;
	const F64 PENDING_TIMEOUT_SECS = 5.0 * 60.0;
	pending_queue_t::const_iterator it = sPendingQueue.find(agent_id);
	if (it != sPendingQueue.end())
	{
		F64 expire_time = LLFrameTimer::getTotalSeconds() - PENDING_TIMEOUT_SECS;
		isPending = (it->second > expire_time);
	}
	return isPending;
}
void LLAvatarNameCache::eraseUnrefreshed()
{
	F64 now = LLFrameTimer::getTotalSeconds();
	F64 max_unrefreshed = now - MAX_UNREFRESHED_TIME;
    if (!sLastExpireCheck || sLastExpireCheck < max_unrefreshed)
    {
        sLastExpireCheck = now;
        S32 expired = 0;
        for (cache_t::iterator it = sCache.begin(); it != sCache.end();)
        {
            const LLAvatarName& av_name = it->second;
            if (av_name.mExpires < max_unrefreshed)
            {
                LL_DEBUGS("AvNameCacheExpired") << "LLAvatarNameCache " << it->first
                                         << " user '" << av_name.getAccountName() << "' "
                                         << "expired " << now - av_name.mExpires << " secs ago"
                                         << LL_ENDL;
                sCache.erase(it++);
                expired++;
            }
			else
			{
				++it;
			}
        }
        LL_INFOS("AvNameCache") << "LLAvatarNameCache expired " << expired << " cached avatar names, "
                                << sCache.size() << " remaining" << LL_ENDL;
	}
}
bool LLAvatarNameCache::get(const LLUUID& agent_id, LLAvatarName *av_name)
{
	if (sRunning)
	{
		std::map<LLUUID,LLAvatarName>::iterator it = sCache.find(agent_id);
		if (it != sCache.end())
		{
			*av_name = it->second;
			if (av_name->mExpires < LLFrameTimer::getTotalSeconds())
			{
				if (!isRequestPending(agent_id))
				{
					LL_DEBUGS("AvNameCache") << "LLAvatarNameCache refresh agent " << agent_id
											 << LL_ENDL;
					sAskQueue.insert(agent_id);
				}
			}
			return true;
		}
		else if (!usePeopleAPI())
		{
			std::string full_name;
			if (gCacheName->getFullName(agent_id, full_name))
			{
				av_name->fromString(full_name);
				sCache[agent_id] = *av_name;
				return true;
			}
		}
	}
	if (!isRequestPending(agent_id))
	{
		LL_DEBUGS("AvNameCache") << "LLAvatarNameCache queue request for agent " << agent_id << LL_ENDL;
		sAskQueue.insert(agent_id);
	}
	return false;
}
const S32& main_name_system()
{
	static const LLCachedControl<S32> name_system("PhoenixNameSystem", 0);
	return name_system;
}
bool LLAvatarNameCache::getNSName(const LLUUID& agent_id, std::string& name, const S32& name_system)
{
	LLAvatarName avatar_name;
	if (get(agent_id, &avatar_name))
		name = avatar_name.getNSName(name_system);
	else return false;
	return true;
}
void LLAvatarNameCache::fireSignal(const LLUUID& agent_id,
								   const callback_slot_t& slot,
								   const LLAvatarName& av_name)
{
	callback_signal_t signal;
	signal.connect(slot);
	signal(agent_id, av_name);
}
LLAvatarNameCache::callback_connection_t LLAvatarNameCache::get(const LLUUID& agent_id, callback_slot_t slot)
{
	callback_connection_t connection;
	if (sRunning)
	{
		std::map<LLUUID,LLAvatarName>::iterator it = sCache.find(agent_id);
		if (it != sCache.end())
		{
			const LLAvatarName& av_name = it->second;
			if (av_name.mExpires > LLFrameTimer::getTotalSeconds())
			{
				fireSignal(agent_id, slot, av_name);
				return connection;
			}
		}
	}
	if (!isRequestPending(agent_id))
	{
		sAskQueue.insert(agent_id);
	}
	signal_map_t::iterator sig_it = sSignalMap.find(agent_id);
	if (sig_it == sSignalMap.end())
	{
		callback_signal_t* signal = new callback_signal_t();
		connection = signal->connect(slot);
		sSignalMap[agent_id] = signal;
	}
	else
	{
		callback_signal_t* signal = sig_it->second;
		connection = signal->connect(slot);
	}
	return connection;
}
bool LLAvatarNameCache::getForceDisplayNames()
{
	return sForceDisplayNames;
}
void LLAvatarNameCache::setForceDisplayNames(bool force)
{
	sForceDisplayNames = force;
	if ( (!LLAvatarName::useDisplayNames()) && (force) )
	{
		setUseDisplayNames(true);
	}
}
void LLAvatarNameCache::setUseDisplayNames(bool use)
{
	use |= getForceDisplayNames();
	if (use != LLAvatarName::useDisplayNames())
	{
		LLAvatarName::setUseDisplayNames(use);
		mUseDisplayNamesSignal();
	}
}
void LLAvatarNameCache::setUseUsernames(bool use)
{
	if (use != LLAvatarName::useUsernames())
	{
		LLAvatarName::setUseUsernames(use);
		mUseDisplayNamesSignal();
	}
}
void LLAvatarNameCache::erase(const LLUUID& agent_id)
{
	sCache.erase(agent_id);
}
void LLAvatarNameCache::insert(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	sCache[agent_id] = av_name;
}
F64 LLAvatarNameCache::nameExpirationFromHeaders(AIHTTPReceivedHeaders const& headers)
{
	F64 expires = 0.0;
	if (expirationFromCacheControl(headers, &expires))
	{
		return expires;
	}
	else
	{
		const F64 DEFAULT_EXPIRES = 60.0 * 60.0;
		F64 now = LLFrameTimer::getTotalSeconds();
		return now + DEFAULT_EXPIRES;
	}
}
bool LLAvatarNameCache::expirationFromCacheControl(AIHTTPReceivedHeaders const& headers, F64* expires)
{
	bool fromCacheControl = false;
	F64 now = LLFrameTimer::getTotalSeconds();
	std::string cache_control;
	if (headers.getFirstValue("cache-control", cache_control))
	{
		S32 max_age = 0;
		if (max_age_from_cache_control(cache_control, &max_age))
		{
			*expires = now + (F64)max_age;
			fromCacheControl = true;
		}
	}
	LL_DEBUGS("AvNameCache") << "LLAvatarNameCache "
		<< ( fromCacheControl ? "expires based on cache control " : "default expiration " )
		<< "in " << *expires - now << " seconds"
		<< LL_ENDL;
	return fromCacheControl;
}
void LLAvatarNameCache::addUseDisplayNamesCallback(const use_display_name_signal_t::slot_type& cb)
{
	mUseDisplayNamesSignal.connect(cb);
}
static const std::string MAX_AGE("max-age");
static const boost::char_separator<char> EQUALS_SEPARATOR("=");
static const boost::char_separator<char> COMMA_SEPARATOR(",");
bool max_age_from_cache_control(const std::string& cache_control, S32 *max_age)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	tokenizer directives(cache_control, COMMA_SEPARATOR);
	tokenizer::iterator token_it = directives.begin();
	for ( ; token_it != directives.end(); ++token_it)
	{
		std::string token = *token_it;
		LLStringUtil::trim(token);
		if (token.compare(0, MAX_AGE.size(), MAX_AGE) == 0)
		{
			tokenizer subtokens(token, EQUALS_SEPARATOR);
			tokenizer::iterator subtoken_it = subtokens.begin();
			if (subtoken_it == subtokens.end()) return false;
			std::string subtoken = *subtoken_it;
			LLStringUtil::trim(subtoken);
			if (subtoken != MAX_AGE) return false;
			++subtoken_it;
			if (subtoken_it == subtokens.end()) return false;
			subtoken = *subtoken_it;
			LLStringUtil::trim(subtoken);
			if (subtoken == "0")
			{
				*max_age = 0;
				return true;
			}
			S32 val = atoi( subtoken.c_str() );
			if (val > 0 && val < S32_MAX)
			{
				*max_age = val;
				return true;
			}
			return false;
		}
	}
	return false;
}
