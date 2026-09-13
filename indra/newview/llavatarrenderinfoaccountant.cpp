/**
 * @file   llavatarrenderinfoaccountant.cpp
 * @author Dave Simmons
 * @date   2013-02-28
 * @brief  
 * 
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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
#include "llavatarrenderinfoaccountant.h"
#include "llcharacter.h"
#include "llhttpclient.h"
#include "lltimer.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llworld.h"
static	const std::string KEY_AGENTS = "agents";
static 	const std::string KEY_WEIGHT = "weight";
static 	const std::string KEY_TOO_COMPLEX  = "tooComplex";
static  const std::string KEY_OVER_COMPLEXITY_LIMIT = "overlimit";
static  const std::string KEY_REPORTING_COMPLEXITY_LIMIT = "reportinglimit";
static	const std::string KEY_IDENTIFIER = "identifier";
static	const std::string KEY_MESSAGE = "message";
static	const std::string KEY_ERROR = "error";
static const F32 SECS_BETWEEN_REGION_SCANS   =  5.f;
static const F32 SECS_BETWEEN_REGION_REQUEST = 15.0;
static const F32 SECS_BETWEEN_REGION_REPORTS = 60.0;
LLFrameTimer LLAvatarRenderInfoAccountant::sRenderInfoReportTimer;
class LLAvatarRenderInfoGetResponder : public LLHTTPClient::ResponderWithResult
{
public:
	LLAvatarRenderInfoGetResponder(U64 region_handle) : mRegionHandle(region_handle)
	{
	}
	virtual void httpFailure();
	virtual void httpSuccess();
	char const* getName() const { return "This is dumb."; }
private:
	U64		mRegionHandle;
};
void LLAvatarRenderInfoGetResponder::httpFailure()
{
	const S32 statusNum = getStatus();
	const std::string& reason = getReason();
	LLViewerRegion * regionp = LLWorld::getInstance()->getRegionFromHandle(mRegionHandle);
	if (regionp)
	{
		LL_WARNS() << "HTTP error result for avatar weight GET: " << statusNum
				<< ", " << reason
				<< " returned by region " << regionp->getName()
				<< LL_ENDL;
	}
	else
	{
		LL_WARNS() << "Avatar render weight GET error recieved but region not found for "
			<< mRegionHandle
			<< ", error " << statusNum
			<< ", " << reason
			<< LL_ENDL;
	}
}
void LLAvatarRenderInfoGetResponder::httpSuccess()
{
	LLViewerRegion * regionp = LLWorld::getInstance()->getRegionFromHandle(mRegionHandle);
	if (!regionp)
	{
		LL_WARNS("AvatarRenderInfoAccountant") << "Avatar render weight info received but region not found for "
			<< mRegionHandle << LL_ENDL;
		return;
	}
	if (LLAvatarRenderInfoAccountant::logRenderInfo())
	{
		LL_INFOS() << "LRI: Result for avatar weights request for region " << regionp->getName() << ":" << LL_ENDL;
	}
	const LLSD& result = getContent();
	if (result.isMap())
	{
		if (result.has(KEY_AGENTS))
		{
			const LLSD & agents = result[KEY_AGENTS];
			if (agents.isMap())
			{
				for (LLSD::map_const_iterator agent_iter = agents.beginMap();
					agent_iter != agents.endMap();
					agent_iter++
					)
				{
					LLUUID target_agent_id = LLUUID(agent_iter->first);
					LLVOAvatar* avatarp = gObjectList.findAvatar(target_agent_id);
					if (avatarp &&
						!avatarp->isControlAvatar() &&
						avatarp->isAvatar())
					{
						const LLSD & agent_info_map = agent_iter->second;
						if (agent_info_map.isMap())
						{
							if (LLAvatarRenderInfoAccountant::logRenderInfo())
							{
								LL_INFOS() << "LRI:  Agent " << target_agent_id
									<< ": " << agent_info_map << LL_ENDL;
							}
							if (agent_info_map.has(KEY_WEIGHT))
							{
								avatarp->setReportedVisualComplexity(agent_info_map[KEY_WEIGHT].asInteger());
							}
						}
						else
						{
							LL_WARNS("AvatarRenderInfo") << "agent entry invalid"
								<< " agent " << target_agent_id
								<< " map " << agent_info_map
								<< LL_ENDL;
						}
					}
					else
					{
						LL_DEBUGS("AvatarRenderInfo") << "Unknown agent " << target_agent_id << LL_ENDL;
					}
				}
			}
			else
			{
				LL_WARNS("AvatarRenderInfo") << "malformed get response '" << KEY_AGENTS << "' is not map" << LL_ENDL;
			}
		}
		else
		{
			LL_INFOS("AvatarRenderInfo") << "no '" << KEY_AGENTS << "' key in get response" << LL_ENDL;
		}
		if (result.has(KEY_REPORTING_COMPLEXITY_LIMIT)
			&& result.has(KEY_OVER_COMPLEXITY_LIMIT))
		{
			U32 reporting = result[KEY_REPORTING_COMPLEXITY_LIMIT].asInteger();
			U32 overlimit = result[KEY_OVER_COMPLEXITY_LIMIT].asInteger();
			LL_DEBUGS("AvatarRenderInfo") << "complexity limit: " << reporting << " reporting, " << overlimit << " over limit" << LL_ENDL;
		}
		if (result.has(KEY_ERROR))
		{
			const LLSD & error = result[KEY_ERROR];
			LL_WARNS() << "Avatar render info GET error: "
				<< error[KEY_IDENTIFIER]
				<< ": " << error[KEY_MESSAGE]
				<< " from region " << regionp->getName()
				<< LL_ENDL;
		}
	}
}
class LLAvatarRenderInfoPostResponder : public LLHTTPClient::ResponderWithResult
{
public:
	LLAvatarRenderInfoPostResponder(U64 region_handle) : mRegionHandle(region_handle)
	{
	}
	virtual void httpFailure();
	virtual void httpSuccess();
	char const* getName() const { return "This is also dumb."; }
private:
	U64		mRegionHandle;
};
void LLAvatarRenderInfoAccountant::sendRenderInfoToRegion(LLViewerRegion * regionp)
{
	std::string url = regionp->getCapability("AvatarRenderInfo");
	if (!url.empty())
	{
	if (logRenderInfo())
	{
		LL_INFOS() << "LRI: Sending avatar render info to region "
			<< regionp->getName()
			<< " from " << url
			<< LL_ENDL;
	}
	U32 num_avs = 0;
	LLSD agents = LLSD::emptyMap();
	std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
	while( iter != LLCharacter::sInstances.end() )
	{
		LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*iter);
		if (avatar &&
			avatar->getRezzedStatus() >= 2 &&
			!avatar->isDead() &&
			!avatar->isControlAvatar() &&
			avatar->getObjectHost() == regionp->getHost())
		{
			avatar->calculateUpdateRenderComplexity();
			LLSD info = LLSD::emptyMap();
			U32 avatar_complexity = avatar->getVisualComplexity();
			if (avatar_complexity > 0)
			{
				info[KEY_WEIGHT] = (S32)(avatar_complexity < S32_MAX ? avatar_complexity : S32_MAX);
				info[KEY_TOO_COMPLEX]  = LLSD::Boolean(avatar->isTooComplex());
				agents[avatar->getID().asString()] = info;
				if (logRenderInfo())
				{
					LL_INFOS("AvatarRenderInfo") << "Sending avatar render info for " << avatar->getID()
							<< ": " << info << LL_ENDL;
				}
				num_avs++;
			}
		}
		iter++;
	}
	if (num_avs == 0)
		 return;
		LLSD report = LLSD::emptyMap();
		report[KEY_AGENTS] = agents;
		if (agents.size() > 0)
		{
			LLHTTPClient::post(url, report, new LLAvatarRenderInfoPostResponder(regionp->getHandle()));
		}
	}
}
void LLAvatarRenderInfoPostResponder::httpSuccess()
{
	LLViewerRegion * regionp = LLWorld::getInstance()->getRegionFromHandle(mRegionHandle);
    if (!regionp)
    {
        LL_INFOS("AvatarRenderInfoAccountant") << "Avatar render weight POST result received but region not found for "
                << mRegionHandle << LL_ENDL;
        return;
    }
	const LLSD& result = getContent();
	if (result.isMap())
	{
		if (result.has(KEY_ERROR))
		{
			const LLSD & error = result[KEY_ERROR];
			LL_WARNS("AvatarRenderInfoAccountant") << "POST error: "
				<< error[KEY_IDENTIFIER]
				<< ": " << error[KEY_MESSAGE]
				<< " from region " << regionp->getName()
				<< LL_ENDL;
		}
		else
		{
			LL_DEBUGS("AvatarRenderInfoAccountant")
				<< "POST result for region " << regionp->getName()
				<< ": " << result
				 << LL_ENDL;
		}
	}
	else
    {
        LL_WARNS("AvatarRenderInfoAccountant") << "Malformed POST response from region '" << regionp->getName()
                                               << LL_ENDL;
    }
}
void LLAvatarRenderInfoPostResponder::httpFailure()
{
	const S32 statusNum = getStatus();
	const std::string& reason = getReason();
	LLViewerRegion * regionp = LLWorld::getInstance()->getRegionFromHandle(mRegionHandle);
	if (regionp)
	{
		LL_WARNS() << "HTTP error result for avatar weight POST: " << statusNum
				<< ", " << reason
			<< " returned by region " << regionp->getName()
			<< LL_ENDL;
	}
	else
	{
		LL_WARNS() << "Avatar render weight POST error recieved but region not found for "
			<< mRegionHandle
			<< ", error " << statusNum
			<< ", " << reason
			<< LL_ENDL;
	}
}
void LLAvatarRenderInfoAccountant::getRenderInfoFromRegion(LLViewerRegion * regionp)
{
	std::string url = regionp->getCapability("AvatarRenderInfo");
	if (!url.empty())
	{
		if (logRenderInfo())
		{
			LL_INFOS() << "LRI: Requesting avatar render info for region "
				<< regionp->getName()
				<< " from " << url
				<< LL_ENDL;
		}
		LLHTTPClient::get(url, new LLAvatarRenderInfoGetResponder(regionp->getHandle()));
	}
}
void LLAvatarRenderInfoAccountant::idle()
{
	if (sRenderInfoReportTimer.hasExpired())
	{
		S32 num_avs = LLCharacter::sInstances.size();
		if (logRenderInfo())
		{
			LL_INFOS() << "LRI: Scanning all regions and checking for render info updates"
				<< LL_ENDL;
		}
		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
				iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLViewerRegion* regionp = *iter;
			if (regionp &&
				regionp->isAlive() &&
				regionp->capabilitiesReceived() &&
				regionp->getRenderInfoRequestTimer().hasExpired())
			{
				sendRenderInfoToRegion(regionp);
				getRenderInfoFromRegion(regionp);
				regionp->getRenderInfoRequestTimer().resetWithExpiry(SECS_BETWEEN_REGION_REQUEST + (2.f * num_avs));
			}
		}
		sRenderInfoReportTimer.resetWithExpiry(SECS_BETWEEN_REGION_SCANS);
	}
}
void LLAvatarRenderInfoAccountant::expireRenderInfoReportTimer(const LLUUID& region_id)
{
	if (logRenderInfo())
	{
		LL_INFOS() << "LRI: Viewer has new region capabilities, clearing global render info timer"
			<< " and timer for region " << region_id
			<< LL_ENDL;
	}
	sRenderInfoReportTimer.reset();
	LLViewerRegion* regionp = LLWorld::instance().getRegionFromID(region_id);
	if (regionp)
	{
		regionp->getRenderInfoRequestTimer().reset();
	}
}
bool LLAvatarRenderInfoAccountant::logRenderInfo()
{
	static LLCachedControl<bool> render_mute_logging_enabled(gSavedSettings, "RenderAutoMuteLogging", false);
	return render_mute_logging_enabled;
}
