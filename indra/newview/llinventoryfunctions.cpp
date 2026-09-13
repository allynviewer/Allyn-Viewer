/** 
 * @file llinventoryfunctions.cpp
 * @brief Implementation of the inventory view and associated stuff.
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
#include <utility>
#include "llinventoryfunctions.h"
#include "llagent.h"
#include "llagentwearables.h"
#include "llcallingcard.h"
#include "llinventorydefines.h"
#include "llsdserialize.h"
#include "llspinctrl.h"
#include "lltrans.h"
#include "llui.h"
#include "message.h"
#include "llappearancemgr.h"
#include "llappviewer.h"
#include "llfloaterinventory.h"
#include "llfloatermarketplacelistings.h"
#include "llfocusmgr.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "lliconctrl.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llinventoryclipboard.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "lllineeditor.h"
#include "llmarketplacenotifications.h"
#include "llmarketplacefunctions.h"
#include "llmenugl.h"
#include "llnotificationsutil.h"
#include "llpanelmaininventory.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llselectmgr.h"
#include "lltabcontainer.h"
#include "lltooldraganddrop.h"
#include "lluictrlfactory.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwearablelist.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
#include <boost/foreach.hpp>
BOOL LLInventoryState::sWearNewClothing = FALSE;
LLUUID LLInventoryState::sWearNewClothingTransactionID;
void update_folder_cb(const LLUUID& dest_folder)
{
	LLViewerInventoryCategory* dest_cat = gInventory.getCategory(dest_folder);
	gInventory.updateCategory(dest_cat);
	gInventory.notifyObservers();
}
S32 count_copyable_items(LLInventoryModel::item_array_t& items)
{
	S32 count = 0;
	for (LLInventoryModel::item_array_t::const_iterator it = items.begin(); it != items.end(); ++it)
	{
		LLViewerInventoryItem* item = *it;
		if (item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
		{
			count++;
		}
	}
	return count;
}
S32 count_stock_items(LLInventoryModel::item_array_t& items)
{
	S32 count = 0;
	for (LLInventoryModel::item_array_t::const_iterator it = items.begin(); it != items.end(); ++it)
	{
		LLViewerInventoryItem* item = *it;
		if (!item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
		{
			count++;
		}
	}
	return count;
}
S32 count_stock_folders(LLInventoryModel::cat_array_t& categories)
{
	S32 count = 0;
	for (LLInventoryModel::cat_array_t::const_iterator it = categories.begin(); it != categories.end(); ++it)
	{
		LLInventoryCategory* cat = *it;
		if (cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
		{
			count++;
		}
	}
	return count;
}
S32 count_descendants_items(const LLUUID& cat_id)
{
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat_id,cat_array,item_array);
	S32 count = item_array->size();
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLViewerInventoryCategory* category = *iter;
		count += count_descendants_items(category->getUUID());
	}
	return count;
}
bool contains_nocopy_items(const LLUUID& id)
{
	LLInventoryCategory* cat = gInventory.getCategory(id);
	if (cat)
	{
		LLInventoryModel::cat_array_t* cat_array;
		LLInventoryModel::item_array_t* item_array;
		gInventory.getDirectDescendentsOf(id,cat_array,item_array);
		for (LLInventoryModel::item_array_t::iterator iter = item_array->begin(); iter != item_array->end(); iter++)
		{
			LLInventoryItem* item = *iter;
			LLViewerInventoryItem * inv_item = (LLViewerInventoryItem *) item;
			if (!inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
			{
				return true;
			}
		}
		for (LLInventoryModel::cat_array_t::iterator iter = cat_array->begin(); iter != cat_array->end(); iter++)
		{
			LLViewerInventoryCategory* cat = *iter;
			if (contains_nocopy_items(cat->getUUID()))
			{
				return true;
			}
		}
	}
	else
	{
		LLInventoryItem* item = gInventory.getItem(id);
		LLViewerInventoryItem * inv_item = (LLViewerInventoryItem *) item;
		if (!inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
		{
			return true;
		}
	}
	return false;
}
void append_path(const LLUUID& id, std::string& path)
{
	std::string temp;
	const LLInventoryObject* obj = gInventory.getObject(id);
	LLUUID parent_id;
	if(obj) parent_id = obj->getParentUUID();
	std::string forward_slash("/");
	while(obj)
	{
		obj = gInventory.getCategory(parent_id);
		if(obj)
		{
			temp.assign(forward_slash + obj->getName() + temp);
			parent_id = obj->getParentUUID();
		}
	}
	path.append(temp);
}
void update_marketplace_folder_hierarchy(const LLUUID cat_id)
{
	gInventory.addChangedMask(LLInventoryObserver::LABEL, cat_id);
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat_id,cat_array,item_array);
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLInventoryCategory* category = *iter;
		update_marketplace_folder_hierarchy(category->getUUID());
	}
    return;
}
void update_marketplace_category(const LLUUID& cur_uuid, bool perform_consistency_enforcement)
{
    if (cur_uuid.isNull()
        || gInventory.getCategory(cur_uuid) == NULL
        || gInventory.getCategory(cur_uuid)->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN)
	{
		return;
	}
	S32 depth = depth_nesting_in_marketplace(cur_uuid);
	if (depth > 0)
	{
		LLUUID listing_uuid = nested_parent_id(cur_uuid, depth);
        LLViewerInventoryCategory* listing_cat = gInventory.getCategory(listing_uuid);
        bool listing_cat_loaded = listing_cat != NULL && listing_cat->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN;
        if (perform_consistency_enforcement
            && listing_cat_loaded
            && LLMarketplaceData::instance().isListed(listing_uuid))
		{
			LLUUID version_folder_uuid = LLMarketplaceData::instance().getVersionFolder(listing_uuid);
			S32 version_depth = depth_nesting_in_marketplace(version_folder_uuid);
			if (version_folder_uuid.notNull() && (!gInventory.isObjectDescendentOf(version_folder_uuid, listing_uuid) || (version_depth != 2)))
			{
				LL_INFOS("SLM") << "Unlist and clear version folder as the version folder is not at the right place anymore!!" << LL_ENDL;
				LLMarketplaceData::instance().setVersionFolder(listing_uuid, LLUUID::null,1);
			}
            else if (version_folder_uuid.notNull()
                     && gInventory.isCategoryComplete(version_folder_uuid)
                     && LLMarketplaceData::instance().getActivationState(version_folder_uuid)
                     && (count_descendants_items(version_folder_uuid) == 0)
                     && !LLMarketplaceData::instance().isUpdating(version_folder_uuid,version_depth))
			{
				LL_INFOS("SLM") << "Unlist as the version folder is empty of any item!!" << LL_ENDL;
				LLNotificationsUtil::add("AlertMerchantVersionFolderEmpty");
				LLMarketplaceData::instance().activateListing(listing_uuid, false,1);
			}
		}
        if (perform_consistency_enforcement
            && listing_cat_loaded
            && (compute_stock_count(listing_uuid) != LLMarketplaceData::instance().getCountOnHand(listing_uuid)))
		{
			LLMarketplaceData::instance().updateCountOnHand(listing_uuid,1);
		}
		update_marketplace_folder_hierarchy(listing_uuid);
	}
	else if (depth == 0)
	{
		if (gInventory.getCategory(cur_uuid))
		{
			update_marketplace_folder_hierarchy(cur_uuid);
		}
	}
	else
	{
		if (perform_consistency_enforcement && LLMarketplaceData::instance().isListed(cur_uuid))
		{
			LL_INFOS("SLM") << "Disassociate as the listing folder is not under the marketplace folder anymore!!" << LL_ENDL;
			LLMarketplaceData::instance().clearListing(cur_uuid);
		}
		if (gInventory.getCategory(cur_uuid))
		{
			update_marketplace_folder_hierarchy(cur_uuid);
		}
	}
    return;
}
void update_all_marketplace_count(const LLUUID& cat_id)
{
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat_id, cat_array, item_array);
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLInventoryCategory* category = *iter;
		if (category->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
		{
			update_marketplace_category(category->getUUID());
		}
		else
		{
			update_all_marketplace_count(category->getUUID());
		}
	}
}
void update_all_marketplace_count()
{
	const LLUUID marketplace_listings_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	if (!marketplace_listings_uuid.isNull())
	{
		update_all_marketplace_count(marketplace_listings_uuid);
	}
	return;
}
void rename_category(LLInventoryModel* model, const LLUUID& cat_id, const std::string& new_name)
{
	LLViewerInventoryCategory* cat;
	if (!model ||
		!get_is_category_renameable(model, cat_id) ||
		(cat = model->getCategory(cat_id)) == NULL ||
		cat->getName() == new_name)
	{
		return;
	}
	LLSD updates;
	updates["name"] = new_name;
	update_inventory_category(cat_id, updates, NULL);
}
void copy_inventory_category(LLInventoryModel* model,
							 LLViewerInventoryCategory* cat,
							 const LLUUID& parent_id,
							 const LLUUID& root_copy_id,
							 bool move_no_copy_items)
{
	LLUUID new_cat_uuid = gInventory.createNewCategory(parent_id, LLFolderType::FT_NONE, cat->getName());
	model->notifyObservers();
	LLUUID root_id = (root_copy_id.isNull() ? new_cat_uuid : root_copy_id);
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);
	if (root_copy_id.isNull())
	{
		LLMarketplaceData::instance().setValidationWaiting(root_id,count_descendants_items(cat->getUUID()));
	}
	LLInventoryModel::item_array_t item_array_copy = *item_array;
	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
		LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(update_folder_cb, new_cat_uuid));
		if (!item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
		{
			if (move_no_copy_items)
			{
				LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) item;
				gInventory.changeItemParent(viewer_inv_item, new_cat_uuid, true);
			}
			LLMarketplaceData::instance().decrementValidationWaiting(root_id);
		}
		else
		{
			copy_inventory_item(
						gAgent.getID(),
						item->getPermissions().getOwner(),
						item->getUUID(),
						new_cat_uuid,
						std::string(),
						cb);
		}
	}
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLViewerInventoryCategory* category = *iter;
		if (category->getUUID() != root_id)
		{
			copy_inventory_category(model, category, new_cat_uuid, root_id, move_no_copy_items);
		}
	}
}
class LLInventoryCollectAllItems : public LLInventoryCollectFunctor
{
public:
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
	{
		return true;
	}
};
BOOL get_is_parent_to_worn_item(const LLUUID& id)
{
	const LLViewerInventoryCategory* cat = gInventory.getCategory(id);
	if (!cat)
	{
		return FALSE;
	}
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLInventoryCollectAllItems collect_all;
	gInventory.collectDescendentsIf(LLAppearanceMgr::instance().getCOF(), cats, items, LLInventoryModel::EXCLUDE_TRASH, collect_all);
	for (LLInventoryModel::item_array_t::const_iterator it = items.begin(); it != items.end(); ++it)
	{
		const LLViewerInventoryItem * const item = *it;
		llassert(item->getIsLinkType());
		LLUUID linked_id = item->getLinkedUUID();
		const LLViewerInventoryItem * const linked_item = gInventory.getItem(linked_id);
		if (linked_item)
		{
			LLUUID parent_id = linked_item->getParentUUID();
			while (!parent_id.isNull())
			{
				LLInventoryCategory * parent_cat = gInventory.getCategory(parent_id);
				if (cat == parent_cat)
				{
					return TRUE;
				}
				parent_id = parent_cat->getParentUUID();
			}
		}
	}
	return FALSE;
}
BOOL get_is_item_worn(const LLUUID& id)
{
	const LLViewerInventoryItem* item = gInventory.getItem(id);
	if (!item)
		return FALSE;
	switch(item->getType())
	{
		case LLAssetType::AT_OBJECT:
		{
			if (isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(item->getLinkedUUID()))
				return TRUE;
			break;
		}
		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
			if(gAgentWearables.isWearingItem(item->getLinkedUUID()))
				return TRUE;
			break;
		case LLAssetType::AT_GESTURE:
			if (LLGestureMgr::instance().isGestureActive(item->getLinkedUUID()))
				return TRUE;
			break;
		default:
			break;
	}
	return FALSE;
}
BOOL get_can_item_be_worn(const LLUUID& id)
{
	const LLViewerInventoryItem* item = gInventory.getItem(id);
	if (!item)
		return FALSE;
	if (LLAppearanceMgr::instance().isLinkedInCOF(item->getLinkedUUID()))
	{
		return FALSE;
	}
	if (gInventory.isObjectDescendentOf(id, LLAppearanceMgr::instance().getCOF()))
	{
		return FALSE;
	}
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(
			LLFolderType::FT_TRASH);
	if (gInventory.isObjectDescendentOf(item->getLinkedUUID(),
			trash_id))
	{
		return false;
	}
	switch(item->getType())
	{
		case LLAssetType::AT_OBJECT:
		{
			if (isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(item->getLinkedUUID()))
			{
				return FALSE;
			}
			else
			{
				return TRUE;
			}
			break;
		}
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_CLOTHING:
			if(gAgentWearables.isWearingItem(item->getLinkedUUID()))
			{
				return FALSE;
			}
			else
			{
				return TRUE;
			}
			break;
		default:
			break;
	}
	return FALSE;
}
BOOL get_is_item_removable(const LLInventoryModel* model, const LLUUID& id)
{
	if (!model)
	{
		return FALSE;
	}
	if(id == gInventory.getRootFolderID())
	{
		return FALSE;
	}
	if(!model->isObjectDescendentOf(id, gInventory.getRootFolderID()))
	{
		return FALSE;
	}
	if (LLAppearanceMgr::instance().getIsProtectedCOFItem(id))
	{
		if (get_is_item_worn(id))
		{
			return FALSE;
		}
	}
	if ( (rlv_handler_t::isEnabled()) &&
		 (RlvFolderLocks::instance().hasLockedFolder(RLV_LOCK_ANY)) && (!RlvFolderLocks::instance().canRemoveItem(id)) )
	{
		return FALSE;
	}
	const LLInventoryObject *obj = model->getItem(id);
	if (obj && obj->getIsLinkType())
	{
		return TRUE;
	}
	if (get_is_item_worn(id))
	{
		return FALSE;
	}
	return TRUE;
}
BOOL get_is_category_removable(const LLInventoryModel* model, const LLUUID& id)
{
	if (!model)
	{
		return FALSE;
	}
	if (!model->isObjectDescendentOf(id, gInventory.getRootFolderID()))
	{
		return FALSE;
	}
	if ( ((rlv_handler_t::isEnabled()) &&
		 (RlvFolderLocks::instance().hasLockedFolder(RLV_LOCK_ANY)) && (!RlvFolderLocks::instance().canRemoveFolder(id))) )
	{
		return FALSE;
	}
	if (!isAgentAvatarValid()) return FALSE;
	const LLInventoryCategory* category = model->getCategory(id);
	if (!category)
	{
		return FALSE;
	}
	const LLFolderType::EType folder_type = category->getPreferredType();
#ifndef DELETE_SYSTEM_FOLDERS
	if (LLFolderType::lookupIsProtectedType(folder_type))
	{
		return FALSE;
	}
#endif
	if (folder_type == LLFolderType::FT_OUTFIT)
	{
		const LLViewerInventoryItem *base_outfit_link = LLAppearanceMgr::instance().getBaseOutfitLink();
		if (base_outfit_link && (category == base_outfit_link->getLinkedCategory()))
		{
			return FALSE;
		}
	}
	return TRUE;
}
BOOL get_is_category_renameable(const LLInventoryModel* model, const LLUUID& id)
{
	if (!model)
	{
		return FALSE;
	}
	if ( (rlv_handler_t::isEnabled()) && (model == &gInventory) && (!RlvFolderLocks::instance().canRenameFolder(id)) )
	{
		return FALSE;
	}
	LLViewerInventoryCategory* cat = model->getCategory(id);
	if (cat && !LLFolderType::lookupIsProtectedType(cat->getPreferredType()) &&
		cat->getOwnerID() == gAgent.getID())
	{
		return TRUE;
	}
	return FALSE;
}
void show_item_profile(const LLUUID& item_uuid)
{
	LLUUID linked_uuid = gInventory.getLinkedItemID(item_uuid);
	if(!LLFloaterProperties::show(linked_uuid, LLUUID::null))
	{
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PropertiesRect");
		rect.translate( left - rect.mLeft, top - rect.mTop );
		LLFloaterProperties* floater;
		floater = new LLFloaterProperties("item properties",
										rect,
										"Inventory Item Properties",
										linked_uuid,
										LLUUID::null);
		gFloaterView->adjustToFitScreen(floater, FALSE);
	}
}
void open_marketplace_listings()
{
	LLFloaterMarketplaceListings::showInstance();
}
S32 depth_nesting_in_marketplace(LLUUID cur_uuid)
{
	const LLUUID marketplace_listings_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	if (marketplace_listings_uuid.isNull())
	{
		return -1;
	}
	if (!gInventory.isObjectDescendentOf(cur_uuid, marketplace_listings_uuid))
	{
		return -1;
	}
	S32 depth = 0;
	LLInventoryObject* cur_object = gInventory.getObject(cur_uuid);
	while (cur_uuid != marketplace_listings_uuid)
	{
		depth++;
		cur_uuid = cur_object->getParentUUID();
		cur_object = gInventory.getCategory(cur_uuid);
	}
	return depth;
}
LLUUID nested_parent_id(LLUUID cur_uuid, S32 depth)
{
	if (depth < 1)
	{
		return LLUUID::null;
	}
	else if (depth == 1)
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(cur_uuid);
		return (cat ? cur_uuid : LLUUID::null);
	}
	LLInventoryObject* cur_object = gInventory.getObject(cur_uuid);
	while (depth > 1)
	{
		depth--;
		cur_uuid = cur_object->getParentUUID();
		cur_object = gInventory.getCategory(cur_uuid);
	}
	return cur_uuid;
}
S32 compute_stock_count(LLUUID cat_uuid, bool force_count )
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(cat_uuid);
	if (!cat)
	{
		return COMPUTE_STOCK_INFINITE;
	}
	if (cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
	{
		if (cat->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			return COMPUTE_STOCK_NOT_EVALUATED;
		}
		LLInventoryModel::cat_array_t* cat_array;
		LLInventoryModel::item_array_t* item_array;
		gInventory.getDirectDescendentsOf(cat_uuid,cat_array,item_array);
		return item_array->size();
	}
	if (!force_count)
	{
		S32 depth = depth_nesting_in_marketplace(cat_uuid);
		LLUUID listing_uuid = nested_parent_id(cat_uuid, depth);
		if (!LLMarketplaceData::instance().isListed(listing_uuid))
		{
			return COMPUTE_STOCK_INFINITE;
		}
		LLUUID version_folder_uuid = LLMarketplaceData::instance().getVersionFolder(listing_uuid);
		if (depth == 1)
		{
			if (version_folder_uuid.notNull())
			{
				return compute_stock_count(version_folder_uuid, true);
			}
			else
			{
				return COMPUTE_STOCK_INFINITE;
			}
		}
		else if (depth == 2)
		{
			if (version_folder_uuid.notNull() && (version_folder_uuid != cat_uuid))
			{
				return COMPUTE_STOCK_INFINITE;
			}
		}
	}
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat_uuid,cat_array,item_array);
	S32 curr_count = COMPUTE_STOCK_INFINITE;
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLInventoryCategory* category = *iter;
		S32 count = compute_stock_count(category->getUUID(), true);
		if ((curr_count == COMPUTE_STOCK_INFINITE) || ((count != COMPUTE_STOCK_INFINITE) && (count < curr_count)))
		{
			curr_count = count;
		}
	}
	return curr_count;
}
bool can_move_to_marketplace(LLInventoryItem* inv_item, std::string& tooltip_msg, bool resolve_links)
{
	LLViewerInventoryItem* viewer_inv_item = (LLViewerInventoryItem *) inv_item;
	LLViewerInventoryItem* linked_item = viewer_inv_item->getLinkedItem();
	LLViewerInventoryCategory* linked_category = viewer_inv_item->getLinkedCategory();
	if (linked_category || linked_item)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxLinked");
		return false;
	}
	if (linked_category != NULL)
	{
		return true;
	}
	if (linked_item != NULL)
	{
		inv_item = linked_item;
	}
	bool allow_transfer = inv_item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID());
	if (!allow_transfer)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxNoTransfer");
		return false;
	}
	bool worn = get_is_item_worn(inv_item->getUUID());
	if (worn)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxWorn");
		return false;
	}
	if (!gInventory.isObjectDescendentOf(inv_item->getUUID(), gInventory.getRootFolderID()))
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
		return false;
	}
	bool calling_card = (LLAssetType::AT_CALLINGCARD == inv_item->getType());
	if (calling_card)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxCallingCard");
		return false;
	}
	return true;
}
int get_folder_levels(LLInventoryCategory* inv_cat)
{
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(inv_cat->getUUID(), cats, items);
	int max_child_levels = 0;
	for (U32 i = 0; i < cats->size(); ++i)
	{
		LLInventoryCategory* category = cats->at(i);
		max_child_levels = llmax(max_child_levels, get_folder_levels(category));
	}
	return 1 + max_child_levels;
}
int get_folder_path_length(const LLUUID& ancestor_id, const LLUUID& descendant_id)
{
	int depth = 0;
	if (ancestor_id == descendant_id) return depth;
	const LLInventoryCategory* category = gInventory.getCategory(descendant_id);
	while (category)
	{
		LLUUID parent_id = category->getParentUUID();
		if (parent_id.isNull()) break;
		depth++;
		if (parent_id == ancestor_id) return depth;
		category = gInventory.getCategory(parent_id);
	}
	LL_WARNS("SLM") << "get_folder_path_length() couldn't trace a path from the descendant to the ancestor" << LL_ENDL;
	return -1;
}
bool has_correct_permissions_for_sale(LLInventoryCategory* cat, std::string& error_msg)
{
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);
	LLInventoryModel::item_array_t item_array_copy = *item_array;
	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
		if (!can_move_to_marketplace(item, error_msg, false))
		{
			return false;
		}
	}
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLInventoryCategory* category = *iter;
		if (!has_correct_permissions_for_sale(category, error_msg))
		{
			return false;
		}
	}
	return true;
}
bool can_move_item_to_marketplace(const LLInventoryCategory* root_folder, LLInventoryCategory* dest_folder, LLInventoryItem* inv_item, std::string& tooltip_msg, S32 bundle_size, bool from_paste)
{
	LLViewerInventoryCategory* view_folder = dynamic_cast<LLViewerInventoryCategory*>(dest_folder);
	bool move_in_stock = (view_folder && (view_folder->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK));
	bool accept = (view_folder && view_folder->acceptItem(inv_item));
	if (!accept)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxMixedStock");
	}
	if (accept)
	{
		accept = can_move_to_marketplace(inv_item, tooltip_msg, true);
	}
	if (accept)
	{
		S32 existing_item_count = (move_in_stock ? 0 : bundle_size);
		S32 existing_stock_count = (move_in_stock ? bundle_size : 0);
		S32 existing_folder_count = 0;
		const LLViewerInventoryCategory* version_folder = ((root_folder && (root_folder != dest_folder)) ? gInventory.getFirstDescendantOf(root_folder->getUUID(), dest_folder->getUUID()) : NULL);
		if (version_folder)
		{
			if (!from_paste && gInventory.isObjectDescendentOf(inv_item->getUUID(), version_folder->getUUID()))
			{
				existing_item_count = 0;
			}
			LLInventoryModel::cat_array_t existing_categories;
			LLInventoryModel::item_array_t existing_items;
			gInventory.collectDescendents(version_folder->getUUID(), existing_categories, existing_items, FALSE);
			existing_item_count += count_copyable_items(existing_items) + count_stock_folders(existing_categories);
			existing_stock_count += count_stock_items(existing_items);
			existing_folder_count += existing_categories.size();
			if (!inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()) && !move_in_stock)
			{
				existing_folder_count += 1;
			}
		}
		if (existing_item_count > (S32)gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
		{
			LLStringUtil::format_map_t args;
			U32 amount = gSavedSettings.getU32("InventoryOutboxMaxItemCount");
			args["[AMOUNT]"] = llformat("%d",amount);
			tooltip_msg = LLTrans::getString("TooltipOutboxTooManyObjects", args);
			accept = false;
		}
		else if (existing_stock_count > (S32)gSavedSettings.getU32("InventoryOutboxMaxStockItemCount"))
		{
			LLStringUtil::format_map_t args;
			U32 amount = gSavedSettings.getU32("InventoryOutboxMaxStockItemCount");
			args["[AMOUNT]"] = llformat("%d",amount);
			tooltip_msg = LLTrans::getString("TooltipOutboxTooManyStockItems", args);
			accept = false;
		}
		else if (existing_folder_count > (S32)gSavedSettings.getU32("InventoryOutboxMaxFolderCount"))
		{
			LLStringUtil::format_map_t args;
			U32 amount = gSavedSettings.getU32("InventoryOutboxMaxFolderCount");
			args["[AMOUNT]"] = llformat("%d",amount);
			tooltip_msg = LLTrans::getString("TooltipOutboxTooManyFolders", args);
			accept = false;
		}
	}
	return accept;
}
bool can_move_folder_to_marketplace(const LLInventoryCategory* root_folder, LLInventoryCategory* dest_folder, LLInventoryCategory* inv_cat, std::string& tooltip_msg, S32 bundle_size, bool check_items, bool from_paste)
{
	bool accept = true;
	int incoming_folder_depth = get_folder_levels(inv_cat);
    int insertion_point_folder_depth = (root_folder ? get_folder_path_length(root_folder->getUUID(), dest_folder->getUUID()) + 1 : 1);
	const LLViewerInventoryCategory* version_folder = (insertion_point_folder_depth >= 2 ? gInventory.getFirstDescendantOf(root_folder->getUUID(), dest_folder->getUUID()) : NULL);
	if ((incoming_folder_depth + insertion_point_folder_depth - 2) > (S32)(gSavedSettings.getU32("InventoryOutboxMaxFolderDepth")))
	{
		LLStringUtil::format_map_t args;
		U32 amount = gSavedSettings.getU32("InventoryOutboxMaxFolderDepth");
		args["[AMOUNT]"] = llformat("%d",amount);
		tooltip_msg = LLTrans::getString("TooltipOutboxFolderLevels", args);
		accept = false;
	}
	if (accept)
	{
		LLInventoryModel::cat_array_t descendent_categories;
		LLInventoryModel::item_array_t descendent_items;
		gInventory.collectDescendents(inv_cat->getUUID(), descendent_categories, descendent_items, FALSE);
		S32 dragged_folder_count = descendent_categories.size() + bundle_size;
		S32 dragged_item_count = count_copyable_items(descendent_items) + count_stock_folders(descendent_categories);
		S32 dragged_stock_count = count_stock_items(descendent_items);
		S32 existing_item_count = 0;
		S32 existing_stock_count = 0;
		S32 existing_folder_count = 0;
		if (version_folder)
		{
			if (!from_paste && gInventory.isObjectDescendentOf(inv_cat->getUUID(), version_folder->getUUID()))
			{
				dragged_folder_count = 0;
				dragged_item_count = 0;
				dragged_stock_count = 0;
			}
			LLInventoryModel::cat_array_t existing_categories;
			LLInventoryModel::item_array_t existing_items;
			gInventory.collectDescendents(version_folder->getUUID(), existing_categories, existing_items, FALSE);
			existing_folder_count += existing_categories.size();
			existing_item_count += count_copyable_items(existing_items) + count_stock_folders(existing_categories);
			existing_stock_count += count_stock_items(existing_items);
		}
		const S32 total_folder_count = existing_folder_count + dragged_folder_count;
		const S32 total_item_count = existing_item_count + dragged_item_count;
		const S32 total_stock_count = existing_stock_count + dragged_stock_count;
		if (total_folder_count > (S32)gSavedSettings.getU32("InventoryOutboxMaxFolderCount"))
		{
			LLStringUtil::format_map_t args;
			U32 amount = gSavedSettings.getU32("InventoryOutboxMaxFolderCount");
			args["[AMOUNT]"] = llformat("%d",amount);
			tooltip_msg = LLTrans::getString("TooltipOutboxTooManyFolders", args);
			accept = false;
		}
		else if (total_item_count > (S32)gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
		{
			LLStringUtil::format_map_t args;
			U32 amount = gSavedSettings.getU32("InventoryOutboxMaxItemCount");
			args["[AMOUNT]"] = llformat("%d",amount);
			tooltip_msg = LLTrans::getString("TooltipOutboxTooManyObjects", args);
			accept = false;
		}
		else if (total_stock_count > (S32)gSavedSettings.getU32("InventoryOutboxMaxStockItemCount"))
		{
			LLStringUtil::format_map_t args;
			U32 amount = gSavedSettings.getU32("InventoryOutboxMaxStockItemCount");
			args["[AMOUNT]"] = llformat("%d",amount);
			tooltip_msg = LLTrans::getString("TooltipOutboxTooManyStockItems", args);
			accept = false;
		}
		if (accept && check_items)
		{
			for (U32 i=0; i < descendent_items.size(); ++i)
			{
				LLInventoryItem* item = descendent_items[i];
				if (!can_move_to_marketplace(item, tooltip_msg, false))
				{
					accept = false;
					break;
				}
			}
		}
	}
	return accept;
}
bool move_item_to_marketplacelistings(LLInventoryItem* inv_item, LLUUID dest_folder, bool copy)
{
	S32 depth = depth_nesting_in_marketplace(dest_folder);
	if (depth < 0)
	{
		LLSD subs;
		subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Merchant");
		LLNotificationsUtil::add("MerchantPasteFailed", subs);
		return false;
	}
	LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) inv_item;
	LLViewerInventoryCategory * linked_category = viewer_inv_item->getLinkedCategory();
	if (linked_category != NULL)
	{
		return move_folder_to_marketplacelistings(linked_category, dest_folder, copy);
	}
	else
	{
		LLViewerInventoryItem * linked_item = viewer_inv_item->getLinkedItem();
		viewer_inv_item = (linked_item != NULL ? linked_item : viewer_inv_item);
		if (copy && !viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
		{
			return false;
		}
		std::string error_msg;
		if (can_move_to_marketplace(inv_item, error_msg, true))
		{
			if (depth == 0)
			{
				dest_folder = gInventory.createNewCategory(dest_folder, LLFolderType::FT_NONE, viewer_inv_item->getName());
				depth++;
			}
			if (depth == 1)
			{
				dest_folder = gInventory.createNewCategory(dest_folder, LLFolderType::FT_NONE, viewer_inv_item->getName());
				depth++;
			}
			LLViewerInventoryCategory* dest_cat = gInventory.getCategory(dest_folder);
			if (!viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()) &&
				(dest_cat->getPreferredType() != LLFolderType::FT_MARKETPLACE_STOCK))
			{
				dest_folder = gInventory.createNewCategory(dest_folder, LLFolderType::FT_MARKETPLACE_STOCK, viewer_inv_item->getName());
				dest_cat = gInventory.getCategory(dest_folder);
				depth++;
			}
			if (!dest_cat->acceptItem(viewer_inv_item))
			{
				LLSD subs;
				subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Accepted");
				LLNotificationsUtil::add("MerchantPasteFailed", subs);
				return false;
			}
			LLUUID src_folder = viewer_inv_item->getParentUUID();
			if (copy)
			{
				LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(update_folder_cb, dest_folder));
				copy_inventory_item(
									 gAgent.getID(),
									 viewer_inv_item->getPermissions().getOwner(),
									 viewer_inv_item->getUUID(),
									 dest_folder,
									 std::string(),
									 cb);
			}
			else
			{
				gInventory.changeItemParent(viewer_inv_item, dest_folder, true);
			}
		}
		else
		{
			LLSD subs;
			subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + error_msg;
			LLNotificationsUtil::add("MerchantPasteFailed", subs);
			return false;
		}
	}
	open_marketplace_listings();
	return true;
}
bool move_folder_to_marketplacelistings(LLInventoryCategory* inv_cat, const LLUUID& dest_folder, bool copy, bool move_no_copy_items)
{
	std::string error_msg;
	if (has_correct_permissions_for_sale(inv_cat, error_msg))
	{
		LLViewerInventoryCategory* dest_cat = gInventory.getCategory(dest_folder);
		if (dest_cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
		{
			LLSD subs;
			subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Accepted");
			LLNotificationsUtil::add("MerchantPasteFailed", subs);
			return false;
		}
		LLUUID src_folder = inv_cat->getParentUUID();
		LLViewerInventoryCategory* viewer_inv_cat = (LLViewerInventoryCategory*) inv_cat;
		if (copy)
		{
			copy_inventory_category(&gInventory, viewer_inv_cat, dest_folder, LLUUID::null, move_no_copy_items);
		}
		else
		{
			gInventory.changeCategoryParent(viewer_inv_cat, dest_folder, false);
			validate_marketplacelistings(dest_cat);
		}
		update_marketplace_category(src_folder);
		update_marketplace_category(dest_folder);
		gInventory.notifyObservers();
	}
	else
	{
		LLSD subs;
		subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + error_msg;
		LLNotificationsUtil::add("MerchantPasteFailed", subs);
		return false;
	}
	open_marketplace_listings();
	return true;
}
bool sort_alpha(const LLViewerInventoryCategory* cat1, const LLViewerInventoryCategory* cat2)
{
	return cat1->getName().compare(cat2->getName()) < 0;
}
void dump_trace(std::string& message, S32 depth, LLError::ELevel log_level)
{
	LL_INFOS() << "validate_marketplacelistings : error = "<< log_level << ", depth = " << depth << ", message = " << message <<  LL_ENDL;
}
bool validate_marketplacelistings(LLInventoryCategory* cat, validation_callback_t cb, bool fix_hierarchy, S32 depth)
{
#if 0
	if (!cb)
	{
		cb =  boost::bind(&dump_trace, _1, _2, _3);
	}
#endif
	bool result = true;
	LLViewerInventoryCategory* viewer_cat = (LLViewerInventoryCategory*) (cat);
	const LLFolderType::EType folder_type = cat->getPreferredType();
	if (depth < 0)
	{
		depth = depth_nesting_in_marketplace(cat->getUUID());
	}
	if (depth < 0)
	{
		depth = 1;
		fix_hierarchy = false;
	}
	std::string indent;
	for (int i = 1; i < depth; i++)
	{
		indent += "    ";
	}
	if (depth == 2)
	{
		std::string message;
		if (!can_move_folder_to_marketplace(cat, cat, cat, message, 0, fix_hierarchy))
		{
			result = false;
			if (cb)
			{
				message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error") + " " + message;
				cb(message, depth, LLError::LEVEL_ERROR);
			}
		}
	}
	if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (depth <= 2))
	{
		if (fix_hierarchy)
		{
			if (cb)
			{
				std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Warning") + " " + LLTrans::getString("Marketplace Validation Warning Stock");
				cb(message, depth, LLError::LEVEL_WARN);
			}
			LLUUID parent_uuid = cat->getParentUUID();
			LLUUID folder_uuid = gInventory.createNewCategory(parent_uuid, LLFolderType::FT_NONE, cat->getName());
			LLInventoryCategory* new_cat = gInventory.getCategory(folder_uuid);
			gInventory.changeCategoryParent(viewer_cat, folder_uuid, false);
			result &= validate_marketplacelistings(new_cat, cb, fix_hierarchy, depth + 1);
			return result;
		}
		else
		{
			result = false;
			if (cb)
			{
				std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error") + " " + LLTrans::getString("Marketplace Validation Warning Stock");
				cb(message, depth, LLError::LEVEL_ERROR);
			}
		}
	}
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);
	std::map<U32, uuid_vec_t > items_vector;
	bool has_bad_items = false;
	LLInventoryModel::item_array_t item_array_copy = *item_array;
	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
		LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) item;
		std::string error_msg;
		if (!can_move_to_marketplace(item, error_msg, false))
		{
			has_bad_items = true;
			if (cb && fix_hierarchy)
			{
				std::string message = indent + viewer_inv_item->getName() + LLTrans::getString("Marketplace Validation Error") + " " + error_msg;
				cb(message, depth, LLError::LEVEL_ERROR);
			}
			continue;
		}
		LLInventoryType::EType type = LLInventoryType::IT_COUNT;
		U32 perms = 0;
		if (!viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
		{
			type = viewer_inv_item->getInventoryType();
			perms = viewer_inv_item->getPermissions().getMaskNextOwner();
		}
		U32 key = (((U32)(type) & 0xFF) << 24) | (perms & 0xFFFFFF);
		items_vector[key].push_back(viewer_inv_item->getUUID());
	}
	S32 count = items_vector.size();
	U32 default_key = (U32)(LLInventoryType::IT_COUNT) << 24;
	U32 unique_key = (count == 1 ? items_vector.begin()->first : default_key);
	if ((count == 0) && !has_bad_items)
	{
		if (cat_array->size() == 0)
		{
			if (depth == 2)
			{
				if (cb)
				{
					std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error Empty Version");
					cb(message, depth, LLError::LEVEL_WARN);
				}
			}
			else if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (depth > 2))
			{
				if (cb)
				{
					std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error Empty Stock");
					cb(message, depth, LLError::LEVEL_WARN);
				}
			}
			else if (cb)
			{
				std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Warning Empty");
				cb(message, depth, LLError::LEVEL_WARN);
			}
		}
		else
		{
			if (cb && result && (depth >= 1))
			{
				std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Log");
				cb(message, depth, LLError::LEVEL_INFO);
			}
		}
	}
	else if ((count == 1) && !has_bad_items && (((unique_key == default_key) && (depth > 1)) || ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (depth > 2) && (cat_array->size() == 0))))
	{
		if (cb && result && (depth >= 1))
		{
			std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Log");
			cb(message, depth, LLError::LEVEL_INFO);
		}
	}
	else
	{
		if (fix_hierarchy && !has_bad_items)
		{
			if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && ((count >= 2) || (cat_array->size() > 0)))
			{
				LLNotificationsUtil::add("AlertMerchantStockFolderSplit");
			}
			if ((count > 1) || (depth == 1) ||
				((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (unique_key == default_key)) ||
				((folder_type != LLFolderType::FT_MARKETPLACE_STOCK) && (unique_key != default_key)))
			{
				auto items_vector_it = items_vector.begin();
				while (items_vector_it != items_vector.end())
				{
					LLUUID parent_uuid = (depth > 2 ? viewer_cat->getParentUUID() : viewer_cat->getUUID());
					LLViewerInventoryItem* viewer_inv_item = gInventory.getItem(items_vector_it->second.back());
					std::string folder_name = (depth >= 1 ? viewer_cat->getName() : viewer_inv_item->getName());
					LLFolderType::EType new_folder_type = (items_vector_it->first == default_key ? LLFolderType::FT_NONE : LLFolderType::FT_MARKETPLACE_STOCK);
					if (cb)
					{
						std::string message = "";
						if (new_folder_type == LLFolderType::FT_MARKETPLACE_STOCK)
						{
							message = indent + folder_name + LLTrans::getString("Marketplace Validation Warning Create Stock");
						}
						else
						{
							message = indent + folder_name + LLTrans::getString("Marketplace Validation Warning Create Version");
						}
						cb(message, depth, LLError::LEVEL_WARN);
					}
					LLUUID folder_uuid = gInventory.createNewCategory(parent_uuid, new_folder_type, folder_name);
					while (!items_vector_it->second.empty())
					{
						LLViewerInventoryItem* viewer_inv_item = gInventory.getItem(items_vector_it->second.back());
						if (cb)
						{
							std::string message = indent + viewer_inv_item->getName() + LLTrans::getString("Marketplace Validation Warning Move");
							cb(message, depth, LLError::LEVEL_WARN);
						}
						gInventory.changeItemParent(viewer_inv_item, folder_uuid, true);
						items_vector_it->second.pop_back();
					}
					update_marketplace_category(parent_uuid);
					update_marketplace_category(folder_uuid);
					gInventory.notifyObservers();
					items_vector_it++;
				}
			}
			if (folder_type == LLFolderType::FT_MARKETPLACE_STOCK)
			{
				LLUUID parent_uuid = cat->getParentUUID();
				gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);
				LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
				for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
				{
					LLViewerInventoryCategory* viewer_cat = (LLViewerInventoryCategory*) (*iter);
					gInventory.changeCategoryParent(viewer_cat, parent_uuid, false);
					result &= validate_marketplacelistings(viewer_cat, cb, fix_hierarchy, depth);
				}
			}
		}
		else if (cb)
		{
			if (result && (depth >= 1))
			{
				if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (count >= 2))
				{
					result = false;
					std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error Mixed Stock");
					cb(message, depth, LLError::LEVEL_ERROR);
				}
				else if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (cat_array->size() != 0))
				{
					result = false;
					std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error Subfolder In Stock");
					cb(message, depth, LLError::LEVEL_ERROR);
				}
				else
				{
					std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Log");
					cb(message, depth, LLError::LEVEL_INFO);
				}
			}
			LLInventoryModel::item_array_t item_array_copy = *item_array;
			for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
			{
				LLInventoryItem* item = *iter;
				LLViewerInventoryItem* viewer_inv_item = (LLViewerInventoryItem*) item;
				std::string error_msg;
				if (!can_move_to_marketplace(item, error_msg, false))
				{
					result = false;
					std::string message = indent + "    " + viewer_inv_item->getName() + LLTrans::getString("Marketplace Validation Error") + " " + error_msg;
					cb(message, depth, LLError::LEVEL_ERROR);
				}
				else if ((!viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID())) && (folder_type != LLFolderType::FT_MARKETPLACE_STOCK))
				{
					result = false;
					std::string message = indent + "    " + viewer_inv_item->getName() + LLTrans::getString("Marketplace Validation Error Stock Item");
					cb(message, depth, LLError::LEVEL_ERROR);
				}
				else if (depth == 1)
				{
					result = false;
					std::string message = indent + "    " + viewer_inv_item->getName() + LLTrans::getString("Marketplace Validation Warning Unwrapped Item");
					cb(message, depth, LLError::LEVEL_ERROR);
				}
			}
		}
		if (viewer_cat->getDescendentCount() == 0)
		{
			if (cb)
			{
				std::string message = indent + viewer_cat->getName() + LLTrans::getString("Marketplace Validation Warning Delete");
				cb(message, depth, LLError::LEVEL_WARN);
			}
			gInventory.removeCategory(cat->getUUID());
			gInventory.notifyObservers();
			return result && !has_bad_items;
		}
	}
	gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
	std::sort(cat_array_copy.begin(), cat_array_copy.end(), sort_alpha);
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLInventoryCategory* category = *iter;
		result &= validate_marketplacelistings(category, cb, fix_hierarchy, depth + 1);
	}
	update_marketplace_category(cat->getUUID());
	gInventory.notifyObservers();
	return result && !has_bad_items;
}
bool LLInventoryCollectFunctor::itemTransferCommonlyAllowed(const LLInventoryItem* item)
{
	if (!item)
		return false;
	switch(item->getType())
	{
		case LLAssetType::AT_OBJECT:
		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
				return true;
			break;
		default:
			return true;
			break;
	}
}
bool LLIsType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if(mType == LLAssetType::AT_CATEGORY)
	{
		if(cat) return TRUE;
	}
	if(item)
	{
		if(item->getType() == mType) return TRUE;
	}
	return FALSE;
}
bool LLIsNotType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if(mType == LLAssetType::AT_CATEGORY)
	{
		if(cat) return FALSE;
	}
	if(item)
	{
		if(item->getType() == mType) return FALSE;
		else return TRUE;
	}
	return TRUE;
}
bool LLIsOfAssetType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if(mType == LLAssetType::AT_CATEGORY)
	{
		if(cat) return TRUE;
	}
	if(item)
	{
		if(item->getActualType() == mType) return TRUE;
	}
	return FALSE;
}
bool LLIsValidItemLink::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	LLViewerInventoryItem *vitem = dynamic_cast<LLViewerInventoryItem*>(item);
	if (!vitem) return false;
	return (vitem->getActualType() == LLAssetType::AT_LINK  && !vitem->getIsBrokenLink());
}
bool LLIsTypeWithPermissions::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if(mType == LLAssetType::AT_CATEGORY)
	{
		if(cat)
		{
			return TRUE;
		}
	}
	if(item)
	{
		if(item->getType() == mType)
		{
			LLPermissions perm = item->getPermissions();
			if ((perm.getMaskBase() & mPerm) == mPerm)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}
bool LLBuddyCollector::operator()(LLInventoryCategory* cat,
								  LLInventoryItem* item)
{
	if(item)
	{
		if((LLAssetType::AT_CALLINGCARD == item->getType())
		   && (!item->getCreatorUUID().isNull())
		   && (item->getCreatorUUID() != gAgent.getID()))
		{
			return true;
		}
	}
	return false;
}
bool LLUniqueBuddyCollector::operator()(LLInventoryCategory* cat,
										LLInventoryItem* item)
{
	if(item)
	{
		if((LLAssetType::AT_CALLINGCARD == item->getType())
 		   && (item->getCreatorUUID().notNull())
 		   && (item->getCreatorUUID() != gAgent.getID()))
		{
			mSeen.insert(item->getCreatorUUID());
			return true;
		}
	}
	return false;
}
bool LLParticularBuddyCollector::operator()(LLInventoryCategory* cat,
											LLInventoryItem* item)
{
	if(item)
	{
		if((LLAssetType::AT_CALLINGCARD == item->getType())
		   && (item->getCreatorUUID() == mBuddyID))
		{
			return TRUE;
		}
	}
	return FALSE;
}
bool LLNameCategoryCollector::operator()(
	LLInventoryCategory* cat, LLInventoryItem* item)
{
	if(cat)
	{
		if (!LLStringUtil::compareInsensitive(mName, cat->getName()))
		{
			return true;
		}
	}
	return false;
}
bool LLFindCOFValidItems::operator()(LLInventoryCategory* cat,
									 LLInventoryItem* item)
{
	LLViewerInventoryItem *linked_item = ((LLViewerInventoryItem*)item)->getLinkedItem();
	if (linked_item)
	{
		LLAssetType::EType type = linked_item->getType();
		return (type == LLAssetType::AT_CLOTHING ||
				type == LLAssetType::AT_BODYPART ||
				type == LLAssetType::AT_GESTURE ||
				type == LLAssetType::AT_OBJECT);
	}
	else
	{
		LLViewerInventoryCategory *linked_category = ((LLViewerInventoryItem*)item)->getLinkedCategory();
		return (linked_category &&
				((linked_category->getPreferredType() == LLFolderType::FT_NONE) ||
				 (LLFolderType::lookupIsEnsembleType(linked_category->getPreferredType()))));
	}
}
bool LLFindWearables::operator()(LLInventoryCategory* cat,
								 LLInventoryItem* item)
{
	if(item)
	{
		if((item->getType() == LLAssetType::AT_CLOTHING)
		   || (item->getType() == LLAssetType::AT_BODYPART))
		{
			return TRUE;
		}
	}
	return FALSE;
}
LLFindWearablesEx::LLFindWearablesEx(bool is_worn, bool include_body_parts)
:	mIsWorn(is_worn)
,	mIncludeBodyParts(include_body_parts)
{}
bool LLFindWearablesEx::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	LLViewerInventoryItem *vitem = dynamic_cast<LLViewerInventoryItem*>(item);
	if (!vitem) return false;
	if (!vitem->isWearableType() && vitem->getType() != LLAssetType::AT_OBJECT && vitem->getType() != LLAssetType::AT_GESTURE)
	{
		return false;
	}
	if (!mIncludeBodyParts && vitem->getType() == LLAssetType::AT_BODYPART)
	{
		return false;
	}
	if (vitem->getIsBrokenLink())
	{
		return false;
	}
	return (bool) get_is_item_worn(item->getUUID()) == mIsWorn;
}
bool LLFindWearablesOfType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if (!item) return false;
	if (item->getType() != LLAssetType::AT_CLOTHING &&
		item->getType() != LLAssetType::AT_BODYPART)
	{
		return false;
	}
	LLViewerInventoryItem *vitem = dynamic_cast<LLViewerInventoryItem*>(item);
	if (!vitem || vitem->getWearableType() != mWearableType) return false;
	return true;
}
void LLFindWearablesOfType::setType(LLWearableType::EType type)
{
	mWearableType = type;
}
bool LLFindNonRemovableObjects::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if (item)
	{
		return !get_is_item_removable(&gInventory, item->getUUID());
	}
	if (cat)
	{
		return !get_is_category_removable(&gInventory, cat->getUUID());
	}
	LL_WARNS() << "Not a category and not an item?" << LL_ENDL;
	return false;
}
bool LLAssetIDMatches ::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	return (item && item->getAssetUUID() == mAssetID);
}
bool LLLinkedItemIDMatches::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	return (item &&
			(item->getIsLinkType()) &&
			(item->getLinkedUUID() == mBaseItemID));
}
void LLSaveFolderState::setApply(BOOL apply)
{
	mApply = apply;
	if(!apply)
	{
		clearOpenFolders();
	}
}
void LLSaveFolderState::doFolder(LLFolderViewFolder* folder)
{
	LLInvFVBridge* bridge = (LLInvFVBridge*)folder->getListener();
	if(!bridge) return;
	if(mApply)
	{
		LLUUID id(bridge->getUUID());
		if(mOpenFolders.find(id) != mOpenFolders.end())
		{
			if (!folder->isOpen())
			{
				folder->setOpen(TRUE);
			}
		}
		else
		{
			if (!folder->isSelected() && folder->isOpen())
			{
				folder->setOpen(FALSE);
			}
		}
	}
	else
	{
		if(folder->isOpen())
		{
			mOpenFolders.insert(bridge->getUUID());
		}
	}
}
void LLOpenFilteredFolders::doItem(LLFolderViewItem *item)
{
	if (item->getFiltered())
	{
		item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}
void LLOpenFilteredFolders::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getFiltered() && folder->getParentFolder())
	{
		folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
	else if (!folder->getFiltered() && !folder->hasFilteredDescendants())
	{
		folder->setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_NO);
	}
}
void LLSelectFirstFilteredItem::doItem(LLFolderViewItem *item)
{
	if (item->getFiltered() && !mItemSelected)
	{
		item->getRoot()->setSelection(item, FALSE, FALSE);
		if (item->getParentFolder())
		{
			item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
		item->getRoot()->scrollToShowSelection();
		mItemSelected = TRUE;
	}
}
void LLSelectFirstFilteredItem::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getFiltered() && !mItemSelected)
	{
		folder->getRoot()->setSelection(folder, FALSE, FALSE);
		if (folder->getParentFolder())
		{
			folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
		folder->getRoot()->scrollToShowSelection();
		mItemSelected = TRUE;
	}
}
void LLOpenFoldersWithSelection::doItem(LLFolderViewItem *item)
{
	if (item->getParentFolder() && item->isSelected())
	{
		item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}
void LLOpenFoldersWithSelection::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getParentFolder() && folder->isSelected())
	{
		folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}
