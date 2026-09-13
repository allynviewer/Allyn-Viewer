/**
 * @file llgiveinventory.cpp
 * @brief LLGiveInventory class implementation
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
#include "llgiveinventory.h"
#include "llnotificationsutil.h"
#include "lltrans.h"
#include "llagent.h"
#include "llagentdata.h"
#include "llagentui.h"
#include "llagentwearables.h"
#include "llavataractions.h"
#include "llfloaterchat.h"
#include "llfloatertools.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llimview.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "llmutelist.h"
#include "llviewerobjectlist.h"
#include "llvoavatarself.h"
#include "llavatarnamecache.h"
#include "rlvhandler.h"
#include "rlvui.h"
const size_t MAX_ITEMS = 50;
class LLGiveable : public LLInventoryCollectFunctor
{
public:
	LLGiveable() : mCountLosing(0) {}
	virtual ~LLGiveable() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
	S32 countNoCopy() const { return mCountLosing; }
protected:
	S32 mCountLosing;
};
bool LLGiveable::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if (cat)
		return true;
	bool allowed = false;
	if(item)
	{
		allowed = itemTransferCommonlyAllowed(item);
		if(allowed &&
		   !item->getPermissions().allowOperationBy(PERM_TRANSFER,
							    gAgent.getID()))
		{
			allowed = FALSE;
		}
		if(allowed &&
		   !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			++mCountLosing;
		}
	}
	return allowed;
}
class LLUncopyableItems : public LLInventoryCollectFunctor
{
public:
	LLUncopyableItems() {}
	virtual ~LLUncopyableItems() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
};
bool LLUncopyableItems::operator()(LLInventoryCategory* cat,
				   LLInventoryItem* item)
{
	bool uncopyable = false;
	if(item)
	{
		if (itemTransferCommonlyAllowed(item) &&
		   !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			uncopyable = true;
		}
	}
	return uncopyable;
}
bool LLGiveInventory::isInventoryGiveAcceptable(const LLInventoryItem* item)
{
	if (!item) return false;
	if (!isAgentAvatarValid()) return false;
	if (!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID))
	{
		return false;
	}
	bool acceptable = true;
	switch(item->getType())
	{
	case LLAssetType::AT_OBJECT:
		break;
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_CLOTHING:
		{
		}
		break;
	default:
		break;
	}
	return acceptable;
}
bool LLGiveInventory::isInventoryGroupGiveAcceptable(const LLInventoryItem* item)
{
	if(!item) return false;
	if (!isAgentAvatarValid()) return false;
	if (!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID))
	{
		return false;
	}
	if (!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		return false;
	}
	bool acceptable = true;
	switch(item->getType())
	{
	case LLAssetType::AT_OBJECT:
		break;
	default:
		break;
	}
	return acceptable;
}
bool LLGiveInventory::doGiveInventoryItem(const LLUUID& to_agent,
									  const LLInventoryItem* item,
									  const LLUUID& im_session_id)
{
	bool res = true;
	LL_INFOS() << "LLGiveInventory::giveInventory()" << LL_ENDL;
	if(!isInventoryGiveAcceptable(item))
	{
		return false;
	}
	if (item->getPermissions().allowCopyBy(gAgentID))
	{
		LLGiveInventory::commitGiveInventoryItem(to_agent, item, im_session_id);
	}
	else
	{
		LLSD substitutions;
		substitutions["ITEMS"] = item->getName();
		LLSD payload;
		payload["agent_id"] = to_agent;
		LLSD items = LLSD::emptyArray();
		items.append(item->getUUID());
		payload["items"] = items;
		LLNotificationsUtil::add("CannotCopyWarning", substitutions, payload,
			&LLGiveInventory::handleCopyProtectedItem);
		res = false;
	}
	return res;
}
bool LLGiveInventory::doGiveInventoryCategory(const LLUUID& to_agent,
											  const LLInventoryCategory* cat,
											  const LLUUID& im_session_id,
											  const std::string& notification_name)
{
	if (!cat)
	{
		return false;
	}
	LL_INFOS() << "LLGiveInventory::giveInventoryCategory() - "
		<< cat->getUUID() << LL_ENDL;
	if (!isAgentAvatarValid())
	{
		return false;
	}
	bool give_successful = true;
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLGiveable giveable;
	gInventory.collectDescendentsIf(cat->getUUID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									giveable);
	size_t count = cats.size();
	bool complete = true;
	for(size_t i = 0; i < count; ++i)
	{
		if(!gInventory.isCategoryComplete(cats.at(i)->getUUID()))
		{
			complete = false;
			break;
		}
	}
	if(!complete)
	{
		LLNotificationsUtil::add("IncompleteInventory");
		give_successful = false;
	}
	count = items.size() + cats.size();
	if(count > MAX_ITEMS)
	{
		LLNotificationsUtil::add("TooManyItems");
		give_successful = false;
	}
	else if(count == 0)
	{
		LLNotificationsUtil::add("NoItems");
		give_successful = false;
	}
	else if (give_successful)
	{
		if(0 == giveable.countNoCopy())
		{
			give_successful = LLGiveInventory::commitGiveInventoryCategory(to_agent, cat, im_session_id);
		}
		else
		{
			LLSD args;
			args["COUNT"] = llformat("%d",giveable.countNoCopy());
			LLSD payload;
			payload["agent_id"] = to_agent;
			payload["folder_id"] = cat->getUUID();
			if (!notification_name.empty())
			{
				payload["success_notification"] = notification_name;
			}
			LLNotificationsUtil::add("CannotCopyCountItems", args, payload, &LLGiveInventory::handleCopyProtectedCategory);
			give_successful = false;
		}
	}
	return give_successful;
}
void LLGiveInventory::logInventoryOffer(const LLUUID& to_agent, const LLUUID &im_session_id)
{
	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, to_agent);
	LLSD args;
	args["user_id"] = to_agent;
	if (im_session_id.notNull())
	{
		gIMMgr->addSystemMessage(im_session_id, "inventory_item_offered", args);
	}
	else if ( (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES) || gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS)) && (RlvUtil::isNearbyAgent(to_agent)) &&
		      (!RlvUIEnabler::hasOpenProfile(to_agent)) )
	{
		std::string strMsgName = "inventory_item_offered-im"; LLSD args; LLAvatarName avName;
		if (LLAvatarNameCache::get(to_agent, &avName))
		{
			args["NAME"] = RlvStrings::getAnonym(avName);
			strMsgName = "inventory_item_offered_rlv";
		}
		gIMMgr->addSystemMessage(LLUUID::null, strMsgName, args);
	}
	else if (gIMMgr->isIMSessionOpen(session_id))
	{
		gIMMgr->addSystemMessage(session_id, "inventory_item_offered", args);
	}
	else
	{
		LLChat chat(LLTrans::getString("inventory_item_offered_to") + ' ' + LLAvatarActions::getSLURL(to_agent));
		chat.mSourceType = CHAT_SOURCE_SYSTEM;
		LLFloaterChat::addChatHistory(chat);
	}
}
bool LLGiveInventory::handleCopyProtectedItem(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLSD itmes = notification["payload"]["items"];
	LLInventoryItem* item = NULL;
	bool give_successful = true;
	switch(option)
	{
	case 0:
		for (LLSD::array_iterator it = itmes.beginArray(); it != itmes.endArray(); it++)
		{
			item = gInventory.getItem((*it).asUUID());
			if (item)
			{
				LLGiveInventory::commitGiveInventoryItem(notification["payload"]["agent_id"].asUUID(),
					item);
				gInventory.deleteObject(item->getUUID());
				gInventory.notifyObservers();
			}
			else
			{
				LLNotificationsUtil::add("CannotGiveItem");
				give_successful = false;
			}
		}
		if (give_successful && notification["payload"]["success_notification"].isDefined())
		{
			LLNotificationsUtil::add(notification["payload"]["success_notification"].asString());
		}
		break;
	default:
		LLNotificationsUtil::add("TransactionCancelled");
		give_successful = false;
		break;
	}
	return give_successful;
}
void LLGiveInventory::commitGiveInventoryItem(const LLUUID& to_agent,
												const LLInventoryItem* item,
												const LLUUID& im_session_id)
{
	if (!item) return;
	std::string name;
	LLAgentUI::buildFullname(name);
	LLUUID transaction_id;
	transaction_id.generate();
	const S32 BUCKET_SIZE = sizeof(U8) + UUID_BYTES;
	U8 bucket[BUCKET_SIZE];
	bucket[0] = (U8)item->getType();
	memcpy(&bucket[1], &(item->getUUID().mData), UUID_BYTES);
	pack_instant_message(
		gMessageSystem,
		gAgentID,
		FALSE,
		gAgentSessionID,
		to_agent,
		name,
		item->getName(),
		IM_ONLINE,
		IM_INVENTORY_OFFERED,
		transaction_id,
		0,
		LLUUID::null,
		gAgent.getPositionAgent(),
		NO_TIMESTAMP,
		bucket,
		BUCKET_SIZE);
	gAgent.sendReliableMessage();
	if (gSavedSettings.getBOOL("BroadcastViewerEffects"))
	{
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
		effectp->setSourceObject(gAgentAvatarp);
		effectp->setTargetObject(gObjectList.findObject(to_agent));
		effectp->setDuration(LL_HUD_DUR_SHORT);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	}
	gFloaterTools->dirty();
	LLMuteList::getInstance()->autoRemove(to_agent, LLMuteList::AR_INVENTORY);
	logInventoryOffer(to_agent, im_session_id);
}
bool LLGiveInventory::handleCopyProtectedCategory(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLInventoryCategory* cat = NULL;
	bool give_successful = true;
	switch(option)
	{
	case 0:
		cat = gInventory.getCategory(notification["payload"]["folder_id"].asUUID());
		if(cat)
		{
			give_successful = LLGiveInventory::commitGiveInventoryCategory(notification["payload"]["agent_id"].asUUID(),
														   cat);
			LLViewerInventoryCategory::cat_array_t cats;
			LLViewerInventoryItem::item_array_t items;
			LLUncopyableItems remove;
			gInventory.collectDescendentsIf(cat->getUUID(),
											cats,
											items,
											LLInventoryModel::EXCLUDE_TRASH,
											remove);
			size_t count = items.size();
			for(size_t i = 0; i < count; ++i)
			{
				gInventory.deleteObject(items.at(i)->getUUID());
			}
			gInventory.notifyObservers();
			if (give_successful && notification["payload"]["success_notification"].isDefined())
			{
				LLNotificationsUtil::add(notification["payload"]["success_notification"].asString());
			}
		}
		else
		{
			LLNotificationsUtil::add("CannotGiveCategory");
			give_successful = false;
		}
		break;
	default:
		LLNotificationsUtil::add("TransactionCancelled");
		give_successful = false;
		break;
	}
	return give_successful;
}
bool LLGiveInventory::commitGiveInventoryCategory(const LLUUID& to_agent,
													const LLInventoryCategory* cat,
													const LLUUID& im_session_id)
{
	if (!cat)
	{
		return false;
	}
	LL_INFOS() << "LLGiveInventory::commitGiveInventoryCategory() - "
			<< cat->getUUID() << LL_ENDL;
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLGiveable giveable;
	gInventory.collectDescendentsIf(cat->getUUID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									giveable);
	bool give_successful = true;
	size_t count = items.size() + cats.size();
	if (count > MAX_ITEMS)
  	{
		LLNotificationsUtil::add("TooManyItems");
		give_successful = false;
  	}
 	else if(count == 0)
  	{
		LLNotificationsUtil::add("NoItems");
		give_successful = false;
  	}
	else
	{
		std::string name;
		LLAgentUI::buildFullname(name);
		LLUUID transaction_id;
		transaction_id.generate();
		size_t bucket_size = (sizeof(U8) + UUID_BYTES) * (count + 1);
		U8* bucket = new U8[bucket_size];
		U8* pos = bucket;
		U8 type = (U8)cat->getType();
		memcpy(pos, &type, sizeof(U8));
		pos += sizeof(U8);
		memcpy(pos, &(cat->getUUID()), UUID_BYTES);
		pos += UUID_BYTES;
		size_t i;
		count = cats.size();
		for(i = 0; i < count; ++i)
		{
			memcpy(pos, &type, sizeof(U8));
			pos += sizeof(U8);
			memcpy(pos, &(cats.at(i)->getUUID()), UUID_BYTES);
			pos += UUID_BYTES;
		}
		count = items.size();
		for(i = 0; i < count; ++i)
		{
			type = (U8)items.at(i)->getType();
			memcpy(pos, &type, sizeof(U8));
			pos += sizeof(U8);
			memcpy(pos, &(items.at(i)->getUUID()), UUID_BYTES);
			pos += UUID_BYTES;
		}
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			to_agent,
			name,
			cat->getName(),
			IM_ONLINE,
			IM_INVENTORY_OFFERED,
			transaction_id,
			0,
			LLUUID::null,
			gAgent.getPositionAgent(),
			NO_TIMESTAMP,
			bucket,
			bucket_size);
		gAgent.sendReliableMessage();
		delete[] bucket;
 		if (gSavedSettings.getBOOL("BroadcastViewerEffects"))
		{
			LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
			effectp->setSourceObject(gAgentAvatarp);
			effectp->setTargetObject(gObjectList.findObject(to_agent));
			effectp->setDuration(LL_HUD_DUR_SHORT);
			effectp->setColor(LLColor4U(gAgent.getEffectColor()));
		}
		gFloaterTools->dirty();
		LLMuteList::getInstance()->autoRemove(to_agent, LLMuteList::AR_INVENTORY);
		logInventoryOffer(to_agent, im_session_id);
	}
	return give_successful;
}
