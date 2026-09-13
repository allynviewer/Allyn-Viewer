/** 
 * @file llurlentry.h
 * @author Martin Reddy
 * @brief Describes the Url types that can be registered in LLUrlRegistry
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#ifndef LL_LLURLENTRY_H
#define LL_LLURLENTRY_H
#include "lluuid.h"
#include "lluicolor.h"
#include "llstyle.h"
#include "llavatarname.h"
#include "llhost.h"
#include <boost/regex.hpp>
class LLAvatarName;
typedef boost::signals2::signal<void (const std::string& url,
									  const std::string& label,
									  const std::string& icon)> LLUrlLabelSignal;
typedef LLUrlLabelSignal::slot_type LLUrlLabelCallback;
class LLUrlEntryBase
{
public:
	LLUrlEntryBase();
	virtual ~LLUrlEntryBase();
	boost::regex getPattern() const { return mPattern; }
	virtual std::string getUrl(const std::string &string) const;
	virtual std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) { return url; }
	virtual std::string getQuery(const std::string &url) const { return ""; }
	virtual std::string getIcon(const std::string &url);
	virtual LLStyleSP getStyle() const;
	virtual std::string getTooltip(const std::string &string) const { return mTooltip; }
	std::string getMenuName() const { return mMenuName; }
	virtual std::string getLocation(const std::string &url) const { return ""; }
	virtual bool underlineOnHoverOnly(const std::string &string) const { return true; }
	virtual bool isTrusted() const { return false; }
	virtual LLUUID	getID(const std::string &string) const { return LLUUID::null; }
	bool isLinkDisabled() const;
	bool isWikiLinkCorrect(const std::string& url);
	virtual bool isSLURLvalid(const std::string &url) const { return TRUE; };
protected:
	std::string getIDStringFromUrl(const std::string &url) const;
	std::string escapeUrl(const std::string &url) const;
	std::string unescapeUrl(const std::string &url) const;
	std::string getLabelFromWikiLink(const std::string &url) const;
	std::string getUrlFromWikiLink(const std::string &string) const;
	void addObserver(const std::string &id, const std::string &url, const LLUrlLabelCallback &cb);
	std::string urlToLabelWithGreyQuery(const std::string &url) const;
	std::string urlToGreyQuery(const std::string &url) const;
	virtual void callObservers(const std::string &id, const std::string &label, const std::string& icon);
	typedef struct {
		std::string url;
		LLUrlLabelSignal *signal;
	} LLUrlEntryObserver;
	boost::regex                                   	mPattern;
	std::string                                    	mIcon;
	std::string                                    	mMenuName;
	std::string                                    	mTooltip;
	std::multimap<std::string, LLUrlEntryObserver>	mObservers;
};
class LLUrlEntryHTTP final : public LLUrlEntryBase
{
public:
	LLUrlEntryHTTP();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getUrl(const std::string &string) const override;
	std::string getTooltip(const std::string &url) const override;
};
class LLUrlEntryHTTPLabel final : public LLUrlEntryBase
{
public:
	LLUrlEntryHTTPLabel();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getTooltip(const std::string &string) const override;
	std::string getUrl(const std::string &string) const override;
};
class LLUrlEntryHTTPNoProtocol final : public LLUrlEntryBase
{
public:
	LLUrlEntryHTTPNoProtocol();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getQuery(const std::string &url) const override;
	std::string getUrl(const std::string &string) const override;
	std::string getTooltip(const std::string &url) const override;
};
class LLUrlEntryInvalidSLURL final : public LLUrlEntryBase
{
public:
	LLUrlEntryInvalidSLURL();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getUrl(const std::string &string) const override;
	std::string getTooltip(const std::string &url) const override;
	bool isSLURLvalid(const std::string &url) const override;
};
class LLUrlEntrySLURL final : public LLUrlEntryBase
{
public:
	LLUrlEntrySLURL();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getLocation(const std::string &url) const override;
};
class LLUrlEntrySecondlifeURL : public LLUrlEntryBase
{
public:
	LLUrlEntrySecondlifeURL();
	bool isTrusted() const override { return true; }
	std::string getUrl(const std::string &string) const override;
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getQuery(const std::string &url) const override;
	std::string getTooltip(const std::string &url) const override;
};
class LLUrlEntrySimpleSecondlifeURL final : public LLUrlEntrySecondlifeURL
{
public:
	LLUrlEntrySimpleSecondlifeURL();
};
class LLUrlEntryAgent : public LLUrlEntryBase
{
public:
	LLUrlEntryAgent();
	~LLUrlEntryAgent()
	{
		for(const auto& conn_pair : mAvatarNameCacheConnections)
		{
			if (conn_pair.second.connected())
			{
				conn_pair.second.disconnect();
			}
		}
		mAvatarNameCacheConnections.clear();
	}
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getIcon(const std::string &url) override;
	std::string getTooltip(const std::string &string) const override;
	LLStyleSP getStyle() const override;
	LLUUID	getID(const std::string &string) const override;
	bool underlineOnHoverOnly(const std::string &string) const override;
protected:
	void callObservers(const std::string &id, const std::string &label, const std::string& icon) override;
private:
	void onAvatarNameCache(const LLUUID& id, const LLAvatarName& av_name);
	using avatar_name_cache_connection_map_t = std::multimap<LLUUID, boost::signals2::connection>;
	avatar_name_cache_connection_map_t mAvatarNameCacheConnections;
};
class LLUrlEntryAgentName : public LLUrlEntryBase, public boost::signals2::trackable
{
public:
	LLUrlEntryAgentName();
	~LLUrlEntryAgentName()
	{
		for (const auto& conn_pair : mAvatarNameCacheConnections)
		{
			if (conn_pair.second.connected())
			{
				conn_pair.second.disconnect();
			}
		}
		mAvatarNameCacheConnections.clear();
	}
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	LLStyleSP getStyle() const override;
protected:
	virtual std::string getName(const LLAvatarName& avatar_name) = 0;
private:
	void onAvatarNameCache(const LLUUID& id, const LLAvatarName& av_name);
	using avatar_name_cache_connection_map_t = std::multimap<LLUUID, boost::signals2::connection>;
	avatar_name_cache_connection_map_t mAvatarNameCacheConnections;
};
class LLUrlEntryAgentCompleteName final : public LLUrlEntryAgentName
{
public:
	LLUrlEntryAgentCompleteName();
private:
	std::string getName(const LLAvatarName& avatar_name) override;
};
class LLUrlEntryAgentLegacyName final : public LLUrlEntryAgentName
{
public:
	LLUrlEntryAgentLegacyName();
private:
	std::string getName(const LLAvatarName& avatar_name) override;
};
class LLUrlEntryAgentDisplayName final : public LLUrlEntryAgentName
{
public:
	LLUrlEntryAgentDisplayName();
private:
	std::string getName(const LLAvatarName& avatar_name) override;
};
class LLUrlEntryAgentUserName final : public LLUrlEntryAgentName
{
public:
	LLUrlEntryAgentUserName();
private:
	std::string getName(const LLAvatarName& avatar_name) override;
};
class LLUrlEntryExperienceProfile final : public LLUrlEntryBase
{
public:
	LLUrlEntryExperienceProfile();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
private:
	void onExperienceDetails(const LLSD& experience_details);
};
class LLUrlEntryGroup final : public LLUrlEntryBase
{
public:
	LLUrlEntryGroup();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	LLStyleSP getStyle() const override;
	LLUUID	getID(const std::string &string) const override;
private:
	void onGroupNameReceived(const LLUUID& id, const std::string& name, bool is_group);
};
class LLUrlEntryInventory final : public LLUrlEntryBase
{
public:
	LLUrlEntryInventory();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
private:
};
class LLUrlEntryObjectIM final : public LLUrlEntryBase
{
public:
	LLUrlEntryObjectIM();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getLocation(const std::string &url) const override;
private:
};
class LLUrlEntryParcel final : public LLUrlEntryBase
{
public:
	struct LLParcelData
	{
		LLUUID		parcel_id;
		std::string	name;
		std::string	sim_name;
		F32			global_x;
		F32			global_y;
		F32			global_z;
	};
	LLUrlEntryParcel();
	~LLUrlEntryParcel();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	void sendParcelInfoRequest(const LLUUID& parcel_id);
	void onParcelInfoReceived(const std::string &id, const std::string &label);
	static void processParcelInfo(const LLParcelData& parcel_data);
	static void setAgentID(const LLUUID& id) { sAgentID = id; }
	static void setSessionID(const LLUUID& id) { sSessionID = id; }
	static void setRegionHost(const LLHost& host) { sRegionHost = host; }
	static void setDisconnected(bool disconnected) { sDisconnected = disconnected; }
private:
	static LLUUID						sAgentID;
	static LLUUID						sSessionID;
	static LLHost						sRegionHost;
	static bool							sDisconnected;
	static std::set<LLUrlEntryParcel*>	sParcelInfoObservers;
};
class LLUrlEntryPlace final : public LLUrlEntryBase
{
public:
	LLUrlEntryPlace();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getLocation(const std::string &url) const override;
};
class LLUrlEntryRegion final : public LLUrlEntryBase
{
public:
	LLUrlEntryRegion();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getLocation(const std::string &url) const override;
};
class LLUrlEntryTeleport final : public LLUrlEntryBase
{
public:
	LLUrlEntryTeleport();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getLocation(const std::string &url) const override;
};
class LLUrlEntrySL final : public LLUrlEntryBase
{
public:
	LLUrlEntrySL();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
};
class LLUrlEntrySLLabel final : public LLUrlEntryBase
{
public:
	LLUrlEntrySLLabel();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getUrl(const std::string &string) const override;
	std::string getTooltip(const std::string &string) const override;
	bool underlineOnHoverOnly(const std::string &string) const override;
};
class LLUrlEntryWorldMap final : public LLUrlEntryBase
{
public:
	LLUrlEntryWorldMap();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getLocation(const std::string &url) const override;
};
class LLUrlEntryNoLink final : public LLUrlEntryBase
{
public:
	LLUrlEntryNoLink();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getUrl(const std::string &string) const override;
	LLStyleSP getStyle() const override;
};
class LLUrlEntryIcon final : public LLUrlEntryBase
{
public:
	LLUrlEntryIcon();
	std::string getUrl(const std::string &string) const override;
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getIcon(const std::string &url) override;
};
class LLUrlEntryEmail final : public LLUrlEntryBase
{
public:
	LLUrlEntryEmail();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getUrl(const std::string &string) const override;
};
class LLUrlEntryJira final : public LLUrlEntryBase
{
public:
	LLUrlEntryJira();
	std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) override;
	std::string getTooltip(const std::string &string) const override;
	std::string getUrl(const std::string &string) const override;
};
#endif
