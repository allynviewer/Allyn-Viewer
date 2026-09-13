/** 
 * @file llviewerdisplayname.cpp
 * @brief Wrapper for display name functionality
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
#include "llviewerprecompiledheaders.h"
#include "llviewerdisplayname.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llavatarnamecache.h"
#include "llhttpclient.h"
#include "llhttpnode.h"
#include "llnotificationsutil.h"
#include "llui.h"
namespace LLViewerDisplayName
{
	set_name_signal_t sSetDisplayNameSignal;
	name_changed_signal_t sNameChangedSignal;
	void addNameChangedCallback(const name_changed_signal_t::slot_type& cb)
	{
		sNameChangedSignal.connect(cb);
	}
	void doNothing() { }
}
class LLSetDisplayNameResponder final : public LLHTTPClient::ResponderIgnoreBody
{
	LOG_CLASS(LLSetDisplayNameResponder);
private:
	void httpFailure() override
	{
        LLViewerDisplayName::sSetDisplayNameSignal(false, LLStringUtil::null, LLSD());
		LLViewerDisplayName::sSetDisplayNameSignal.disconnect_all_slots();
	}
	char const* getName() const override { return "LLSetDisplayNameResponder"; }
};
void LLViewerDisplayName::set(const std::string& display_name, const set_name_slot_t& slot)
{
	LLViewerRegion* region = gAgent.getRegion();
	llassert(region);
	std::string cap_url = region->getCapability("SetDisplayName");
	if (cap_url.empty())
	{
		slot(false, "unsupported", LLSD());
		return;
	}
	AIHTTPHeaders headers("Accept-Language", LLUI::getLanguage());
	LLAvatarName av_name;
	if (!LLAvatarNameCache::get(gAgent.getID(), &av_name))
	{
		slot(false, "name unavailable", LLSD());
		return;
	}
	LLSD change_array = LLSD::emptyArray();
	change_array.append(av_name.getDisplayName());
	change_array.append(display_name);
	sSetDisplayNameSignal.connect(slot);
	LLSD body;
	body["display_name"] = change_array;
	LLHTTPClient::post(cap_url, body, new LLSetDisplayNameResponder, headers);
}
class LLSetDisplayNameReply final : public LLHTTPNode
{
	LOG_CLASS(LLSetDisplayNameReply);
public:
	void post(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const override
	{
		LLSD body = input["body"];
		S32 status = body["status"].asInteger();
		bool success = (status == HTTP_OK);
		std::string reason = body["reason"].asString();
		LLSD content = body["content"];
		LL_INFOS() << "status " << status << " reason " << reason << LL_ENDL;
		if (status == HTTP_CONFLICT)
		{
			LLUUID agent_id = gAgent.getID();
			LLAvatarNameCache::erase( agent_id );
			LLAvatarNameCache::get(agent_id, boost::bind(&LLViewerDisplayName::doNothing));
			LLVOAvatar::invalidateNameTag( agent_id );
		}
		LLViewerDisplayName::sSetDisplayNameSignal(success, reason, content);
		LLViewerDisplayName::sSetDisplayNameSignal.disconnect_all_slots();
	}
};
class LLDisplayNameUpdate final : public LLHTTPNode
{
	void post(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const override
	{
		LLSD body = input["body"];
		LLUUID agent_id = body["agent_id"];
		std::string old_display_name = body["old_display_name"];
		LLSD name_data = body["agent"];
		LLAvatarName av_name;
		av_name.fromLLSD( name_data );
		LL_INFOS() << "name-update now " << LLDate::now()
			<< " next_update " << LLDate(av_name.mNextUpdate)
			<< LL_ENDL;
		AIHTTPReceivedHeaders headers;
		av_name.mExpires =
			LLAvatarNameCache::nameExpirationFromHeaders(headers);
		LLAvatarNameCache::insert(agent_id, av_name);
		LLVOAvatar::invalidateNameTag(agent_id);
		if (gSavedSettings.getBOOL("ShowDisplayNameChanges"))
		{
			LLSD args;
			args["OLD_NAME"] = old_display_name;
			args["SLID"] = "secondlife:///app/agent/" + agent_id.asString() + "/username";
			args["NEW_NAME"] = av_name.getDisplayName();
			LLNotificationsUtil::add("DisplayNameUpdate", args);
		}
		if (agent_id == gAgent.getID())
		{
			LLViewerDisplayName::sNameChangedSignal();
		}
	}
};
LLHTTPRegistration<LLSetDisplayNameReply>
    gHTTPRegistrationMessageSetDisplayNameReply(
		"/message/SetDisplayNameReply");
LLHTTPRegistration<LLDisplayNameUpdate>
    gHTTPRegistrationMessageDisplayNameUpdate(
		"/message/DisplayNameUpdate");
