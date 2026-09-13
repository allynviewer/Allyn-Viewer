/** 
 * @file llinventorymodel.cpp
 * @brief Implementation of the inventory model used to track agent inventory.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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
#include <typeinfo>
#include "llinventorymodel.h"
#include "llaisapi.h"
#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llavatarnamecache.h"
#include "llinventoryclipboard.h"
#include "llinventorypanel.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llnotificationsutil.h"
#include "llmarketplacefunctions.h"
#include "llwindow.h"
#include "llviewercontrol.h"
#include "llpreview.h"
#include "llviewermessage.h"
#include "llviewerfoldertype.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "llviewerregion.h"
#include "llcallbacklist.h"
#include "llvoavatarself.h"
#include "llgesturemgr.h"
#include "llsdutil.h"
#include "statemachine/aievent.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
#ifdef DIFF_INVENTORY_FILES
#include "process.h"
#endif
const S32 LLInventoryModel::sCurrentInvCacheVersion = 2;
BOOL LLInventoryModel::sFirstTimeInViewer2 = TRUE;
static const char CACHE_FORMAT_STRING[] = "%s.inv";
static const char * const LOG_INV("Inventory");
struct InventoryIDPtrLess
{
	bool operator()(const LLViewerInventoryCategory* i1, const LLViewerInventoryCategory* i2) const
	{
		return (i1->getUUID() < i2->getUUID());
	}
};
class LLCanCache : public LLInventoryCollectFunctor
{
public:
	LLCanCache(LLInventoryModel* model) : mModel(model) {}
	virtual ~LLCanCache() = default;
	bool operator()(LLInventoryCategory* cat, LLInventoryItem* item) override;
protected:
	LLInventoryModel* mModel;
	uuid_set_t mCachedCatIDs;
};
bool LLCanCache::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	bool rv = false;
	if(item)
	{
		if(mCachedCatIDs.find(item->getParentUUID()) != mCachedCatIDs.end())
		{
			rv = true;
		}
	}
	else if(cat)
	{
		LLViewerInventoryCategory* c = (LLViewerInventoryCategory*)cat;
		if(c->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			S32 descendents_server = c->getDescendentCount();
			S32 descendents_actual = c->getViewerDescendentCount();
			if(descendents_server == descendents_actual)
			{
				mCachedCatIDs.insert(c->getUUID());
				rv = true;
			}
		}
	}
	return rv;
}
LLInventoryModel gInventory;
LLInventoryModel::LLInventoryModel()
:
	mIsAgentInvUsable(false),
	mRootFolderID(),
	mLibraryRootFolderID(),
	mLibraryOwnerID(),
	mCategoryMap(),
	mItemMap(),
	mParentChildCategoryTree(),
	mParentChildItemTree(),
	mBacklinkMMap(),
	mLastItem(nullptr),
	mIsNotifyObservers(FALSE),
	mModifyMask(LLInventoryObserver::ALL),
	mChangedItemIDs(),
	mObservers(),
	mCategoryLock(),
	mItemLock()
{}
LLInventoryModel::~LLInventoryModel()
{
	cleanupInventory();
}
void LLInventoryModel::cleanupInventory()
{
	empty();
	while (!mObservers.empty())
	{
		observer_list_t::iterator iter = mObservers.begin();
		LLInventoryObserver* observer = *iter;
		mObservers.erase(iter);
		delete observer;
	}
	mObservers.clear();
}
BOOL LLInventoryModel::isObjectDescendentOf(const LLUUID& obj_id,
											const LLUUID& cat_id,
											const BOOL break_on_recursion) const
{
	if (obj_id == cat_id) return TRUE;
	if(cat_id.isNull()) return FALSE;
	const LLInventoryObject* obj = getObject(obj_id);
	int depthCounter = 0;
	while(obj)
	{
		const LLUUID& parent_id = obj->getParentUUID();
		if(break_on_recursion)
		{
			if(depthCounter++ >= 100)
			{
				LL_WARNS() << "In way too damn deep, possibly recursive parenting" << LL_ENDL;
				LL_INFOS() << obj->getName() << " : " << obj->getUUID() << " : " << parent_id << LL_ENDL;
				return FALSE;
			}
			if(parent_id == obj->getUUID())
			{
				LL_WARNS() << "This shit has itself as parent! " << parent_id.asString() << ", " << obj->getName() << LL_ENDL;
				return FALSE;
			}
		}
		if( parent_id.isNull() )
		{
			return FALSE;
		}
		if(parent_id == cat_id)
		{
			return TRUE;
		}
		obj = getCategory(parent_id);
	}
	return FALSE;
}
const LLViewerInventoryCategory *LLInventoryModel::getFirstNondefaultParent(const LLUUID& obj_id) const
{
	const LLInventoryObject* obj = getObject(obj_id);
	if(!obj)
	{
		LL_WARNS(LOG_INV) << "Non-existent object [ id: " << obj_id << " ] " << LL_ENDL;
		return nullptr;
	}
	LLUUID parent_id = obj->getParentUUID();
	while (!parent_id.isNull())
	{
		const LLViewerInventoryCategory *cat = getCategory(parent_id);
		if (!cat) break;
		const LLFolderType::EType folder_type = cat->getPreferredType();
		if (folder_type != LLFolderType::FT_NONE &&
			folder_type != LLFolderType::FT_ROOT_INVENTORY &&
			!LLFolderType::lookupIsEnsembleType(folder_type))
		{
			return cat;
		}
		parent_id = cat->getParentUUID();
	}
	return nullptr;
}
const LLViewerInventoryCategory* LLInventoryModel::getFirstDescendantOf(const LLUUID& master_parent_id, const LLUUID& obj_id) const
{
	if (master_parent_id == obj_id)
	{
		return nullptr;
	}
	const LLViewerInventoryCategory* current_cat = getCategory(obj_id);
	if (current_cat == nullptr)
	{
		current_cat = getCategory(getObject(obj_id)->getParentUUID());
	}
	while (current_cat != nullptr)
	{
		const LLUUID& current_parent_id = current_cat->getParentUUID();
		if (current_parent_id == master_parent_id)
		{
			return current_cat;
		}
		current_cat = getCategory(current_parent_id);
	}
	return nullptr;
}
bool LLInventoryModel::getObjectTopmostAncestor(const LLUUID& object_id, LLUUID& result) const
{
	LLInventoryObject *object = getObject(object_id);
	while (object && object->getParentUUID().notNull())
	{
		LLInventoryObject *parent_object = getObject(object->getParentUUID());
		if (!parent_object)
		{
			LL_WARNS(LOG_INV) << "unable to trace topmost ancestor, missing item for uuid " << object->getParentUUID() << LL_ENDL;
			return false;
		}
		object = parent_object;
	}
	result = object->getUUID();
	return true;
}
LLInventoryObject* LLInventoryModel::getObject(const LLUUID& id) const
{
	LLViewerInventoryCategory* cat = getCategory(id);
	if (cat)
	{
		return cat;
	}
	LLViewerInventoryItem* item = getItem(id);
	if (item)
	{
		return item;
	}
	return nullptr;
}
LLViewerInventoryItem* LLInventoryModel::getItem(const LLUUID& id) const
{
	LLViewerInventoryItem* item = nullptr;
	if(mLastItem.notNull() && mLastItem->getUUID() == id)
	{
		item = mLastItem;
	}
	else
	{
		const auto iter = mItemMap.find(id);
		if (iter != mItemMap.cend())
		{
			item = iter->second;
			mLastItem = item;
		}
	}
	return item;
}
LLViewerInventoryCategory* LLInventoryModel::getCategory(const LLUUID& id) const
{
	const auto iter = mCategoryMap.find(id);
	if (iter != mCategoryMap.cend())
	{
		return iter->second;
	}
	return nullptr;
}
S32 LLInventoryModel::getItemCount() const
{
	return mItemMap.size();
}
S32 LLInventoryModel::getCategoryCount() const
{
	return mCategoryMap.size();
}
void LLInventoryModel::getDirectDescendentsOf(const LLUUID& cat_id,
											  cat_array_t*& categories,
											  item_array_t*& items) const
{
	categories = get_ptr_in_map(mParentChildCategoryTree, cat_id);
	items = get_ptr_in_map(mParentChildItemTree, cat_id);
}
void LLInventoryModel::getDirectDescendentsOf(const LLUUID& cat_id,
											  cat_array_t*& categories) const
{
	categories = get_ptr_in_map(mParentChildCategoryTree, cat_id);
}
LLMD5 LLInventoryModel::hashDirectDescendentNames(const LLUUID& cat_id) const
{
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	getDirectDescendentsOf(cat_id,cat_array,item_array);
	LLMD5 item_name_hash;
	if (!item_array)
	{
		item_name_hash.finalize();
		return item_name_hash;
	}
	for (LLInventoryModel::item_array_t::const_iterator iter = item_array->begin();
		 iter != item_array->end();
		 ++iter)
	{
		const LLViewerInventoryItem *item = (*iter);
		if (!item)
			continue;
		item_name_hash.update(item->getName());
	}
	item_name_hash.finalize();
	return item_name_hash;
}
void LLInventoryModel::lockDirectDescendentArrays(const LLUUID& cat_id,
												  cat_array_t*& categories,
												  item_array_t*& items)
{
	getDirectDescendentsOf(cat_id, categories, items);
	if (categories)
	{
		mCategoryLock[cat_id] = true;
	}
	if (items)
	{
		mItemLock[cat_id] = true;
	}
}
void LLInventoryModel::unlockDirectDescendentArrays(const LLUUID& cat_id)
{
	mCategoryLock[cat_id] = false;
	mItemLock[cat_id] = false;
}
void LLInventoryModel::consolidateForType(const LLUUID& main_id, LLFolderType::EType type)
{
	uuid_vec_t folder_ids;
    for (const auto& cat_pair : mCategoryMap)
	{
        LLViewerInventoryCategory* cat = cat_pair.second;
		if ((cat->getPreferredType() == type) && (cat->getUUID() != main_id))
		{
			folder_ids.push_back(cat->getUUID());
		}
	}
	for (LLUUID const& folder_id: folder_ids)
	{
		cat_array_t* cats;
		item_array_t* items;
		getDirectDescendentsOf(folder_id, cats, items);
		uuid_vec_t list_uuids;
		for (item_array_t::const_iterator it = items->begin(); it != items->end(); ++it)
		{
			list_uuids.push_back((*it)->getUUID());
		}
		for (auto it = list_uuids.begin(); it != list_uuids.end(); ++it)
		{
			LLViewerInventoryItem* item = getItem(*it);
			changeItemParent(item, main_id, TRUE);
		}
		list_uuids.clear();
		for (cat_array_t::const_iterator it = cats->begin(); it != cats->end(); ++it)
		{
			list_uuids.push_back((*it)->getUUID());
		}
		for (auto it = list_uuids.begin(); it != list_uuids.end(); ++it)
		{
			LLViewerInventoryCategory* cat = getCategory(*it);
			changeCategoryParent(cat, main_id, TRUE);
		}
		removeCategory(folder_id);
		gInventory.emptyFolderType("", LLFolderType::FT_TRASH);
	}
}
const LLUUID LLInventoryModel::findCategoryUUIDForTypeInRoot(
	LLFolderType::EType preferred_type,
	bool create_folder,
	const LLUUID& root_id)
{
	LLUUID rv = LLUUID::null;
	if(LLFolderType::FT_ROOT_INVENTORY == preferred_type)
	{
		rv = root_id;
	}
	else if (root_id.notNull())
	{
		cat_array_t* cats = nullptr;
		cats = get_ptr_in_map(mParentChildCategoryTree, root_id);
		if(cats)
		{
			S32 count = cats->size();
			for(S32 i = 0; i < count; ++i)
			{
				if(cats->at(i)->getPreferredType() == preferred_type)
				{
					const LLUUID& folder_id = cats->at(i)->getUUID();
					if (rv.isNull() || folder_id < rv)
					{
						rv = folder_id;
					}
				}
			}
		}
	}
	if(rv.isNull() && isInventoryUsable() && create_folder)
	{
		if(root_id.notNull())
		{
			return createNewCategory(root_id, preferred_type, LLStringUtil::null);
		}
	}
	return rv;
}
const LLUUID LLInventoryModel::findCategoryUUIDForType(LLFolderType::EType preferred_type, bool create_folder)
{
	return findCategoryUUIDForTypeInRoot(preferred_type, create_folder, gInventory.getRootFolderID());
}
const LLUUID LLInventoryModel::findLibraryCategoryUUIDForType(LLFolderType::EType preferred_type, bool create_folder)
{
	return findCategoryUUIDForTypeInRoot(preferred_type, create_folder, gInventory.getLibraryRootFolderID());
}
LLUUID LLInventoryModel::findCategoryByName(std::string name)
{
	LLUUID root_id = gInventory.getRootFolderID();
	if(root_id.notNull())
	{
		cat_array_t* cats = NULL;
		cats = get_ptr_in_map(mParentChildCategoryTree, root_id);
		if(cats)
		{
			S32 count = cats->size();
			for(S32 i = 0; i < count; ++i)
			{
				if(cats->at(i)->getName() == name)
				{
					return cats->at(i)->getUUID();
				}
			}
		}
	}
	return LLUUID::null;
}
class LLCreateInventoryCategoryResponder final : public LLHTTPClient::ResponderWithResult
{
	LOG_CLASS(LLCreateInventoryCategoryResponder);
public:
	LLCreateInventoryCategoryResponder(LLInventoryModel* model,
									   boost::optional<inventory_func_type> callback):
		mModel(model),
		mCallback(callback)
	{
	}
protected:
	void httpFailure() override
	{
		LL_WARNS(LOG_INV) << dumpResponse() << LL_ENDL;
	}
	void httpSuccess() override
	{
		const LLSD& content = getContent();
		if (!content.isMap() || !content.has("folder_id"))
		{
			failureResult(400, "Malformed response contents", content);
			return;
		}
		LLUUID category_id = content["folder_id"].asUUID();
		LL_DEBUGS(LOG_INV) << ll_pretty_print_sd(content) << LL_ENDL;
		LLPointer<LLViewerInventoryCategory> cat =
		new LLViewerInventoryCategory( category_id,
									  content["parent_id"].asUUID(),
									  (LLFolderType::EType)content["type"].asInteger(),
									  content["name"].asString(),
									  gAgent.getID() );
		cat->setVersion(LLViewerInventoryCategory::VERSION_INITIAL);
		cat->setDescendentCount(0);
		LLInventoryModel::LLCategoryUpdate update(cat->getParentUUID(), 1);
		mModel->accountForUpdate(update);
		mModel->updateCategory(cat);
		if (mCallback)
		{
			mCallback.get()(category_id);
		}
	}
	char const* getName(void) const override { return "LLCreateInventoryCategoryResponder"; }
private:
	boost::optional<inventory_func_type> mCallback;
	LLInventoryModel* mModel;
};
LLUUID LLInventoryModel::createNewCategory(const LLUUID& parent_id,
										   LLFolderType::EType preferred_type,
										   const std::string& pname,
										   boost::optional<inventory_func_type> callback)
{
	LLUUID id;
	if(!isInventoryUsable())
	{
		LL_WARNS(LOG_INV) << "Inventory is broken." << LL_ENDL;
		return id;
	}
	if(LLFolderType::lookup(preferred_type) == LLFolderType::badLookup())
	{
		LL_DEBUGS(LOG_INV) << "Attempt to create undefined category." << LL_ENDL;
		return id;
	}
	id.generate();
	std::string name = pname;
	if(!pname.empty())
	{
		name.assign(pname);
	}
	else
	{
		name.assign(LLViewerFolderType::lookupNewCategoryName(preferred_type));
	}
	LLViewerRegion* viewer_region = gAgent.getRegion();
	std::string url;
	if ( viewer_region )
		url = viewer_region->getCapability("CreateInventoryCategory");
	if (!url.empty() && callback.get_ptr())
	{
		LLSD request, body;
		body["folder_id"] = id;
		body["parent_id"] = parent_id;
		body["type"] = (LLSD::Integer) preferred_type;
		body["name"] = name;
		request["message"] = "CreateInventoryCategory";
		request["payload"] = body;
		LL_DEBUGS(LOG_INV) << "create category request: " << ll_pretty_print_sd(request) << LL_ENDL;
		LLHTTPClient::post(
			url,
			body,
			new LLCreateInventoryCategoryResponder(this, callback) );
		return LLUUID::null;
	}
	if (!gMessageSystem)
	{
		return LLUUID::null;
	}
	LLPointer<LLViewerInventoryCategory> cat =
		new LLViewerInventoryCategory(id, parent_id, preferred_type, name, gAgent.getID());
	cat->setVersion(LLViewerInventoryCategory::VERSION_INITIAL - 1);
	cat->setDescendentCount(0);
	LLCategoryUpdate update(cat->getParentUUID(), 1);
	accountForUpdate(update);
	updateCategory(cat);
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("CreateInventoryFolder");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlock("FolderData");
	cat->packMessage(msg);
	gAgent.sendReliableMessage();
	return id;
}
bool LLInventoryModel::hasMatchingDirectDescendent(const LLUUID& cat_id,
												   LLInventoryCollectFunctor& filter, bool follow_folder_links)
{
	LLInventoryModel::cat_array_t *cats;
	LLInventoryModel::item_array_t *items;
	getDirectDescendentsOf(cat_id, cats, items);
	if (cats)
	{
		for (LLInventoryModel::cat_array_t::const_iterator it = cats->begin();
			 it != cats->end(); ++it)
		{
			if (filter(*it, nullptr))
			{
				return true;
			}
		}
	}
	if (items)
	{
		for (LLInventoryModel::item_array_t::const_iterator it = items->begin();
			 it != items->end(); ++it)
		{
			if (filter(nullptr, *it))
			{
				return true;
			}
			else if (follow_folder_links && *it && it->get()->getActualType() == LLAssetType::AT_LINK_FOLDER)
			{
				LLViewerInventoryCategory *linked_cat = it->get()->getLinkedCategory();
				if (linked_cat && linked_cat->getPreferredType() != LLFolderType::FT_OUTFIT)
				{
					if (hasMatchingDirectDescendent(linked_cat->getUUID(), filter))
						return true;
				}
			}
		}
	}
	return false;
}
class LLAlwaysCollect : public LLInventoryCollectFunctor
{
public:
	virtual ~LLAlwaysCollect() = default;
	bool operator()(LLInventoryCategory* cat,
	                        LLInventoryItem* item) override
	{
		return TRUE;
	}
};
void LLInventoryModel::collectDescendents(const LLUUID& id,
										  cat_array_t& cats,
										  item_array_t& items,
										  BOOL include_trash)
{
	LLAlwaysCollect always;
	collectDescendentsIf(id, cats, items, include_trash, always);
}
void LLInventoryModel::collectDescendentsIf(const LLUUID& id,
											cat_array_t& cats,
											item_array_t& items,
											BOOL include_trash,
											LLInventoryCollectFunctor& add,
											bool follow_folder_links)
{
	if(!include_trash)
	{
		const LLUUID trash_id = findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if(trash_id.notNull() && (trash_id == id))
			return;
	}
	cat_array_t* cat_array = get_ptr_in_map(mParentChildCategoryTree, id);
	if(cat_array)
	{
		S32 count = cat_array->size();
		for(S32 i = 0; i < count; ++i)
		{
			LLViewerInventoryCategory* cat = cat_array->at(i);
			if(add(cat, nullptr))
			{
				cats.push_back(cat);
			}
			collectDescendentsIf(cat->getUUID(), cats, items, include_trash, add, follow_folder_links);
		}
	}
	LLViewerInventoryItem* item = nullptr;
	item_array_t* item_array = get_ptr_in_map(mParentChildItemTree, id);
	if(item_array)
	{
		S32 count = item_array->size();
		for(S32 i = 0; i < count; ++i)
		{
			item = item_array->at(i);
			if(add(nullptr, item))
			{
				items.push_back(item);
			}
		}
	}
	if (follow_folder_links && item_array)
	{
		S32 count = item_array->size();
		for(S32 i = 0; i < count; ++i)
		{
			item = item_array->at(i);
			if (item && item->getActualType() == LLAssetType::AT_LINK_FOLDER)
			{
				LLViewerInventoryCategory *linked_cat = item->getLinkedCategory();
				if (linked_cat && linked_cat->getPreferredType() != LLFolderType::FT_OUTFIT)
				{
					if(add(linked_cat,NULL))
					{
						cats.push_back(LLPointer<LLViewerInventoryCategory>(linked_cat));
					}
					collectDescendentsIf(linked_cat->getUUID(), cats, items, include_trash, add, false);
				}
			}
		}
	}
}
void LLInventoryModel::addChangedMaskForLinks(const LLUUID& object_id, U32 mask)
{
	const LLInventoryObject *obj = getObject(object_id);
	if (!obj || obj->getIsLinkType())
		return;
	LLInventoryModel::item_array_t item_array = collectLinksTo(object_id);
	for (auto& iter : item_array)
	{
		LLViewerInventoryItem *linked_item = iter;
		addChangedMask(mask, linked_item->getUUID());
	};
}
const LLUUID& LLInventoryModel::getLinkedItemID(const LLUUID& object_id) const
{
	const LLInventoryItem *item = gInventory.getItem(object_id);
	if (!item)
	{
		return object_id;
	}
	return item->getLinkedUUID();
}
LLViewerInventoryItem* LLInventoryModel::getLinkedItem(const LLUUID& object_id) const
{
	return object_id.notNull() ? getItem(getLinkedItemID(object_id)) : NULL;
}
LLInventoryModel::item_array_t LLInventoryModel::collectLinksTo(const LLUUID& id,
																	const LLUUID& start_folder_id)
{
	item_array_t items;
	const LLInventoryObject *obj = getObject(id);
	if (!obj || obj->getIsLinkType())
		return items;
	std::pair<backlink_mmap_t::iterator, backlink_mmap_t::iterator> range = mBacklinkMMap.equal_range(id);
	for (backlink_mmap_t::iterator it = range.first; it != range.second; ++it)
	{
		LLViewerInventoryItem *item = getItem(it->second);
		if (item)
		{
			if(start_folder_id.isNull() || isObjectDescendentOf(it->second, start_folder_id))
			items.push_back(item);
		}
	}
	return items;
}
bool LLInventoryModel::isInventoryUsable() const
{
	bool result = false;
	if(gInventory.getRootFolderID().notNull() && mIsAgentInvUsable)
	{
		result = true;
	}
	return result;
}
U32 LLInventoryModel::updateItem(const LLViewerInventoryItem* item, U32 mask)
{
	if(item->getUUID().isNull())
	{
		return mask;
	}
	if(!isInventoryUsable())
	{
		LL_WARNS(LOG_INV) << "Inventory is broken." << LL_ENDL;
		return mask;
	}
	LLPointer<LLViewerInventoryItem> old_item = getItem(item->getUUID());
	LLPointer<LLViewerInventoryItem> new_item;
	if(old_item)
	{
		new_item = old_item;
		LLUUID old_parent_id = old_item->getParentUUID();
		LLUUID new_parent_id = item->getParentUUID();
		bool update_parent_on_server = false;
		if (new_parent_id.isNull())
		{
			LL_WARNS(LOG_INV) << "Update attempts to reparent item " << item->getUUID()
				<< " to null folder. Moving to Lost&Found. Old item name: " << old_item->getName()
				<< ". New name: " << item->getName()
				<< "." << LL_ENDL;
			new_parent_id = findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
			update_parent_on_server = true;
		}
		if(old_parent_id != new_parent_id)
		{
			item_array_t * item_array = get_ptr_in_map(mParentChildItemTree, old_parent_id);
			if(item_array)
			{
				vector_replace_with_last(*item_array, old_item);
			}
			item_array = get_ptr_in_map(mParentChildItemTree, new_parent_id);
			if(item_array)
			{
				if (update_parent_on_server)
				{
					LLInventoryModel::LLCategoryUpdate update(new_parent_id, 1);
					gInventory.accountForUpdate(update);
				}
				item_array->push_back(old_item);
			}
			mask |= LLInventoryObserver::STRUCTURE;
		}
		if(old_item->getName() != item->getName())
		{
			mask |= LLInventoryObserver::LABEL;
		}
		if(old_item->getDescription() != item->getDescription())
		{
			mask |= LLInventoryObserver::DESCRIPTION;
		}
		old_item->copyViewerItem(item);
		if (update_parent_on_server)
		{
			old_item->setParent(new_parent_id);
			new_item->updateParentOnServer(FALSE);
		}
		mask |= LLInventoryObserver::INTERNAL;
	}
	else
	{
		new_item = new LLViewerInventoryItem(item);
		addItem(new_item);
		if(item->getParentUUID().isNull())
		{
			const LLUUID category_id = findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(new_item->getType()));
			new_item->setParent(category_id);
			item_array_t* item_array = get_ptr_in_map(mParentChildItemTree, category_id);
			if( item_array )
			{
				LLInventoryModel::LLCategoryUpdate update(category_id, 1);
				gInventory.accountForUpdate(update);
				new_item->updateParentOnServer(FALSE);
				item_array->push_back(new_item);
			}
			else
			{
				LL_WARNS(LOG_INV) << "Couldn't find parent-child item tree for " << new_item->getName() << LL_ENDL;
			}
		}
		else
		{
			LLUUID parent_id = item->getParentUUID();
			if(parent_id == CATEGORIZE_LOST_AND_FOUND_ID)
			{
				parent_id = findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
				new_item->setParent(parent_id);
				LLInventoryModel::update_list_t update;
				LLInventoryModel::LLCategoryUpdate new_folder(parent_id, 1);
				update.push_back(new_folder);
				accountForUpdate(update);
			}
			item_array_t* item_array = get_ptr_in_map(mParentChildItemTree, parent_id);
			if(item_array)
			{
				item_array->push_back(new_item);
			}
			else
			{
				LL_INFOS(LOG_INV) << "Lost item: " << new_item->getUUID() << " - "
								  << new_item->getName() << LL_ENDL;
				parent_id = findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
				new_item->setParent(parent_id);
				item_array = get_ptr_in_map(mParentChildItemTree, parent_id);
				if(item_array)
				{
					LLInventoryModel::LLCategoryUpdate update(parent_id, 1);
					gInventory.accountForUpdate(update);
					new_item->updateParentOnServer(FALSE);
					item_array->push_back(new_item);
				}
				else
				{
					LL_WARNS(LOG_INV) << "Lost and found Not there!!" << LL_ENDL;
				}
			}
		}
		mask |= LLInventoryObserver::ADD;
	}
	if(new_item->getType() == LLAssetType::AT_CALLINGCARD)
	{
		mask |= LLInventoryObserver::CALLING_CARD;
		LLUUID id;
		std::string desc = new_item->getDescription();
		BOOL isId = desc.empty() ? FALSE : id.set(desc, FALSE);
		if (isId)
		{
			new_item->setCreator(id);
			LLAvatarName av_name;
			if (LLAvatarNameCache::get(id, &av_name))
			{
				new_item->rename(av_name.getLegacyName());
				mask |= LLInventoryObserver::LABEL;
			}
			else
			{
				LLAvatarNameCache::get(id, boost::bind(&LLViewerInventoryItem::onCallingCardNameLookup, new_item.get(), _1, _2));
			}
		}
	}
	else if (new_item->getType() == LLAssetType::AT_GESTURE)
	{
		mask |= LLInventoryObserver::GESTURE;
	}
	addChangedMask(mask, new_item->getUUID());
	return mask;
}
LLInventoryModel::cat_array_t* LLInventoryModel::getUnlockedCatArray(const LLUUID& id)
{
	cat_array_t* cat_array = get_ptr_in_map(mParentChildCategoryTree, id);
	if (cat_array)
	{
		llassert_always(mCategoryLock[id] == false);
	}
	return cat_array;
}
LLInventoryModel::item_array_t* LLInventoryModel::getUnlockedItemArray(const LLUUID& id)
{
	item_array_t* item_array = get_ptr_in_map(mParentChildItemTree, id);
	if (item_array)
	{
		llassert_always(mItemLock[id] == false);
	}
	return item_array;
}
void LLInventoryModel::updateCategory(const LLViewerInventoryCategory* cat, U32 mask)
{
	if(!cat || cat->getUUID().isNull())
	{
		return;
	}
	if(!isInventoryUsable())
	{
		LL_WARNS(LOG_INV) << "Inventory is broken." << LL_ENDL;
		return;
	}
	LLPointer<LLViewerInventoryCategory> old_cat = getCategory(cat->getUUID());
	if(old_cat)
	{
		LLUUID old_parent_id = old_cat->getParentUUID();
		LLUUID new_parent_id = cat->getParentUUID();
		if(old_parent_id != new_parent_id)
		{
			cat_array_t* cat_array = getUnlockedCatArray(old_parent_id);
			if(cat_array)
			{
				vector_replace_with_last(*cat_array, old_cat);
			}
			cat_array = getUnlockedCatArray(new_parent_id);
			if(cat_array)
			{
				cat_array->push_back(old_cat);
			}
			mask |= LLInventoryObserver::STRUCTURE;
            mask |= LLInventoryObserver::INTERNAL;
		}
		if(old_cat->getName() != cat->getName())
		{
			mask |= LLInventoryObserver::LABEL;
		}
		const LLUUID marketplace_id = findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
		if (marketplace_id.notNull() && isObjectDescendentOf(cat->getUUID(), marketplace_id))
		{
			mask |= LLInventoryObserver::LABEL;
		}
		old_cat->copyViewerCategory(cat);
		addChangedMask(mask, cat->getUUID());
	}
	else
	{
		LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat->getParentUUID());
		new_cat->copyViewerCategory(cat);
		addCategory(new_cat);
		cat_array_t* cat_array = getUnlockedCatArray(cat->getParentUUID());
		if(cat_array)
		{
			cat_array->push_back(new_cat);
		}
		llassert_always(mCategoryLock[new_cat->getUUID()] == false);
		llassert_always(mItemLock[new_cat->getUUID()] == false);
		cat_array_t* catsp = new cat_array_t;
		item_array_t* itemsp = new item_array_t;
		mParentChildCategoryTree[new_cat->getUUID()] = catsp;
		mParentChildItemTree[new_cat->getUUID()] = itemsp;
		mask |= LLInventoryObserver::ADD;
		addChangedMask(mask, cat->getUUID());
	}
}
void LLInventoryModel::moveObject(const LLUUID& object_id, const LLUUID& cat_id)
{
	LL_DEBUGS(LOG_INV) << "LLInventoryModel::moveObject()" << LL_ENDL;
	if(!isInventoryUsable())
	{
		LL_WARNS(LOG_INV) << "Inventory is broken." << LL_ENDL;
		return;
	}
	if((object_id == cat_id) || !is_in_map(mCategoryMap, cat_id))
	{
		LL_WARNS(LOG_INV) << "Could not move inventory object " << object_id << " to "
						  << cat_id << LL_ENDL;
		return;
	}
	LLPointer<LLViewerInventoryCategory> cat = getCategory(object_id);
	if(cat && (cat->getParentUUID() != cat_id))
	{
		cat_array_t* cat_array = getUnlockedCatArray(cat->getParentUUID());
		if(cat_array) vector_replace_with_last(*cat_array, cat);
		cat_array = getUnlockedCatArray(cat_id);
		cat->setParent(cat_id);
		if(cat_array) cat_array->push_back(cat);
		addChangedMask(LLInventoryObserver::STRUCTURE, object_id);
		return;
	}
	LLPointer<LLViewerInventoryItem> item = getItem(object_id);
	if(item && (item->getParentUUID() != cat_id))
	{
		item_array_t* item_array = getUnlockedItemArray(item->getParentUUID());
		if(item_array) vector_replace_with_last(*item_array, item);
		item_array = getUnlockedItemArray(cat_id);
		item->setParent(cat_id);
		if(item_array) item_array->push_back(item);
		addChangedMask(LLInventoryObserver::STRUCTURE, object_id);
		return;
	}
}
void LLInventoryModel::changeItemParent(LLViewerInventoryItem* item,
										const LLUUID& new_parent_id,
										BOOL restamp)
{
	if (item->getParentUUID() == new_parent_id)
	{
		LL_DEBUGS(LOG_INV) << "'" << item->getName() << "' (" << item->getUUID()
						   << ") is already in folder " << new_parent_id << LL_ENDL;
	}
	else
	{
		LL_INFOS(LOG_INV) << "Moving '" << item->getName() << "' (" << item->getUUID()
						  << ") from " << item->getParentUUID() << " to folder "
						  << new_parent_id << LL_ENDL;
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(item->getParentUUID(),-1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(new_parent_id, 1);
		update.push_back(new_folder);
		accountForUpdate(update);
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setParent(new_parent_id);
		if(isObjectDescendentOf(item->getUUID(), gInventory.getRootFolderID()))
		new_item->updateParentOnServer(restamp);
		updateItem(new_item);
		notifyObservers();
	}
}
void LLInventoryModel::changeCategoryParent(LLViewerInventoryCategory* cat,
											const LLUUID& new_parent_id,
											BOOL restamp)
{
	if (!cat)
	{
		return;
	}
	if (isObjectDescendentOf(new_parent_id, cat->getUUID()))
	{
		return;
	}
	LLInventoryModel::update_list_t update;
	LLInventoryModel::LLCategoryUpdate old_folder(cat->getParentUUID(), -1);
	update.push_back(old_folder);
	LLInventoryModel::LLCategoryUpdate new_folder(new_parent_id, 1);
	update.push_back(new_folder);
	accountForUpdate(update);
	LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);
	new_cat->setParent(new_parent_id);
	new_cat->updateParentOnServer(restamp);
	updateCategory(new_cat);
	notifyObservers();
}
void LLInventoryModel::onAISUpdateReceived(const std::string& context, const LLSD& update)
{
	LLTimer timer;
	static LLCachedControl<bool> debug_ava_appr_msg(gSavedSettings, "DebugAvatarAppearanceMessage");
	if (debug_ava_appr_msg)
	{
		dump_sequential_xml(gAgentAvatarp->getFullname() + "_ais_update", update);
	}
	AISUpdate ais_update(update);
	ais_update.doUpdate();
	LL_INFOS(LOG_INV) << "elapsed: " << timer.getElapsedTimeF32() << LL_ENDL;
}
void LLInventoryModel::onItemUpdated(const LLUUID& item_id, const LLSD& updates, bool update_parent_version)
{
	U32 mask = LLInventoryObserver::NONE;
	LLPointer<LLViewerInventoryItem> item = gInventory.getItem(item_id);
	LL_DEBUGS(LOG_INV) << "item_id: [" << item_id << "] name " << (item ? item->getName() : "(NOT FOUND)") << LL_ENDL;
	if(item)
	{
		for (LLSD::map_const_iterator it = updates.beginMap();
			 it != updates.endMap(); ++it)
		{
			if (it->first == "name")
			{
				LL_INFOS(LOG_INV) << "Updating name from " << item->getName() << " to " << it->second.asString() << LL_ENDL;
				item->rename(it->second.asString());
				mask |= LLInventoryObserver::LABEL;
			}
			else if (it->first == "desc")
			{
				LL_INFOS(LOG_INV) << "Updating description from " << item->getActualDescription()
								  << " to " << it->second.asString() << LL_ENDL;
				item->setDescription(it->second.asString());
			}
			else
			{
				LL_ERRS(LOG_INV) << "unhandled updates for field: " << it->first << LL_ENDL;
			}
		}
		mask |= LLInventoryObserver::INTERNAL;
		addChangedMask(mask, item->getUUID());
		if (update_parent_version)
		{
			LLInventoryModel::LLCategoryUpdate up(item->getParentUUID(), 0);
			accountForUpdate(up);
		}
		notifyObservers();
	}
}
void LLInventoryModel::onCategoryUpdated(const LLUUID& cat_id, const LLSD& updates)
{
	U32 mask = LLInventoryObserver::NONE;
	LLPointer<LLViewerInventoryCategory> cat = gInventory.getCategory(cat_id);
	LL_DEBUGS(LOG_INV) << "cat_id: [" << cat_id << "] name " << (cat ? cat->getName() : "(NOT FOUND)") << LL_ENDL;
	if(cat)
	{
		for (LLSD::map_const_iterator it = updates.beginMap();
			 it != updates.endMap(); ++it)
		{
			if (it->first == "name")
			{
				LL_INFOS(LOG_INV) << "Updating name from " << cat->getName() << " to " << it->second.asString() << LL_ENDL;
				cat->rename(it->second.asString());
				mask |= LLInventoryObserver::LABEL;
			}
			else
			{
				LL_ERRS(LOG_INV) << "unhandled updates for field: " << it->first << LL_ENDL;
			}
		}
		mask |= LLInventoryObserver::INTERNAL;
		addChangedMask(mask, cat->getUUID());
		notifyObservers();
	}
}
void LLInventoryModel::onDescendentsPurgedFromServer(const LLUUID& object_id, bool fix_broken_links)
{
	LLPointer<LLViewerInventoryCategory> cat = getCategory(object_id);
	if (cat.notNull())
	{
		S32 descendents = cat->getDescendentCount();
		if(descendents > 0)
		{
			LLInventoryModel::LLCategoryUpdate up(object_id, -descendents);
			accountForUpdate(up);
		}
		cat->setDescendentCount(0);
		LLInventoryModel::cat_array_t categories;
		LLInventoryModel::item_array_t items;
		collectDescendents(object_id,
						   categories,
						   items,
						   LLInventoryModel::INCLUDE_TRASH);
		S32 count = items.size();
		LLUUID uu_id;
		for(S32 i = 0; i < count; ++i)
		{
			uu_id = items.at(i)->getUUID();
			if (getItem(uu_id))
			{
				deleteObject(uu_id, fix_broken_links);
			}
		}
		count = categories.size();
		S32 deleted_count;
		S32 total_deleted_count = 0;
		do
		{
			deleted_count = 0;
			for(S32 i = 0; i < count; ++i)
			{
				uu_id = categories.at(i)->getUUID();
				if (getCategory(uu_id))
				{
					cat_array_t* cat_list = getUnlockedCatArray(uu_id);
					if (!cat_list || (cat_list->empty()))
					{
						deleteObject(uu_id, fix_broken_links);
						deleted_count++;
					}
				}
			}
			total_deleted_count += deleted_count;
		}
		while (deleted_count > 0);
		if (total_deleted_count != count)
		{
			LL_WARNS(LOG_INV) << "Unexpected count of categories deleted, got "
							  << total_deleted_count << " expected " << count << LL_ENDL;
		}
	}
}
void LLInventoryModel::onObjectDeletedFromServer(const LLUUID& object_id, bool fix_broken_links, bool update_parent_version, bool do_notify_observers)
{
	LLPointer<LLInventoryObject> obj = getObject(object_id);
	if(obj)
	{
		if (getCategory(object_id))
		{
			onDescendentsPurgedFromServer(object_id, fix_broken_links);
		}
		if (update_parent_version)
		{
			LLInventoryModel::LLCategoryUpdate up(obj->getParentUUID(), -1);
			accountForUpdate(up);
		}
		LLPreview::hide(object_id);
		deleteObject(object_id, fix_broken_links, do_notify_observers);
	}
}
void LLInventoryModel::deleteObject(const LLUUID& id, bool fix_broken_links, bool do_notify_observers)
{
	LL_DEBUGS(LOG_INV) << "LLInventoryModel::deleteObject()" << LL_ENDL;
	LLPointer<LLInventoryObject> obj = getObject(id);
	if (!obj)
	{
		LL_WARNS(LOG_INV) << "Deleting non-existent object [ id: " << id << " ] " << LL_ENDL;
		return;
	}
	LL_DEBUGS(LOG_INV) << "Deleting inventory object " << id << LL_ENDL;
	mLastItem = nullptr;
	LLUUID parent_id = obj->getParentUUID();
	mCategoryMap.erase(id);
	mItemMap.erase(id);
	item_array_t* item_list = getUnlockedItemArray(parent_id);
	if(item_list)
	{
		LLPointer<LLViewerInventoryItem> item = (LLViewerInventoryItem*)((LLInventoryObject*)obj);
		vector_replace_with_last(*item_list, item);
	}
	cat_array_t* cat_list = getUnlockedCatArray(parent_id);
	if(cat_list)
	{
		LLPointer<LLViewerInventoryCategory> cat = (LLViewerInventoryCategory*)((LLInventoryObject*)obj);
		vector_replace_with_last(*cat_list, cat);
	}
	addChangedMask(LLInventoryObserver::REMOVE, id);
	gInventory.notifyObservers();
	item_list = getUnlockedItemArray(id);
	if(item_list)
	{
		if (!item_list->empty())
		{
			LL_WARNS(LOG_INV) << "Deleting cat " << id << " while it still has child items" << LL_ENDL;
		}
		delete item_list;
		mParentChildItemTree.erase(id);
	}
	cat_list = getUnlockedCatArray(id);
	if(cat_list)
	{
		if (!cat_list->empty())
		{
			LL_WARNS(LOG_INV) << "Deleting cat " << id << " while it still has child cats" << LL_ENDL;
		}
		delete cat_list;
		mParentChildCategoryTree.erase(id);
	}
	addChangedMask(LLInventoryObserver::REMOVE, id);
	bool is_link_type = obj->getIsLinkType();
	if (is_link_type)
	{
		removeBacklinkInfo(obj->getUUID(), obj->getLinkedUUID());
	}
	if (fix_broken_links && !is_link_type)
	{
		updateLinkedObjectsFromPurge(id);
	}
	obj = nullptr;
	if (do_notify_observers)
	{
		notifyObservers();
	}
}
void LLInventoryModel::updateLinkedObjectsFromPurge(const LLUUID &baseobj_id)
{
	LLInventoryModel::item_array_t item_array = collectLinksTo(baseobj_id);
	if (!item_array.empty())
	{
		notifyObservers();
		for (LLInventoryModel::item_array_t::const_iterator iter = item_array.begin();
			 iter != item_array.end();
		     ++iter)
		{
			const LLViewerInventoryItem *linked_item = (*iter);
			const LLUUID &item_id = linked_item->getUUID();
			if (item_id == baseobj_id) continue;
			addChangedMask(LLInventoryObserver::REBUILD, item_id);
		}
		notifyObservers();
	}
}
void LLInventoryModel::addObserver(LLInventoryObserver* observer)
{
	mObservers.insert(observer);
}
void LLInventoryModel::removeObserver(LLInventoryObserver* observer)
{
	mObservers.erase(observer);
}
BOOL LLInventoryModel::containsObserver(LLInventoryObserver* observer) const
{
	return mObservers.find(observer) != mObservers.end();
}
void LLInventoryModel::idleNotifyObservers()
{
	if (mModifyMask == LLInventoryObserver::NONE && (mChangedItemIDs.empty()))
	{
		return;
	}
	notifyObservers();
}
void LLInventoryModel::notifyObservers()
{
	if (mIsNotifyObservers)
	{
		LL_WARNS(LOG_INV) << "Call was made to notifyObservers within notifyObservers!" << LL_ENDL;
		return;
	}
	mIsNotifyObservers = TRUE;
	for (observer_list_t::iterator iter = mObservers.begin();
		 iter != mObservers.end(); )
	{
		LLInventoryObserver* observer = *iter;
		observer->changed(mModifyMask);
		iter = mObservers.upper_bound(observer);
	}
	mModifyMask = LLInventoryObserver::NONE;
	mChangedItemIDs.clear();
	mAddedItemIDs.clear();
	mIsNotifyObservers = FALSE;
}
void LLInventoryModel::addChangedMask(U32 mask, const LLUUID& referent)
{
	if (mIsNotifyObservers)
	{
		LL_WARNS(LOG_INV) << "Adding changed mask within notify observers!  Change will likely be lost." << LL_ENDL;
		LLViewerInventoryItem *item = getItem(referent);
		if (item)
		{
			LL_WARNS(LOG_INV) << "Item " << item->getName() << LL_ENDL;
		}
		else
		{
			LLViewerInventoryCategory *cat = getCategory(referent);
			if (cat)
			{
				LL_WARNS(LOG_INV) << "Category " << cat->getName() << LL_ENDL;
			}
		}
	}
	mModifyMask |= mask;
	if (referent.notNull() && (mChangedItemIDs.find(referent) == mChangedItemIDs.end()))
	{
		mChangedItemIDs.insert(referent);
		update_marketplace_category(referent, false);
		if (mask & LLInventoryObserver::ADD)
		{
			mAddedItemIDs.insert(referent);
		}
		if (mask & LLInventoryObserver::LABEL)
		{
			addChangedMaskForLinks(referent, LLInventoryObserver::LABEL);
		}
	}
}
bool LLInventoryModel::fetchDescendentsOf(const LLUUID& folder_id) const
{
	if(folder_id.isNull())
	{
		LL_WARNS(LOG_INV) << "Calling fetch descendents on NULL folder id!" << LL_ENDL;
		return false;
	}
	LLViewerInventoryCategory* cat = getCategory(folder_id);
	if(!cat)
	{
		LL_WARNS(LOG_INV) << "Asked to fetch descendents of non-existent folder: "
						  << folder_id << LL_ENDL;
		return false;
	}
	return cat->fetch();
}
void LLInventoryModel::cache(
	const LLUUID& parent_folder_id,
	const LLUUID& agent_id)
{
	LL_DEBUGS(LOG_INV) << "Caching " << parent_folder_id << " for " << agent_id
					   << LL_ENDL;
	LLViewerInventoryCategory* root_cat = getCategory(parent_folder_id);
	if(!root_cat) return;
	cat_array_t categories;
	categories.push_back(root_cat);
	item_array_t items;
	LLCanCache can_cache(this);
	can_cache(root_cat, nullptr);
	collectDescendentsIf(
		parent_folder_id,
		categories,
		items,
		INCLUDE_TRASH,
		can_cache);
	std::string agent_id_str;
	std::string inventory_filename;
	agent_id.toString(agent_id_str);
	std::string path(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, agent_id_str));
	inventory_filename = llformat(CACHE_FORMAT_STRING, path.c_str());
	saveToFile(inventory_filename, categories, items);
	std::string gzip_filename(inventory_filename);
	gzip_filename.append(".gz");
	if(gzip_file(inventory_filename, gzip_filename))
	{
		LL_DEBUGS(LOG_INV) << "Successfully compressed " << inventory_filename << LL_ENDL;
		LLFile::remove(inventory_filename);
	}
	else
	{
		LL_WARNS(LOG_INV) << "Unable to compress " << inventory_filename << LL_ENDL;
	}
}
void LLInventoryModel::addCategory(LLViewerInventoryCategory* category)
{
	if(category)
	{
		if (category->mPreferredType == LLFolderType::FT_MESH)
		{
			return;
		}
		category->localizeName();
		mCategoryMap[category->getUUID()] = category;
	}
}
bool LLInventoryModel::hasBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id) const
{
	std::pair<backlink_mmap_t::const_iterator, backlink_mmap_t::const_iterator> range = mBacklinkMMap.equal_range(target_id);
	for (backlink_mmap_t::const_iterator it = range.first; it != range.second; ++it)
	{
		if (it->second == link_id)
		{
			return true;
		}
	}
	return false;
}
void LLInventoryModel::addBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id)
{
	if (!hasBacklinkInfo(link_id, target_id))
	{
		mBacklinkMMap.insert(std::make_pair(target_id, link_id));
	}
}
void LLInventoryModel::removeBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id)
{
	std::pair<backlink_mmap_t::iterator, backlink_mmap_t::iterator> range = mBacklinkMMap.equal_range(target_id);
	for (backlink_mmap_t::iterator it = range.first; it != range.second; )
	{
		if (it->second == link_id)
		{
			backlink_mmap_t::iterator delete_it = it;
			++it;
			mBacklinkMMap.erase(delete_it);
		}
		else
		{
			++it;
		}
	}
}
void LLInventoryModel::addItem(LLViewerInventoryItem* item)
{
	llassert(item);
	if(item)
	{
		if (item->getType() <= LLAssetType::AT_NONE)
		{
			LL_WARNS(LOG_INV) << "Got bad asset type for item [ name: " << item->getName()
							  << " type: " << item->getType()
							  << " inv-type: " << item->getInventoryType() << " ], ignoring." << LL_ENDL;
			return;
		}
		if (LLAssetType::lookup(item->getType()) == LLAssetType::badLookup())
		{
			if (item->getType() >= LLAssetType::AT_COUNT)
			{
				LL_DEBUGS(LOG_INV) << "Got unknown asset type for item [ name: " << item->getName()
					<< " type: " << item->getType()
					<< " inv-type: " << item->getInventoryType() << " ]." << LL_ENDL;
			}
			else
			{
				LL_WARNS(LOG_INV) << "Got unknown asset type for item [ name: " << item->getName()
					<< " type: " << item->getType()
					<< " inv-type: " << item->getInventoryType() << " ]." << LL_ENDL;
			}
		}
		if (item->getIsBrokenLink())
		{
			LL_INFOS(LOG_INV) << "Adding broken link [ name: " << item->getName()
							  << " itemID: " << item->getUUID()
							  << " assetID: " << item->getAssetUUID() << " )  parent: " << item->getParentUUID() << LL_ENDL;
		}
		if (item->getIsLinkType())
		{
			const LLUUID& link_id = item->getUUID();
			const LLUUID& target_id = item->getLinkedUUID();
			addBacklinkInfo(link_id, target_id);
		}
		mItemMap[item->getUUID()] = item;
	}
}
void LLInventoryModel::empty()
{
	std::for_each(
		mParentChildCategoryTree.begin(),
		mParentChildCategoryTree.end(),
		DeletePairedPointer());
	mParentChildCategoryTree.clear();
	std::for_each(
		mParentChildItemTree.begin(),
		mParentChildItemTree.end(),
		DeletePairedPointer());
	mParentChildItemTree.clear();
	mBacklinkMMap.clear();
	mCategoryMap.clear();
	mItemMap.clear();
	mLastItem = NULL;
}
void LLInventoryModel::accountForUpdate(const LLCategoryUpdate& update) const
{
	LLViewerInventoryCategory* cat = getCategory(update.mCategoryID);
	if(cat)
	{
		S32 version = cat->getVersion();
		if(version != LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			S32 descendents_server = cat->getDescendentCount();
			S32 descendents_actual = cat->getViewerDescendentCount();
			if(descendents_server == descendents_actual)
			{
				descendents_actual += update.mDescendentDelta;
				cat->setDescendentCount(descendents_actual);
				cat->setVersion(++version);
				LL_DEBUGS(LOG_INV) << "accounted: '" << cat->getName() << "' "
								   << version << " with " << descendents_actual
								   << " descendents." << LL_ENDL;
			}
			else
			{
				LL_WARNS(LOG_INV) << "Accounting failed for '" << cat->getName() << "' version:"
								  << version << " due to mismatched descendent count:  server == "
								  << descendents_server << ", viewer == " << descendents_actual << LL_ENDL;
			}
		}
		else
		{
			LL_WARNS(LOG_INV) << "Accounting failed for '" << cat->getName() << "' version: unknown ("
							  << version << ")" << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS(LOG_INV) << "No category found for update " << update.mCategoryID << LL_ENDL;
	}
}
void LLInventoryModel::accountForUpdate(const LLInventoryModel::update_list_t& update)
{
	update_list_t::const_iterator it = update.begin();
	update_list_t::const_iterator end = update.end();
	for(; it != end; ++it)
	{
		accountForUpdate(*it);
	}
}
void LLInventoryModel::accountForUpdate(const LLInventoryModel::update_map_t& update)
{
	LLCategoryUpdate up;
	update_map_t::const_iterator it = update.begin();
	update_map_t::const_iterator end = update.end();
	for(; it != end; ++it)
	{
		up.mCategoryID = (*it).first;
		up.mDescendentDelta = (*it).second.mValue;
		accountForUpdate(up);
	}
}
LLInventoryModel::EHasChildren LLInventoryModel::categoryHasChildren(const LLUUID& cat_id) const
{
	LLViewerInventoryCategory* cat = getCategory(cat_id);
	if(!cat) return CHILDREN_NO;
	if(cat->getDescendentCount() > 0)
	{
		return CHILDREN_YES;
	}
	if(cat->getDescendentCount() == 0)
	{
		return CHILDREN_NO;
	}
	if((cat->getDescendentCount() == LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN)
	   || (cat->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN))
	{
		return CHILDREN_MAYBE;
	}
	const auto cat_it = mParentChildCategoryTree.find(cat->getUUID());
	if (cat_it != mParentChildCategoryTree.cend() && !cat_it->second->empty())
	{
		return CHILDREN_YES;
	}
	const auto item_it = mParentChildItemTree.find(cat->getUUID());
	if (item_it != mParentChildItemTree.cend() && !item_it->second->empty())
	{
		return CHILDREN_YES;
	}
	return CHILDREN_NO;
}
bool LLInventoryModel::isCategoryComplete(const LLUUID& cat_id) const
{
	LLViewerInventoryCategory* cat = getCategory(cat_id);
	if(cat && (cat->getVersion()!=LLViewerInventoryCategory::VERSION_UNKNOWN))
	{
		S32 descendents_server = cat->getDescendentCount();
		S32 descendents_actual = cat->getViewerDescendentCount();
		if(descendents_server == descendents_actual)
		{
			return true;
		}
	}
	return false;
}
bool LLInventoryModel::loadSkeleton(
	const LLSD& options,
	const LLUUID& owner_id)
{
	LL_DEBUGS(LOG_INV) << "importing inventory skeleton for " << owner_id << LL_ENDL;
	typedef std::set<LLPointer<LLViewerInventoryCategory>, InventoryIDPtrLess> cat_set_t;
	cat_set_t temp_cats;
	bool rv = true;
	for(LLSD::array_const_iterator it = options.beginArray(),
		end = options.endArray(); it != end; ++it)
	{
		LLSD name = (*it)["name"];
		LLSD folder_id = (*it)["folder_id"];
		LLSD parent_id = (*it)["parent_id"];
		LLSD version = (*it)["version"];
		if(name.isDefined()
			&& folder_id.isDefined()
			&& parent_id.isDefined()
			&& version.isDefined()
			&& folder_id.asUUID().notNull()
			)
		{
			LLPointer<LLViewerInventoryCategory> cat = new LLViewerInventoryCategory(owner_id);
			cat->rename(name.asString());
			cat->setUUID(folder_id.asUUID());
			cat->setParent(parent_id.asUUID());
			LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
			LLSD type_default = (*it)["type_default"];
			if(type_default.isDefined())
            {
				preferred_type = (LLFolderType::EType)type_default.asInteger();
            }
            cat->setPreferredType(preferred_type);
			cat->setVersion(version.asInteger());
            temp_cats.insert(cat);
		}
		else
		{
			LL_WARNS(LOG_INV) << "Unable to import near " << name.asString() << LL_ENDL;
            rv = false;
		}
	}
	S32 cached_category_count = 0;
	S32 cached_item_count = 0;
	if(!temp_cats.empty())
	{
		update_map_t child_counts;
		cat_array_t categories;
		item_array_t items;
		changed_items_t categories_to_update;
		item_array_t possible_broken_links;
		cat_set_t invalid_categories;
		std::string owner_id_str;
		owner_id.toString(owner_id_str);
		std::string path(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, owner_id_str));
		std::string inventory_filename;
		inventory_filename = llformat(CACHE_FORMAT_STRING, path.c_str());
		const S32 NO_VERSION = LLViewerInventoryCategory::VERSION_UNKNOWN;
		std::string gzip_filename(inventory_filename);
		gzip_filename.append(".gz");
		LLFILE* fp = LLFile::fopen(gzip_filename, "rb");
		bool remove_inventory_file = false;
		if(fp)
		{
			fclose(fp);
			fp = nullptr;
			if(gunzip_file(gzip_filename, inventory_filename))
			{
				remove_inventory_file = true;
			}
			else
			{
				LL_INFOS(LOG_INV) << "Unable to gunzip " << gzip_filename << LL_ENDL;
			}
		}
		bool is_cache_obsolete = false;
		if (loadFromFile(inventory_filename, categories, items, categories_to_update, is_cache_obsolete))
		{
			S32 count = categories.size();
			cat_set_t::iterator not_cached = temp_cats.end();
			uuid_set_t cached_ids;
			for(S32 i = 0; i < count; ++i)
			{
				LLViewerInventoryCategory* cat = categories[i];
				cat_set_t::iterator cit = temp_cats.find(cat);
				if (cit == temp_cats.end())
				{
					continue;
				}
				LLViewerInventoryCategory* tcat = *cit;
				if (categories_to_update.find(tcat->getUUID()) != categories_to_update.end())
				{
					tcat->setVersion(NO_VERSION);
					LL_WARNS() << "folder to update: " << tcat->getName() << LL_ENDL;
				}
				if (cit == not_cached)
				{
					continue;
				}
				else if (cat->getVersion() != tcat->getVersion())
				{
					tcat->setVersion(NO_VERSION);
				}
				else
				{
					cached_ids.insert(tcat->getUUID());
				}
			}
			auto not_cached_id = cached_ids.end();
			cached_category_count = cached_ids.size();
			for (const auto& temp_cat : temp_cats)
			{
				if(cached_ids.find(temp_cat->getUUID()) == not_cached_id)
				{
					LLViewerInventoryCategory *llvic = temp_cat;
					llvic->setVersion(NO_VERSION);
				}
				addCategory(temp_cat);
				++child_counts[temp_cat->getParentUUID()];
			}
			S32 bad_link_count = 0;
			S32 good_link_count = 0;
			S32 recovered_link_count = 0;
			const auto unparented = mCategoryMap.cend();
			for(item_array_t::const_iterator item_iter = items.begin();
				item_iter != items.end();
				++item_iter)
			{
				LLViewerInventoryItem *item = (*item_iter).get();
				const auto cit = mCategoryMap.find(item->getParentUUID());
				if(cit != unparented)
				{
					const LLViewerInventoryCategory* cat = cit->second.get();
					if(cat->getVersion() != NO_VERSION)
					{
						if (item->getIsBrokenLink())
						{
							LL_DEBUGS(LOG_INV) << "Attempted to add cached link item without baseobj present ( name: "
											   << item->getName() << " itemID: " << item->getUUID()
											   << " assetID: " << item->getAssetUUID()
											   << " ).  Ignoring and invalidating " << cat->getName() << " . " << LL_ENDL;
							possible_broken_links.push_back(item);
							continue;
						}
						if (item->getIsLinkType())
						{
							good_link_count++;
						}
						addItem(item);
						cached_item_count += 1;
						++child_counts[cat->getUUID()];
					}
				}
			}
			if (!possible_broken_links.empty())
			{
				for(item_array_t::const_iterator item_iter = possible_broken_links.begin();
				    item_iter != possible_broken_links.end();
				    ++item_iter)
				{
					LLViewerInventoryItem *item = (*item_iter).get();
					const auto cit = mCategoryMap.find(item->getParentUUID());
					const LLViewerInventoryCategory* cat = cit->second.get();
					if (item->getIsBrokenLink())
					{
						bad_link_count++;
						invalid_categories.insert(cit->second);
					}
					else
					{
						addItem(item);
						cached_item_count += 1;
						++child_counts[cat->getUUID()];
						recovered_link_count++;
					}
				}
 				LL_INFOS(LOG_INV) << "Attempted to add " << bad_link_count
								  << " cached link items without baseobj present. "
								  << good_link_count << " link items were successfully added. "
								  << recovered_link_count << " links added in recovery. "
								  << "The corresponding categories were invalidated." << LL_ENDL;
			}
		}
		else
		{
			for (const auto& temp_cat : temp_cats)
			{
				LLViewerInventoryCategory *llvic = temp_cat;
				llvic->setVersion(NO_VERSION);
				addCategory(temp_cat);
			}
		}
		for (const auto& invalid_categorie : invalid_categories)
		{
			LLViewerInventoryCategory* cat = invalid_categorie.get();
			cat->setVersion(NO_VERSION);
			LL_DEBUGS(LOG_INV) << "Invalidating category name: " << cat->getName() << " UUID: " << cat->getUUID() << " due to invalid descendents cache" << LL_ENDL;
		}
		LL_INFOS(LOG_INV) << "Invalidated " << invalid_categories.size() << " categories due to invalid descendents cache" << LL_ENDL;
		update_map_t::const_iterator no_child_counts = child_counts.end();
		for (const auto& temp_cat : temp_cats)
		{
			LLViewerInventoryCategory* cat = temp_cat.get();
			if(cat->getVersion() != NO_VERSION)
			{
				update_map_t::const_iterator the_count = child_counts.find(cat->getUUID());
				if(the_count != no_child_counts)
				{
					const S32 num_descendents = (*the_count).second.mValue;
					cat->setDescendentCount(num_descendents);
				}
				else
				{
					cat->setDescendentCount(0);
				}
			}
		}
		if(remove_inventory_file)
		{
			LLFile::remove(inventory_filename);
		}
		if(is_cache_obsolete)
		{
			LL_WARNS(LOG_INV) << "Inv cache out of date, removing" << LL_ENDL;
			LLFile::remove(gzip_filename);
		}
		categories.clear();
	}
	LL_INFOS(LOG_INV) << "Successfully loaded " << cached_category_count
					  << " categories and " << cached_item_count << " items from cache."
					  << LL_ENDL;
	return rv;
}
void LLInventoryModel::buildParentChildMap()
{
	LL_INFOS(LOG_INV) << "LLInventoryModel::buildParentChildMap()" << LL_ENDL;
	cat_array_t cats;
	cat_array_t* catsp;
	item_array_t* itemsp;
	for (auto& cit : mCategoryMap)
	{
		LLViewerInventoryCategory* cat = cit.second;
		cats.push_back(cat);
		if (mParentChildCategoryTree.count(cat->getUUID()) == 0)
		{
			llassert_always(mCategoryLock[cat->getUUID()] == false);
			catsp = new cat_array_t;
			mParentChildCategoryTree[cat->getUUID()] = catsp;
		}
		if (mParentChildItemTree.count(cat->getUUID()) == 0)
		{
			llassert_always(mItemLock[cat->getUUID()] == false);
			itemsp = new item_array_t;
			mParentChildItemTree[cat->getUUID()] = itemsp;
		}
	}
	if (mParentChildCategoryTree.count(LLUUID::null) == 0)
	{
		catsp = new cat_array_t;
		mParentChildCategoryTree[LLUUID::null] = catsp;
	}
	U32 count = cats.size();
	U32 i;
	U32 lost = 0;
	cat_array_t lost_cats;
	for(i = 0; i < count; ++i)
	{
		LLViewerInventoryCategory* cat = cats.at(i);
		catsp = getUnlockedCatArray(cat->getParentUUID());
		if (catsp &&
			(cat->getParentUUID().notNull() ||
				(cat->getPreferredType() == LLFolderType::FT_ROOT_INVENTORY
					|| (cat->getParentUUID().isNull() && cat->getName() == "My Inventory"))))
		{
			catsp->push_back(cat);
		}
		else
		{
			LL_INFOS(LOG_INV) << "Lost category: " << cat->getUUID() << " - "
							  << cat->getName() << LL_ENDL;
			++lost;
			lost_cats.push_back(cat);
		}
	}
	if(lost)
	{
		LL_WARNS(LOG_INV) << "Found  " << lost << " lost categories." << LL_ENDL;
	}
	for(i = 0; i<lost_cats.size(); ++i)
	{
		LLViewerInventoryCategory *cat = lost_cats.at(i);
		LLFolderType::EType pref = cat->getPreferredType();
		if(LLFolderType::FT_NONE == pref)
		{
			cat->setParent(findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND));
		}
		else if(LLFolderType::FT_ROOT_INVENTORY == pref)
		{
			cat->setParent(LLUUID::null);
		}
		else
		{
			cat->setParent(gInventory.getRootFolderID());
		}
		cat->updateParentOnServer(FALSE);
		catsp = getUnlockedCatArray(cat->getParentUUID());
		if(catsp)
		{
			catsp->push_back(cat);
		}
		else
		{
			LL_WARNS(LOG_INV) << "Lost and found Not there!!" << LL_ENDL;
		}
	}
	const BOOL COF_exists = (findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, FALSE) != LLUUID::null);
	sFirstTimeInViewer2 = !COF_exists || gAgent.isFirstLogin();
	item_array_t items;
	if(!mItemMap.empty())
	{
		for (auto& iit : mItemMap)
		{
			LLPointer<LLViewerInventoryItem> item = iit.second;
			items.push_back(item);
		}
	}
	count = items.size();
	lost = 0;
	uuid_vec_t lost_item_ids;
	for(i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> item = items.at(i);
		itemsp = getUnlockedItemArray(item->getParentUUID());
		if(itemsp)
		{
			itemsp->push_back(item);
		}
		else
		{
			LL_INFOS(LOG_INV) << "Lost item: " << item->getUUID() << " - "
							  << item->getName() << LL_ENDL;
			++lost;
			item->setParent(findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND));
			lost_item_ids.push_back(item->getUUID());
			itemsp = getUnlockedItemArray(item->getParentUUID());
			if(itemsp)
			{
				itemsp->push_back(item);
			}
			else
			{
				LL_WARNS(LOG_INV) << "Lost and found Not there!!" << LL_ENDL;
			}
		}
	}
	if(lost)
	{
		LL_WARNS(LOG_INV) << "Found " << lost << " lost items." << LL_ENDL;
		LLMessageSystem* msg = gMessageSystem;
		BOOL start_new_message = TRUE;
		const LLUUID lnf = findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
		for(uuid_vec_t::iterator it = lost_item_ids.begin() ; it < lost_item_ids.end(); ++it)
		{
			if(start_new_message)
			{
				start_new_message = FALSE;
				msg->newMessageFast(_PREHASH_MoveInventoryItem);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addBOOLFast(_PREHASH_Stamp, FALSE);
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_ItemID, (*it));
			msg->addUUIDFast(_PREHASH_FolderID, lnf);
			msg->addString("NewName", nullptr);
			if(msg->isSendFull(NULL))
			{
				start_new_message = TRUE;
				gAgent.sendReliableMessage();
			}
		}
		if(!start_new_message)
		{
			gAgent.sendReliableMessage();
		}
	}
	const LLUUID &agent_inv_root_id = gInventory.getRootFolderID();
	if (agent_inv_root_id.notNull())
	{
		cat_array_t* catsp = get_ptr_in_map(mParentChildCategoryTree, agent_inv_root_id);
		if(catsp)
		{
			static const std::string name = "My Inventory";
			LLUUID prev_root_id = mRootFolderID;
			for (parent_cat_map_t::const_iterator it = mParentChildCategoryTree.begin(),
					 it_end = mParentChildCategoryTree.end(); it != it_end; ++it)
			{
				cat_array_t* cat_array = it->second;
				for (cat_array_t::const_iterator cat_it = cat_array->begin(),
						 cat_it_end = cat_array->end(); cat_it != cat_it_end; ++cat_it)
					{
					LLPointer<LLViewerInventoryCategory> category = *cat_it;
					if(category && category->getPreferredType() != LLFolderType::FT_ROOT_INVENTORY)
						continue;
					if ( category && 0 == LLStringUtil::compareInsensitive(name, category->getName()) )
					{
						if(category->getUUID()!=mRootFolderID)
						{
							LLUUID& new_inv_root_folder_id = const_cast<LLUUID&>(mRootFolderID);
							new_inv_root_folder_id = category->getUUID();
						}
					}
				}
			}
			mIsAgentInvUsable = true;
			AIEvent::trigger(AIEvent::LLInventoryModel_mIsAgentInvUsable_true);
		}
	}
	if (!gInventory.validate())
	{
	 	LL_WARNS(LOG_INV) << "model failed validity check!" << LL_ENDL;
	}
}
void LLInventoryModel::createCommonSystemCategories()
{
	gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH,true);
	gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE,true);
	gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD,true);
	gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS,true);
}
struct LLUUIDAndName
{
	LLUUIDAndName() = default;
	LLUUIDAndName(const LLUUID& id, const std::string& name);
	bool operator==(const LLUUIDAndName& rhs) const;
	bool operator<(const LLUUIDAndName& rhs) const;
	bool operator>(const LLUUIDAndName& rhs) const;
	LLUUID mID;
	std::string mName;
};
LLUUIDAndName::LLUUIDAndName(const LLUUID& id, const std::string& name) :
	mID(id), mName(name)
{
}
bool LLUUIDAndName::operator==(const LLUUIDAndName& rhs) const
{
	return ((mID == rhs.mID) && (mName == rhs.mName));
}
bool LLUUIDAndName::operator<(const LLUUIDAndName& rhs) const
{
	return (mID < rhs.mID);
}
bool LLUUIDAndName::operator>(const LLUUIDAndName& rhs) const
{
	return (mID > rhs.mID);
}
bool LLInventoryModel::loadFromFile(const std::string& filename,
									LLInventoryModel::cat_array_t& categories,
									LLInventoryModel::item_array_t& items,
									LLInventoryModel::changed_items_t& cats_to_update,
									bool &is_cache_obsolete)
{
	if(filename.empty())
	{
		LL_ERRS(LOG_INV) << "Filename is Null!" << LL_ENDL;
		return false;
	}
	LL_INFOS(LOG_INV) << "LLInventoryModel::loadFromFile(" << filename << ")" << LL_ENDL;
	LLFILE* file = LLFile::fopen(filename, "rb");
	if(!file)
	{
		LL_INFOS(LOG_INV) << "unable to load inventory from: " << filename << LL_ENDL;
		return false;
	}
	char buffer[MAX_STRING];
	char keyword[MAX_STRING];
	char value[MAX_STRING];
	is_cache_obsolete = true;
	while(!feof(file) && fgets(buffer, MAX_STRING, file))
	{
		sscanf(buffer, " %126s %126s", keyword, value);
		if(0 == strcmp("inv_cache_version", keyword))
		{
			S32 version;
			int succ = sscanf(value,"%d",&version);
			if ((1 == succ) && (version == sCurrentInvCacheVersion))
			{
				is_cache_obsolete = false;
				continue;
			}
			else
			{
				break;
			}
		}
		else if(0 == strcmp("inv_category", keyword))
		{
			if (is_cache_obsolete)
				break;
			LLPointer<LLViewerInventoryCategory> inv_cat = new LLViewerInventoryCategory(LLUUID::null);
			if(inv_cat->importFileLocal(file))
			{
				categories.push_back(inv_cat);
			}
			else
			{
				LL_WARNS(LOG_INV) << "loadInventoryFromFile().  Ignoring invalid inventory category: " << inv_cat->getName() << LL_ENDL;
			}
		}
		else if(0 == strcmp("inv_item", keyword))
		{
			if (is_cache_obsolete)
				break;
			LLPointer<LLViewerInventoryItem> inv_item = new LLViewerInventoryItem;
			if( inv_item->importFileLocal(file) )
			{
				if(inv_item->getUUID().isNull())
				{
					LL_WARNS(LOG_INV) << "Ignoring inventory with null item id: "
									  << inv_item->getName() << LL_ENDL;
				}
				else
				{
					if (inv_item->getType() == LLAssetType::AT_UNKNOWN)
					{
						cats_to_update.insert(inv_item->getParentUUID());
					}
					else
					{
						items.push_back(inv_item);
					}
				}
			}
			else
			{
				LL_WARNS(LOG_INV) << "loadInventoryFromFile().  Ignoring invalid inventory item: " << inv_item->getName() << LL_ENDL;
			}
		}
		else
		{
			LL_WARNS(LOG_INV) << "Unknown token in inventory file '" << keyword << "'"
							  << LL_ENDL;
		}
	}
	fclose(file);
	if (is_cache_obsolete)
		return false;
	return true;
}
bool LLInventoryModel::saveToFile(const std::string& filename,
								  const cat_array_t& categories,
								  const item_array_t& items)
{
	if(filename.empty())
	{
		LL_ERRS(LOG_INV) << "Filename is Null!" << LL_ENDL;
		return false;
	}
	LL_INFOS(LOG_INV) << "LLInventoryModel::saveToFile(" << filename << ")" << LL_ENDL;
	LLFILE* file = LLFile::fopen(filename, "wb");
	if(!file)
	{
		LL_WARNS(LOG_INV) << "unable to save inventory to: " << filename << LL_ENDL;
		return false;
	}
	fprintf(file, "\tinv_cache_version\t%d\n",sCurrentInvCacheVersion);
	S32 count = categories.size();
	S32 i;
	for(i = 0; i < count; ++i)
	{
		LLViewerInventoryCategory* cat = categories[i];
		if(cat->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			cat->exportFileLocal(file);
		}
	}
	count = items.size();
	for(i = 0; i < count; ++i)
	{
		items[i]->exportFile(file);
	}
	fclose(file);
	return true;
}
void LLInventoryModel::registerCallbacks(LLMessageSystem* msg)
{
	msg->setHandlerFuncFast(_PREHASH_UpdateCreateInventoryItem,
						processUpdateCreateInventoryItem,
						NULL);
	msg->setHandlerFuncFast(_PREHASH_RemoveInventoryItem,
						processRemoveInventoryItem,
						NULL);
	msg->setHandlerFuncFast(_PREHASH_UpdateInventoryFolder,
						processUpdateInventoryFolder,
						NULL);
	msg->setHandlerFuncFast(_PREHASH_RemoveInventoryFolder,
						processRemoveInventoryFolder,
						NULL);
	msg->setHandlerFuncFast(_PREHASH_RemoveInventoryObjects,
							processRemoveInventoryObjects,
							NULL);
	msg->setHandlerFuncFast(_PREHASH_SaveAssetIntoInventory,
						processSaveAssetIntoInventory,
						NULL);
	msg->setHandlerFuncFast(_PREHASH_BulkUpdateInventory,
							processBulkUpdateInventory,
							NULL);
	msg->setHandlerFunc("InventoryDescendents", processInventoryDescendents);
	msg->setHandlerFunc("MoveInventoryItem", processMoveInventoryItem);
	msg->setHandlerFunc("FetchInventoryReply", processFetchInventoryReply);
}
void LLInventoryModel::processUpdateCreateInventoryItem(LLMessageSystem* msg, void**)
{
	if (gInventory.messageUpdateCore(msg, true, LLInventoryObserver::UPDATE_CREATE))
	{
		U32 callback_id;
		LLUUID item_id;
		msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id);
		msg->getU32Fast(_PREHASH_InventoryData, _PREHASH_CallbackID, callback_id);
		gInventoryCallbacks.fire(callback_id, item_id);
	}
}
void LLInventoryModel::processFetchInventoryReply(LLMessageSystem* msg, void**)
{
	gInventory.messageUpdateCore(msg, false);
}
bool LLInventoryModel::messageUpdateCore(LLMessageSystem* msg, bool account, U32 mask)
{
	start_new_inventory_observer();
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS(LOG_INV) << "Got a inventory update for the wrong agent: " << agent_id
						  << LL_ENDL;
		return false;
	}
	item_array_t items;
	update_map_t update;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_InventoryData);
	for(S32 i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
		titem->unpackMessage(msg, _PREHASH_InventoryData, i);
		LL_DEBUGS(LOG_INV) << "LLInventoryModel::messageUpdateCore() item id: "
						   << titem->getUUID() << LL_ENDL;
		items.push_back(titem);
		LLViewerInventoryItem* itemp = gInventory.getItem(titem->getUUID());
		if(itemp)
		{
			if(titem->getParentUUID() == itemp->getParentUUID())
			{
				update[titem->getParentUUID()];
			}
			else
			{
				++update[titem->getParentUUID()];
				--update[itemp->getParentUUID()];
			}
		}
		else
		{
			++update[titem->getParentUUID()];
		}
	}
	if(account)
	{
		gInventory.accountForUpdate(update);
	}
	U32 changes = 0x0;
	if (account)
	{
		mask |= LLInventoryObserver::CREATE;
	}
	for (auto& item : items)
	{
		changes |= gInventory.updateItem(item, mask);
	}
	gInventory.notifyObservers();
	gViewerWindow->getWindow()->decBusyCount();
	return true;
}
void LLInventoryModel::removeInventoryItem(LLUUID agent_id, LLMessageSystem* msg, const char* msg_label)
{
	LLUUID item_id;
	S32 count = msg->getNumberOfBlocksFast(msg_label);
	LL_DEBUGS(LOG_INV) << "Message has " << count << " item blocks" << LL_ENDL;
	uuid_vec_t item_ids;
	update_map_t update;
	for(S32 i = 0; i < count; ++i)
	{
		msg->getUUIDFast(msg_label, _PREHASH_ItemID, item_id, i);
		LL_DEBUGS(LOG_INV) << "Checking for item-to-be-removed " << item_id << LL_ENDL;
		LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
		if(itemp)
		{
			LL_DEBUGS(LOG_INV) << "Item will be removed " << item_id << LL_ENDL;
			--update[itemp->getParentUUID()];
			item_ids.push_back(item_id);
		}
	}
	gInventory.accountForUpdate(update);
	for (auto& item_id : item_ids)
	{
		LL_DEBUGS(LOG_INV) << "Calling deleteObject " << item_id << LL_ENDL;
		gInventory.deleteObject(item_id);
	}
}
void LLInventoryModel::processRemoveInventoryItem(LLMessageSystem* msg, void**)
{
	LL_DEBUGS(LOG_INV) << "LLInventoryModel::processRemoveInventoryItem()" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS(LOG_INV) << "Got a RemoveInventoryItem for the wrong agent."
						  << LL_ENDL;
		return;
	}
	LLInventoryModel::removeInventoryItem(agent_id, msg, _PREHASH_InventoryData);
	gInventory.notifyObservers();
}
void LLInventoryModel::processUpdateInventoryFolder(LLMessageSystem* msg,
													void**)
{
	LL_DEBUGS(LOG_INV) << "LLInventoryModel::processUpdateInventoryFolder()" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_FolderData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS(LOG_INV) << "Got an UpdateInventoryFolder for the wrong agent."
						  << LL_ENDL;
		return;
	}
	LLPointer<LLViewerInventoryCategory> lastfolder;
	cat_array_t folders;
	update_map_t update;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
	for(S32 i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryCategory> tfolder = new LLViewerInventoryCategory(gAgent.getID());
		lastfolder = tfolder;
		tfolder->unpackMessage(msg, _PREHASH_FolderData, i);
		tfolder->setPreferredType(LLFolderType::FT_NONE);
		folders.push_back(tfolder);
		LLViewerInventoryCategory* folderp = gInventory.getCategory(tfolder->getUUID());
		if(folderp)
		{
			if(tfolder->getParentUUID() == folderp->getParentUUID())
			{
				update[tfolder->getParentUUID()];
			}
			else
			{
				++update[tfolder->getParentUUID()];
				--update[folderp->getParentUUID()];
			}
		}
		else
		{
			++update[tfolder->getParentUUID()];
		}
	}
	gInventory.accountForUpdate(update);
	for (auto& folder : folders)
	{
		gInventory.updateCategory(folder);
	}
	gInventory.notifyObservers();
	LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();
	if (active_panel)
	{
		active_panel->setSelection(lastfolder->getUUID(), TAKE_FOCUS_NO);
	}
}
void LLInventoryModel::removeInventoryFolder(LLUUID agent_id, LLMessageSystem* msg)
{
	LLUUID folder_id;
	uuid_vec_t folder_ids;
	update_map_t update;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
	for(S32 i = 0; i < count; ++i)
	{
		msg->getUUIDFast(_PREHASH_FolderData, _PREHASH_FolderID, folder_id, i);
		LLViewerInventoryCategory* folderp = gInventory.getCategory(folder_id);
		if(folderp)
		{
			--update[folderp->getParentUUID()];
			folder_ids.push_back(folder_id);
		}
	}
	gInventory.accountForUpdate(update);
	for (auto& folder_id : folder_ids)
	{
		gInventory.deleteObject(folder_id);
	}
}
void LLInventoryModel::processRemoveInventoryFolder(LLMessageSystem* msg, void**)
{
	LL_DEBUGS() << "LLInventoryModel::processRemoveInventoryFolder()" << LL_ENDL;
	LLUUID agent_id, session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got a RemoveInventoryFolder for the wrong agent."
		<< LL_ENDL;
		return;
	}
	LLInventoryModel::removeInventoryFolder( agent_id, msg );
	gInventory.notifyObservers();
}
void LLInventoryModel::processRemoveInventoryObjects(LLMessageSystem* msg, void**)
{
	LL_DEBUGS() << "LLInventoryModel::processRemoveInventoryObjects()" << LL_ENDL;
	LLUUID agent_id, session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got a RemoveInventoryObjects for the wrong agent."
		<< LL_ENDL;
		return;
	}
	LLInventoryModel::removeInventoryFolder( agent_id, msg );
	LLInventoryModel::removeInventoryItem( agent_id, msg, _PREHASH_ItemData );
	gInventory.notifyObservers();
}
void LLInventoryModel::processSaveAssetIntoInventory(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got a SaveAssetIntoInventory message for the wrong agent."
				<< LL_ENDL;
		return;
	}
	LLUUID item_id;
	msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id);
	LL_DEBUGS() << "LLInventoryModel::processSaveAssetIntoInventory itemID="
		<< item_id << LL_ENDL;
	LLViewerInventoryItem* item = gInventory.getItem( item_id );
	if( item )
	{
		LLCategoryUpdate up(item->getParentUUID(), 0);
		gInventory.accountForUpdate(up);
		gInventory.addChangedMask( LLInventoryObserver::INTERNAL, item_id);
		gInventory.notifyObservers();
	}
	else
	{
		LL_INFOS() << "LLInventoryModel::processSaveAssetIntoInventory item"
			" not found: " << item_id << LL_ENDL;
	}
	if (rlv_handler_t::isEnabled())
	{
		RlvAttachmentLockWatchdog::instance().onSavedAssetIntoInventory(item_id);
	}
	if(gViewerWindow)
	{
		gViewerWindow->getWindow()->decBusyCount();
	}
}
struct InventoryCallbackInfo
{
	InventoryCallbackInfo(U32 callback, const LLUUID& inv_id) :
		mCallback(callback), mInvID(inv_id) {}
	U32 mCallback;
	LLUUID mInvID;
};
void LLInventoryModel::processBulkUpdateInventory(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got a BulkUpdateInventory for the wrong agent." << LL_ENDL;
		return;
	}
	LLUUID tid;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TransactionID, tid);
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LL_DEBUGS("Inventory") << "Bulk inventory: " << tid << LL_ENDL;
#endif
	update_map_t update;
	cat_array_t folders;
	S32 i;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
	for(i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryCategory> tfolder = new LLViewerInventoryCategory(gAgent.getID());
		tfolder->unpackMessage(msg, _PREHASH_FolderData, i);
		LL_DEBUGS("Inventory") << "unpacked folder '" << tfolder->getName() << "' ("
							   << tfolder->getUUID() << ") in " << tfolder->getParentUUID()
							   << LL_ENDL;
		int depth_folder = depth_nesting_in_marketplace(tfolder->getUUID());
		if ((depth_folder == 1) || (depth_folder == 2))
		{
			LLUUID listing_uuid = (depth_folder == 1 ? tfolder->getUUID() : tfolder->getParentUUID());
			S32 listing_id = LLMarketplaceData::instance().getListingID(listing_uuid);
			LLMarketplaceData::instance().getListing(listing_id);
		}
		else if(tfolder->getUUID().notNull())
		{
			folders.push_back(tfolder);
			LLViewerInventoryCategory* folderp = gInventory.getCategory(tfolder->getUUID());
			if(folderp)
			{
				if(tfolder->getParentUUID() == folderp->getParentUUID())
				{
					if ( (rlv_handler_t::isEnabled()) && (!folderp->getName().empty()) && (tfolder->getName() != folderp->getName()) &&
						 ((tfolder->getName().find(RLV_PUTINV_PREFIX) == 0)) )
					{
						tfolder->rename(folderp->getName());
					}
					update[tfolder->getParentUUID()];
				}
				else
				{
					++update[tfolder->getParentUUID()];
					--update[folderp->getParentUUID()];
				}
			}
			else
			{
				folderp = gInventory.getCategory(tfolder->getParentUUID());
				if(folderp)
				{
					++update[tfolder->getParentUUID()];
				}
			}
		}
	}
	count = msg->getNumberOfBlocksFast(_PREHASH_ItemData);
	uuid_vec_t wearable_ids;
	item_array_t items;
	std::list<InventoryCallbackInfo> cblist;
	for(i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
		titem->unpackMessage(msg, _PREHASH_ItemData, i);
		LL_DEBUGS("Inventory") << "unpacked item '" << titem->getName() << "' in "
							   << titem->getParentUUID() << LL_ENDL;
		U32 callback_id;
		msg->getU32Fast(_PREHASH_ItemData, _PREHASH_CallbackID, callback_id);
		if(titem->getUUID().notNull() )
		{
			items.push_back(titem);
			cblist.emplace_back(callback_id, titem->getUUID());
			if (titem->getInventoryType() == LLInventoryType::IT_WEARABLE)
			{
				wearable_ids.push_back(titem->getUUID());
			}
			LLViewerInventoryItem* itemp = gInventory.getItem(titem->getUUID());
			if(itemp)
			{
				if(titem->getParentUUID() == itemp->getParentUUID())
				{
					update[titem->getParentUUID()];
				}
				else
				{
					++update[titem->getParentUUID()];
					--update[itemp->getParentUUID()];
				}
			}
			else
			{
				LLViewerInventoryCategory* folderp = gInventory.getCategory(titem->getParentUUID());
				if(folderp)
				{
					++update[titem->getParentUUID()];
				}
			}
		}
		else
		{
			cblist.emplace_back(callback_id, LLUUID::null);
		}
	}
	gInventory.accountForUpdate(update);
	for (auto& folder : folders)
	{
		gInventory.updateCategory(folder);
	}
	for (auto& item : items)
	{
		gInventory.updateItem(item);
	}
	gInventory.notifyObservers();
	if (LLInventoryState::sWearNewClothing)
	{
		LLInventoryState::sWearNewClothingTransactionID = tid;
		LLInventoryState::sWearNewClothing = FALSE;
	}
	if (tid.notNull() && tid == LLInventoryState::sWearNewClothingTransactionID)
	{
		count = wearable_ids.size();
		for (i = 0; i < count; ++i)
		{
			LLViewerInventoryItem * wearable_item = gInventory.getItem(wearable_ids[i]);
			LLAppearanceMgr::instance().wearItemOnAvatar(wearable_item->getUUID(), true, true);
		}
	}
	for (auto& inv_it : cblist)
	{
		InventoryCallbackInfo cbinfo = inv_it;
		gInventoryCallbacks.fire(cbinfo.mCallback, cbinfo.mInvID);
	}
}
void LLInventoryModel::processInventoryDescendents(LLMessageSystem* msg,void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got a UpdateInventoryItem for the wrong agent." << LL_ENDL;
		return;
	}
	LLUUID parent_id;
	msg->getUUID("AgentData", "FolderID", parent_id);
	LLUUID owner_id;
	msg->getUUID("AgentData", "OwnerID", owner_id);
	S32 version;
	msg->getS32("AgentData", "Version", version);
	S32 descendents;
	msg->getS32("AgentData", "Descendents", descendents);
	S32 i;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
	LLPointer<LLViewerInventoryCategory> tcategory = new LLViewerInventoryCategory(owner_id);
	for(i = 0; i < count; ++i)
	{
		tcategory->unpackMessage(msg, _PREHASH_FolderData, i);
		gInventory.updateCategory(tcategory);
	}
	count = msg->getNumberOfBlocksFast(_PREHASH_ItemData);
	LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
	for(i = 0; i < count; ++i)
	{
		titem->unpackMessage(msg, _PREHASH_ItemData, i);
		if (gInventory.getItem(titem->getUUID()))
		{
			LL_DEBUGS("Inventory") << "Skipping prefetched item [ Name: " << titem->getName()
								   << " | Type: " << titem->getActualType() << " | ItemUUID: " << titem->getUUID() << " ] " << LL_ENDL;
			continue;
		}
		gInventory.updateItem(titem);
	}
	LLViewerInventoryCategory* cat = gInventory.getCategory(parent_id);
	if(cat)
	{
		cat->setVersion(version);
		cat->setDescendentCount(descendents);
		gInventory.addChangedMask(LLInventoryObserver::INTERNAL, cat->getUUID());
	}
	gInventory.notifyObservers();
}
void LLInventoryModel::processMoveInventoryItem(LLMessageSystem* msg, void**)
{
	LL_DEBUGS() << "LLInventoryModel::processMoveInventoryItem()" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got a MoveInventoryItem message for the wrong agent."
				<< LL_ENDL;
		return;
	}
	LLUUID item_id;
	LLUUID folder_id;
	std::string new_name;
	bool anything_changed = false;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_InventoryData);
	for(S32 i = 0; i < count; ++i)
	{
		msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id, i);
		LLViewerInventoryItem* item = gInventory.getItem(item_id);
		if(item)
		{
			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
			msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_FolderID, folder_id, i);
			msg->getString("InventoryData", "NewName", new_name, i);
			LL_DEBUGS() << "moving item " << item_id << " to folder "
					 << folder_id << LL_ENDL;
			update_list_t update;
			LLCategoryUpdate old_folder(item->getParentUUID(), -1);
			update.push_back(old_folder);
			LLCategoryUpdate new_folder(folder_id, 1);
			update.push_back(new_folder);
			gInventory.accountForUpdate(update);
			new_item->setParent(folder_id);
			if (new_name.length() > 0)
			{
				new_item->rename(new_name);
			}
			gInventory.updateItem(new_item);
			anything_changed = true;
		}
		else
		{
			LL_INFOS() << "LLInventoryModel::processMoveInventoryItem item not found: " << item_id << LL_ENDL;
		}
	}
	if(anything_changed)
	{
		gInventory.notifyObservers();
	}
}
bool LLInventoryModel::callbackEmptyFolderType(const LLSD& notification, const LLSD& response, LLFolderType::EType preferred_type)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		const LLUUID folder_id = findCategoryUUIDForType(preferred_type);
		purge_descendents_of(folder_id, nullptr);
	}
	return false;
}
void LLInventoryModel::emptyFolderType(const std::string notification, LLFolderType::EType preferred_type)
{
	if (!notification.empty())
	{
		LLNotificationsUtil::add(notification, LLSD(), LLSD(),
										boost::bind(&LLInventoryModel::callbackEmptyFolderType, this, _1, _2, preferred_type));
	}
	else
	{
		const LLUUID folder_id = findCategoryUUIDForType(preferred_type);
		purge_descendents_of(folder_id, nullptr);
	}
}
void LLInventoryModel::removeItem(const LLUUID& item_id)
{
	LLViewerInventoryItem* item = getItem(item_id);
	if (! item)
	{
		LL_WARNS("Inventory") << "couldn't find inventory item " << item_id << LL_ENDL;
	}
	else
	{
		const LLUUID new_parent = findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if (new_parent.notNull())
		{
			LL_INFOS("Inventory") << "Moving to Trash (" << new_parent << "):" << LL_ENDL;
			changeItemParent(item, new_parent, TRUE);
		}
	}
}
void LLInventoryModel::removeCategory(const LLUUID& category_id)
{
	if (! get_is_category_removable(this, category_id))
	{
		return;
	}
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	collectDescendents(category_id, descendent_categories, descendent_items, FALSE);
	for (LLInventoryModel::item_array_t::const_iterator iter = descendent_items.begin();
		 iter != descendent_items.end();
		 ++iter)
	{
		const LLViewerInventoryItem* item = (*iter);
		const LLUUID& item_id = item->getUUID();
		if (item->getType() == LLAssetType::AT_GESTURE
			&& LLGestureMgr::instance().isGestureActive(item_id))
		{
			LLGestureMgr::instance().deactivateGesture(item_id);
		}
	}
	LLViewerInventoryCategory* cat = getCategory(category_id);
	if (cat)
	{
		const LLUUID trash_id = findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if (trash_id.notNull())
		{
			changeCategoryParent(cat, trash_id, TRUE);
		}
	}
}
void LLInventoryModel::removeObject(const LLUUID& object_id)
{
	if(object_id.isNull())
	{
		return;
	}
	LLInventoryObject* obj = getObject(object_id);
	if (dynamic_cast<LLViewerInventoryItem*>(obj))
	{
		removeItem(object_id);
	}
	else if (dynamic_cast<LLViewerInventoryCategory*>(obj))
	{
		removeCategory(object_id);
	}
	else if (obj)
	{
		LL_WARNS("Inventory") << "object ID " << object_id
							  << " is an object of unrecognized class "
							  << typeid(*obj).name() << LL_ENDL;
	}
	else
	{
		LL_WARNS("Inventory") << "object ID " << object_id << " not found" << LL_ENDL;
	}
}
const LLUUID &LLInventoryModel::getRootFolderID() const
{
	return mRootFolderID;
}
void LLInventoryModel::setRootFolderID(const LLUUID& val)
{
	mRootFolderID = val;
}
const LLUUID &LLInventoryModel::getLibraryRootFolderID() const
{
	return mLibraryRootFolderID;
}
void LLInventoryModel::setLibraryRootFolderID(const LLUUID& val)
{
	mLibraryRootFolderID = val;
}
const LLUUID &LLInventoryModel::getLibraryOwnerID() const
{
	return mLibraryOwnerID;
}
void LLInventoryModel::setLibraryOwnerID(const LLUUID& val)
{
	mLibraryOwnerID = val;
}
BOOL LLInventoryModel::getIsFirstTimeInViewer2()
{
	if (!gInventory.mIsAgentInvUsable)
	{
		LL_WARNS() << "Parent Child Map not yet built; guessing as first time in viewer2." << LL_ENDL;
		return TRUE;
	}
	return sFirstTimeInViewer2;
}
LLInventoryModel::item_array_t::iterator LLInventoryModel::findItemIterByUUID(LLInventoryModel::item_array_t& items, const LLUUID& id)
{
	LLInventoryModel::item_array_t::iterator curr_item = items.begin();
	while (curr_item != items.end())
	{
		if ((*curr_item)->getUUID() == id)
		{
			break;
		}
		++curr_item;
	}
	return curr_item;
}
void LLInventoryModel::updateItemsOrder(LLInventoryModel::item_array_t& items, const LLUUID& src_item_id, const LLUUID& dest_item_id, bool insert_before)
{
	LLInventoryModel::item_array_t::iterator it_src = findItemIterByUUID(items, src_item_id);
	LLInventoryModel::item_array_t::iterator it_dest = findItemIterByUUID(items, dest_item_id);
	if ((it_src == items.end()) || (it_dest == items.end()))
		return;
	LLViewerInventoryItem* src_item = *it_src;
	items.erase(it_src);
	it_dest = findItemIterByUUID(items, dest_item_id);
	if (!insert_before)
	{
		++it_dest;
	}
	if (it_dest != items.end())
	{
		items.insert(it_dest, src_item);
	}
	else
	{
		items.push_back(src_item);
	}
}
class LLViewerInventoryItemSort
{
public:
	bool operator()(const LLPointer<LLViewerInventoryItem>& a, const LLPointer<LLViewerInventoryItem>& b) const
	{
		return a->getSortField() < b->getSortField();
	}
};
void LLInventoryModel::dumpInventory() const
{
	LL_INFOS() << "\nBegin Inventory Dump\n**********************:" << LL_ENDL;
	LL_INFOS() << "mCategory[] contains " << mCategoryMap.size() << " items." << LL_ENDL;
	for (const auto& cit : mCategoryMap)
	{
		const LLViewerInventoryCategory* cat = cit.second;
		if(cat)
		{
			LL_INFOS() << "  " <<  cat->getUUID() << " '" << cat->getName() << "' "
					<< cat->getVersion() << " " << cat->getDescendentCount()
					<< " parent: " << cat->getParentUUID()
					<< LL_ENDL;
		}
		else
		{
			LL_INFOS() << "  NULL!" << LL_ENDL;
		}
	}
	LL_INFOS() << "mItemMap[] contains " << mItemMap.size() << " items." << LL_ENDL;
	for (const auto& iit : mItemMap)
	{
		const LLViewerInventoryItem* item = iit.second;
		if(item)
		{
			LL_INFOS() << "  " << item->getUUID() << " "
					<< item->getName() << LL_ENDL;
		}
		else
		{
			LL_INFOS() << "  NULL!" << LL_ENDL;
		}
	}
	LL_INFOS() << "\n**********************\nEnd Inventory Dump" << LL_ENDL;
}
bool LLInventoryModel::validate() const
{
	bool valid = true;
	if (getRootFolderID().isNull())
	{
		LL_WARNS() << "no root folder id" << LL_ENDL;
		valid = false;
	}
	if (getLibraryRootFolderID().isNull())
	{
		LL_WARNS() << "no root folder id" << LL_ENDL;
		valid = false;
	}
	if (mCategoryMap.size() + 1 != mParentChildCategoryTree.size())
	{
		LL_INFOS() << "unexpected sizes: cat map size " << mCategoryMap.size()
				<< " parent/child " << mParentChildCategoryTree.size() << LL_ENDL;
		valid = false;
	}
	S32 cat_lock = 0;
	S32 item_lock = 0;
	S32 desc_unknown_count = 0;
	S32 version_unknown_count = 0;
	for (const auto& cit : mCategoryMap)
	{
		const LLUUID& cat_id = cit.first;
		const LLViewerInventoryCategory *cat = cit.second;
		if (!cat)
		{
			LL_WARNS() << "invalid cat" << LL_ENDL;
			valid = false;
			continue;
		}
		if (cat_id != cat->getUUID())
		{
			LL_WARNS() << "cat id/index mismatch " << cat_id << " " << cat->getUUID() << LL_ENDL;
			valid = false;
		}
		if (cat->getParentUUID().isNull())
		{
			if (cat_id != getRootFolderID() && cat_id != getLibraryRootFolderID())
			{
				LL_WARNS() << "cat " << cat_id << " has no parent, but is not root ("
						<< getRootFolderID() << ") or library root ("
						<< getLibraryRootFolderID() << ")" << LL_ENDL;
			}
		}
		cat_array_t* cats;
		item_array_t* items;
		getDirectDescendentsOf(cat_id,cats,items);
		if (!cats || !items)
		{
			LL_WARNS() << "invalid direct descendents for " << cat_id << LL_ENDL;
			valid = false;
			continue;
		}
		if (cat->getDescendentCount() == LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN)
		{
			desc_unknown_count++;
		}
		else if (cats->size() + items->size() != cat->getDescendentCount())
		{
			LL_WARNS() << "invalid desc count for " << cat_id << " name [" << cat->getName()
					<< "] parent " << cat->getParentUUID()
					<< " cached " << cat->getDescendentCount()
					<< " expected " << cats->size() << "+" << items->size()
					<< "=" << cats->size() +items->size() << LL_ENDL;
			valid = false;
		}
		if (cat->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			version_unknown_count++;
		}
		if (mCategoryLock.count(cat_id))
		{
			cat_lock++;
		}
		if (mItemLock.count(cat_id))
		{
			item_lock++;
		}
		for (U32 i = 0; i<items->size(); i++)
		{
			LLViewerInventoryItem *item = items->at(i);
			if (!item)
			{
				LL_WARNS() << "null item at index " << i << " for cat " << cat_id << LL_ENDL;
				valid = false;
				continue;
			}
			const LLUUID& item_id = item->getUUID();
			if (item->getParentUUID() != cat_id)
			{
				LL_WARNS() << "wrong parent for " << item_id << " found "
						<< item->getParentUUID() << " expected " << cat_id
						<< LL_ENDL;
				valid = false;
			}
			item_map_t::const_iterator it = mItemMap.find(item_id);
			if (it == mItemMap.end())
			{
				LL_WARNS() << "item " << item_id << " found as child of "
						<< cat_id << " but not in top level mItemMap" << LL_ENDL;
				valid = false;
			}
			else
			{
				LLViewerInventoryItem *top_item = it->second;
				if (top_item != item)
				{
					LL_WARNS() << "item mismatch, item_id " << item_id
							<< " top level entry is different, uuid " << top_item->getUUID() << LL_ENDL;
				}
			}
			LLUUID topmost_ancestor_id;
			bool found = getObjectTopmostAncestor(item_id, topmost_ancestor_id);
			if (!found)
			{
				LL_WARNS() << "unable to find topmost ancestor for " << item_id << LL_ENDL;
				valid = false;
			}
			else
			{
				if (topmost_ancestor_id != getRootFolderID() &&
					topmost_ancestor_id != getLibraryRootFolderID())
				{
					LL_WARNS() << "unrecognized top level ancestor for " << item_id
							<< " got " << topmost_ancestor_id
							<< " expected " << getRootFolderID()
							<< " or " << getLibraryRootFolderID() << LL_ENDL;
					valid = false;
				}
			}
		}
		const LLUUID& parent_id = cat->getParentUUID();
		if (!parent_id.isNull())
		{
			cat_array_t* cats;
			item_array_t* items;
			getDirectDescendentsOf(parent_id,cats,items);
			if (!cats)
			{
				LL_WARNS() << "cat " << cat_id << " name [" << cat->getName()
						<< "] orphaned - no child cat array for alleged parent " << parent_id << LL_ENDL;
				valid = false;
			}
			else
			{
				bool found = false;
				for (auto& i : *cats)
				{
					LLViewerInventoryCategory *kid_cat = i;
					if (kid_cat == cat)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					LL_WARNS() << "cat " << cat_id << " name [" << cat->getName()
							<< "] orphaned - not found in child cat array of alleged parent " << parent_id << LL_ENDL;
				}
			}
		}
	}
	for (const auto& iit : mItemMap)
	{
		const LLUUID& item_id = iit.first;
		LLViewerInventoryItem *item = iit.second;
		if (item->getUUID() != item_id)
		{
			LL_WARNS() << "item_id " << item_id << " does not match " << item->getUUID() << LL_ENDL;
			valid = false;
		}
		const LLUUID& parent_id = item->getParentUUID();
		if (parent_id.isNull())
		{
			LL_WARNS() << "item " << item_id << " name [" << item->getName() << "] has null parent id!" << LL_ENDL;
		}
		else
		{
			cat_array_t* cats;
			item_array_t* items;
			getDirectDescendentsOf(parent_id,cats,items);
			if (!items)
			{
				LL_WARNS() << "item " << item_id << " name [" << item->getName()
						<< "] orphaned - alleged parent has no child items list " << parent_id << LL_ENDL;
			}
			else
			{
				bool found = false;
				for (auto& i : *items)
				{
					if (i == item)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					LL_WARNS() << "item " << item_id << " name [" << item->getName()
							<< "] orphaned - not found as child of alleged parent " << parent_id << LL_ENDL;
				}
			}
		}
		if (item->getIsLinkType())
		{
			const LLUUID& link_id = item->getUUID();
			const LLUUID& target_id = item->getLinkedUUID();
			LLViewerInventoryItem *target_item = getItem(target_id);
			LLViewerInventoryCategory *target_cat = getCategory(target_id);
			if (!hasBacklinkInfo(link_id, target_id))
			{
				LL_WARNS() << "link " << item->getUUID() << " type " << item->getActualType()
						<< " missing backlink info at target_id " << target_id
						<< LL_ENDL;
			}
			if (item->getActualType() == LLAssetType::AT_LINK && !target_item)
			{
				LL_WARNS() << "broken item link " << item->getName() << " id " << item->getUUID() << LL_ENDL;
			}
			else if (item->getActualType() == LLAssetType::AT_LINK_FOLDER && !target_cat)
			{
				LL_WARNS() << "broken folder link " << item->getName() << " id " << item->getUUID() << LL_ENDL;
			}
			if (target_item && target_item->getIsLinkType())
			{
				LL_WARNS() << "link " << item->getName() << " references a link item "
						<< target_item->getName() << " " << target_item->getUUID() << LL_ENDL;
			}
			std::pair<backlink_mmap_t::const_iterator, backlink_mmap_t::const_iterator> range = mBacklinkMMap.equal_range(link_id);
			if (range.first != range.second)
			{
				LL_WARNS() << "Link item " << item->getName() << " has backlinks!" << LL_ENDL;
			}
		}
		else
		{
			const LLUUID& target_id = item->getUUID();
			std::pair<backlink_mmap_t::const_iterator, backlink_mmap_t::const_iterator> range = mBacklinkMMap.equal_range(target_id);
			for (backlink_mmap_t::const_iterator it = range.first; it != range.second; ++it)
			{
				const LLUUID& link_id = it->second;
				LLViewerInventoryItem *link_item = getItem(link_id);
				if (!link_item || !link_item->getIsLinkType())
				{
					LL_WARNS() << "invalid backlink from target " << item->getName() << " to " << link_id << LL_ENDL;
				}
			}
		}
	}
	if (cat_lock > 0 || item_lock > 0)
	{
		LL_INFOS() << "Found locks on some categories: sub-cat arrays "
				<< cat_lock << ", item arrays " << item_lock << LL_ENDL;
	}
	if (desc_unknown_count != 0)
	{
		LL_INFOS() << "Found " << desc_unknown_count << " cats with unknown descendent count" << LL_ENDL;
	}
	if (version_unknown_count != 0)
	{
		LL_INFOS() << "Found " << version_unknown_count << " cats with unknown version" << LL_ENDL;
	}
	LL_INFOS() << "Validate done, valid = " << (U32) valid << LL_ENDL;
	return valid;
}
#if 0
BOOL decompress_file(const char* src_filename, const char* dst_filename)
{
	BOOL rv = FALSE;
	gzFile src = NULL;
	U8* buffer = NULL;
	LLFILE* dst = NULL;
	S32 bytes = 0;
	const S32 DECOMPRESS_BUFFER_SIZE = 32000;
#if LL_WINDOWS
	src = gzopen_w(utf8str_to_utf16str(src_filename).c_str(), "rb");
#else
	src = gzopen(src_filename, "rb");
#endif
	if(!src) goto err_decompress;
	dst = LLFile::fopen(dst_filename, "wb");
	if(!dst) goto err_decompress;
	buffer = new U8[DECOMPRESS_BUFFER_SIZE + 1];
	do
	{
		bytes = gzread(src, buffer, DECOMPRESS_BUFFER_SIZE);
		if (bytes < 0)
		{
			goto err_decompress;
		}
		fwrite(buffer, bytes, 1, dst);
	} while(gzeof(src) == 0);
	rv = TRUE;
 err_decompress:
	if(src != NULL) gzclose(src);
	if(buffer != NULL) delete[] buffer;
	if(dst != NULL) fclose(dst);
	return rv;
}
#endif
void  LLInventoryModel::FetchItemHttpHandler::httpSuccess()
{
	start_new_inventory_observer();
#if 0
	LLUUID agent_id;
	agent_id = mContent["agent_id"].asUUID();
	if (agent_id != gAgent.getID())
	{
		LL_WARNS(LOG_INV) << "Got a inventory update for the wrong agent: " << agent_id
						  << LL_ENDL;
		return;
	}
#endif
	LLInventoryModel::item_array_t items;
	LLInventoryModel::update_map_t update;
	LLUUID folder_id;
	LLSD content_items(mContent["items"]);
	const S32 count(content_items.size());
	for (S32 i(0); i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
		titem->unpackMessage(content_items[i]);
		LL_DEBUGS(LOG_INV) << "ItemHttpHandler::httpSuccess item id: "
						   << titem->getUUID() << LL_ENDL;
		items.push_back(titem);
		LLViewerInventoryItem * itemp(gInventory.getItem(titem->getUUID()));
		if (itemp)
		{
			if (titem->getParentUUID() == itemp->getParentUUID())
			{
				update[titem->getParentUUID()];
			}
			else
			{
				++update[titem->getParentUUID()];
				--update[itemp->getParentUUID()];
			}
		}
		else
		{
			++update[titem->getParentUUID()];
		}
		if (folder_id.isNull())
		{
			folder_id = titem->getParentUUID();
		}
	}
	U32 changes(0U);
	for (auto& item : items)
	{
		changes |= gInventory.updateItem(item);
	}
	gInventory.notifyObservers();
	gViewerWindow->getWindow()->decBusyCount();
}
void LLInventoryModel::FetchItemHttpHandler::httpFailure()
{
	LL_WARNS(LOG_INV) << "FetchItemHttpHandler::error "
		<< mStatus << ": " << mReason << LL_ENDL;
	gInventory.notifyObservers();
}
