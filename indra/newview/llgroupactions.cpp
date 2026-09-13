/**
 * @file llgroupactions.cpp
 * @brief Group-related actions (join, leave, new, delete, etc)
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
#include "llviewerprecompiledheaders.h"
#include "llgroupactions.h"
#include "llagent.h"
#include "llcommandhandler.h"
#include "llfloaterchatterbox.h"
#include "llfloatergroupinfo.h"
#include "llfloatersearch.h"
#include "llgroupmgr.h"
#include "llimview.h"
#include "llnotificationsutil.h"
#include "llpanelgroup.h"
#include "llviewermessage.h"
#include "groupchatlistener.h"
#include "llslurl.h"
#include "rlvactions.h"
#include "rlvcommon.h"
#include "rlvhandler.h"
static GroupChatListener sGroupChatListener;
class LLGroupHandler : public LLCommandHandler
{
public:
	LLGroupHandler() : LLCommandHandler("group", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		if (tokens.size() < 1)
		{
			return false;
		}
		if (tokens[0].asString() == "create")
		{
			LLGroupActions::createGroup();
			return true;
		}
		if (tokens.size() < 2)
		{
			return false;
		}
		if (tokens[0].asString() == "list")
		{
			if (tokens[1].asString() == "show")
			{
				LLFloaterMyFriends::showInstance( 1 );
				return true;
			}
            return false;
		}
		LLUUID group_id;
		if (!group_id.set(tokens[0], FALSE))
		{
			return false;
		}
		if (tokens[1].asString() == "about")
		{
			if (group_id.isNull())
				return true;
			LLGroupActions::show(group_id);
			return true;
		}
		if (tokens[1].asString() == "inspect")
		{
			if (group_id.isNull())
				return true;
			LLGroupActions::inspect(group_id);
			return true;
		}
		return false;
	}
};
LLGroupHandler gGroupHandler;
class LLFetchGroupMemberData : public LLGroupMgrObserver
{
public:
	LLFetchGroupMemberData(const LLUUID& group_id) :
		mGroupId(group_id),
		mRequestProcessed(false),
		LLGroupMgrObserver(group_id)
	{
		LL_INFOS() << "Sending new group member request for group_id: "<< group_id << LL_ENDL;
		LLGroupMgr* mgr = LLGroupMgr::getInstance();
		mgr->addObserver(this);
		mgr->sendGroupPropertiesRequest(group_id);
		mgr->sendCapGroupMembersRequest(group_id);
	}
	~LLFetchGroupMemberData()
	{
		if (!mRequestProcessed)
		{
			LL_WARNS() << "Destroying pending group member request for group_id: "
				<< mGroupId << LL_ENDL;
		}
		LLGroupMgr::getInstance()->removeObserver(this);
	}
	void changed(LLGroupChange gc)
	{
		if (gc == GC_PROPERTIES && !mRequestProcessed)
		{
			LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mGroupId);
			if (!gdatap)
			{
				LL_WARNS() << "LLGroupMgr::getInstance()->getGroupData() was NULL" << LL_ENDL;
			}
			else if (!gdatap->isMemberDataComplete())
			{
				LL_WARNS() << "LLGroupMgr::getInstance()->getGroupData()->isMemberDataComplete() was FALSE" << LL_ENDL;
				processGroupData();
				mRequestProcessed = true;
			}
		}
	}
	LLUUID getGroupId() { return mGroupId; }
	virtual void processGroupData() = 0;
protected:
	LLUUID mGroupId;
private:
	bool mRequestProcessed;
};
class LLFetchLeaveGroupData : public LLFetchGroupMemberData
{
public:
	LLFetchLeaveGroupData(const LLUUID& group_id)
		: LLFetchGroupMemberData(group_id)
	{}
	void processGroupData()
	{
		LLGroupActions::processLeaveGroupDataResponse(mGroupId);
	}
};
LLFetchLeaveGroupData* gFetchLeaveGroupData = NULL;
void LLGroupActions::search()
{
	LLFloaterSearch::SearchQuery search;
	search.category = "groups";
	LLFloaterSearch::showInstance(search);
}
void LLGroupActions::startCall(const LLUUID& group_id)
{
	LLGroupData gdata;
	if (!gAgent.getGroupData(group_id, gdata))
	{
		LL_WARNS() << "Error getting group data" << LL_ENDL;
		return;
	}
	if ( (!RlvActions::canStartIM(group_id)) && (!RlvActions::hasOpenGroupSession(group_id)) )
	{
		make_ui_sound("UISndInvalidOp");
		RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTIM, LLSD().with("RECIPIENT", LLSLURL("group", group_id, "about").getSLURLString()));
		return;
	}
	LLUUID session_id = gIMMgr->addSession(gdata.mName, IM_SESSION_GROUP_START, group_id);
	if (session_id.isNull())
	{
		LL_WARNS() << "Error adding session" << LL_ENDL;
		return;
	}
	gIMMgr->autoStartCallOnStartup(session_id);
	make_ui_sound("UISndStartIM");
}
void LLGroupActions::join(const LLUUID& group_id)
{
	if (!gAgent.canJoinGroups())
	{
		LLNotificationsUtil::add("JoinedTooManyGroups");
		return;
	}
	LLGroupMgrGroupData* gdatap =
		LLGroupMgr::getInstance()->getGroupData(group_id);
	if (gdatap)
	{
		S32 cost = gdatap->mMembershipFee;
		LLSD args;
		args["COST"] = llformat("%d", cost);
		args["NAME"] = gdatap->mName;
		LLSD payload;
		payload["group_id"] = group_id;
		if (can_afford_transaction(cost))
		{
			if(cost > 0)
				LLNotificationsUtil::add("JoinGroupCanAfford", args, payload, onJoinGroup);
			else
				LLNotificationsUtil::add("JoinGroupNoCost", args, payload, onJoinGroup);
		}
		else
		{
			LLNotificationsUtil::add("JoinGroupCannotAfford", args, payload);
		}
	}
	else
	{
		LL_WARNS() << "LLGroupMgr::getInstance()->getGroupData(" << group_id
			<< ") was NULL" << LL_ENDL;
	}
}
bool LLGroupActions::onJoinGroup(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 1)
	{
		return false;
	}
	LLGroupMgr::getInstance()->
		sendGroupMemberJoin(notification["payload"]["group_id"].asUUID());
	return false;
}
void LLGroupActions::leave(const LLUUID& group_id)
{
	if ( (group_id.isNull()) || ((gAgent.getGroupID() == group_id) && (gRlvHandler.hasBehaviour(RLV_BHVR_SETGROUP))) )
	{
		return;
	}
	LLGroupData group_data;
	if (gAgent.getGroupData(group_id, group_data))
	{
		LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(group_id);
		if (!gdatap || !gdatap->isMemberDataComplete())
		{
			if (gFetchLeaveGroupData != NULL)
			{
				delete gFetchLeaveGroupData;
				gFetchLeaveGroupData = NULL;
			}
			gFetchLeaveGroupData = new LLFetchLeaveGroupData(group_id);
	}
		else
		{
			processLeaveGroupDataResponse(group_id);
		}
	}
}
void LLGroupActions::processLeaveGroupDataResponse(const LLUUID group_id)
{
	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(group_id);
	LLUUID agent_id = gAgent.getID();
	LLGroupMgrGroupData::member_list_t::iterator mit = gdatap->mMembers.find(agent_id);
	if ( mit != gdatap->mMembers.end() )
	{
		LLGroupMemberData* member_data = (*mit).second;
		if ( member_data && member_data->isOwner() && gdatap->mMemberCount == 1)
		{
			LLNotificationsUtil::add("OwnerCannotLeaveGroup");
			return;
		}
	}
	LLSD args;
	args["GROUP"] = gdatap->mName;
	LLSD payload;
	payload["group_id"] = group_id;
	LLNotificationsUtil::add("GroupLeaveConfirmMember", args, payload, onLeaveGroup);
}
void LLGroupActions::activate(const LLUUID& group_id)
{
	if ( (gRlvHandler.hasBehaviour(RLV_BHVR_SETGROUP)) && (gRlvHandler.getAgentGroup() != group_id) )
	{
		return;
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ActivateGroup);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_GroupID, group_id);
	gAgent.sendReliableMessage();
}
void LLGroupActions::inspect(const LLUUID& group_id)
{
	openGroupProfile(group_id);
}
void LLGroupActions::show(const LLUUID& group_id)
{
	if (group_id.isNull())
		return;
	openGroupProfile(group_id);
}
void LLGroupActions::showProfiles(const uuid_vec_t& ids)
{
	for (const auto& id : ids)
		show(id);
}
void LLGroupActions::showTab(const LLUUID& group_id, const std::string& tab_name)
{
	if (group_id.isNull()) return;
	openGroupProfile(group_id)->selectTabByName(tab_name);
}
void LLGroupActions::showNotice(const std::string& subj, const std::string& mes, const LLUUID& group_id, const bool& has_inventory, const std::string& item_name, LLOfferInfo* info)
{
	if (LLFloaterGroupInfo* fgi = LLFloaterGroupInfo::getInstance(group_id))
	{
		fgi->mPanelGroupp->showNotice(subj, mes, has_inventory, item_name, info);
	}
	else
	{
		if (info)
		{
			info->forceResponse(IOR_DECLINE);
		}
	}
}
void LLGroupActions::refresh(const LLUUID& group_id)
{
	if (LLFloaterGroupInfo* fgi = LLFloaterGroupInfo::getInstance(group_id))
		if (LLPanelGroup* pg = fgi->mPanelGroupp)
			pg->refreshData();
}
void LLGroupActions::createGroup()
{
	openGroupProfile(LLUUID::null);
}
void LLGroupActions::closeGroup(const LLUUID& group_id)
{
	if (LLFloaterGroupInfo* fgi = LLFloaterGroupInfo::getInstance(group_id))
		fgi->close();
}
LLUUID LLGroupActions::startIM(const LLUUID& group_id)
{
	if (group_id.isNull()) return LLUUID::null;
	if ( (!RlvActions::canStartIM(group_id)) && (!RlvActions::hasOpenGroupSession(group_id)) )
	{
		make_ui_sound("UISndInvalidOp");
		RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTIM, LLSD().with("RECIPIENT", LLSLURL("group", group_id, "about").getSLURLString()));
		return LLUUID::null;
	}
	LLGroupData group_data;
	if (gAgent.getGroupData(group_id, group_data))
	{
		static LLCachedControl<bool> tear_off("OtherChatsTornOff");
		if (!tear_off) gIMMgr->setFloaterOpen(TRUE);
		LLUUID session_id = gIMMgr->addSession(
			group_data.mName,
			IM_SESSION_GROUP_START,
			group_id);
		make_ui_sound("UISndStartIM");
		return session_id;
	}
	else
	{
		make_ui_sound("UISndInvalidOp");
		return LLUUID::null;
	}
}
void LLGroupActions::endIM(const LLUUID& group_id)
{
	if (group_id.isNull())
		return;
	LLUUID session_id = gIMMgr->computeSessionID(IM_SESSION_GROUP_START, group_id);
	if (session_id.notNull())
	{
		gIMMgr->removeSession(session_id);
	}
}
bool LLGroupActions::isInGroup(const LLUUID& group_id)
{
	return gAgent.isInGroup(group_id);
}
bool LLGroupActions::isAvatarMemberOfGroup(const LLUUID& group_id, const LLUUID& avatar_id)
{
	if(group_id.isNull() || avatar_id.isNull())
	{
		return false;
	}
	LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(group_id);
	if(!group_data)
	{
		return false;
	}
	if(group_data->mMembers.end() == group_data->mMembers.find(avatar_id))
	{
		return false;
	}
	return true;
}
bool LLGroupActions::onLeaveGroup(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLUUID group_id = notification["payload"]["group_id"].asUUID();
	if(option == 0)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_LeaveGroupRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_GroupData);
		msg->addUUIDFast(_PREHASH_GroupID, group_id);
		gAgent.sendReliableMessage();
	}
	return false;
}
LLFloaterGroupInfo* LLGroupActions::openGroupProfile(const LLUUID& group_id)
{
	LLFloaterGroupInfo* fgi = LLFloaterGroupInfo::getInstance(group_id);
	if (!fgi) fgi = new LLFloaterGroupInfo(group_id);
	fgi->center();
	fgi->open();
	return fgi;
}
std::string LLGroupActions::getSLURL(const LLUUID& id)
{
	return llformat("secondlife:///app/group/%s/about", id.asString().c_str());
}
