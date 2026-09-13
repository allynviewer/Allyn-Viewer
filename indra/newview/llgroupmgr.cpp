/** 
 * @file llgroupmgr.cpp
 * @brief LLGroupMgr class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llgroupmgr.h"
#include <vector>
#include <algorithm>
#include "llappviewer.h"
#include "llagent.h"
#include "llavatarnamecache.h"
#include "llui.h"
#include "message.h"
#include "roles_constants.h"
#include "lltransactiontypes.h"
#include "llstatusbar.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llfloaterdirectory.h"
#include "llfloatergroupinfo.h"
#include "llgroupactions.h"
#include "llnotificationsutil.h"
#include "lluictrlfactory.h"
#include "lltrans.h"
#include <boost/regex.hpp>
#if LL_MSVC
#pragma warning(push)
#pragma warning (disable:4702)
#endif
#include <boost/lexical_cast.hpp>
#if LL_MSVC
#pragma warning(pop)
#endif
const U32 MAX_CACHED_GROUPS = 20;
LLRoleActionSet::LLRoleActionSet()
: mActionSetData(NULL)
{ }
LLRoleActionSet::~LLRoleActionSet()
{
	delete mActionSetData;
	std::for_each(mActions.begin(), mActions.end(), DeletePointer());
	mActions.clear();
}
LLGroupMemberData::LLGroupMemberData(const LLUUID& id,
										S32 contribution,
										U64 agent_powers,
										const std::string& title,
										const std::string& online_status,
										BOOL is_owner) :
	mID(id),
	mContribution(contribution),
	mAgentPowers(agent_powers),
	mTitle(title),
	mOnlineStatus(online_status),
	mIsOwner(is_owner)
{
}
LLGroupMemberData::~LLGroupMemberData()
{
}
void LLGroupMemberData::addRole(const LLUUID& role, LLGroupRoleData* rd)
{
	mRolesList[role] = rd;
}
bool LLGroupMemberData::removeRole(const LLUUID& role)
{
	role_list_t::iterator it = mRolesList.find(role);
	if (it != mRolesList.end())
	{
		mRolesList.erase(it);
		return true;
	}
	return false;
}
LLGroupRoleData::LLGroupRoleData(const LLUUID& role_id,
								const std::string& role_name,
								const std::string& role_title,
								const std::string& role_desc,
								const U64 role_powers,
								const S32 member_count) :
	mRoleID(role_id),
	mMemberCount(member_count),
	mMembersNeedsSort(FALSE)
{
	mRoleData.mRoleName = role_name;
	mRoleData.mRoleTitle = role_title;
	mRoleData.mRoleDescription = role_desc;
	mRoleData.mRolePowers = role_powers;
	mRoleData.mChangeType = RC_UPDATE_NONE;
}
LLGroupRoleData::LLGroupRoleData(const LLUUID& role_id,
								LLRoleData role_data,
								const S32 member_count) :
	mRoleID(role_id),
	mRoleData(role_data),
	mMemberCount(member_count),
	mMembersNeedsSort(FALSE)
{
}
LLGroupRoleData::~LLGroupRoleData()
{
}
S32 LLGroupRoleData::getMembersInRole(uuid_vec_t& members,
									  BOOL needs_sort)
{
	if (mRoleID.isNull())
	{
		return members.size();
	}
	if (mMembersNeedsSort)
	{
		std::sort(mMemberIDs.begin(), mMemberIDs.end());
		mMembersNeedsSort = FALSE;
	}
	if (needs_sort)
	{
		std::sort(members.begin(), members.end());
	}
	S32 max_size = llmin( members.size(), mMemberIDs.size() );
	uuid_vec_t in_role( max_size );
	uuid_vec_t::iterator in_role_end;
	in_role_end = std::set_intersection(mMemberIDs.begin(), mMemberIDs.end(),
									members.begin(), members.end(),
									in_role.begin());
	return in_role_end - in_role.begin();
}
void LLGroupRoleData::addMember(const LLUUID& member)
{
	mMembersNeedsSort = TRUE;
	mMemberIDs.push_back(member);
}
bool LLGroupRoleData::removeMember(const LLUUID& member)
{
	uuid_vec_t::iterator it = std::find(mMemberIDs.begin(),mMemberIDs.end(),member);
	if (it != mMemberIDs.end())
	{
		mMembersNeedsSort = TRUE;
		mMemberIDs.erase(it);
		return true;
	}
	return false;
}
void LLGroupRoleData::clearMembers()
{
	mMembersNeedsSort = FALSE;
	mMemberIDs.clear();
}
LLGroupMgrGroupData::LLGroupMgrGroupData(const LLUUID& id) :
	mID(id),
	mShowInList(TRUE),
	mOpenEnrollment(FALSE),
	mMembershipFee(0),
	mAllowPublish(FALSE),
	mMaturePublish(FALSE),
	mChanged(FALSE),
	mMemberCount(0),
	mRoleCount(0),
	mReceivedRoleMemberPairs(0),
	mMemberDataComplete(false),
	mRoleDataComplete(false),
	mRoleMemberDataComplete(false),
	mGroupPropertiesDataComplete(false),
	mPendingRoleMemberRequest(false),
	mAccessTime(0.0f),
	mPendingBanRequest(false)
{
	mMemberVersion.generate();
}
void LLGroupMgrGroupData::setAccessed()
{
	mAccessTime = (F32)LLFrameTimer::getTotalSeconds();
}
BOOL LLGroupMgrGroupData::getRoleData(const LLUUID& role_id, LLRoleData& role_data)
{
	role_data_map_t::const_iterator it;
	it = mRoleChanges.find(role_id);
	if (it != mRoleChanges.end())
	{
		if ((*it).second.mChangeType == RC_DELETE) return FALSE;
		role_data = (*it).second;
		return TRUE;
	}
	role_list_t::const_iterator rit = mRoles.find(role_id);
	if (rit != mRoles.end())
	{
		role_data = (*rit).second->getRoleData();
		return TRUE;
	}
	return FALSE;
}
void LLGroupMgrGroupData::setRoleData(const LLUUID& role_id, LLRoleData role_data)
{
	role_data_map_t::iterator it;
	it = mRoleChanges.find(role_id);
	if (it != mRoleChanges.end())
	{
		if ((*it).second.mChangeType == RC_CREATE)
		{
			role_data.mChangeType = RC_CREATE;
			mRoleChanges[role_id] = role_data;
			return;
		}
		else if ((*it).second.mChangeType == RC_DELETE)
		{
			return;
		}
	}
	LLRoleData old_role_data;
	role_list_t::iterator rit = mRoles.find(role_id);
	if (rit != mRoles.end())
	{
		bool data_change = ( ((*rit).second->mRoleData.mRoleDescription != role_data.mRoleDescription)
							|| ((*rit).second->mRoleData.mRoleName != role_data.mRoleName)
							|| ((*rit).second->mRoleData.mRoleTitle != role_data.mRoleTitle) );
		bool powers_change = ((*rit).second->mRoleData.mRolePowers != role_data.mRolePowers);
		if (!data_change && !powers_change)
		{
			mRoleChanges.erase(role_id);
			return;
		}
		if (data_change && powers_change)
		{
			role_data.mChangeType = RC_UPDATE_ALL;
		}
		else if (data_change)
		{
			role_data.mChangeType = RC_UPDATE_DATA;
		}
		else
	{
			role_data.mChangeType = RC_UPDATE_POWERS;
		}
		mRoleChanges[role_id] = role_data;
	}
	else
		{
		LL_WARNS() << "Change being made to non-existant role " << role_id << LL_ENDL;
	}
}
BOOL LLGroupMgrGroupData::pendingRoleChanges()
{
	return (!mRoleChanges.empty());
}
void LLGroupMgrGroupData::createRole(const LLUUID& role_id, LLRoleData role_data)
{
	if (mRoleChanges.find(role_id) != mRoleChanges.end())
	{
		LL_WARNS() << "create role for existing role! " << role_id << LL_ENDL;
	}
	else
	{
		role_data.mChangeType = RC_CREATE;
		mRoleChanges[role_id] = role_data;
	}
}
void LLGroupMgrGroupData::deleteRole(const LLUUID& role_id)
{
	role_data_map_t::iterator it;
	it = mRoleChanges.find(role_id);
	if (it != mRoleChanges.end()
		&& (*it).second.mChangeType == RC_CREATE)
	{
		mRoleChanges.erase(it);
		return;
	}
	LLRoleData rd;
	rd.mChangeType = RC_DELETE;
	mRoleChanges[role_id] = rd;
}
void LLGroupMgrGroupData::addRolePower(const LLUUID &role_id, U64 power)
{
	LLRoleData rd;
	if (getRoleData(role_id,rd))
	{
		rd.mRolePowers |= power;
		setRoleData(role_id,rd);
	}
	else
	{
		LL_WARNS() << "addRolePower: no role data found for " << role_id << LL_ENDL;
	}
}
void LLGroupMgrGroupData::removeRolePower(const LLUUID &role_id, U64 power)
{
	LLRoleData rd;
	if (getRoleData(role_id,rd))
	{
		rd.mRolePowers &= ~power;
		setRoleData(role_id,rd);
	}
	else
	{
		LL_WARNS() << "removeRolePower: no role data found for " << role_id << LL_ENDL;
	}
}
U64 LLGroupMgrGroupData::getRolePowers(const LLUUID& role_id)
{
	LLRoleData rd;
	if (getRoleData(role_id,rd))
	{
		return rd.mRolePowers;
	}
	else
	{
		LL_WARNS() << "getRolePowers: no role data found for " << role_id << LL_ENDL;
		return GP_NO_POWERS;
	}
}
void LLGroupMgrGroupData::removeData()
{
	removeMemberData();
	removeRoleData();
}
void LLGroupMgrGroupData::removeMemberData()
{
	for (member_list_t::iterator mi = mMembers.begin(); mi != mMembers.end(); ++mi)
	{
		delete mi->second;
	}
	mMembers.clear();
	mMemberDataComplete = false;
	mMemberVersion.generate();
}
void LLGroupMgrGroupData::removeRoleData()
{
	for (member_list_t::iterator mi = mMembers.begin(); mi != mMembers.end(); ++mi)
	{
		LLGroupMemberData* data = mi->second;
		if (data)
		{
			data->clearRoles();
		}
	}
	for (role_list_t::iterator ri = mRoles.begin(); ri != mRoles.end(); ++ri)
	{
		LLGroupRoleData* data = ri->second;
		delete data;
	}
	mRoles.clear();
	mReceivedRoleMemberPairs = 0;
	mRoleDataComplete = false;
	mRoleMemberDataComplete = false;
}
void LLGroupMgrGroupData::removeRoleMemberData()
{
	for (member_list_t::iterator mi = mMembers.begin(); mi != mMembers.end(); ++mi)
	{
		LLGroupMemberData* data = mi->second;
		if (data)
		{
			data->clearRoles();
		}
	}
	for (role_list_t::iterator ri = mRoles.begin(); ri != mRoles.end(); ++ri)
	{
		LLGroupRoleData* data = ri->second;
		if (data)
		{
			data->clearMembers();
		}
	}
	mReceivedRoleMemberPairs = 0;
	mRoleMemberDataComplete = false;
}
LLGroupMgrGroupData::~LLGroupMgrGroupData()
{
	removeData();
}
bool LLGroupMgrGroupData::changeRoleMember(const LLUUID& role_id,
										   const LLUUID& member_id,
										   LLRoleMemberChangeType rmc)
{
	role_list_t::iterator ri = mRoles.find(role_id);
	member_list_t::iterator mi = mMembers.find(member_id);
	if (ri == mRoles.end()
		|| mi == mMembers.end() )
	{
		if (ri == mRoles.end()) LL_WARNS() << "LLGroupMgrGroupData::changeRoleMember couldn't find role " << role_id << LL_ENDL;
		if (mi == mMembers.end()) LL_WARNS() << "LLGroupMgrGroupData::changeRoleMember couldn't find member " << member_id << LL_ENDL;
		return false;
	}
	LLGroupRoleData* grd = ri->second;
	LLGroupMemberData* gmd = mi->second;
	if (!grd || !gmd)
	{
		LL_WARNS() << "LLGroupMgrGroupData::changeRoleMember couldn't get member or role data." << LL_ENDL;
		return false;
	}
	if (RMC_ADD == rmc)
	{
		LL_INFOS() << " adding member to role." << LL_ENDL;
		grd->addMember(member_id);
		gmd->addRole(role_id,grd);
		gmd->mIsOwner = (role_id == mOwnerRole) ? TRUE : gmd->mIsOwner;
	}
	else if (RMC_REMOVE == rmc)
	{
		LL_INFOS() << " removing member from role." << LL_ENDL;
		grd->removeMember(member_id);
		gmd->removeRole(role_id);
		gmd->mIsOwner = (role_id == mOwnerRole) ? FALSE : gmd->mIsOwner;
	}
	lluuid_pair role_member;
	role_member.first = role_id;
	role_member.second = member_id;
	change_map_t::iterator it = mRoleMemberChanges.find(role_member);
	if (it != mRoleMemberChanges.end())
	{
		if (it->second.mChange == rmc)
		{
			LL_INFOS() << "Received duplicate change for "
					<< " role: " << role_id << " member " << member_id
					<< " change " << (rmc == RMC_ADD ? "ADD" : "REMOVE") << LL_ENDL;
		}
		else
		{
			if (rmc == RMC_NONE)
			{
				LL_WARNS() << "changeRoleMember: existing entry with 'RMC_NONE' change! This shouldn't happen." << LL_ENDL;
				LLRoleMemberChange rc(role_id,member_id,rmc);
				mRoleMemberChanges[role_member] = rc;
			}
			else
			{
				mRoleMemberChanges.erase(it);
			}
		}
	}
	else
	{
		LLRoleMemberChange rc(role_id,member_id,rmc);
		mRoleMemberChanges[role_member] = rc;
	}
	recalcAgentPowers(member_id);
	mChanged = TRUE;
	return true;
}
void LLGroupMgrGroupData::recalcAllAgentPowers()
{
	LLGroupMemberData* gmd;
	for (member_list_t::iterator mit = mMembers.begin();
		 mit != mMembers.end(); ++mit)
	{
		gmd = mit->second;
		if (!gmd) continue;
		gmd->mAgentPowers = 0;
		for (LLGroupMemberData::role_list_t::iterator it = gmd->mRolesList.begin();
			 it != gmd->mRolesList.end(); ++it)
		{
			LLGroupRoleData* grd = (*it).second;
			if (!grd) continue;
			gmd->mAgentPowers |= grd->mRoleData.mRolePowers;
		}
	}
}
void LLGroupMgrGroupData::recalcAgentPowers(const LLUUID& agent_id)
{
	member_list_t::iterator mi = mMembers.find(agent_id);
	if (mi == mMembers.end()) return;
	LLGroupMemberData* gmd = mi->second;
	if (!gmd) return;
	gmd->mAgentPowers = 0;
	for (LLGroupMemberData::role_list_t::iterator it = gmd->mRolesList.begin();
		 it != gmd->mRolesList.end(); ++it)
	{
		LLGroupRoleData* grd = (*it).second;
		if (!grd) continue;
		gmd->mAgentPowers |= grd->mRoleData.mRolePowers;
	}
}
bool LLGroupMgrGroupData::isSingleMemberNotOwner()
{
	return mMembers.size() == 1 && !mMembers.begin()->second->isOwner();
}
bool packRoleUpdateMessageBlock(LLMessageSystem* msg,
								const LLUUID& group_id,
								const LLUUID& role_id,
								const LLRoleData& role_data,
								bool start_message)
{
	if (start_message)
	{
		msg->newMessage("GroupRoleUpdate");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID",gAgent.getID());
		msg->addUUID("SessionID",gAgent.getSessionID());
		msg->addUUID("GroupID",group_id);
		start_message = false;
	}
	msg->nextBlock("RoleData");
	msg->addUUID("RoleID",role_id);
	msg->addString("Name", role_data.mRoleName);
	msg->addString("Description", role_data.mRoleDescription);
	msg->addString("Title", role_data.mRoleTitle);
	msg->addU64("Powers", role_data.mRolePowers);
	msg->addU8("UpdateType", (U8)role_data.mChangeType);
	if (msg->isSendFullFast())
	{
		gAgent.sendReliableMessage();
		start_message = true;
	}
	return start_message;
}
#if SHY_MOD
#include "shcommandhandler.h"
class SH_GroupRoleChanger
{
	std::pair<LLUUID, std::string> gForceRole;
	typedef std::map<const bool,std::string> title_map_t;
	typedef std::map<std::string,title_map_t> role_map_t;
	typedef std::map<const LLUUID,role_map_t> group_map_t;
	group_map_t gPendingRoleChanges;
public:
	virtual ~SH_GroupRoleChanger(){}
	void CheckUpdateRole(const LLUUID &group_id,LLGroupMgrGroupData::role_list_t &roles)
	{
		group_map_t::iterator group_it = gPendingRoleChanges.find(group_id);
		if(group_it != gPendingRoleChanges.end())
		{
			for(LLGroupMgrGroupData::role_list_t::iterator roles_it = roles.begin();roles_it!=roles.end();++roles_it)
			{
				LLGroupRoleData &role = *(roles_it->second);
				LLRoleData data = role.getRoleData();
				role_map_t::iterator stored_role_it = group_it->second.find(data.mRoleName);
				if(stored_role_it != group_it->second.end())
				{
					if(!(stored_role_it->second.empty()))
					{
						title_map_t::iterator entry_it = stored_role_it->second.begin();
						for(;entry_it!=stored_role_it->second.end();++entry_it)
						{
							if(entry_it->first == false)
								data.mRoleDescription = entry_it->second;
							else
								data.mRoleTitle = entry_it->second;
						}
						if(gForceRole.first == group_id && gForceRole.second == data.mRoleName)
						{
							if(gAgent.getGroupID() != group_id)
							{
								LLMessageSystem* msg = gMessageSystem;
								msg->newMessageFast(_PREHASH_ActivateGroup);
								msg->nextBlockFast(_PREHASH_AgentData);
								msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
								msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
								msg->addUUIDFast(_PREHASH_GroupID, group_id);
								gAgent.sendReliableMessage();
							}
							LLGroupMgr::getInstance()->sendGroupTitleUpdate(group_id,role.getID());
						}
						LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(group_id);
						gdatap->setRoleData(role.getID(),data);
					}
				}
			}
			gPendingRoleChanges.erase(group_it);
		}
		LLGroupMgr::getInstance()->sendGroupRoleChanges(group_id);
		gForceRole.first = LLUUID::null;
	}
	void RequestUpdateRole(const LLUUID& group_id,	const std::string &rolename, bool title, const std::string &string, bool force = false)
	{
		if(force)
		{
			gForceRole.first	= group_id;
			gForceRole.second	= rolename;
		}
		gPendingRoleChanges[group_id][rolename][title]=string;
		LLGroupMgr::getInstance()->sendGroupRoleDataRequest(group_id);
	}
}gGroupRoleChanger;
CMD_SCRIPT(setroletext)
{
	const LLUUID group_id = args[1].asUUID();
	const std::string rolename = args[2].asString();
	const bool title = args[3].asInteger() == 1;
	const std::string str = args[4].asString();
	const bool force = args[5].asInteger() == 1;
	LL_INFOS() <<"Updating role- group_id:"<<group_id<<" rolename:"<<rolename<<" title:"<<title<<" string:"<<str<<LL_ENDL;
	gGroupRoleChanger.RequestUpdateRole(group_id,rolename,title,str,force);
}
#endif
void LLGroupMgrGroupData::sendRoleChanges()
{
	LLGroupRoleData* grd;
	role_list_t::iterator role_it;
	LLMessageSystem* msg = gMessageSystem;
	bool start_message = true;
	bool need_role_cleanup = false;
	bool need_role_data = false;
	bool need_power_recalc = false;
	for (role_data_map_t::iterator iter = mRoleChanges.begin();
		 iter != mRoleChanges.end(); )
	{
		role_data_map_t::iterator it = iter++;
		const LLUUID& role_id = (*it).first;
		const LLRoleData& role_data = (*it).second;
		role_it = mRoles.find((*it).first);
		if ( (mRoles.end() == role_it
				&& RC_CREATE != role_data.mChangeType)
			|| (mRoles.end() != role_it
				&& RC_CREATE == role_data.mChangeType))
		{
			continue;
		}
		switch (role_data.mChangeType)
		{
			case RC_CREATE:
			{
				grd = new LLGroupRoleData(role_id, role_data, 0);
				mRoles[role_id] = grd;
				need_role_data = true;
				break;
			}
			case RC_DELETE:
			{
				LLGroupRoleData* group_role_data = (*role_it).second;
				delete group_role_data;
				mRoles.erase(role_it);
				need_role_cleanup = true;
				need_power_recalc = true;
				break;
			}
			case RC_UPDATE_ALL:
			case RC_UPDATE_POWERS:
				need_power_recalc = true;
			case RC_UPDATE_DATA:
			default:
			{
				LLGroupRoleData* group_role_data = (*role_it).second;
				group_role_data->setRoleData(role_data);
				break;
			}
		}
		start_message = packRoleUpdateMessageBlock(msg,getID(),role_id,role_data,start_message);
	}
	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}
	if (need_role_cleanup)
	{
		removeRoleMemberData();
	}
	if (need_role_data)
	{
		LLGroupMgr::getInstance()->sendGroupRoleDataRequest(getID());
	}
	mRoleChanges.clear();
	if (need_power_recalc)
	{
		recalcAllAgentPowers();
	}
}
void LLGroupMgrGroupData::cancelRoleChanges()
{
	mRoleChanges.clear();
}
void LLGroupMgrGroupData::createBanEntry(const LLUUID& ban_id, const LLGroupBanData& ban_data)
{
	mBanList[ban_id] = ban_data;
}
void LLGroupMgrGroupData::removeBanEntry(const LLUUID& ban_id)
{
	mBanList.erase(ban_id);
}
void LLGroupMgrGroupData::banMemberById(const LLUUID& participant_uuid)
{
	if (!mMemberDataComplete ||
		!mRoleDataComplete ||
		!(mRoleMemberDataComplete && mMembers.size()))
	{
		LL_WARNS() << "No Role-Member data yet, setting ban request to pending." << LL_ENDL;
		mPendingBanRequest = true;
		mPendingBanMemberID = participant_uuid;
		if (!mMemberDataComplete)
		{
			LLGroupMgr::getInstance()->sendCapGroupMembersRequest(mID);
		}
		if (!mRoleDataComplete)
		{
			LLGroupMgr::getInstance()->sendGroupRoleDataRequest(mID);
		}
		return;
	}
	LLGroupMgrGroupData::member_list_t::iterator mi = mMembers.find((participant_uuid));
	if (mi == mMembers.end())
	{
		if (!mPendingBanRequest)
		{
			mPendingBanRequest = true;
			mPendingBanMemberID = participant_uuid;
			LLGroupMgr::getInstance()->sendCapGroupMembersRequest(mID);
		}
		else
		{
			mPendingBanRequest = false;
		}
		return;
	}
	mPendingBanRequest = false;
	LLGroupMemberData* member_data = (*mi).second;
	if (member_data && member_data->isInRole(mOwnerRole))
	{
		return;
	}
	uuid_vec_t ids;
	ids.push_back(participant_uuid);
	LLGroupBanData ban_data;
	createBanEntry(participant_uuid, ban_data);
	LLGroupMgr::getInstance()->sendGroupBanRequest(LLGroupMgr::REQUEST_POST, mID, LLGroupMgr::BAN_CREATE, ids);
	LLGroupMgr::getInstance()->sendGroupMemberEjects(mID, ids);
	LLGroupMgr::getInstance()->sendGroupMembersRequest(mID);
	LLSD args;
	std::string name;
	LLAvatarNameCache::getNSName(participant_uuid, name);
	args["AVATAR_NAME"] = name;
	args["GROUP_NAME"] = mName;
	LLNotifications::instance().add(LLNotification::Params("EjectAvatarFromGroup").substitutions(args));
}
LLGroupMgr::LLGroupMgr()
{
	mLastGroupMembersRequestFrame = 0;
}
LLGroupMgr::~LLGroupMgr()
{
	clearGroups();
}
void LLGroupMgr::clearGroups()
{
	std::for_each(mRoleActionSets.begin(), mRoleActionSets.end(), DeletePointer());
	mRoleActionSets.clear();
	std::for_each(mGroups.begin(), mGroups.end(), DeletePairedPointer());
	mGroups.clear();
	mObservers.clear();
}
void LLGroupMgr::clearGroupData(const LLUUID& group_id)
{
	group_map_t::iterator iter = mGroups.find(group_id);
	if (iter != mGroups.end())
	{
		delete (*iter).second;
		mGroups.erase(iter);
	}
}
void LLGroupMgr::addObserver(LLGroupMgrObserver* observer)
{
	if( observer->getID() != LLUUID::null )
		mObservers.insert(std::pair<LLUUID, LLGroupMgrObserver*>(observer->getID(), observer));
}
void LLGroupMgr::addObserver(const LLUUID& group_id, LLParticularGroupObserver* observer)
{
	if(group_id.notNull() && observer)
	{
		mParticularObservers[group_id].insert(observer);
	}
}
void LLGroupMgr::removeObserver(LLGroupMgrObserver* observer)
{
	if (!observer)
	{
		return;
	}
	observer_multimap_t::iterator it;
	it = mObservers.find(observer->getID());
	while (it != mObservers.end())
	{
		if (it->second == observer)
		{
			mObservers.erase(it);
			break;
		}
		else
		{
			++it;
		}
	}
}
void LLGroupMgr::removeObserver(const LLUUID& group_id, LLParticularGroupObserver* observer)
{
	if(group_id.isNull() || !observer)
	{
		return;
	}
    observer_map_t::iterator obs_it = mParticularObservers.find(group_id);
    if(obs_it == mParticularObservers.end())
        return;
    obs_it->second.erase(observer);
    if (obs_it->second.size() == 0)
    	mParticularObservers.erase(obs_it);
}
LLGroupMgrGroupData* LLGroupMgr::getGroupData(const LLUUID& id)
{
	group_map_t::iterator gi = mGroups.find(id);
	if (gi != mGroups.end())
	{
		return gi->second;
	}
	return NULL;
}
static void formatDateString(std::string &date_string)
{
	using namespace boost;
	cmatch result;
	const regex expression("([0-9]{1,2})/([0-9]{1,2})/([0-9]{4})");
	try
	{
	if (regex_match(date_string.c_str(), result, expression))
	{
		S32 year	= boost::lexical_cast<S32>(result[3]);
		S32 month	= boost::lexical_cast<S32>(result[1]);
		S32 day		= boost::lexical_cast<S32>(result[2]);
		date_string = llformat("%04d-%02d-%02dT00:00:00Z", year, month, day);
	}
	}
	catch(...)
	{
		LL_ERRS() << "Decode of non-compliant DateTime format from [GroupMembersReply.GroupData.MemberData.OnlineStatus]. Please notify grid operators of this defect." << LL_ENDL;
	}
}
const std::string& localized_online()
{
	static const std::string online(LLTrans::getString("group_member_status_online"));
	return online;
}
const std::string& localized_unknown()
{
	static const std::string unknown(LLTrans::getString("group_member_status_unknown"));
	return unknown;
}
void LLGroupMgr::processGroupMembersReply(LLMessageSystem* msg, void** data)
{
	LL_DEBUGS() << "LLGroupMgr::processGroupMembersReply" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		LL_WARNS() << "Got group members reply for another agent!" << LL_ENDL;
		return;
	}
	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id );
	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_RequestID, request_id);
	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap || (group_datap->mMemberRequestID != request_id))
	{
		LL_WARNS() << "processGroupMembersReply: Received incorrect (stale?) group or request id" << LL_ENDL;
		return;
	}
	msg->getS32(_PREHASH_GroupData, "MemberCount", group_datap->mMemberCount );
	if (group_datap->mMemberCount > 0)
	{
		S32 contribution = 0;
		std::string online_status;
		std::string title;
		U64 agent_powers = 0;
		BOOL is_owner = FALSE;
		S32 num_members = msg->getNumberOfBlocksFast(_PREHASH_MemberData);
		for (S32 i = 0; i < num_members; i++)
		{
			LLUUID member_id;
			msg->getUUIDFast(_PREHASH_MemberData, _PREHASH_AgentID, member_id, i );
			msg->getS32(_PREHASH_MemberData, _PREHASH_Contribution, contribution, i);
			msg->getU64(_PREHASH_MemberData, "AgentPowers", agent_powers, i);
			msg->getStringFast(_PREHASH_MemberData, _PREHASH_OnlineStatus, online_status, i);
			msg->getString(_PREHASH_MemberData, "Title", title, i);
			msg->getBOOL(_PREHASH_MemberData,"IsOwner",is_owner,i);
			if (member_id.notNull())
			{
				if (online_status == "Online")
				{
					online_status = localized_online();
				}
				else if (online_status == "unknown")
				{
					online_status = localized_unknown();
				}
				else
				{
					formatDateString(online_status);
				}
				LLGroupMemberData* newdata = new LLGroupMemberData(member_id,
																	contribution,
																	agent_powers,
																	title,
																	online_status,
																	is_owner);
#if LL_DEBUG
				LLGroupMgrGroupData::member_list_t::iterator mit = group_datap->mMembers.find(member_id);
				if (mit != group_datap->mMembers.end())
				{
					LL_INFOS() << " *** Received duplicate member data for agent " << member_id << LL_ENDL;
				}
#endif
				group_datap->mMembers[member_id] = newdata;
			}
			else
			{
				LL_INFOS() << "Received null group member data." << LL_ENDL;
			}
		}
		if(group_datap->mTitles.size() < 1)
		{
			LLGroupMgr::getInstance()->sendGroupTitlesRequest(group_id);
		}
	}
	group_datap->mMemberVersion.generate();
	if (group_datap->mMembers.size() ==  (U32)group_datap->mMemberCount)
	{
		group_datap->mMemberDataComplete = true;
		group_datap->mMemberRequestID.setNull();
		if (group_datap->mPendingRoleMemberRequest)
		{
			group_datap->mPendingRoleMemberRequest = false;
			LLGroupMgr::getInstance()->sendGroupRoleMembersRequest(group_datap->mID);
		}
	}
	group_datap->mChanged = TRUE;
	LLGroupMgr::getInstance()->notifyObservers(GC_MEMBER_DATA);
}
void LLGroupMgr::processGroupPropertiesReply(LLMessageSystem* msg, void** data)
{
	LL_DEBUGS() << "LLGroupMgr::processGroupPropertiesReply" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		LL_WARNS() << "Got group properties reply for another agent!" << LL_ENDL;
		return;
	}
	LLUUID group_id;
	std::string	name;
	std::string	charter;
	BOOL	show_in_list = FALSE;
	LLUUID	founder_id;
	U64		powers_mask = GP_NO_POWERS;
	S32		money = 0;
	std::string	member_title;
	LLUUID	insignia_id;
	LLUUID	owner_role;
	U32		membership_fee = 0;
	BOOL	open_enrollment = FALSE;
	S32		num_group_members = 0;
	S32		num_group_roles = 0;
	BOOL	allow_publish = FALSE;
	BOOL	mature = FALSE;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id );
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_FounderID, founder_id);
	msg->getStringFast(_PREHASH_GroupData, _PREHASH_Name, name );
	msg->getStringFast(_PREHASH_GroupData, _PREHASH_Charter, charter );
	msg->getBOOLFast(_PREHASH_GroupData, _PREHASH_ShowInList, show_in_list );
	msg->getStringFast(_PREHASH_GroupData, _PREHASH_MemberTitle, member_title );
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_InsigniaID, insignia_id );
	msg->getU64Fast(_PREHASH_GroupData, _PREHASH_PowersMask, powers_mask );
	msg->getU32Fast(_PREHASH_GroupData, _PREHASH_MembershipFee, membership_fee );
	msg->getBOOLFast(_PREHASH_GroupData, _PREHASH_OpenEnrollment, open_enrollment );
	msg->getS32Fast(_PREHASH_GroupData, _PREHASH_GroupMembershipCount, num_group_members);
	msg->getS32(_PREHASH_GroupData, "GroupRolesCount", num_group_roles);
	msg->getS32Fast(_PREHASH_GroupData, _PREHASH_Money, money);
	msg->getBOOL("GroupData", "AllowPublish", allow_publish);
	msg->getBOOL("GroupData", "MaturePublish", mature);
	msg->getUUID(_PREHASH_GroupData, "OwnerRole", owner_role);
	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->createGroupData(group_id);
	group_datap->mName = name;
	group_datap->mCharter = charter;
	group_datap->mShowInList = show_in_list;
	group_datap->mInsigniaID = insignia_id;
	group_datap->mFounderID = founder_id;
	group_datap->mMembershipFee = membership_fee;
	group_datap->mOpenEnrollment = open_enrollment;
	group_datap->mAllowPublish = allow_publish;
	group_datap->mMaturePublish = mature;
	group_datap->mOwnerRole = owner_role;
	group_datap->mMemberCount = num_group_members;
	group_datap->mRoleCount = num_group_roles + 1;
	group_datap->mGroupPropertiesDataComplete = true;
	group_datap->mChanged = TRUE;
	LLGroupMgr::getInstance()->notifyObservers(GC_PROPERTIES);
}
void LLGroupMgr::processGroupRoleDataReply(LLMessageSystem* msg, void** data)
{
	LL_DEBUGS() << "LLGroupMgr::processGroupRoleDataReply" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		LL_WARNS() << "Got group role data reply for another agent!" << LL_ENDL;
		return;
	}
	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id );
	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_RequestID, request_id);
	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap || (group_datap->mRoleDataRequestID != request_id))
	{
		LL_WARNS() << "processGroupPropertiesReply: Received incorrect (stale?) group or request id" << LL_ENDL;
		return;
	}
	msg->getS32(_PREHASH_GroupData, "RoleCount", group_datap->mRoleCount );
	std::string	name;
	std::string	title;
	std::string	desc;
	U64		powers = 0;
	U32		member_count = 0;
	LLUUID role_id;
	U32 num_blocks = msg->getNumberOfBlocks("RoleData");
	U32 i = 0;
	for (i=0; i< num_blocks; ++i)
	{
		msg->getUUID("RoleData", "RoleID", role_id, i );
		msg->getString("RoleData","Name",name,i);
		msg->getString("RoleData","Title",title,i);
		msg->getString("RoleData","Description",desc,i);
		msg->getU64("RoleData","Powers",powers,i);
		msg->getU32("RoleData","Members",member_count,i);
		if(name == "Everyone")
		{
			name = LLTrans::getString("group_role_everyone");
		}
		else if(name == "Officers")
		{
			name = LLTrans::getString("group_role_officers");
		}
		else if(name == "Owners")
		{
			name = LLTrans::getString("group_role_owners");
		}
		LL_DEBUGS() << "Adding role data: " << name << " {" << role_id << "}" << LL_ENDL;
		LLGroupRoleData* rd = new LLGroupRoleData(role_id,name,title,desc,powers,member_count);
		group_datap->mRoles[role_id] = rd;
	}
	if (group_datap->mRoles.size() == (U32)group_datap->mRoleCount)
	{
		group_datap->mRoleDataComplete = true;
		group_datap->mRoleDataRequestID.setNull();
		if (group_datap->mPendingRoleMemberRequest)
		{
			group_datap->mPendingRoleMemberRequest = false;
			LLGroupMgr::getInstance()->sendGroupRoleMembersRequest(group_datap->mID);
		}
	}
	group_datap->mChanged = TRUE;
#if SHY_MOD
	gGroupRoleChanger.CheckUpdateRole(group_id,group_datap->mRoles);
#endif
	LLGroupMgr::getInstance()->notifyObservers(GC_ROLE_DATA);
}
void LLGroupMgr::processGroupRoleMembersReply(LLMessageSystem* msg, void** data)
{
	LL_DEBUGS() << "LLGroupMgr::processGroupRoleMembersReply" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		LL_WARNS() << "Got group role members reply for another agent!" << LL_ENDL;
		return;
	}
	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_RequestID, request_id);
	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_GroupID, group_id );
	U32 total_pairs;
	msg->getU32(_PREHASH_AgentData, "TotalPairs", total_pairs);
	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap || (group_datap->mRoleMembersRequestID != request_id))
	{
		LL_WARNS() << "processGroupRoleMembersReply: Received incorrect (stale?) group or request id" << LL_ENDL;
		return;
	}
	U32 num_blocks = msg->getNumberOfBlocks("MemberData");
	U32 i;
	LLUUID member_id;
	LLUUID role_id;
	LLGroupRoleData* rd = NULL;
	LLGroupMemberData* md = NULL;
	LLGroupMgrGroupData::role_list_t::iterator ri;
	LLGroupMgrGroupData::member_list_t::iterator mi;
	if (total_pairs > 0)
	{
		for (i = 0;i < num_blocks; ++i)
		{
			msg->getUUID("MemberData","RoleID",role_id,i);
			msg->getUUID("MemberData","MemberID",member_id,i);
			if (role_id.notNull() && member_id.notNull() )
			{
				rd = NULL;
				ri = group_datap->mRoles.find(role_id);
				if (ri != group_datap->mRoles.end())
				{
					rd = ri->second;
				}
				md = NULL;
				mi = group_datap->mMembers.find(member_id);
				if (mi != group_datap->mMembers.end())
				{
					md = mi->second;
				}
				if (rd && md)
				{
					LL_DEBUGS() << "Adding role-member pair: " << role_id << ", " << member_id << LL_ENDL;
					rd->addMember(member_id);
					md->addRole(role_id,rd);
				}
				else
				{
					if (!rd) LL_WARNS() << "Received role data for unknown role " << role_id << " in group " << group_id << LL_ENDL;
					if (!md) LL_WARNS() << "Received role data for unknown member " << member_id << " in group " << group_id << LL_ENDL;
				}
			}
		}
		group_datap->mReceivedRoleMemberPairs += num_blocks;
	}
	if (group_datap->mReceivedRoleMemberPairs == total_pairs)
	{
		LLGroupRoleData* everyone = group_datap->mRoles[LLUUID::null];
		if (!everyone)
		{
			LL_WARNS() << "Everyone role not found!" << LL_ENDL;
		}
		else
		{
			for (LLGroupMgrGroupData::member_list_t::iterator mi = group_datap->mMembers.begin();
				 mi != group_datap->mMembers.end(); ++mi)
			{
				LLGroupMemberData* data = mi->second;
				if (data)
				{
					data->addRole(LLUUID::null,everyone);
				}
			}
		}
		group_datap->mRoleMemberDataComplete = true;
		group_datap->mRoleMembersRequestID.setNull();
	}
	group_datap->mChanged = TRUE;
	LLGroupMgr::getInstance()->notifyObservers(GC_ROLE_MEMBER_DATA);
	if (group_datap->mPendingBanRequest)
	{
		group_datap->banMemberById(group_datap->mPendingBanMemberID);
	}
}
void LLGroupMgr::processGroupTitlesReply(LLMessageSystem* msg, void** data)
{
	LL_DEBUGS() << "LLGroupMgr::processGroupTitlesReply" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		LL_WARNS() << "Got group properties reply for another agent!" << LL_ENDL;
		return;
	}
	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_GroupID, group_id );
	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_RequestID, request_id);
	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap || (group_datap->mTitlesRequestID != request_id))
	{
		LL_WARNS() << "processGroupTitlesReply: Received incorrect (stale?) group" << LL_ENDL;
		return;
	}
	LLGroupTitle title;
	S32 i = 0;
	S32 blocks = msg->getNumberOfBlocksFast(_PREHASH_GroupData);
	for (i=0; i<blocks; ++i)
	{
		msg->getString("GroupData","Title",title.mTitle,i);
		msg->getUUID("GroupData","RoleID",title.mRoleID,i);
		msg->getBOOL("GroupData","Selected",title.mSelected,i);
		if (!title.mTitle.empty())
		{
			LL_DEBUGS() << "LLGroupMgr adding title: " << title.mTitle << ", " << title.mRoleID << ", " << (title.mSelected ? 'Y' : 'N') << LL_ENDL;
			group_datap->mTitles.push_back(title);
		}
	}
	group_datap->mChanged = TRUE;
	LLGroupMgr::getInstance()->notifyObservers(GC_TITLES);
}
void LLGroupMgr::processEjectGroupMemberReply(LLMessageSystem* msg, void ** data)
{
	LL_DEBUGS() << "processEjectGroupMemberReply" << LL_ENDL;
	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id);
	BOOL success;
	msg->getBOOLFast(_PREHASH_EjectData, _PREHASH_Success, success);
	if (!success)
	{
		LLGroupActions::refresh(group_id);
	}
}
void LLGroupMgr::processJoinGroupReply(LLMessageSystem* msg, void ** data)
{
	LL_DEBUGS() << "processJoinGroupReply" << LL_ENDL;
	LLUUID group_id;
	BOOL success;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id);
	msg->getBOOLFast(_PREHASH_GroupData, _PREHASH_Success, success);
	if (success)
	{
		gAgent.sendAgentDataUpdateRequest();
		LLGroupMgr::getInstance()->clearGroupData(group_id);
		LLGroupActions::refresh(group_id);
		LLFloaterDirectory::refreshGroup(group_id);
	}
}
void LLGroupMgr::processLeaveGroupReply(LLMessageSystem* msg, void ** data)
{
	LL_DEBUGS() << "processLeaveGroupReply" << LL_ENDL;
	LLUUID group_id;
	BOOL success;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id);
	msg->getBOOLFast(_PREHASH_GroupData, _PREHASH_Success, success);
	if (success)
	{
		gAgent.sendAgentDataUpdateRequest();
		LLGroupMgr::getInstance()->clearGroupData(group_id);
		LLGroupActions::closeGroup(group_id);
		LLFloaterDirectory::refreshGroup(group_id);
	}
}
void LLGroupMgr::processCreateGroupReply(LLMessageSystem* msg, void ** data)
{
	LLUUID group_id;
	BOOL success;
	std::string message;
	msg->getUUIDFast(_PREHASH_ReplyData, _PREHASH_GroupID, group_id );
	msg->getBOOLFast(_PREHASH_ReplyData, _PREHASH_Success,	success );
	msg->getStringFast(_PREHASH_ReplyData, _PREHASH_Message, message );
	if (success)
	{
		gAgent.sendAgentDataUpdateRequest();
		LLGroupData gd;
		gd.mAcceptNotices = TRUE;
		gd.mListInProfile = TRUE;
		gd.mContribution = 0;
		gd.mID = group_id;
		gd.mName = "new group";
		gd.mPowers = GP_ALL_POWERS;
		gAgent.mGroups.push_back(gd);
		if (LLFloaterGroupInfo* flooter = LLFloaterGroupInfo::getInstance(LLUUID::null))
			flooter->onClose(false);
		LLGroupActions::showTab(group_id, "roles_tab");
	}
	else
	{
		LLSD args;
		args["MESSAGE"] = message;
		LLNotificationsUtil::add("UnableToCreateGroup", args);
	}
}
LLGroupMgrGroupData* LLGroupMgr::createGroupData(const LLUUID& id)
{
	LLGroupMgrGroupData* group_datap = NULL;
	group_map_t::iterator existing_group = LLGroupMgr::getInstance()->mGroups.find(id);
	if (existing_group == LLGroupMgr::getInstance()->mGroups.end())
	{
		group_datap = new LLGroupMgrGroupData(id);
		LLGroupMgr::getInstance()->addGroup(group_datap);
	}
	else
	{
		group_datap = existing_group->second;
	}
	if (group_datap)
	{
		group_datap->setAccessed();
	}
	return group_datap;
}
void LLGroupMgr::notifyObservers(LLGroupChange gc)
{
	for (group_map_t::iterator gi = mGroups.begin(); gi != mGroups.end(); ++gi)
	{
		LLUUID group_id = gi->first;
		if (gi->second->mChanged)
		{
			observer_multimap_t observers = mObservers;
			observer_multimap_t::iterator oi = observers.lower_bound(group_id);
			observer_multimap_t::iterator end = observers.upper_bound(group_id);
			for (; oi != end; ++oi)
			{
				oi->second->changed(gc);
			}
			gi->second->mChanged = FALSE;
		    observer_map_t::iterator obs_it = mParticularObservers.find(group_id);
		    if(obs_it == mParticularObservers.end())
		        return;
		    observer_set_t& obs = obs_it->second;
		    for (observer_set_t::iterator ob_it = obs.begin(); ob_it != obs.end(); ++ob_it)
		    {
		        (*ob_it)->changed(group_id, gc);
		    }
		}
	}
}
void LLGroupMgr::addGroup(LLGroupMgrGroupData* group_datap)
{
	while (mGroups.size() >= MAX_CACHED_GROUPS)
	{
		F32 oldest_access = LLFrameTimer::getTotalSeconds();
		group_map_t::iterator oldest_gi = mGroups.end();
		for (group_map_t::iterator gi = mGroups.begin(); gi != mGroups.end(); ++gi )
		{
			observer_multimap_t::iterator oi = mObservers.find(gi->first);
			if (oi == mObservers.end())
			{
				if (gi->second
						&& (gi->second->getAccessTime() < oldest_access))
				{
					oldest_access = gi->second->getAccessTime();
					oldest_gi = gi;
				}
			}
		}
		if (oldest_gi != mGroups.end())
		{
			delete oldest_gi->second;
			mGroups.erase(oldest_gi);
		}
		else
		{
			break;
		}
	}
	mGroups[group_datap->getID()] = group_datap;
}
void LLGroupMgr::sendGroupPropertiesRequest(const LLUUID& group_id)
{
	LL_DEBUGS() << "LLGroupMgr::sendGroupPropertiesRequest" << LL_ENDL;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GroupProfileRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID",gAgent.getID());
	msg->addUUID("SessionID",gAgent.getSessionID());
	msg->nextBlock("GroupData");
	msg->addUUID("GroupID",group_id);
	gAgent.sendReliableMessage();
}
void LLGroupMgr::sendGroupMembersRequest(const LLUUID& group_id)
{
	LL_DEBUGS() << "LLGroupMgr::sendGroupMembersRequest" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	if (group_datap->mMemberRequestID.isNull())
	{
		group_datap->removeMemberData();
		group_datap->mMemberRequestID.generate();
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("GroupMembersRequest");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID",gAgent.getID());
		msg->addUUID("SessionID",gAgent.getSessionID());
		msg->nextBlock("GroupData");
		msg->addUUID("GroupID",group_id);
		msg->addUUID("RequestID",group_datap->mMemberRequestID);
		gAgent.sendReliableMessage();
	}
}
void LLGroupMgr::sendGroupRoleDataRequest(const LLUUID& group_id)
{
	LL_DEBUGS() << "LLGroupMgr::sendGroupRoleDataRequest" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	if (group_datap->mRoleDataRequestID.isNull())
	{
		group_datap->removeRoleData();
		group_datap->mRoleDataRequestID.generate();
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("GroupRoleDataRequest");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID",gAgent.getID());
		msg->addUUID("SessionID",gAgent.getSessionID());
		msg->nextBlock("GroupData");
		msg->addUUID("GroupID",group_id);
		msg->addUUID("RequestID",group_datap->mRoleDataRequestID);
		gAgent.sendReliableMessage();
	}
}
void LLGroupMgr::sendGroupRoleMembersRequest(const LLUUID& group_id)
{
	LL_DEBUGS() << "LLGroupMgr::sendGroupRoleMembersRequest" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	if (group_datap->mRoleMembersRequestID.isNull())
	{
		if (!group_datap->isMemberDataComplete()
			|| !group_datap->isRoleDataComplete())
		{
			LL_INFOS() << " Pending: " << (group_datap->mPendingRoleMemberRequest ? "Y" : "N")
				<< " MemberDataComplete: " << (group_datap->mMemberDataComplete ? "Y" : "N")
				<< " RoleDataComplete: " << (group_datap->mRoleDataComplete ? "Y" : "N") << LL_ENDL;
			group_datap->mPendingRoleMemberRequest = TRUE;
			return;
		}
		group_datap->removeRoleMemberData();
		group_datap->mRoleMembersRequestID.generate();
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("GroupRoleMembersRequest");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID",gAgent.getID());
		msg->addUUID("SessionID",gAgent.getSessionID());
		msg->nextBlock("GroupData");
		msg->addUUID("GroupID",group_id);
		msg->addUUID("RequestID",group_datap->mRoleMembersRequestID);
		gAgent.sendReliableMessage();
	}
}
void LLGroupMgr::sendGroupTitlesRequest(const LLUUID& group_id)
{
	LL_DEBUGS() << "LLGroupMgr::sendGroupTitlesRequest" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	group_datap->mTitles.clear();
	group_datap->mTitlesRequestID.generate();
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GroupTitlesRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID",gAgent.getID());
	msg->addUUID("SessionID",gAgent.getSessionID());
	msg->addUUID("GroupID",group_id);
	msg->addUUID("RequestID",group_datap->mTitlesRequestID);
	gAgent.sendReliableMessage();
}
void LLGroupMgr::sendGroupTitleUpdate(const LLUUID& group_id, const LLUUID& title_role_id)
{
	LL_DEBUGS() << "LLGroupMgr::sendGroupTitleUpdate" << LL_ENDL;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GroupTitleUpdate");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID",gAgent.getID());
	msg->addUUID("SessionID",gAgent.getSessionID());
	msg->addUUID("GroupID",group_id);
	msg->addUUID("TitleRoleID",title_role_id);
	gAgent.sendReliableMessage();
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	for (std::vector<LLGroupTitle>::iterator iter = group_datap->mTitles.begin();
		 iter != group_datap->mTitles.end(); ++iter)
	{
		if (iter->mRoleID == title_role_id)
		{
			iter->mSelected = TRUE;
		}
		else if (iter->mSelected)
		{
			iter->mSelected = FALSE;
		}
	}
}
void LLGroupMgr::sendCreateGroupRequest(const std::string& name,
										const std::string& charter,
										U8 show_in_list,
										const LLUUID& insignia,
										S32 membership_fee,
										BOOL open_enrollment,
										BOOL allow_publish,
										BOOL mature_publish)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("CreateGroupRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID",gAgent.getID());
	msg->addUUID("SessionID",gAgent.getSessionID());
	msg->nextBlock("GroupData");
	msg->addString("Name",name);
	msg->addString("Charter",charter);
	msg->addBOOL("ShowInList",show_in_list);
	msg->addUUID("InsigniaID",insignia);
	msg->addS32("MembershipFee",membership_fee);
	msg->addBOOL("OpenEnrollment",open_enrollment);
	msg->addBOOL("AllowPublish",allow_publish);
	msg->addBOOL("MaturePublish",mature_publish);
	gAgent.sendReliableMessage();
}
void LLGroupMgr::sendUpdateGroupInfo(const LLUUID& group_id)
{
	LL_DEBUGS() << "LLGroupMgr::sendUpdateGroupInfo" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateGroupInfo);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_GroupData);
	msg->addUUIDFast(_PREHASH_GroupID,group_datap->getID());
	msg->addStringFast(_PREHASH_Charter,group_datap->mCharter);
	msg->addBOOLFast(_PREHASH_ShowInList,group_datap->mShowInList);
	msg->addUUIDFast(_PREHASH_InsigniaID,group_datap->mInsigniaID);
	msg->addS32Fast(_PREHASH_MembershipFee,group_datap->mMembershipFee);
	msg->addBOOLFast(_PREHASH_OpenEnrollment,group_datap->mOpenEnrollment);
	msg->addBOOLFast(_PREHASH_AllowPublish,group_datap->mAllowPublish);
	msg->addBOOLFast(_PREHASH_MaturePublish,group_datap->mMaturePublish);
	gAgent.sendReliableMessage();
	group_datap->mChanged = TRUE;
	notifyObservers(GC_PROPERTIES);
}
void LLGroupMgr::sendGroupRoleMemberChanges(const LLUUID& group_id)
{
	LL_DEBUGS() << "LLGroupMgr::sendGroupRoleMemberChanges" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	if (group_datap->mRoleMemberChanges.empty()) return;
	LLMessageSystem* msg = gMessageSystem;
	bool start_message = true;
	for (LLGroupMgrGroupData::change_map_t::const_iterator citer = group_datap->mRoleMemberChanges.begin();
		 citer != group_datap->mRoleMemberChanges.end(); ++citer)
	{
		if (start_message)
		{
			msg->newMessage("GroupRoleChanges");
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID,gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
			msg->addUUIDFast(_PREHASH_GroupID,group_id);
			start_message = false;
		}
		msg->nextBlock("RoleChange");
		msg->addUUID("RoleID",citer->second.mRole);
		msg->addUUID("MemberID",citer->second.mMember);
		msg->addU32("Change",(U32)citer->second.mChange);
		if (msg->isSendFullFast())
		{
			gAgent.sendReliableMessage();
			start_message = true;
		}
	}
	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}
	group_datap->mRoleMemberChanges.clear();
	group_datap->mChanged = TRUE;
	notifyObservers(GC_ROLE_MEMBER_DATA);
}
void LLGroupMgr::sendGroupMemberJoin(const LLUUID& group_id)
{
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_JoinGroupRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_GroupData);
	msg->addUUIDFast(_PREHASH_GroupID, group_id);
	gAgent.sendReliableMessage();
}
void LLGroupMgr::sendGroupMemberInvites(const LLUUID& group_id, std::map<LLUUID,LLUUID>& member_role_pairs)
{
	bool start_message = true;
	LLMessageSystem* msg = gMessageSystem;
	for (std::map<LLUUID,LLUUID>::iterator it = member_role_pairs.begin();
		 it != member_role_pairs.end(); ++it)
	{
		if (start_message)
		{
			msg->newMessage("InviteGroupRequest");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID",gAgent.getID());
			msg->addUUID("SessionID",gAgent.getSessionID());
			msg->nextBlock("GroupData");
			msg->addUUID("GroupID",group_id);
			start_message = false;
		}
		msg->nextBlock("InviteData");
		msg->addUUID("InviteeID",(*it).first);
		msg->addUUID("RoleID",(*it).second);
		if (msg->isSendFull())
		{
			gAgent.sendReliableMessage();
			start_message = true;
		}
	}
	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}
}
void LLGroupMgr::sendGroupMemberEjects(const LLUUID& group_id,
									   uuid_vec_t& member_ids)
{
	bool start_message = true;
	LLMessageSystem* msg = gMessageSystem;
	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap) return;
	for (uuid_vec_t::iterator it = member_ids.begin();
		 it != member_ids.end(); ++it)
	{
		LLUUID& ejected_member_id = (*it);
		if (ejected_member_id == gAgent.getID()) continue;
		LLGroupMgrGroupData::member_list_t::iterator mit = group_datap->mMembers.find(ejected_member_id);
		if (mit != group_datap->mMembers.end())
		{
			if (start_message)
			{
				msg->newMessage("EjectGroupMemberRequest");
				msg->nextBlock("AgentData");
				msg->addUUID("AgentID",gAgent.getID());
				msg->addUUID("SessionID",gAgent.getSessionID());
				msg->nextBlock("GroupData");
				msg->addUUID("GroupID",group_id);
				start_message = false;
			}
			msg->nextBlock("EjectData");
			msg->addUUID("EjecteeID",ejected_member_id);
			if (msg->isSendFull())
			{
				gAgent.sendReliableMessage();
				start_message = true;
			}
			LLGroupMemberData* member_data = (*mit).second;
			for (LLGroupMemberData::role_list_t::iterator rit = member_data->roleBegin();
				 rit != member_data->roleEnd(); ++rit)
			{
				if ((*rit).first.notNull() && (*rit).second!=0)
				{
					(*rit).second->removeMember(ejected_member_id);
				}
			}
			group_datap->mMembers.erase(ejected_member_id);
			delete member_data;
		}
	}
	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}
	group_datap->mMemberVersion.generate();
}
class GroupBanDataResponder : public LLHTTPClient::ResponderWithResult
{
public:
	GroupBanDataResponder(const LLUUID& gropup_id, BOOL force_refresh=false);
	virtual ~GroupBanDataResponder() {}
	virtual void httpSuccess();
	virtual void httpFailure();
	virtual char const* getName() const { return "GroupBanDataResponder"; }
private:
	LLUUID mGroupID;
	BOOL mForceRefresh;
};
GroupBanDataResponder::GroupBanDataResponder(const LLUUID& gropup_id, BOOL force_refresh) :
	mGroupID(gropup_id),
	mForceRefresh(force_refresh)
{}
void GroupBanDataResponder::httpFailure()
{
	LL_WARNS("GrpMgr") << "Error receiving group member data [status:"
		<< mStatus << "]: " << mContent << LL_ENDL;
}
void GroupBanDataResponder::httpSuccess()
{
	if (mContent.has("ban_list"))
	{
		LLGroupMgr::processGroupBanRequest(mContent);
	}
	else if (mForceRefresh)
	{
		LLGroupMgr::getInstance()->sendGroupBanRequest(LLGroupMgr::REQUEST_GET, mGroupID);
	}
}
void LLGroupMgr::sendGroupBanRequest(	EBanRequestType request_type,
										const LLUUID& group_id,
										U32 ban_action,
										const uuid_vec_t& ban_list)
{
	LLViewerRegion* currentRegion = gAgent.getRegion();
	if (!currentRegion)
	{
		LL_WARNS("GrpMgr") << "Agent does not have a current region. Uh-oh!" << LL_ENDL;
		return;
	}
	if (!currentRegion->capabilitiesReceived())
	{
		LL_WARNS("GrpMgr") << " Capabilities not received!" << LL_ENDL;
		return;
	}
	std::string cap_url = currentRegion->getCapability("GroupAPIv1");
	if (cap_url.empty())
	{
		return;
	}
	cap_url += "?group_id=" + group_id.asString();
	LLSD body = LLSD::emptyMap();
	body["ban_action"] = (LLSD::Integer)(ban_action & ~BAN_UPDATE);
	body["ban_ids"]	= LLSD::emptyArray();
	LLSD ban_entry;
	uuid_vec_t::const_iterator iter = ban_list.begin();
	for(;iter != ban_list.end(); ++iter)
	{
		ban_entry = (*iter);
		body["ban_ids"].append(ban_entry);
	}
	LLHTTPClient::ResponderPtr grp_ban_responder = new GroupBanDataResponder(group_id, ban_action & BAN_UPDATE);
	switch(request_type)
	{
	case REQUEST_GET:
		LLHTTPClient::get(cap_url, grp_ban_responder);
		break;
	case REQUEST_POST:
		LLHTTPClient::post(cap_url, body, grp_ban_responder);
		break;
	case REQUEST_PUT:
	case REQUEST_DEL:
		break;
	}
}
void LLGroupMgr::processGroupBanRequest(const LLSD& content)
{
	if (!content.size())
	{
		LL_WARNS("GrpMgr") << "No group member data received." << LL_ENDL;
		return;
	}
	LLUUID group_id = content["group_id"].asUUID();
	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!gdatap)
		return;
	gdatap->clearBanList();
	LLSD::map_const_iterator i		= content["ban_list"].beginMap();
	LLSD::map_const_iterator iEnd	= content["ban_list"].endMap();
	for(;i != iEnd; ++i)
	{
		const LLUUID ban_id(i->first);
		LLSD ban_entry(i->second);
		LLGroupBanData ban_data;
		if (ban_entry.has("ban_date"))
		{
			ban_data.mBanDate = ban_entry["ban_date"].asDate();
		}
		gdatap->createBanEntry(ban_id, ban_data);
	}
	gdatap->mChanged = TRUE;
	LLGroupMgr::getInstance()->notifyObservers(GC_BANLIST);
}
class GroupMemberDataResponder : public LLHTTPClient::ResponderWithResult
{
	LOG_CLASS(GroupMemberDataResponder);
public:
	GroupMemberDataResponder() {}
	virtual ~GroupMemberDataResponder() {}
private:
	void httpSuccess();
	void httpFailure();
	char const* getName() const { return "GroupMemberDataResponder"; }
	LLSD mMemberData;
};
void GroupMemberDataResponder::httpFailure()
{
	LL_WARNS("GrpMgr") << "Error receiving group member data "
		<< dumpResponse() << LL_ENDL;
}
void GroupMemberDataResponder::httpSuccess()
{
	const LLSD& content = getContent();
	if (!content.isMap())
	{
		failureResult(HTTP_INTERNAL_ERROR_OTHER, "Malformed response contents", content);
		return;
	}
	LLGroupMgr::processCapGroupMembersRequest(content);
}
void LLGroupMgr::sendCapGroupMembersRequest(const LLUUID& group_id)
{
	if (mLastGroupMembersRequestFrame == gFrameCount)
		return;
	LLViewerRegion* currentRegion = gAgent.getRegion();
	if (!currentRegion)
	{
		LL_WARNS("GrpMgr") << "Agent does not have a current region. Uh-oh!" << LL_ENDL;
		return;
	}
	if (!currentRegion->capabilitiesReceived())
	{
		LL_WARNS("GrpMgr") << " Capabilities not received!" << LL_ENDL;
		return;
	}
	std::string cap_url =  currentRegion->getCapability("GroupMemberData");
	if (cap_url.empty())
	{
		LL_INFOS("GrpMgr") << "Region has no GroupMemberData capability.  Falling back to UDP fetch." << LL_ENDL;
		sendGroupMembersRequest(group_id);
		return;
	}
	LLSD body = LLSD::emptyMap();
	body["group_id"] = group_id;
	LLHTTPClient::ResponderPtr grp_data_responder = new GroupMemberDataResponder();
	LLHTTPClient::post(cap_url, body, grp_data_responder);
	mLastGroupMembersRequestFrame = gFrameCount;
}
void LLGroupMgr::processCapGroupMembersRequest(const LLSD& content)
{
	if (!content.size())
	{
		LL_DEBUGS("GrpMgr") << "No group member data received." << LL_ENDL;
		return;
	}
	LLUUID group_id = content["group_id"].asUUID();
	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap)
	{
		LL_WARNS("GrpMgr") << "Received incorrect, possibly stale, group or request id" << LL_ENDL;
		return;
	}
	S32 num_members = content["member_count"];
	if (num_members < 1)
	{
		LL_INFOS("GrpMgr") << "Received empty group members list for group id: " << group_id.asString() << LL_ENDL;
		group_datap->mMemberDataComplete = true;
		group_datap->mChanged = TRUE;
		LLGroupMgr::getInstance()->notifyObservers(GC_MEMBER_DATA);
		return;
	}
	group_datap->mMemberCount = num_members;
	LLSD member_list = content["members"];
	LLSD titles = content["titles"];
	LLSD defaults = content["defaults"];
	std::string online_status;
	std::string title;
	S32 contribution;
	U64 member_powers;
	BOOL is_owner;
	U64 default_powers = llstrtou64(defaults["default_powers"].asString().c_str(), NULL, 16);
	LLSD::map_const_iterator member_iter_start = member_list.beginMap();
	LLSD::map_const_iterator member_iter_end = member_list.endMap();
	for ( ; member_iter_start != member_iter_end; ++member_iter_start)
	{
		online_status = localized_unknown();
		title = titles[0].asString();
		contribution = 0;
		member_powers = default_powers;
		is_owner = false;
		const LLUUID member_id(member_iter_start->first);
		LLSD member_info = member_iter_start->second;
		if (member_info.has("last_login"))
		{
			online_status = member_info["last_login"].asString();
			if (online_status == "Online")
				online_status = localized_online();
			else if (online_status == "unknown")
				online_status = localized_unknown();
			else
				formatDateString(online_status);
		}
		if (member_info.has("title"))
			title = titles[member_info["title"].asInteger()].asString();
		if (member_info.has("powers"))
			member_powers = llstrtou64(member_info["powers"].asString().c_str(), NULL, 16);
		if (member_info.has("donated_square_meters"))
			contribution = member_info["donated_square_meters"];
		if (member_info.has("owner"))
			is_owner = true;
		LLGroupMemberData* data = new LLGroupMemberData(member_id,
			contribution,
			member_powers,
			title,
			online_status,
			is_owner);
		LLGroupMemberData* member_old = group_datap->mMembers[member_id];
		if (member_old && group_datap->mRoleMemberDataComplete)
		{
			LLGroupMemberData::role_list_t::iterator rit = member_old->roleBegin();
			LLGroupMemberData::role_list_t::iterator end = member_old->roleEnd();
			for ( ; rit != end; ++rit)
			{
				data->addRole((*rit).first, (*rit).second);
			}
		}
		else
		{
			group_datap->mRoleMemberDataComplete = false;
		}
		group_datap->mMembers[member_id] = data;
	}
	group_datap->mMemberVersion.generate();
	if (group_datap->mTitles.size() < 1)
		LLGroupMgr::getInstance()->sendGroupTitlesRequest(group_id);
	group_datap->mMemberDataComplete = true;
	group_datap->mMemberRequestID.setNull();
	if (group_datap->mPendingRoleMemberRequest || !group_datap->mRoleMemberDataComplete)
	{
		group_datap->mPendingRoleMemberRequest = false;
		LLGroupMgr::getInstance()->sendGroupRoleMembersRequest(group_id);
	}
	group_datap->mChanged = TRUE;
	LLGroupMgr::getInstance()->notifyObservers(GC_MEMBER_DATA);
}
void LLGroupMgr::sendGroupRoleChanges(const LLUUID& group_id)
{
	LL_DEBUGS() << "LLGroupMgr::sendGroupRoleChanges" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = getGroupData(group_id);
	if (group_datap && group_datap->pendingRoleChanges())
	{
		group_datap->sendRoleChanges();
		group_datap->mChanged = TRUE;
		notifyObservers(GC_ROLE_DATA);
	}
}
void LLGroupMgr::cancelGroupRoleChanges(const LLUUID& group_id)
{
	LL_DEBUGS() << "LLGroupMgr::cancelGroupRoleChanges" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = getGroupData(group_id);
	if (group_datap) group_datap->cancelRoleChanges();
}
bool LLGroupMgr::parseRoleActions(const std::string& xml_filename)
{
	LLXMLNodePtr root;
	BOOL success = LLUICtrlFactory::getLayeredXMLNode(xml_filename, root);
	if (!success || !root || !root->hasName( "role_actions" ))
	{
		LL_ERRS() << "Problem reading UI role_actions file: " << xml_filename << LL_ENDL;
		return false;
	}
	LLXMLNodeList role_list;
	root->getChildren("action_set", role_list, false);
	for (LLXMLNodeList::iterator role_iter = role_list.begin(); role_iter != role_list.end(); ++role_iter)
	{
		LLXMLNodePtr action_set = role_iter->second;
		LLRoleActionSet* role_action_set = new LLRoleActionSet();
		LLRoleAction* role_action_data = new LLRoleAction();
		std::string action_set_name;
		if (action_set->getAttributeString("name", action_set_name))
		{
			LL_DEBUGS() << "Loading action set " << action_set_name << LL_ENDL;
			role_action_data->mName = action_set_name;
		}
		else
		{
			LL_WARNS() << "Unable to parse action set with no name" << LL_ENDL;
			delete role_action_set;
			delete role_action_data;
			continue;
		}
		std::string set_description;
		if (action_set->getAttributeString("description", set_description))
		{
			role_action_data->mDescription = set_description;
		}
		std::string set_longdescription;
		if (action_set->getAttributeString("longdescription", set_longdescription))
		{
			role_action_data->mLongDescription = set_longdescription;
		}
		U64 set_power_mask = 0;
		LLXMLNodeList action_list;
		LLXMLNodeList::iterator action_iter;
		action_set->getChildren("action", action_list, false);
		for (action_iter = action_list.begin(); action_iter != action_list.end(); ++action_iter)
		{
			LLXMLNodePtr action = action_iter->second;
			LLRoleAction* role_action = new LLRoleAction();
			std::string action_name;
			if (action->getAttributeString("name", action_name))
			{
				LL_DEBUGS() << "Loading action " << action_name << LL_ENDL;
				role_action->mName = action_name;
			}
			else
			{
				LL_WARNS() << "Unable to parse action with no name" << LL_ENDL;
				delete role_action;
				continue;
			}
			std::string description;
			if (action->getAttributeString("description", description))
			{
				role_action->mDescription = description;
			}
			std::string longdescription;
			if (action->getAttributeString("longdescription", longdescription))
			{
				role_action->mLongDescription = longdescription;
			}
			S32 power_bit = 0;
			if (action->getAttributeS32("value", power_bit))
			{
				if (0 <= power_bit && power_bit < 64)
				{
					role_action->mPowerBit = 0x1LL << power_bit;
				}
			}
			set_power_mask |= role_action->mPowerBit;
			role_action_set->mActions.push_back(role_action);
		}
		role_action_data->mPowerBit = set_power_mask;
		role_action_set->mActionSetData = role_action_data;
		LLGroupMgr::getInstance()->mRoleActionSets.push_back(role_action_set);
	}
	return true;
}
void LLGroupMgr::debugClearAllGroups(void*)
{
	LLGroupMgr::getInstance()->clearGroups();
	LLGroupMgr::parseRoleActions("role_actions.xml");
}
