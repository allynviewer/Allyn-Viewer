/** 
 * @file llinventorymodel.h
 * @brief LLInventoryModel class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#ifndef LL_LLINVENTORYMODEL_H
#define LL_LLINVENTORYMODEL_H
#include <boost/unordered_map.hpp>
#include <set>
#include <string>
#include <vector>
#include "llassettype.h"
#include "llfoldertype.h"
#include "llframetimer.h"
#include "llhttpclient.h"
#include "lluuid.h"
#include "llpermissionsflags.h"
#include "llviewerinventory.h"
#include "llstring.h"
#include "llmd5.h"
class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy FetchItemHttpHandler_timeout;
class LLInventoryObserver;
class LLInventoryObject;
class LLInventoryItem;
class LLInventoryCategory;
class LLMessageSystem;
class LLInventoryCollectFunctor;
class LLInventoryModel
{
	LOG_CLASS(LLInventoryModel);
public:
	friend class LLInventoryModelFetchDescendentsResponder;
	enum EHasChildren
	{
		CHILDREN_NO,
		CHILDREN_YES,
		CHILDREN_MAYBE
	};
	typedef std::vector<LLPointer<LLViewerInventoryCategory> > cat_array_t;
	typedef std::vector<LLPointer<LLViewerInventoryItem> > item_array_t;
	typedef uuid_set_t changed_items_t;
	class FetchItemHttpHandler : public LLHTTPClient::ResponderWithResult
	{
	public:
		LOG_CLASS(FetchItemHttpHandler);
		FetchItemHttpHandler(const LLSD& request_sd) : mRequestSD(request_sd) {};
		void httpSuccess(void);
		void httpFailure(void);
		AICapabilityType capability_type(void) const { return cap_inventory; }
		AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return FetchItemHttpHandler_timeout; }
		char const* getName(void) const { return "FetchItemHttpHandler"; }
	protected:
		LLSD mRequestSD;
	};
public:
	LLInventoryModel();
	~LLInventoryModel();
	void cleanupInventory();
	void empty();
public:
	bool isInventoryUsable() const;
private:
	bool mIsAgentInvUsable;
public:
	void setRootFolderID(const LLUUID& id);
	void setLibraryOwnerID(const LLUUID& id);
	void setLibraryRootFolderID(const LLUUID& id);
	const LLUUID &getRootFolderID() const;
	const LLUUID &getLibraryOwnerID() const;
	const LLUUID &getLibraryRootFolderID() const;
private:
	LLUUID mRootFolderID;
	LLUUID mLibraryRootFolderID;
	LLUUID mLibraryOwnerID;
public:
	bool loadSkeleton(const LLSD& options, const LLUUID& owner_id);
	void buildParentChildMap();
	void createCommonSystemCategories();
	void cache(const LLUUID& parent_folder_id, const LLUUID& agent_id);
private:
	typedef boost::unordered_map<LLUUID, LLPointer<LLViewerInventoryCategory> > cat_map_t;
	typedef boost::unordered_map<LLUUID, LLPointer<LLViewerInventoryItem> > item_map_t;
	cat_map_t mCategoryMap;
	item_map_t mItemMap;
	typedef boost::unordered_map<LLUUID, cat_array_t*> parent_cat_map_t;
	typedef boost::unordered_map<LLUUID, item_array_t*> parent_item_map_t;
	parent_cat_map_t mParentChildCategoryTree;
	parent_item_map_t mParentChildItemTree;
	typedef std::multimap<LLUUID, LLUUID> backlink_mmap_t;
	backlink_mmap_t mBacklinkMMap;
	bool hasBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id) const;
	void addBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id);
	void removeBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id);
public:
	static BOOL getIsFirstTimeInViewer2();
private:
	static BOOL sFirstTimeInViewer2;
	const static S32 sCurrentInvCacheVersion;
public:
	bool fetchDescendentsOf(const LLUUID& folder_id) const;
	void getDirectDescendentsOf(const LLUUID& cat_id,
								cat_array_t*& categories,
								item_array_t*& items) const;
	void getDirectDescendentsOf(const LLUUID& cat_id,
								cat_array_t*& categories) const;
	LLMD5 hashDirectDescendentNames(const LLUUID& cat_id) const;
	enum {
		EXCLUDE_TRASH = FALSE,
		INCLUDE_TRASH = TRUE
	};
	bool hasMatchingDirectDescendent(const LLUUID& cat_id,
									 LLInventoryCollectFunctor& filter, bool follow_folder_links = false);
	void collectDescendents(const LLUUID& id,
							cat_array_t& categories,
							item_array_t& items,
							BOOL include_trash);
	void collectDescendentsIf(const LLUUID& id,
							  cat_array_t& categories,
							  item_array_t& items,
							  BOOL include_trash,
							  LLInventoryCollectFunctor& add,
							  bool follow_folder_links = false);
	item_array_t collectLinksTo(const LLUUID& item_id,
									const LLUUID& start_folder_id = LLUUID::null);
	BOOL isObjectDescendentOf(const LLUUID& obj_id, const LLUUID& cat_id, const BOOL break_on_recursion=FALSE) const;
	bool getObjectTopmostAncestor(const LLUUID& object_id, LLUUID& result) const;
public:
	const LLUUID findCategoryUUIDForTypeInRoot(
		LLFolderType::EType preferred_type,
		bool create_folder,
		const LLUUID& root_id);
	const LLUUID findCategoryUUIDForType(LLFolderType::EType preferred_type,
										 bool create_folder = true);
	const LLUUID findLibraryCategoryUUIDForType(LLFolderType::EType preferred_type,
												bool create_folder = true);
	const LLViewerInventoryCategory *getFirstNondefaultParent(const LLUUID& obj_id) const;
	const LLViewerInventoryCategory *getFirstDescendantOf(const LLUUID& master_parent_id, const LLUUID& obj_id) const;
	LLInventoryObject* getObject(const LLUUID& id) const;
	LLViewerInventoryItem* getItem(const LLUUID& id) const;
	LLViewerInventoryCategory* getCategory(const LLUUID& id) const;
	const LLUUID& getLinkedItemID(const LLUUID& object_id) const;
	LLViewerInventoryItem* getLinkedItem(const LLUUID& object_id) const;
	LLUUID findCategoryByName(std::string name);
	void consolidateForType(const LLUUID& id, LLFolderType::EType type);
private:
	mutable LLPointer<LLViewerInventoryItem> mLastItem;
public:
	S32 getItemCount() const;
	S32 getCategoryCount() const;
public:
	U32 updateItem(const LLViewerInventoryItem* item, U32 mask = 0);
	void updateCategory(const LLViewerInventoryCategory* cat, U32 mask = 0);
	void moveObject(const LLUUID& object_id, const LLUUID& cat_id);
	void changeItemParent(LLViewerInventoryItem* item,
						  const LLUUID& new_parent_id,
						  BOOL restamp);
	void changeCategoryParent(LLViewerInventoryCategory* cat,
							  const LLUUID& new_parent_id,
							  BOOL restamp);
public:
	void onAISUpdateReceived(const std::string& context, const LLSD& update);
	void onObjectDeletedFromServer(const LLUUID& item_id,
								   bool fix_broken_links = true,
								   bool update_parent_version = true,
								   bool do_notify_observers = true);
	void onDescendentsPurgedFromServer(const LLUUID& object_id, bool fix_broken_links = true);
	void onItemUpdated(const LLUUID& item_id, const LLSD& updates, bool update_parent_version);
	void onCategoryUpdated(const LLUUID& cat_id, const LLSD& updates);
	void deleteObject(const LLUUID& id, bool fix_broken_links = true, bool do_notify_observers = true);
	void removeItem(const LLUUID& item_id);
	void removeCategory(const LLUUID& category_id);
	void removeObject(const LLUUID& object_id);
protected:
	void updateLinkedObjectsFromPurge(const LLUUID& baseobj_id);
public:
	static void updateItemsOrder(LLInventoryModel::item_array_t& items,
								 const LLUUID& src_item_id,
								 const LLUUID& dest_item_id,
								 bool insert_before = true);
	static LLInventoryModel::item_array_t::iterator findItemIterByUUID(LLInventoryModel::item_array_t& items, const LLUUID& id);
public:
	LLUUID createNewCategory(const LLUUID& parent_id,
							 LLFolderType::EType preferred_type,
							 const std::string& name,
							 boost::optional<inventory_func_type> callback = boost::optional<inventory_func_type>());
protected:
	void addCategory(LLViewerInventoryCategory* category);
	void addItem(LLViewerInventoryItem* item);
public:
	struct LLCategoryUpdate
	{
		LLCategoryUpdate() : mDescendentDelta(0) {}
		LLCategoryUpdate(const LLUUID& category_id, S32 delta) :
			mCategoryID(category_id),
			mDescendentDelta(delta) {}
		LLUUID mCategoryID;
		S32 mDescendentDelta;
	};
	typedef std::vector<LLCategoryUpdate> update_list_t;
	struct LLInitializedS32
	{
		LLInitializedS32() : mValue(0) {}
		LLInitializedS32(S32 value) : mValue(value) {}
		S32 mValue;
		LLInitializedS32& operator++() { ++mValue; return *this; }
		LLInitializedS32& operator--() { --mValue; return *this; }
	};
	typedef boost::unordered_map<LLUUID, LLInitializedS32> update_map_t;
	void accountForUpdate(const LLCategoryUpdate& update) const;
	void accountForUpdate(const update_list_t& updates);
	void accountForUpdate(const update_map_t& updates);
	EHasChildren categoryHasChildren(const LLUUID& cat_id) const;
	bool isCategoryComplete(const LLUUID& cat_id) const;
public:
	void idleNotifyObservers();
	void notifyObservers();
	void addChangedMask(U32 mask, const LLUUID& referent);
	const changed_items_t& getChangedIDs() const { return mChangedItemIDs; }
	const changed_items_t& getAddedIDs() const { return mAddedItemIDs; }
protected:
	void addChangedMaskForLinks(const LLUUID& object_id, U32 mask);
private:
	BOOL mIsNotifyObservers;
	U32 mModifyMask;
	changed_items_t mChangedItemIDs;
	changed_items_t mAddedItemIDs;
public:
	void addObserver(LLInventoryObserver* observer);
	void removeObserver(LLInventoryObserver* observer);
	BOOL containsObserver(LLInventoryObserver* observer) const;
private:
	typedef std::set<LLInventoryObserver*> observer_list_t;
	observer_list_t mObservers;
public:
	void emptyFolderType(const std::string notification, LLFolderType::EType folder_type);
	bool callbackEmptyFolderType(const LLSD& notification, const LLSD& response, LLFolderType::EType preferred_type);
	static void registerCallbacks(LLMessageSystem* msg);
protected:
	static bool loadFromFile(const std::string& filename,
							 cat_array_t& categories,
							 item_array_t& items,
							 changed_items_t& cats_to_update,
							 bool& is_cache_obsolete);
	static bool saveToFile(const std::string& filename,
						   const cat_array_t& categories,
						   const item_array_t& items);
public:
	static void processUpdateCreateInventoryItem(LLMessageSystem* msg, void**);
	static void removeInventoryItem(LLUUID agent_id, LLMessageSystem* msg, const char* msg_label);
	static void processRemoveInventoryItem(LLMessageSystem* msg, void**);
	static void processUpdateInventoryFolder(LLMessageSystem* msg, void**);
	static void removeInventoryFolder(LLUUID agent_id, LLMessageSystem* msg);
	static void processRemoveInventoryFolder(LLMessageSystem* msg, void**);
	static void processRemoveInventoryObjects(LLMessageSystem* msg, void**);
	static void processSaveAssetIntoInventory(LLMessageSystem* msg, void**);
	static void processBulkUpdateInventory(LLMessageSystem* msg, void**);
	static void processInventoryDescendents(LLMessageSystem* msg, void**);
	static void processMoveInventoryItem(LLMessageSystem* msg, void**);
	static void processFetchInventoryReply(LLMessageSystem* msg, void**);
protected:
	bool messageUpdateCore(LLMessageSystem* msg, bool do_accounting, U32 mask = 0x0);
public:
	void lockDirectDescendentArrays(const LLUUID& cat_id,
									cat_array_t*& categories,
									item_array_t*& items);
	void unlockDirectDescendentArrays(const LLUUID& cat_id);
protected:
	cat_array_t* getUnlockedCatArray(const LLUUID& id);
	item_array_t* getUnlockedItemArray(const LLUUID& id);
private:
	boost::unordered_map<LLUUID, bool> mCategoryLock;
	boost::unordered_map<LLUUID, bool> mItemLock;
public:
	void dumpInventory() const;
	bool validate() const;
};
extern LLInventoryModel gInventory;
#endif
