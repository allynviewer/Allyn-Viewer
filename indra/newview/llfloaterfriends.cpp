/**
 * @file llfloaterfriends.cpp
 * @author Phoenix
 * @date 2005-01-13
 * @brief Implementation of the friends floater
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llfloaterfriends.h"
#include "llsdutil_math.h"
#include "llagent.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llbutton.h"
#include "lldir.h"
#include "lleventtimer.h"
#include "llfiltereditor.h"
#include "llfloateravatarpicker.h"
#include "llnotificationsutil.h"
#include "llscrolllistcolumn.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsdserialize.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "statemachine/aifilepicker.h"
#include "hippogridmanager.h"
class LLLocalFriendsObserver : public LLFriendObserver, public LLEventTimer
{
	LOG_CLASS(LLLocalFriendsObserver);
public:
	LLLocalFriendsObserver(LLPanelFriends* floater)
	:	mFloater(floater), LLEventTimer(0.5)
	{
		mEventTimer.stop();
		LLAvatarTracker::instance().addObserver(this);
	}
	~LLLocalFriendsObserver()
	{
		LLAvatarTracker::instance().removeObserver(this);
	}
	void changed(U32 mask)
	{
		mEventTimer.start();
		mMask |= mask;
	}
	BOOL tick()
	{
		mFloater->updateFriends(mMask);
		mEventTimer.stop();
		mMask = 0;
		return false;
	}
protected:
	LLPanelFriends* mFloater;
	U32 mMask;
};
LLPanelFriends::LLPanelFriends() :
	mObserver(NULL),
	mNumOnline(0)
{
	mObserver = new LLLocalFriendsObserver(this);
}
LLPanelFriends::~LLPanelFriends()
{
	delete mObserver;
}
void LLPanelFriends::updateFriends(U32 changed_mask)
{
	LLUUID selected_id;
	const uuid_vec_t selected_friends = mFriendsList->getSelectedIDs();
	if (changed_mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE | LLFriendObserver::POWERS))
	{
		refreshNames(changed_mask);
	}
	if (!selected_friends.empty())
	{
		mFriendsList->setCurrentByID(selected_id);
		for(uuid_vec_t::const_iterator itr = selected_friends.begin(); itr != selected_friends.end(); ++itr)
		{
			mFriendsList->setSelectedByValue(*itr, true);
		}
	}
	refreshUI();
}
void LLPanelFriends::filterContacts(const std::string& search_name)
{
	std::string friend_name;
	if (!search_name.empty())
	{
		if (search_name.find(mLastContactSearch) == std::string::npos)
		{
			refreshNames(LLFriendObserver::ADD);
		}
		std::vector<LLScrollListItem*> vFriends = mFriendsList->getAllData();
		for (std::vector<LLScrollListItem*>::iterator itr = vFriends.begin(); itr != vFriends.end(); ++itr)
		{
			friend_name = utf8str_tolower((*itr)->getColumn(LIST_FRIEND_NAME)->getValue().asString());
			bool show_entry = (friend_name.find(utf8str_tolower(search_name)) != std::string::npos);
			if (!show_entry)
			{
				mFriendsList->deleteItems((*itr)->getValue());
			}
		}
		mFriendsList->updateLayout();
		refreshUI();
	}
	else if (!mLastContactSearch.empty()) refreshNames(LLFriendObserver::ADD);
	mLastContactSearch = search_name;
}
BOOL LLPanelFriends::postBuild()
{
	mFriendsList = getChild<LLScrollListCtrl>("friend_list");
	mFriendsList->setCommitOnSelectionChange(true);
	auto single_selection(boost::bind(&LLScrollListCtrl::getCurrentID, mFriendsList));
	auto selection(boost::bind(&LLScrollListCtrl::getSelectedIDs, mFriendsList));
	mFriendsList->setCommitCallback(boost::bind(&LLPanelFriends::onSelectName, this));
	mFriendsList->setDoubleClickCallback(boost::bind(LLAvatarActions::startIM, single_selection));
	if (LLFilterEditor* contact = getChild<LLFilterEditor>("buddy_search_lineedit"))
	{
		contact->setCommitCallback(boost::bind(&LLPanelFriends::filterContacts, this, _2));
	}
	getChild<LLTextBox>("s_num")->setValue("0");
	getChild<LLTextBox>("f_num")->setValue(llformat("%d", mFriendsList->getItemCount()));
	U32 changed_mask = LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE;
	refreshNames(changed_mask);
	getChild<LLUICtrl>("im_btn")->setCommitCallback(boost::bind(&LLPanelFriends::onClickIM, this, selection));
	getChild<LLUICtrl>("profile_btn")->setCommitCallback(boost::bind(LLAvatarActions::showProfiles, selection, false));
	getChild<LLUICtrl>("offer_teleport_btn")->setCommitCallback(boost::bind(static_cast<void(*)(const uuid_vec_t&)>(LLAvatarActions::offerTeleport), selection));
	getChild<LLUICtrl>("pay_btn")->setCommitCallback(boost::bind(LLAvatarActions::pay, single_selection));
	getChild<LLUICtrl>("add_btn")->setCommitCallback(boost::bind(&LLPanelFriends::onClickAddFriend, this));
	getChild<LLUICtrl>("remove_btn")->setCommitCallback(boost::bind(LLAvatarActions::removeFriendsDialog, selection));
	setDefaultBtn("im_btn");
	updateFriends(LLFriendObserver::ADD);
	refreshUI();
	mFriendsList->sortByColumn(std::string("friend_name"), true);
	mFriendsList->sortByColumn(std::string("icon_online_status"), false);
	if (LLControlVariable* ctrl = gSavedSettings.getControl("ContactListCollapsed"))
	{
		updateColumns(ctrl->getValue());
		ctrl->getSignal()->connect(boost::bind(&LLPanelFriends::updateColumns, this, _2));
	}
	return true;
}
const S32& friend_name_system()
{
	static const LLCachedControl<S32> name_system("FriendNameSystem", 0);
	return name_system;
}
static void update_friend_name(LLScrollListCtrl* list, const LLUUID& id, const LLAvatarName& avname)
{
	if (LLScrollListItem* item = list->getItem(id))
		item->getColumn(1)->setValue(avname.getNSName(friend_name_system()));
}
void LLPanelFriends::addFriend(const LLUUID& agent_id)
{
	const LLRelationship* relation_info = LLAvatarTracker::instance().getBuddyInfo(agent_id);
	if (!relation_info) return;
	bool isOnline = relation_info->isOnline();
	std::string fullname;
	bool have_name = LLAvatarNameCache::getNSName(agent_id, fullname, friend_name_system());
	if (!have_name) gCacheName->getFullName(agent_id, fullname);
	LLScrollListItem::Params element;
	element.value = agent_id;
	LLScrollListCell::Params friend_column;
	friend_column.column("friend_name").value(fullname).font("SANSSERIF");
	static const LLCachedControl<LLColor4> sDefaultColor(gColors, "DefaultListText");
	static const LLCachedControl<LLColor4> sMutedColor("AscentMutedColor");
	friend_column.color(LLAvatarActions::isBlocked(agent_id) ? sMutedColor : sDefaultColor);
	LLScrollListCell::Params cell;
	cell.column("icon_online_status").type("icon");
	if (isOnline)
	{
		++mNumOnline;
		friend_column.font_style("BOLD");
		cell.value("icon_avatar_online.tga");
	}
	element.columns.add(cell);
	element.columns.add(friend_column);
	cell.font_style("NORMAL").type("checkbox")
		.column("icon_visible_online")
		.value(relation_info->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS));
	element.columns.add(cell);
	cell.column("icon_visible_map")
		.value(relation_info->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION));
	element.columns.add(cell);
	cell.column("icon_edit_mine")
		.value(relation_info->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS));
	element.columns.add(cell);
	cell.enabled(false)
		.column("icon_visible_online_theirs")
		.value(relation_info->isRightGrantedFrom(LLRelationship::GRANT_ONLINE_STATUS));
	element.columns.add(cell);
	cell.column("icon_visible_map_theirs")
		.value(relation_info->isRightGrantedFrom(LLRelationship::GRANT_MAP_LOCATION));
	element.columns.add(cell);
	cell.column("icon_edit_theirs")
		.value(relation_info->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS));
	element.columns.add(cell);
	cell.type("text")
		.column("friend_last_update_generation")
		.value(have_name ? relation_info->getChangeSerialNum() : -1);
	element.columns.add(cell);
	mFriendsList->addRow(element);
	if (!have_name) LLAvatarNameCache::get(agent_id, boost::bind(update_friend_name, mFriendsList, _1, _2));
}
void LLPanelFriends::updateFriendItem(const LLUUID& agent_id, const LLRelationship* info)
{
	if (!info) return;
	LLScrollListItem* itemp = mFriendsList->getItem(agent_id);
	if (!itemp) return;
	bool isOnline = info->isOnline();
	std::string fullname;
	if (LLAvatarNameCache::getNSName(agent_id, fullname, friend_name_system()))
	{
		itemp->getColumn(LIST_FRIEND_UPDATE_GEN)->setValue(info->getChangeSerialNum());
	}
	else
	{
		gCacheName->getFullName(agent_id, fullname);
		LLAvatarNameCache::get(agent_id, boost::bind(update_friend_name, mFriendsList, _1, _2));
		itemp->getColumn(LIST_FRIEND_UPDATE_GEN)->setValue(-1);
	}
	if (LLScrollListCell* cell = itemp->getColumn(LIST_ONLINE_STATUS))
	{
		if (cell->getValue().asString().empty() == isOnline)
		{
			std::string statusIcon;
			if (isOnline)
			{
				++mNumOnline;
				statusIcon = "icon_avatar_online.tga";
			}
			else
			{
				--mNumOnline;
			}
			cell->setValue(statusIcon);
		}
	}
	itemp->getColumn(LIST_FRIEND_NAME)->setValue(fullname);
	static_cast<LLScrollListText*>(itemp->getColumn(LIST_FRIEND_NAME))->setFontStyle(isOnline ? LLFontGL::BOLD : LLFontGL::NORMAL);
	itemp->getColumn(LIST_VISIBLE_ONLINE)->setValue(info->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS));
	itemp->getColumn(LIST_VISIBLE_MAP)->setValue(info->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION));
	itemp->getColumn(LIST_EDIT_MINE)->setValue(info->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS));
	itemp->getColumn(LIST_VISIBLE_ONLINE_THEIRS)->setValue(info->isRightGrantedFrom(LLRelationship::GRANT_ONLINE_STATUS));
	itemp->getColumn(LIST_VISIBLE_MAP_THEIRS)->setValue(info->isRightGrantedFrom(LLRelationship::GRANT_MAP_LOCATION));
	itemp->getColumn(LIST_EDIT_THEIRS)->setValue(info->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS));
	mFriendsList->setNeedsSort();
}
void LLPanelFriends::refreshRightsChangeList()
{
	S32 num_selected = mFriendsList->getNumSelected();
	getChild<LLTextBox>("s_num")->setValue(llformat("%d", num_selected));
	getChild<LLTextBox>("f_num")->setValue(llformat("%d / %d", mNumOnline, mFriendsList->getItemCount()));
	if (!num_selected)
	{
		getChildView("im_btn")->setEnabled(false);
		getChildView("offer_teleport_btn")->setEnabled(false);
	}
	else
	{
		getChildView("im_btn")->setEnabled(true);
		getChildView("offer_teleport_btn")->setEnabled(true);
	}
}
struct SortFriendsByID
{
	bool operator() (const LLScrollListItem* const a, const LLScrollListItem* const b) const
	{
		return a->getValue().asUUID() < b->getValue().asUUID();
	}
};
void LLPanelFriends::refreshNames(U32 changed_mask)
{
	const uuid_vec_t selected_ids = mFriendsList->getSelectedIDs();
	S32 pos = mFriendsList->getScrollPos();
	LLAvatarTracker::buddy_map_t all_buddies;
	LLAvatarTracker::instance().copyBuddyList(all_buddies);
	if (changed_mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE))
	{
		refreshNamesSync(all_buddies);
	}
	if (changed_mask & LLFriendObserver::ONLINE)
	{
		refreshNamesPresence(all_buddies);
	}
	mFriendsList->updateSort();
	mFriendsList->selectMultiple(selected_ids);
	mFriendsList->setScrollPos(pos);
}
void LLPanelFriends::refreshNamesSync(const LLAvatarTracker::buddy_map_t& all_buddies)
{
	mNumOnline = 0;
	mFriendsList->deleteAllItems();
	for(LLAvatarTracker::buddy_map_t::const_iterator buddy_it = all_buddies.begin(); buddy_it != all_buddies.end(); ++buddy_it)
	{
		addFriend(buddy_it->first);
	}
}
void LLPanelFriends::refreshNamesPresence(const LLAvatarTracker::buddy_map_t& all_buddies)
{
	std::vector<LLScrollListItem*> items = mFriendsList->getAllData();
	std::sort(items.begin(), items.end(), SortFriendsByID());
	LLAvatarTracker::buddy_map_t::const_iterator buddy_it  = all_buddies.begin();
	std::vector<LLScrollListItem*>::const_iterator item_it = items.begin();
	while(item_it != items.end() && buddy_it != all_buddies.end())
	{
		const LLUUID& buddy_uuid = buddy_it->first;
		const LLUUID& item_uuid  = (*item_it)->getValue().asUUID();
		if (item_uuid == buddy_uuid)
		{
			const LLRelationship* info = buddy_it->second;
			if (!info)
			{
				++item_it;
				continue;
			}
			S32 last_change_generation = (*item_it)->getColumn(LIST_FRIEND_UPDATE_GEN)->getValue().asInteger();
			if (last_change_generation < info->getChangeSerialNum())
			{
				updateFriendItem(buddy_it->first, info);
			}
			++buddy_it;
			++item_it;
		}
		else if (item_uuid < buddy_uuid)
		{
			++item_it;
		}
		else
		{
			++buddy_it;
		}
	}
}
void LLPanelFriends::refreshUI()
{
	bool single_selected = false;
	bool multiple_selected = false;
	size_t num_selected = mFriendsList->getAllSelected().size();
	if (num_selected)
	{
		single_selected = true;
		if (num_selected > 1)
		{
			multiple_selected = true;
		}
	}
	getChildView("pay_btn")->setEnabled(single_selected && !multiple_selected);
	getChildView("profile_btn")->setEnabled(single_selected);
	getChildView("remove_btn")->setEnabled(single_selected);
	getChildView("im_btn")->setEnabled(single_selected);
	refreshRightsChangeList();
}
void LLPanelFriends::onSelectName()
{
	refreshUI();
	applyRightsToFriends();
}
void LLPanelFriends::updateColumns(bool collapsed)
{
	S32 width = collapsed ? 0 : 22;
	LLScrollListColumn* column = mFriendsList->getColumn(5);
	mFriendsList->updateStaticColumnWidth(column, width);
	column->setWidth(width);
	column = mFriendsList->getColumn(6);
	mFriendsList->updateStaticColumnWidth(column, width);
	column->setWidth(width);
	column = mFriendsList->getColumn(7);
	mFriendsList->updateStaticColumnWidth(column, width);
	column->setWidth(width);
	mFriendsList->updateLayout();
	if (!collapsed) updateFriends(LLFriendObserver::ADD);
}
void LLPanelFriends::onClickIM(const uuid_vec_t& ids)
{
	if (!ids.empty())
		ids.size() == 1 ? LLAvatarActions::startIM(ids[0]) : LLAvatarActions::startConference(ids);
}
void LLPanelFriends::onPickAvatar(const uuid_vec_t& ids, const std::vector<LLAvatarName>& names)
{
	if (names.empty() || ids.empty()) return;
	LLAvatarActions::requestFriendshipDialog(ids[0], names[0].getCompleteName());
}
void LLPanelFriends::onClickAddFriend()
{
	if (LLFloater* root_floater = gFloaterView->getParentFloater(this))
		root_floater->addDependentFloater(LLFloaterAvatarPicker::show(boost::bind(&LLPanelFriends::onPickAvatar, _1, _2), false, true));
}
void LLPanelFriends::confirmModifyRights(rights_map_t& rights, EGrantRevoke command)
{
	if (rights.empty()) return;
	if (rights.size() == 1)
	{
		LLSD args;
		std::string fullname;
		if (LLAvatarNameCache::getNSName(rights.begin()->first, fullname, friend_name_system()))
			args["NAME"] = fullname;
		LLNotificationsUtil::add(command == GRANT ? "GrantModifyRights" : "RevokeModifyRights",
				args, LLSD(),
				boost::bind(&LLPanelFriends::modifyRightsConfirmation, this, _1, _2, rights));
	}
	else
	{
		LLNotificationsUtil::add(command == GRANT ? "GrantModifyRightsMultiple" : "RevokeModifyRightsMultiple",
				LLSD(), LLSD(),
				boost::bind(&LLPanelFriends::modifyRightsConfirmation, this, _1, _2, rights));
	}
}
bool LLPanelFriends::modifyRightsConfirmation(const LLSD& notification, const LLSD& response, rights_map_t rights)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(0 == option)
	{
		sendRightsGrant(rights);
	}
	else
	{
		for (rights_map_t::iterator rights_it = rights.begin(); rights_it != rights.end(); ++rights_it)
		{
			const LLRelationship* info = LLAvatarTracker::instance().getBuddyInfo(rights_it->first);
			updateFriendItem(rights_it->first, info);
		}
	}
	refreshUI();
	return false;
}
void LLPanelFriends::applyRightsToFriends()
{
	rights_map_t rights_updates;
	bool need_confirmation = false;
	EGrantRevoke confirmation_type = GRANT;
	std::vector<LLScrollListItem*> selected = mFriendsList->getAllSelected();
	for(std::vector<LLScrollListItem*>::iterator itr = selected.begin(); itr != selected.end(); ++itr)
	{
		LLUUID id = (*itr)->getValue();
		const LLRelationship* buddy_relationship = LLAvatarTracker::instance().getBuddyInfo(id);
		if (!buddy_relationship) continue;
		bool show_online_staus = (*itr)->getColumn(LIST_VISIBLE_ONLINE)->getValue().asBoolean();
		bool show_map_location = (*itr)->getColumn(LIST_VISIBLE_MAP)->getValue().asBoolean();
		bool allow_modify_objects = (*itr)->getColumn(LIST_EDIT_MINE)->getValue().asBoolean();
		bool rights_changed(false);
		S32 rights = buddy_relationship->getRightsGrantedTo();
		if (buddy_relationship->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS) != show_online_staus)
		{
			rights_changed = true;
			if (show_online_staus)
			{
				rights |= LLRelationship::GRANT_ONLINE_STATUS;
			}
			else
			{
				rights &= ~LLRelationship::GRANT_ONLINE_STATUS;
				rights &= ~LLRelationship::GRANT_MAP_LOCATION;
				(*itr)->getColumn(LIST_VISIBLE_MAP)->setValue(false);
				mFriendsList->setNeedsSortColumn(LIST_VISIBLE_MAP);
			}
		}
		if (buddy_relationship->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION) != show_map_location)
		{
			rights_changed = true;
			if (show_map_location)
			{
				rights |= LLRelationship::GRANT_MAP_LOCATION;
				rights |= LLRelationship::GRANT_ONLINE_STATUS;
				(*itr)->getColumn(LIST_VISIBLE_ONLINE)->setValue(true);
				mFriendsList->setNeedsSortColumn(LIST_VISIBLE_ONLINE);
			}
			else
			{
				rights &= ~LLRelationship::GRANT_MAP_LOCATION;
			}
		}
		if (buddy_relationship->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS) != allow_modify_objects)
		{
			rights_changed = true;
			need_confirmation = true;
			if (allow_modify_objects)
			{
				rights |= LLRelationship::GRANT_MODIFY_OBJECTS;
				confirmation_type = GRANT;
			}
			else
			{
				rights &= ~LLRelationship::GRANT_MODIFY_OBJECTS;
				confirmation_type = REVOKE;
			}
		}
		if (rights_changed)
			rights_updates.insert(std::make_pair(id, rights));
	}
	if (need_confirmation)
	{
		confirmModifyRights(rights_updates, confirmation_type);
	}
	else
	{
		sendRightsGrant(rights_updates);
	}
}
void LLPanelFriends::sendRightsGrant(rights_map_t& ids)
{
	if (ids.empty()) return;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_GrantUserRights);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUID(_PREHASH_AgentID, gAgentID);
	msg->addUUID(_PREHASH_SessionID, gAgentSessionID);
	rights_map_t::iterator id_it;
	rights_map_t::iterator end_it = ids.end();
	for(id_it = ids.begin(); id_it != end_it; ++id_it)
	{
		msg->nextBlockFast(_PREHASH_Rights);
		msg->addUUID(_PREHASH_AgentRelated, id_it->first);
		msg->addS32(_PREHASH_RelatedRights, id_it->second);
	}
	gAgent.sendReliableMessage();
}
