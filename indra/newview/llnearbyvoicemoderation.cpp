/**
 * @file llnearbyvoicemoderation.cpp
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
#include "llagent.h"
#include "llnotificationsutil.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llvoicechannel.h"
#include "llvoiceclient.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "roles_constants.h"
#include "llnearbyvoicemoderation.h"
#include "llcorehttputil.h"
LLNearbyVoiceModeration::LLNearbyVoiceModeration()
{
}
LLNearbyVoiceModeration::~LLNearbyVoiceModeration()
{
}
LLVOAvatar* LLNearbyVoiceModeration::getVOAvatarFromId(const LLUUID& agent_id)
{
    LLViewerObject *obj = gObjectList.findObject(agent_id);
    while (obj && obj->isAttachment())
    {
        obj = (LLViewerObject*)obj->getParent();
    }
    if (obj && obj->isAvatar())
    {
        return (LLVOAvatar*)obj;
    }
    else
    {
        return NULL;
    }
}
const std::string LLNearbyVoiceModeration::getCapUrlFromRegion(LLViewerRegion* region)
{
    if (! region || ! region->capabilitiesReceived())
    {
        return std::string();
    }
    std::string url = region->getCapability("SpatialVoiceModerationRequest");
    if (url.empty())
    {
        LL_INFOS() << "Capability URL for region " << region->getName() << " is empty" << LL_ENDL;
        return std::string();
    }
    LL_INFOS() << "Capability URL for region " << region->getName() << " is " << url << LL_ENDL;
    return url;
}
void LLNearbyVoiceModeration::requestMuteIndividual(const LLUUID& agent_id, bool mute)
{
    LLVOAvatar* avatar = getVOAvatarFromId(agent_id);
    if (avatar)
    {
        const std::string cap_url = getCapUrlFromRegion(avatar->getRegion());
        if (cap_url.length())
        {
            const std::string operand = mute ? "mute" : "unmute";
            LLSD body;
            body["operand"] = operand;
            body["agent_id"] = agent_id;
            const std::string agent_name = avatar->getFullname();
            LL_INFOS() << "Resident " << agent_name
                       << " (" << agent_id << ")" << " applying " << operand << LL_ENDL;
            std::string success_msg =
                llformat("Resident %s (%s) nearby voice was set to %s", agent_name.c_str(), agent_id.asString().c_str(), operand.c_str());
            std::string failure_msg =
                llformat("Unable to change voice muting for resident %s (%s)", agent_name.c_str(), agent_id.asString().c_str());
            LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(
                cap_url,
                body,
                success_msg,
                failure_msg);
        }
    }
}
void LLNearbyVoiceModeration::requestMuteAll(bool mute)
{
    LLViewerRegion* region = gAgent.getRegion();
    const std::string cap_url = getCapUrlFromRegion(region);
    if (cap_url.length())
    {
        const std::string operand = mute ? "mute_all" : "unmute_all";
        LLSD body;
        body["operand"] = operand;
        LL_INFOS() << "For all residents in this region, applying: " << operand << LL_ENDL;
        std::string success_msg =
            llformat("Nearby voice for all residents was set to: %s", operand.c_str());
        std::string failure_msg =
            llformat("Unable to set nearby voice for all residents to: %s", operand.c_str());
        LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(
            cap_url,
            body,
            success_msg,
            failure_msg);
    }
}
void LLNearbyVoiceModeration::setMutedInfo(const std::string& channelID, bool mute)
{
    auto it = mChannelMuteMap.find(channelID);
    if (it == mChannelMuteMap.end())
    {
        if (mute)
        {
            showMutedNotification(true);
        }
        mChannelMuteMap[channelID] = mute;
    }
    else
    {
        if (it->second != mute)
        {
            showMutedNotification(mute);
            it->second = mute;
        }
    }
    if (mute && LLVoiceClient::getInstance()->getUserPTTState())
    {
        LLVoiceClient::getInstance()->setUserPTTState(false);
    }
}
bool LLNearbyVoiceModeration::showNotificationIfNeeded()
{
    if (LLVoiceClient::getInstance()->inProximalChannel() &&
        LLVoiceClient::getInstance()->getIsModeratorMuted(gAgentID))
    {
        return showMutedNotification(true);
    }
    return false;
}
bool LLNearbyVoiceModeration::showMutedNotification(bool is_muted)
{
    if (LLVoiceClient::getInstance()->inProximalChannel())
    {
        LLNotificationsUtil::add(is_muted ? "NearbyVoiceMutedByModerator" : "NearbyVoiceUnmutedByModerator");
        return true;
    }
    return false;
}
bool LLNearbyVoiceModeration::isNearbyChatModerator()
{
    if (!gAgent.getRegion() || false )
    {
        return false;
    }
    LLVoiceChannel* channel = LLVoiceChannel::getCurrentVoiceChannel();
    if (!channel || channel->getSessionID().notNull() || false)
    {
        return false;
    }
    if (false )
    {
        return gAgent.canManageEstate();
    }
    else
    {
        return gAgent.canManageEstate();
    }
}
