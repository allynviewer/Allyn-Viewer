/** 
 * @file llexperiencecache.h
 * @brief Caches information relating to experience keys
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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
#ifndef LL_LLEXPERIENCECACHE_H
#define LL_LLEXPERIENCECACHE_H
#include "linden_common.h"
#include "llsingleton.h"
#include "llframetimer.h"
#include "llsd.h"
#include <boost/signals2.hpp>
struct LLCoroResponder;
class LLSD;
class LLUUID;
class LLExperienceCache final : public LLSingleton < LLExperienceCache >
{
    friend class LLSingleton<LLExperienceCache>;
    LLExperienceCache();
public:
    typedef std::function<std::string(const std::string &)> CapabilityQuery_t;
    typedef std::function<void(const LLSD &)> ExperienceGetFn_t;
    void idleCoro();
    void setCapabilityQuery(CapabilityQuery_t queryfn);
    void cleanup();
    void erase(const LLUUID& key);
    bool fetch(const LLUUID& key, bool refresh = false);
    void insert(const LLSD& experience_data);
    const LLSD& get(const LLUUID& key);
    void get(const LLUUID& key, ExperienceGetFn_t slot);
    bool isRequestPending(const LLUUID& public_key);
    void fetchAssociatedExperience(const LLUUID& objectId, const LLUUID& itemId, ExperienceGetFn_t fn) { fetchAssociatedExperience(objectId, itemId, LLStringUtil::null, fn); }
    void fetchAssociatedExperience(const LLUUID& objectId, const LLUUID& itemId, std::string url, ExperienceGetFn_t fn);
    void findExperienceByName(const std::string text, int page, ExperienceGetFn_t fn);
    void getGroupExperiences(const LLUUID &groupId, ExperienceGetFn_t fn);
    void getRegionExperiences(CapabilityQuery_t regioncaps, ExperienceGetFn_t fn);
    void setRegionExperiences(CapabilityQuery_t regioncaps, const LLSD &experiences, ExperienceGetFn_t fn);
    void getExperiencePermission(const LLUUID &experienceId, ExperienceGetFn_t fn);
    void setExperiencePermission(const LLUUID &experienceId, const std::string &permission, ExperienceGetFn_t fn);
    void forgetExperiencePermission(const LLUUID &experienceId, ExperienceGetFn_t fn);
    void getExperienceAdmin(const LLUUID &experienceId, ExperienceGetFn_t fn);
    void updateExperience(LLSD updateData, ExperienceGetFn_t fn);
    static const std::string NAME;
    static const std::string EXPERIENCE_ID;
    static const std::string AGENT_ID;
    static const std::string GROUP_ID;
    static const std::string PROPERTIES;
    static const std::string EXPIRES;
    static const std::string DESCRIPTION;
    static const std::string QUOTA;
    static const std::string MATURITY;
    static const std::string METADATA;
    static const std::string SLURL;
    static const std::string MISSING;
    static const int PROPERTY_INVALID;
    static const int PROPERTY_PRIVILEGED;
    static const int PROPERTY_GRID;
    static const int PROPERTY_PRIVATE;
    static const int PROPERTY_DISABLED;
    static const int PROPERTY_SUSPENDED;
private:
    virtual ~LLExperienceCache();
	void initSingleton() override;
    typedef boost::signals2::signal < void(const LLSD &) > callback_signal_t;
	typedef boost::shared_ptr<callback_signal_t> signal_ptr;
	typedef std::map<LLUUID, signal_ptr> signal_map_t;
	typedef std::map<LLUUID, LLSD> cache_t;
	typedef uuid_set_t RequestQueue_t;
    typedef std::map<LLUUID, F64> PendingQueue_t;
	static const std::string PRIVATE_KEY;
	static const F64 DEFAULT_EXPIRATION;
	static const S32 DEFAULT_QUOTA;
    static const int SEARCH_PAGE_SIZE;
    void processExperience(const LLUUID& public_key, const LLSD& experience);
	cache_t			mCache;
	signal_map_t	mSignalMap;
	RequestQueue_t	mRequestQueue;
    PendingQueue_t  mPendingQueue;
    LLFrameTimer    mEraseExpiredTimer;
    CapabilityQuery_t mCapability;
    std::string     mCacheFileName;
    bool            mShutdown;
	void eraseExpired();
    void requestExperiencesCoro(const LLCoroResponder& responder, RequestQueue_t);
    void requestExperiences();
    void fetchAssociatedExperienceCoro(const LLCoroResponder& responder, ExperienceGetFn_t);
    void findExperienceByNameCoro(const LLCoroResponder& responder, ExperienceGetFn_t);
    void getGroupExperiencesCoro(const LLCoroResponder& responder, ExperienceGetFn_t);
    void regionExperiences(CapabilityQuery_t regioncaps, const LLSD& experiences, bool update, ExperienceGetFn_t fn);
    void regionExperiencesCoro(const LLCoroResponder& responder, ExperienceGetFn_t fn);
    void experiencePermissionCoro(const LLCoroResponder& responder, ExperienceGetFn_t fn);
    void bootstrap(const LLSD& legacyKeys, int initialExpiration);
    void exportFile(std::ostream& ostr) const;
    void importFile(std::istream& istr);
	const cache_t& getCached();
	LLUUID getExperienceId(const LLUUID& private_key, bool null_if_not_found=false);
    inline friend std::ostream &operator << (std::ostream &os, const LLExperienceCache &cache)
    {
        cache.exportFile(os);
        return os;
    }
    inline friend std::istream &operator >> (std::istream &is, LLExperienceCache &cache)
    {
        cache.importFile(is);
        return is;
    }
};
#endif
