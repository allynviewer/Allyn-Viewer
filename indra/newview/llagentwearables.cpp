/** 
 * @file llagentwearables.cpp
 * @brief LLAgentWearables class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llagentwearables.h"
#include "llattachmentsmgr.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearablesfetch.h"
#include "llappearancemgr.h"
#include "llattachmentsmgr.h"
#include "llcallbacklist.h"
#include "llfloatercustomize.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "lllocaltextureobject.h"
#include "llpanelmaininventory.h"
#include "llmd5.h"
#include "llnotificationsutil.h"
#include "lloutfitobserver.h"
#include "lltexlayer.h"
#include "lltooldraganddrop.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llviewerwearable.h"
#include "llwearablelist.h"
#include "lllocaltextureobject.h"
#include "llfloaterperms.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
#include "hippogridmanager.h"
#include <boost/scoped_ptr.hpp>
LLAgentWearables gAgentWearables;
BOOL LLAgentWearables::mInitialWearablesUpdateReceived = FALSE;
bool LLAgentWearables::mInitialWearablesLoaded = false;
bool LLAgentWearables::mInitialAttachmentsRequested = false;
using namespace LLAvatarAppearanceDefines;
void set_default_permissions(LLViewerInventoryItem* item)
{
	llassert(item);
	LLPermissions perm = item->getPermissions();
	if (perm.getMaskNextOwner() != LLFloaterPerms::getNextOwnerPerms("Wearables")
		|| perm.getMaskEveryone() != LLFloaterPerms::getEveryonePerms("Wearables")
		|| perm.getMaskGroup() != LLFloaterPerms::getGroupPerms("Wearables"))
	{
		perm.setMaskNext(LLFloaterPerms::getNextOwnerPerms("Wearables"));
		perm.setMaskEveryone(LLFloaterPerms::getEveryonePerms("Wearables"));
		perm.setMaskGroup(LLFloaterPerms::getGroupPerms("Wearables"));
		item->setPermissions(perm);
		item->updateServer(FALSE);
	}
}
void wear_and_edit_cb(const LLUUID& inv_item)
{
	if (inv_item.isNull()) return;
	LLViewerInventoryItem* item = gInventory.getItem(inv_item);
	if (!item) return;
	set_default_permissions(item);
	gInventory.updateItem(item);
	gInventory.notifyObservers();
	gAgentWearables.requestEditingWearable(inv_item);
	LLAppearanceMgr::instance().wearItemOnAvatar(inv_item,true);
}
void wear_cb(const LLUUID& inv_item)
{
	if (!inv_item.isNull())
	{
		LLViewerInventoryItem* item = gInventory.getItem(inv_item);
		if (item)
		{
			set_default_permissions(item);
			gInventory.updateItem(item);
			gInventory.notifyObservers();
		}
	}
}
void checkWearableAgainstInventory(LLViewerWearable *wearable)
{
	if (wearable->getItemID().isNull())
		return;
	LLViewerInventoryItem *item = gInventory.getItem(wearable->getItemID());
	if (item)
	{
		if (!item->isWearableType())
		{
			LL_WARNS() << "wearable associated with non-wearable item" << LL_ENDL;
		}
		if (item->getWearableType() != wearable->getType())
		{
			LL_WARNS() << "type mismatch: wearable " << wearable->getName()
					<< " has type " << wearable->getType()
					<< " but inventory item " << item->getName()
					<< " has type "  << item->getWearableType() << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS() << "wearable inventory item not found" << wearable->getName()
				<< " itemID " << wearable->getItemID().asString() << LL_ENDL;
	}
}
void LLAgentWearables::dump()
{
	LL_INFOS() << "LLAgentWearablesDump" << LL_ENDL;
	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		U32 count = getWearableCount((LLWearableType::EType)i);
		LL_INFOS() << "Type: " << i << " count " << count << LL_ENDL;
		for (U32 j=0; j<count; j++)
		{
			LLViewerWearable* wearable = getViewerWearable((LLWearableType::EType)i,j);
			if (wearable == NULL)
			{
				LL_INFOS() << "    " << j << " NULL wearable" << LL_ENDL;
			}
			LL_INFOS() << "    " << j << " Name " << wearable->getName()
					<< " description " << wearable->getDescription() << LL_ENDL;
		}
	}
}
struct LLAgentDumper
{
	LLAgentDumper(std::string name):
		mName(std::move(name))
	{
		LL_INFOS() << LL_ENDL;
		LL_INFOS() << "LLAgentDumper " << mName << LL_ENDL;
		gAgentWearables.dump();
	}
	~LLAgentDumper()
	{
		LL_INFOS() << LL_ENDL;
		LL_INFOS() << "~LLAgentDumper " << mName << LL_ENDL;
		gAgentWearables.dump();
	}
	std::string mName;
};
LLAgentWearables::LLAgentWearables() :
	LLWearableData(),
	mWearablesLoaded(FALSE)
,	mCOFChangeInProgress(false)
{
}
LLAgentWearables::~LLAgentWearables()
{
	cleanup();
}
void LLAgentWearables::cleanup()
{
}
void LLAgentWearables::initClass()
{
	LLOutfitObserver::instance().addCOFSavedCallback(boost::bind(&LLAgentWearables::notifyLoadingFinished, &gAgentWearables));
}
void LLAgentWearables::setAvatarObject(LLVOAvatarSelf *avatar)
{
	llassert(avatar);
	if (avatar)
	{
		if(!gHippoGridManager->getConnectedGrid()->isSecondLife())
		{
			avatar->outputRezTiming("Sending wearables request");
			gMessageSystem->newMessageFast(_PREHASH_AgentWearablesRequest);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
			gAgent.sendReliableMessage();
		}
		setAvatarAppearance(avatar);
	}
}
LLAgentWearables::AddWearableToAgentInventoryCallback::AddWearableToAgentInventoryCallback(
	LLPointer<LLRefCount> cb, LLWearableType::EType type, U32 index, LLViewerWearable* wearable, U32 todo, const std::string description) :
	mType(type),
	mIndex(index),
	mWearable(wearable),
	mTodo(todo),
	mCB(std::move(cb)),
	mDescription(description)
{
	LL_INFOS() << "constructor" << LL_ENDL;
}
void LLAgentWearables::AddWearableToAgentInventoryCallback::fire(const LLUUID& inv_item)
{
	if (inv_item.isNull())
		return;
	gAgentWearables.addWearabletoAgentInventoryDone(mType, mIndex, inv_item, mWearable);
	if (mTodo & CALL_MAKENEWOUTFITDONE)
	{
		gAgentWearables.makeNewOutfitDone(mType, mIndex);
	}
	if (mTodo & CALL_WEARITEM)
	{
		LLAppearanceMgr::instance().addCOFItemLink(inv_item,
			new LLUpdateAppearanceOnDestroy(true, true, boost::bind(&edit_wearable_and_customize_avatar, inv_item)), mDescription);
	}
}
void LLAgentWearables::addWearabletoAgentInventoryDone(const LLWearableType::EType type,
													   const U32 index,
													   const LLUUID& item_id,
													   LLViewerWearable* wearable)
{
	LL_INFOS() << "type " << type << " index " << index << " item " << item_id.asString() << LL_ENDL;
	if (item_id.isNull())
		return;
	LLUUID old_item_id = getWearableItemID(type,index);
	if (wearable)
	{
		wearable->setItemID(item_id);
		if (old_item_id.notNull())
		{
			gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
			setWearable(type,index,wearable);
		}
		else
		{
			pushWearable(type,wearable);
		}
	}
	gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
	LLViewerInventoryItem* item = gInventory.getItem(item_id);
	if (item && wearable)
	{
		item->setAssetUUID(wearable->getAssetID());
		item->setTransactionID(wearable->getTransactionID());
		gInventory.addChangedMask(LLInventoryObserver::INTERNAL, item_id);
		item->updateServer(FALSE);
	}
	gInventory.notifyObservers();
}
void LLAgentWearables::saveWearable(const LLWearableType::EType type, const U32 index,
									const std::string new_name)
{
	LLViewerWearable* old_wearable = getViewerWearable(type, index);
	if(!old_wearable) return;
	bool name_changed = !new_name.empty() && (new_name != old_wearable->getName());
	if (name_changed || old_wearable->isDirty() || old_wearable->isOldVersion())
	{
		LLUUID old_item_id = old_wearable->getItemID();
		LLViewerWearable* new_wearable = LLWearableList::instance().createCopy(old_wearable);
		new_wearable->setItemID(old_item_id);
		setWearable(type,index,new_wearable);
		old_wearable->revertValues();
		LLInventoryItem* item = gInventory.getItem(old_item_id);
		if (item)
		{
			std::string item_name = item->getName();
			if (name_changed)
			{
				LL_INFOS() << "saveWearable changing name from "  << item->getName() << " to " << new_name << LL_ENDL;
				item_name = new_name;
			}
			LLPointer<LLViewerInventoryItem> template_item =
				new LLViewerInventoryItem(item->getUUID(),
										  item->getParentUUID(),
										  item->getPermissions(),
										  new_wearable->getAssetID(),
										  new_wearable->getAssetType(),
										  item->getInventoryType(),
										  item_name,
										  item->getDescription(),
										  item->getSaleInfo(),
										  item->getFlags(),
										  item->getCreationDate());
			template_item->setTransactionID(new_wearable->getTransactionID());
			update_inventory_item(template_item, gAgentAvatarp->mEndCustomizeCallback);
		}
		else
		{
			U32 todo = AddWearableToAgentInventoryCallback::CALL_NONE;
			LLPointer<LLInventoryCallback> cb =
				new AddWearableToAgentInventoryCallback(
					LLPointer<LLRefCount>(NULL),
					type,
					index,
					new_wearable,
					todo);
			addWearableToAgentInventory(cb, new_wearable);
			return;
		}
		gAgentAvatarp->wearableUpdated( type, TRUE );
	}
}
LLViewerWearable* LLAgentWearables::saveWearableAs(const LLWearableType::EType type,
									  const U32 index,
									  const std::string& new_name,
									  const std::string& new_description,
									  BOOL save_in_lost_and_found)
{
	if (!isWearableCopyable(type, index))
	{
		LL_WARNS() << "LLAgent::saveWearableAs() not copyable." << LL_ENDL;
		return nullptr;
	}
	LLViewerWearable* old_wearable = getViewerWearable(type, index);
	if (!old_wearable)
	{
		LL_WARNS() << "LLAgent::saveWearableAs() no old wearable." << LL_ENDL;
		return nullptr;
	}
	LLInventoryItem* item = gInventory.getItem(getWearableItemID(type,index));
	if (!item)
	{
		LL_WARNS() << "LLAgent::saveWearableAs() no inventory item." << LL_ENDL;
		return nullptr;
	}
	std::string trunc_name(new_name);
	LLStringUtil::truncate(trunc_name, DB_INV_ITEM_NAME_STR_LEN);
	std::string trunc_description(new_description);
	LLStringUtil::truncate(trunc_description, DB_INV_ITEM_DESC_STR_LEN);
	LLViewerWearable* new_wearable = LLWearableList::instance().createCopy(
		old_wearable,
		trunc_name,
		trunc_description);
	LLPointer<LLInventoryCallback> cb =
		new AddWearableToAgentInventoryCallback(
			LLPointer<LLRefCount>(NULL),
			type,
			index,
			new_wearable,
			AddWearableToAgentInventoryCallback::CALL_WEARITEM,
			trunc_description
			);
	LLUUID category_id;
	if (save_in_lost_and_found)
	{
		category_id = gInventory.findCategoryUUIDForType(
			LLFolderType::FT_LOST_AND_FOUND);
	}
	else
	{
		category_id = item->getParentUUID();
	}
	copy_inventory_item(
		gAgent.getID(),
		item->getPermissions().getOwner(),
		item->getUUID(),
		category_id,
		new_name,
		cb);
	old_wearable->revertValues();
	return new_wearable;
}
void LLAgentWearables::revertWearable(const LLWearableType::EType type, const U32 index)
{
	LLViewerWearable* wearable = getViewerWearable(type, index);
	llassert(wearable);
	if (wearable)
	{
		wearable->revertValues();
	}
	gAgent.sendAgentSetAppearance();
}
void LLAgentWearables::saveAllWearables()
{
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((LLWearableType::EType)i); j++)
			saveWearable((LLWearableType::EType)i, j);
	}
}
void LLAgentWearables::setWearableName(const LLUUID& item_id, const std::string& new_name)
{
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			LLUUID curr_item_id = getWearableItemID((LLWearableType::EType)i,j);
			if (curr_item_id == item_id)
			{
				LLViewerWearable* old_wearable = getViewerWearable((LLWearableType::EType)i,j);
				llassert(old_wearable);
				if (!old_wearable) continue;
				std::string old_name = old_wearable->getName();
				old_wearable->setName(new_name);
				LLViewerWearable* new_wearable = LLWearableList::instance().createCopy(old_wearable);
				new_wearable->setItemID(item_id);
				LLInventoryItem* item = gInventory.getItem(item_id);
				if (item)
				{
					new_wearable->setPermissions(item->getPermissions());
				}
				old_wearable->setName(old_name);
				setWearable((LLWearableType::EType)i,j,new_wearable);
				break;
			}
		}
	}
}
void LLAgentWearables::nameOrDescriptionChanged(LLUUID const& item_id)
{
	for (int i = 0; i < LLWearableType::WT_COUNT; ++i)
	{
		LLWearableType::EType type = (LLWearableType::EType)i;
		for (U32 j = 0; j < getWearableCount(type); ++j)
		{
			LLUUID curr_item_id = getWearableItemID(type, j);
			if (curr_item_id == item_id)
			{
				LLViewerWearable* wearable = getViewerWearable(type, j);
				llassert(wearable);
				if (!wearable) continue;
				wearable->refreshNameAndDescription();
				if (LLFloaterCustomize::instanceExists())
				{
					LLFloaterCustomize::getInstance()->wearablesChanged(type);
				}
				return;
			}
		}
	}
}
BOOL LLAgentWearables::isWearableModifiable(LLWearableType::EType type, U32 index) const
{
	LLUUID item_id = getWearableItemID(type, index);
	return item_id.notNull() ? isWearableModifiable(item_id) : FALSE;
}
BOOL LLAgentWearables::isWearableModifiable(const LLUUID& item_id) const
{
	const LLUUID& linked_id = gInventory.getLinkedItemID(item_id);
	if (linked_id.notNull())
	{
		LLInventoryItem* item = gInventory.getItem(linked_id);
		if (item && item->getPermissions().allowModifyBy(gAgent.getID(),
														gAgent.getGroupID()))
		{
			return TRUE;
		}
	}
	return FALSE;
}
BOOL LLAgentWearables::isWearableCopyable(LLWearableType::EType type, U32 index) const
{
	LLUUID item_id = getWearableItemID(type, index);
	if (!item_id.isNull())
	{
		LLInventoryItem* item = gInventory.getItem(item_id);
		if (item && item->getPermissions().allowCopyBy(gAgent.getID(),
													  gAgent.getGroupID()))
		{
			return TRUE;
		}
	}
	return FALSE;
}
LLInventoryItem* LLAgentWearables::getWearableInventoryItem(LLWearableType::EType type, U32 index)
{
	LLUUID item_id = getWearableItemID(type,index);
	LLInventoryItem* item = NULL;
	if (item_id.notNull())
	{
		 item = gInventory.getItem(item_id);
	}
	return item;
}
const LLViewerWearable* LLAgentWearables::getWearableFromItemID( const LLUUID& item_id ) const
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			const LLViewerWearable * curr_wearable = getViewerWearable((LLWearableType::EType)i, j);
			if (curr_wearable && (curr_wearable->getItemID() == base_item_id))
			{
				return curr_wearable;
			}
		}
	}
	return NULL;
}
LLViewerWearable* LLAgentWearables::getWearableFromItemID(const LLUUID& item_id)
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			LLViewerWearable * curr_wearable = getViewerWearable((LLWearableType::EType)i, j);
			if (curr_wearable && (curr_wearable->getItemID() == base_item_id))
			{
				return curr_wearable;
			}
		}
	}
	return NULL;
}
LLViewerWearable*	LLAgentWearables::getWearableFromAssetID(const LLUUID& asset_id)
{
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			LLViewerWearable * curr_wearable = getViewerWearable((LLWearableType::EType)i, j);
			if (curr_wearable && (curr_wearable->getAssetID() == asset_id))
			{
				return curr_wearable;
			}
		}
	}
	return NULL;
}
LLViewerWearable* LLAgentWearables::getViewerWearable(const LLWearableType::EType type, U32 index )
{
	return dynamic_cast<LLViewerWearable*> (getWearable(type, index));
}
const LLViewerWearable* LLAgentWearables::getViewerWearable(const LLWearableType::EType type, U32 index ) const
{
	return dynamic_cast<const LLViewerWearable*> (getWearable(type, index));
}
BOOL LLAgentWearables::selfHasWearable(LLWearableType::EType type)
{
	return (gAgentWearables.getWearableCount(type) > 0);
}
void LLAgentWearables::wearableUpdated(LLWearable *wearable, BOOL removed)
{
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->wearableUpdated(wearable->getType(), removed);
	}
	LLWearableData::wearableUpdated(wearable, removed);
	if (!removed)
	{
		LLViewerWearable* viewer_wearable = dynamic_cast<LLViewerWearable*>(wearable);
		viewer_wearable->refreshNameAndDescription();
		if( wearable->getDefinitionVersion() == 24 )
		{
			U32 index;
			if (getWearableIndex(wearable,index))
			{
				LL_INFOS() << "forcing wearable type " << wearable->getType() << " to version 22 from 24" << LL_ENDL;
				wearable->setDefinitionVersion(22);
				saveWearable(wearable->getType(),index);
			}
		}
		if( LLFloaterCustomize::instanceExists() )
			LLFloaterCustomize::getInstance()->wearablesChanged(wearable->getType());
		checkWearableAgainstInventory(viewer_wearable);
	}
}
const LLUUID LLAgentWearables::getWearableItemID(LLWearableType::EType type, U32 index) const
{
	const LLViewerWearable *wearable = getViewerWearable(type,index);
	if (wearable)
		return wearable->getItemID();
	else
		return LLUUID();
}
void LLAgentWearables::getWearableItemIDs(uuid_vec_t& idItems) const
{
	for (wearableentry_map_t::const_iterator itWearableType = mWearableDatas.begin();
			itWearableType != mWearableDatas.end(); ++itWearableType)
	{
		getWearableItemIDs(itWearableType->first, idItems);
	}
}
void LLAgentWearables::getWearableItemIDs(LLWearableType::EType eType, uuid_vec_t& idItems) const
{
	wearableentry_map_t::const_iterator itWearableType = mWearableDatas.find(eType);
	if (mWearableDatas.end() != itWearableType)
	{
		for (wearableentry_vec_t::const_iterator itWearable = itWearableType->second.begin();
				itWearable != itWearableType->second.end(); ++itWearable)
		{
			const LLViewerWearable* pWearable = dynamic_cast<LLViewerWearable*>(*itWearable);
			if (pWearable)
			{
				idItems.push_back(pWearable->getItemID());
			}
		}
	}
}
const LLUUID LLAgentWearables::getWearableAssetID(LLWearableType::EType type, U32 index) const
{
	const LLViewerWearable *wearable = getViewerWearable(type,index);
	if (wearable)
		return wearable->getAssetID();
	else
		return LLUUID();
}
BOOL LLAgentWearables::isWearingItem(const LLUUID& item_id) const
{
	return getWearableFromItemID(item_id) != NULL;
}
void LLAgentWearables::addLocalTextureObject(const LLWearableType::EType wearable_type, const LLAvatarAppearanceDefines::ETextureIndex texture_type, U32 wearable_index)
{
	LLViewerWearable* wearable = getViewerWearable((LLWearableType::EType)wearable_type, wearable_index);
	if (!wearable)
	{
		LL_ERRS() << "Tried to add local texture object to invalid wearable with type " << wearable_type << " and index " << wearable_index << LL_ENDL;
		return;
	}
	LLLocalTextureObject lto;
	wearable->setLocalTextureObject(texture_type, lto);
}
class OnWearableItemCreatedCB: public LLInventoryCallback
{
public:
	OnWearableItemCreatedCB():
		mWearablesAwaitingItems(LLWearableType::WT_COUNT,NULL)
	{
		LL_INFOS() << "created callback" << LL_ENDL;
	}
	void fire(const LLUUID& inv_item)
	{
		LL_INFOS() << "One item created " << inv_item.asString() << LL_ENDL;
		LLConstPointer<LLInventoryObject> item = gInventory.getItem(inv_item);
		mItemsToLink.push_back(item);
		updatePendingWearable(inv_item);
	}
	~OnWearableItemCreatedCB()
	{
		LL_INFOS() << "All items created" << LL_ENDL;
		LLPointer<LLInventoryCallback> link_waiter = new LLUpdateAppearanceOnDestroy;
		link_inventory_array(LLAppearanceMgr::instance().getCOF(),
							 mItemsToLink,
							 link_waiter);
	}
	void addPendingWearable(LLViewerWearable *wearable)
	{
		if (!wearable)
		{
			LL_WARNS() << "no wearable" << LL_ENDL;
			return;
		}
		LLWearableType::EType type = wearable->getType();
		if (type<LLWearableType::WT_COUNT)
		{
			mWearablesAwaitingItems[type] = wearable;
		}
		else
		{
			LL_WARNS() << "invalid type " << type << LL_ENDL;
		}
	}
	void updatePendingWearable(const LLUUID& inv_item)
	{
		LLViewerInventoryItem *item = gInventory.getItem(inv_item);
		if (!item)
		{
			LL_WARNS() << "no item found" << LL_ENDL;
			return;
		}
		if (!item->isWearableType())
		{
			LL_WARNS() << "non-wearable item found" << LL_ENDL;
			return;
		}
		if (item && item->isWearableType())
		{
			LLWearableType::EType type = item->getWearableType();
			if (type < LLWearableType::WT_COUNT)
			{
				LLViewerWearable *wearable = mWearablesAwaitingItems[type];
				if (wearable)
					wearable->setItemID(inv_item);
			}
			else
			{
				LL_WARNS() << "invalid wearable type " << type << LL_ENDL;
			}
		}
	}
private:
	LLInventoryObject::const_object_list_t mItemsToLink;
	std::vector<LLViewerWearable*> mWearablesAwaitingItems;
};
void LLAgentWearables::createStandardWearables()
{
	LL_WARNS() << "Creating standard wearables" << LL_ENDL;
	if (!isAgentAvatarValid()) return;
	const BOOL create[LLWearableType::WT_COUNT] =
		{
			TRUE,
			TRUE,
			TRUE,
			TRUE,
			TRUE,
			TRUE,
			TRUE,
			TRUE,
			FALSE,
			FALSE,
			TRUE,
			TRUE,
			FALSE,
			FALSE,
			FALSE,
			FALSE
	};
	LLPointer<LLInventoryCallback> cb = new OnWearableItemCreatedCB;
	for (S32 i=0; i < LLWearableType::WT_COUNT; i++)
	{
		if (create[i])
		{
			llassert(getWearableCount((LLWearableType::EType)i) == 0);
			LLViewerWearable* wearable = LLWearableList::instance().createNewWearable((LLWearableType::EType)i, gAgentAvatarp);
			((OnWearableItemCreatedCB*)(&(*cb)))->addPendingWearable(wearable);
			LLUUID category_id = LLUUID::null;
			create_inventory_item(gAgent.getID(),
								  gAgent.getSessionID(),
								  category_id,
								  wearable->getTransactionID(),
								  wearable->getName(),
								  wearable->getDescription(),
								  wearable->getAssetType(),
								  LLInventoryType::IT_WEARABLE,
								  wearable->getType(),
								  wearable->getPermissions().getMaskNextOwner(),
								  cb);
		}
	}
}
void LLAgentWearables::sendDummyAgentWearablesUpdate()
{
	LL_DEBUGS("Avatar") << "sendAgentWearablesUpdate()" << LL_ENDL;
	gMessageSystem->newMessageFast(_PREHASH_AgentIsNowWearing);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_WearableData);
	gMessageSystem->addU8Fast(_PREHASH_WearableType, U8(1));
	gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID("db5a4e5f-9da3-44c8-992d-1181c5795498"));
	gMessageSystem->nextBlockFast(_PREHASH_WearableData);
	gMessageSystem->addU8Fast(_PREHASH_WearableType, U8(2));
	gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID("6969c7cc-f72f-4a76-a19b-c293cce8ce4f"));
	gMessageSystem->nextBlockFast(_PREHASH_WearableData);
	gMessageSystem->addU8Fast(_PREHASH_WearableType, U8(3));
	gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID("7999702b-b291-48f9-8903-c91dfb828408"));
	gMessageSystem->nextBlockFast(_PREHASH_WearableData);
	gMessageSystem->addU8Fast(_PREHASH_WearableType, U8(4));
	gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID("566cb59e-ef60-41d7-bfa6-e0f293fbea40"));
	gAgent.sendReliableMessage();
}
void LLAgentWearables::makeNewOutfitDone(S32 type, U32 index)
{
	LLUUID first_item_id = getWearableItemID((LLWearableType::EType)type, index);
	if (first_item_id.notNull())
	{
		LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();
		if (active_panel)
		{
			active_panel->setSelection(first_item_id, TAKE_FOCUS_NO);
		}
	}
}
void LLAgentWearables::addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
												   LLViewerWearable* wearable,
												   const LLUUID& category_id,
												   BOOL notify)
{
	create_inventory_item(gAgent.getID(),
						  gAgent.getSessionID(),
						  category_id,
						  wearable->getTransactionID(),
						  wearable->getName(),
						  wearable->getDescription(),
						  wearable->getAssetType(),
						  LLInventoryType::IT_WEARABLE,
						  wearable->getType(),
						  wearable->getPermissions().getMaskNextOwner(),
						  cb);
}
void LLAgentWearables::removeWearable(const LLWearableType::EType type, bool do_remove_all, U32 index)
{
	if (gAgent.isTeen() &&
		(type == LLWearableType::WT_UNDERSHIRT || type == LLWearableType::WT_UNDERPANTS))
	{
		return;
	}
	if (getWearableCount(type) == 0)
	{
		return;
	}
	if (do_remove_all)
	{
		removeWearableFinal(type, do_remove_all, index);
	}
	else
	{
		LLViewerWearable* old_wearable = getViewerWearable(type,index);
		if (old_wearable)
		{
			if (old_wearable->isDirty())
			{
				LLSD payload;
				payload["wearable_type"] = (S32)type;
				payload["wearable_index"] = (S32)index;
				LLNotificationsUtil::add("WearableSave", LLSD(), payload, &LLAgentWearables::onRemoveWearableDialog);
				return;
			}
			else
			{
				removeWearableFinal(type, do_remove_all, index);
			}
		}
	}
}
bool LLAgentWearables::onRemoveWearableDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLWearableType::EType type = (LLWearableType::EType)notification["payload"]["wearable_type"].asInteger();
	S32 index = (S32)notification["payload"]["wearable_index"].asInteger();
	switch(option)
	{
		case 0:
			gAgentWearables.saveWearable(type, index);
			gAgentWearables.removeWearableFinal(type, false, index);
			break;
		case 1:
			gAgentWearables.removeWearableFinal(type, false, index);
			break;
	case 2:
		break;
	default:
		llassert(0);
		break;
	}
	return false;
}
void LLAgentWearables::removeWearableFinal( LLWearableType::EType type, bool do_remove_all, U32 index)
{
	if (do_remove_all)
	{
		S32 max_entry = getWearableCount(type)-1;
		for (S32 i=max_entry; i>=0; i--)
		{
			LLViewerWearable* old_wearable = getViewerWearable(type,i);
			if (old_wearable)
			{
				eraseWearable(old_wearable);
				old_wearable->removeFromAvatar(TRUE);
			}
		}
		RLV_VERIFY(0 == getWearableCount(type));
	}
	else
	{
		LLViewerWearable* old_wearable = getViewerWearable(type, index);
		if (old_wearable)
		{
			eraseWearable(old_wearable);
			old_wearable->removeFromAvatar(TRUE);
		}
	}
	gInventory.notifyObservers();
}
void LLAgentWearables::setWearableOutfit(const LLInventoryItem::item_array_t& items,
										 const std::vector< LLViewerWearable* >& wearables)
{
	LL_INFOS() << "setWearableOutfit() start" << LL_ENDL;
	S32 count = wearables.size();
	llassert(items.size() == count);
	S32 matched = 0, mismatched = 0;
	const S32 arr_size = LLWearableType::WT_COUNT;
	S32 type_counts[arr_size];
	BOOL update_inventory = FALSE;
	std::fill(type_counts,type_counts+arr_size,0);
	for (S32 i = 0; i < count; i++)
	{
		LLViewerWearable* new_wearable = wearables[i];
		LLPointer<LLInventoryItem> new_item = items[i];
		const LLWearableType::EType type = new_wearable->getType();
		if (type < 0 || type>=LLWearableType::WT_COUNT)
		{
			LL_WARNS() << "invalid type " << type << LL_ENDL;
			mismatched++;
			continue;
		}
		S32 index = type_counts[type];
		type_counts[type]++;
		LLViewerWearable *curr_wearable = dynamic_cast<LLViewerWearable*>(getWearable(type,index));
		if (!new_wearable || !curr_wearable ||
			new_wearable->getAssetID() != curr_wearable->getAssetID())
		{
			LL_DEBUGS("Avatar") << "mismatch, type " << type << " index " << index
								<< " names " << (curr_wearable ? curr_wearable->getName() : "NONE")  << ","
								<< " names " << (new_wearable ? new_wearable->getName() : "NONE")  << LL_ENDL;
			mismatched++;
			continue;
		}
		if (curr_wearable->getName() != new_item->getName() ||
			curr_wearable->getItemID() != new_item->getUUID())
		{
			LL_DEBUGS("Avatar") << "mismatch on name or inventory id, names "
								<< curr_wearable->getName() << " vs " << new_item->getName()
								<< " item ids " << curr_wearable->getItemID() << " vs " << new_item->getUUID()
								<< LL_ENDL;
			update_inventory = TRUE;
			continue;
		}
		matched++;
	}
	LL_DEBUGS("Avatar") << "matched " << matched << " mismatched " << mismatched << LL_ENDL;
	for (S32 j=0; j<LLWearableType::WT_COUNT; j++)
	{
		LLWearableType::EType type = (LLWearableType::EType) j;
		if (getWearableCount(type) != type_counts[j])
		{
			LL_DEBUGS("Avatar") << "count mismatch for type " << j << " current " << getWearableCount(j) << " requested " << type_counts[j] << LL_ENDL;
			mismatched++;
		}
	}
	if (mismatched == 0 && !update_inventory)
	{
		LL_DEBUGS("Avatar") << "no changes, bailing out" << LL_ENDL;
		notifyLoadingFinished();
		return;
	}
	for (S32 j = 0; j < (S32)LLWearableType::WT_COUNT; j++)
	{
		if (LLWearableType::getAssetType((LLWearableType::EType)j) == LLAssetType::AT_CLOTHING)
		{
			removeWearable((LLWearableType::EType)j, true, 0);
		}
	}
	for (S32 i = 0; i < count; i++)
	{
		LLViewerWearable* new_wearable = wearables[i];
		LLPointer<LLInventoryItem> new_item = items[i];
		llassert(new_wearable);
		if (new_wearable)
		{
			const LLWearableType::EType type = new_wearable->getType();
			LLUUID old_wearable_id = new_wearable->getItemID();
			new_wearable->setName(new_item->getName());
			new_wearable->setItemID(new_item->getUUID());
			if (LLWearableType::getAssetType(type) == LLAssetType::AT_BODYPART)
			{
				setWearable(type,0,new_wearable);
				if (old_wearable_id.notNull())
				{
					gInventory.addChangedMask(LLInventoryObserver::LABEL, old_wearable_id);
				}
			}
			else
			{
				pushWearable(type,new_wearable);
			}
			const BOOL removed = FALSE;
			wearableUpdated(new_wearable, removed);
		}
	}
	gInventory.notifyObservers();
	if (mismatched == 0)
	{
		LL_DEBUGS("Avatar") << "inventory updated, wearable assets not changed, bailing out" << LL_ENDL;
		notifyLoadingFinished();
		return;
	}
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->setCompositeUpdatesEnabled(TRUE);
		if (!gAgentAvatarp->getIsCloud())
		{
			gAgentAvatarp->invalidateAll();
		}
	}
	mWearablesLoaded = TRUE;
	if (!mInitialWearablesLoaded)
	{
		mInitialWearablesLoaded = true;
		mInitialWearablesLoadedSignal();
	}
	notifyLoadingFinished();
	gAgentAvatarp->writeWearablesToAvatar();
	gAgentAvatarp->updateVisualParams();
	gAgentAvatarp->dumpAvatarTEs("setWearableOutfit");
	LL_DEBUGS("Avatar") << "setWearableOutfit() end" << LL_ENDL;
}
void LLAgentWearables::findAttachmentsAddRemoveInfo(LLInventoryModel::item_array_t& obj_item_array,
													llvo_vec_t& objects_to_remove,
													llvo_vec_t& objects_to_retain,
													LLInventoryModel::item_array_t& items_to_add)
{
	if (!isAgentAvatarValid()) return;
	uuid_set_t requested_item_ids;
	uuid_set_t current_item_ids;
	for (U32 i=0; i<obj_item_array.size(); i++)
	{
		const LLUUID & requested_id = obj_item_array[i].get()->getLinkedUUID();
		requested_item_ids.insert(requested_id);
	}
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
		 iter != gAgentAvatarp->mAttachmentPoints.end();)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			LLViewerObject *objectp = (*attachment_iter);
			if (objectp)
			{
				LLUUID object_item_id = objectp->getAttachmentItemID();
				bool remove_attachment = true;
				if (requested_item_ids.find(object_item_id) != requested_item_ids.end())
				{
					remove_attachment = false;
				}
				else if (objectp->isTempAttachment())
				{
					remove_attachment = LLAppearanceMgr::instance().shouldRemoveTempAttachment(objectp->getID());
				}
				if (remove_attachment)
				{
					objects_to_remove.push_back(objectp);
				}
				else
				{
					current_item_ids.insert(object_item_id);
					objects_to_retain.push_back(objectp);
				}
			}
		}
	}
	for (LLInventoryModel::item_array_t::iterator it = obj_item_array.begin();
		 it != obj_item_array.end();
		 ++it)
	{
		LLUUID linked_id = (*it).get()->getLinkedUUID();
		if (current_item_ids.find(linked_id) != current_item_ids.end())
		{
		}
		else
		{
			items_to_add.push_back(*it);
		}
	}
}
void LLAgentWearables::userRemoveMultipleAttachments(llvo_vec_t& objects_to_remove)
{
	if (!isAgentAvatarValid()) return;
	if ( (rlv_handler_t::isEnabled()) && (gRlvAttachmentLocks.hasLockedAttachmentPoint(RLV_LOCK_REMOVE)) )
	{
		llvo_vec_t::iterator itObj = objects_to_remove.begin();
		while (objects_to_remove.end() != itObj)
		{
			const LLViewerObject* pAttachObj = *itObj;
			if (gRlvAttachmentLocks.isLockedAttachment(pAttachObj))
			{
				itObj = objects_to_remove.erase(itObj);
				bool fInCOF = LLAppearanceMgr::instance().isLinkedInCOF(pAttachObj->getAttachmentItemID());
				RLV_ASSERT(fInCOF);
				if (!fInCOF)
				{
					LLAppearanceMgr::instance().registerAttachment(pAttachObj->getAttachmentItemID());
				}
			}
			else
			{
				++itObj;
			}
		}
	}
	if (objects_to_remove.empty())
		return;
	LL_DEBUGS("Avatar") << "ATT [ObjectDetach] removing " << objects_to_remove.size() << " objects" << LL_ENDL;
	gMessageSystem->newMessage("ObjectDetach");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	for (llvo_vec_t::iterator it = objects_to_remove.begin();
		 it != objects_to_remove.end();
		 ++it)
	{
		LLViewerObject *objectp = *it;
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, objectp->getLocalID());
		const LLUUID& item_id = objectp->getAttachmentItemID();
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LL_DEBUGS("Avatar") << "ATT removing object, item is " << (item ? item->getName() : "UNKNOWN") << " " << item_id << LL_ENDL;
        LLAttachmentsMgr::instance().onDetachRequested(item_id);
	}
	gMessageSystem->sendReliable(gAgent.getRegionHost());
}
void LLAgentWearables::userAttachMultipleAttachments(LLInventoryModel::item_array_t& obj_item_array)
{
	S32 obj_count = obj_item_array.size();
	if (obj_count > 0)
	{
		LL_DEBUGS("Avatar") << "ATT attaching multiple, total obj_count " << obj_count << LL_ENDL;
	}
    for(LLInventoryModel::item_array_t::const_iterator it = obj_item_array.begin();
        it != obj_item_array.end();
        ++it)
    {
		const LLInventoryItem* item = *it;
        LLAttachmentsMgr::instance().addAttachmentRequest(item->getLinkedUUID(), 0, TRUE);
    }
	mInitialAttachmentsRequested = true;
}
bool LLAgentWearables::canMoveWearable(const LLUUID& item_id, bool closer_to_body) const
{
	const LLWearable* wearable = getWearableFromItemID(item_id);
	if (!wearable) return false;
	LLWearableType::EType wtype = wearable->getType();
	const LLWearable* marginal_wearable = closer_to_body ? getBottomWearable(wtype) : getTopWearable(wtype);
	if (!marginal_wearable) return false;
	return wearable != marginal_wearable;
}
BOOL LLAgentWearables::areWearablesLoaded() const
{
	return mWearablesLoaded;
}
bool LLAgentWearables::canWearableBeRemoved(const LLViewerWearable* wearable) const
{
	if (!wearable) return false;
	LLWearableType::EType type = wearable->getType();
	return !(((type == LLWearableType::WT_SHAPE) || (type == LLWearableType::WT_SKIN) || (type == LLWearableType::WT_HAIR) || (type == LLWearableType::WT_EYES))
			 && (getWearableCount(type) <= 1) );
}
void LLAgentWearables::animateAllWearableParams(F32 delta, bool upload_bake)
{
	for( S32 type = 0; type < LLWearableType::WT_COUNT; ++type )
	{
		for (S32 count = 0; count < (S32)getWearableCount((LLWearableType::EType)type); ++count)
		{
			LLViewerWearable *wearable = getViewerWearable((LLWearableType::EType)type,count);
			llassert(wearable);
			if (wearable)
			{
				wearable->animateParams(delta, upload_bake);
			}
		}
	}
}
bool LLAgentWearables::moveWearable(const LLViewerInventoryItem* item, bool closer_to_body)
{
	if (!item) return false;
	if (!item->isWearableType()) return false;
	LLWearableType::EType type = item->getWearableType();
	U32 wearable_count = getWearableCount(type);
	if (0 == wearable_count) return false;
	const LLUUID& asset_id = item->getAssetUUID();
	if (closer_to_body)
	{
		LLViewerWearable* bottom_wearable = dynamic_cast<LLViewerWearable*>( getBottomWearable(type) );
		if (bottom_wearable->getAssetID() == asset_id)
		{
			return false;
		}
	}
	else
	{
		LLViewerWearable* top_wearable = dynamic_cast<LLViewerWearable*>( getTopWearable(type) );
		if (top_wearable->getAssetID() == asset_id)
		{
			return false;
		}
	}
	for (U32 i = 0; i < wearable_count; ++i)
	{
		LLViewerWearable* wearable = getViewerWearable(type, i);
		if (!wearable) continue;
		if (wearable->getAssetID() != asset_id) continue;
		U32 swap_i = closer_to_body ? i-1 : i+1;
		swapWearables(type, i, swap_i);
		if( LLFloaterCustomize::instanceExists() )
		{
			LLFloaterCustomize::getInstance()->wearablesChanged(item->getWearableType());
		}
		return true;
	}
	return false;
}
void LLAgentWearables::createWearable(LLWearableType::EType type, bool wear, const LLUUID& parent_id)
{
	if (type == LLWearableType::WT_INVALID || type == LLWearableType::WT_NONE) return;
	if (type == LLWearableType::WT_UNIVERSAL && !gAgent.getRegion()->bakesOnMeshEnabled())
	{
		LL_WARNS("Inventory") << "Can't create WT_UNIVERSAL type " << LL_ENDL;
		return;
	}
	LLViewerWearable* wearable = LLWearableList::instance().createNewWearable(type, gAgentAvatarp);
	LLAssetType::EType asset_type = wearable->getAssetType();
	LLInventoryType::EType inv_type = LLInventoryType::IT_WEARABLE;
	LLPointer<LLInventoryCallback> cb;
	if(wear)
	{
		cb = new LLBoostFuncInventoryCallback(wear_and_edit_cb);
	}
	else
	{
		cb = new LLBoostFuncInventoryCallback(wear_cb);
	}
	LLUUID folder_id;
	if (parent_id.notNull())
	{
		folder_id = parent_id;
	}
	else
	{
		LLFolderType::EType folder_type = LLFolderType::assetTypeToFolderType(asset_type);
		folder_id = gInventory.findCategoryUUIDForType(folder_type);
	}
	create_inventory_item(gAgent.getID(),
						  gAgent.getSessionID(),
						  folder_id,
						  wearable->getTransactionID(),
						  wearable->getName(),
						  wearable->getDescription(),
						  asset_type, inv_type,
						  wearable->getType(),
						  LLFloaterPerms::getNextOwnerPerms("Wearables"),
						  cb);
}
void LLAgentWearables::editWearable(const LLUUID& item_id)
{
	LLViewerInventoryItem* item = gInventory.getLinkedItem(item_id);
	if (!item)
	{
		LL_WARNS() << "Failed to get linked item" << LL_ENDL;
		return;
	}
	LLViewerWearable* wearable = gAgentWearables.getWearableFromItemID(item_id);
	if (!wearable)
	{
		LL_WARNS() << "Cannot get wearable" << LL_ENDL;
		return;
	}
	if (!gAgentWearables.isWearableModifiable(item->getUUID()))
	{
		LL_WARNS() << "Cannot modify wearable" << LL_ENDL;
		return;
	}
	const BOOL disable_camera_switch = LLWearableType::getDisableCameraSwitch(wearable->getType());
	LLFloaterCustomize::editWearable( wearable, disable_camera_switch );
}
void LLAgentWearables::requestEditingWearable(const LLUUID& item_id)
{
	mItemToEdit = gInventory.getLinkedItemID(item_id);
}
void LLAgentWearables::editWearableIfRequested(const LLUUID& item_id)
{
	if (mItemToEdit.notNull() &&
		mItemToEdit == gInventory.getLinkedItemID(item_id))
	{
		LLAgentWearables::editWearable(item_id);
		mItemToEdit.setNull();
	}
}
boost::signals2::connection LLAgentWearables::addLoadingStartedCallback(loading_started_callback_t cb)
{
	return mLoadingStartedSignal.connect(cb);
}
boost::signals2::connection LLAgentWearables::addLoadedCallback(loaded_callback_t cb)
{
	return mLoadedSignal.connect(cb);
}
bool LLAgentWearables::changeInProgress() const
{
	return mCOFChangeInProgress;
}
boost::signals2::connection LLAgentWearables::addInitialWearablesLoadedCallback(const loaded_callback_t& cb)
{
	return mInitialWearablesLoadedSignal.connect(cb);
}
void LLAgentWearables::notifyLoadingStarted()
{
	mCOFChangeInProgress = true;
	mCOFChangeTimer.reset();
	mLoadingStartedSignal();
}
void LLAgentWearables::notifyLoadingFinished()
{
	mCOFChangeInProgress = false;
	mLoadedSignal();
}
