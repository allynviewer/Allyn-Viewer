/** 
 * @file llappearancemgr.h
 * @brief Manager for initiating appearance changes on the viewer
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
#ifndef LL_LLAPPEARANCEMGR_H
#define LL_LLAPPEARANCEMGR_H
#include "llsingleton.h"
#include "llagentwearables.h"
#include "llcallbacklist.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llviewerinventory.h"
#include "llhttpclient.h"
class LLWearable;
class LLWearableHoldingPattern;
class LLInventoryCallback;
class LLOutfitUnLockTimer;
class RequestAgentUpdateAppearanceResponder;
class LLAppearanceMgr: public LLSingleton<LLAppearanceMgr>
{
	LOG_CLASS(LLAppearanceMgr);
	friend class LLSingleton<LLAppearanceMgr>;
	friend class LLOutfitUnLockTimer;
public:
	typedef std::vector<LLInventoryModel::item_array_t> wearables_by_type_t;
	void updateAppearanceFromCOF(bool enforce_item_restrictions = true,
								 bool enforce_ordering = true,
								 nullary_func_t post_update_func = no_op);
	void updateAppearanceFromInitialWearables(LLInventoryObject::const_object_list_t& initial_items);
	void updateCOF(const LLUUID& category, bool append = false);
	void updateCOF(LLInventoryModel::item_array_t& body_items_new, LLInventoryModel::item_array_t& wear_items_new,
				   LLInventoryModel::item_array_t& obj_items_new, LLInventoryModel::item_array_t& gest_items_new,
				   bool append = false, const LLUUID& idOutfit = LLUUID::null, LLPointer<LLInventoryCallback> link_waiter = NULL);
	void wearInventoryCategory(LLInventoryCategory* category, bool copy, bool append);
	void wearInventoryCategoryOnAvatar(LLInventoryCategory* category, bool append);
	void wearCategoryFinal(LLUUID& cat_id, bool copy_items, bool append);
	void wearOutfitByName(const std::string& name);
	void changeOutfit(bool proceed, const LLUUID& category, bool append);
	void replaceCurrentOutfit(const LLUUID& new_outfit);
	void renameOutfit(const LLUUID& outfit_id);
	void takeOffOutfit(const LLUUID& cat_id);
	void addCategoryToCurrentOutfit(const LLUUID& cat_id);
	S32 findExcessOrDuplicateItems(const LLUUID& cat_id,
								   LLAssetType::EType type,
								   S32 max_items_per_type,
								   S32 max_items_total,
								   LLInventoryObject::object_list_t& items_to_kill);
	void findAllExcessOrDuplicateItems(const LLUUID& cat_id,
									  LLInventoryObject::object_list_t& items_to_kill);
	void enforceCOFItemRestrictions(LLPointer<LLInventoryCallback> cb);
	S32 getActiveCopyOperations() const;
	void slamCategoryLinks(const LLUUID& src_id, const LLUUID& dst_id,
						   bool include_folder_links, LLPointer<LLInventoryCallback> cb);
	void shallowCopyCategory(const LLUUID& src_id, const LLUUID& dst_id,
							 LLPointer<LLInventoryCallback> cb);
	BOOL getCanMakeFolderIntoOutfit(const LLUUID& folder_id);
	bool getCanRemoveOutfit(const LLUUID& outfit_cat_id);
	static bool getCanRemoveFromCOF(const LLUUID& outfit_cat_id);
	static bool getCanAddToCOF(const LLUUID& outfit_cat_id);
	bool getCanReplaceCOF(const LLUUID& outfit_cat_id);
    bool canAddWearables(const uuid_vec_t& item_ids);
	void shallowCopyCategoryContents(const LLUUID& src_id, const LLUUID& dst_id,
									 LLPointer<LLInventoryCallback> cb);
	const LLUUID getCOF() const;
	S32 getCOFVersion() const;
	LLSD dumpCOF() const;
	const LLViewerInventoryItem *getBaseOutfitLink();
	bool getBaseOutfitName(std::string &name);
	const LLUUID getBaseOutfitUUID();
    void wearItemsOnAvatar(const uuid_vec_t& item_ids_to_wear,
                           bool do_update,
                           bool replace,
                           LLPointer<LLInventoryCallback> cb = NULL);
	void wearItemOnAvatar(const LLUUID& item_to_wear, bool do_update, bool replace = false,
						  LLPointer<LLInventoryCallback> cb = NULL);
	void updatePanelOutfitName(const std::string& name);
	void purgeBaseOutfitLink(const LLUUID& category, LLPointer<LLInventoryCallback> cb = NULL);
	void createBaseOutfitLink(const LLUUID& category, LLPointer<LLInventoryCallback> link_waiter);
	void updateAgentWearables(LLWearableHoldingPattern* holder);
	S32 countActiveHoldingPatterns();
	void dumpCat(const LLUUID& cat_id, const std::string& msg);
	void dumpItemArray(const LLInventoryModel::item_array_t& items, const std::string& msg);
	void unregisterAttachment(const LLUUID& item_id);
	void registerAttachment(const LLUUID& item_id);
	bool getAttachmentInvLinkEnable() { return mAttachmentInvLinkEnabled; }
	void setAttachmentInvLinkEnable(bool val);
	void addCOFItemLink(const LLUUID& item_id, LLPointer<LLInventoryCallback> cb = NULL, const std::string description = "");
	void addCOFItemLink(const LLInventoryItem *item, LLPointer<LLInventoryCallback> cb = NULL, const std::string description = "");
	LLInventoryModel::item_array_t findCOFItemLinks(const LLUUID& item_id);
	bool isLinkedInCOF(const LLUUID& item_id);
	void removeCOFItemLinks(const LLUUID& item_id, LLPointer<LLInventoryCallback> cb = NULL, bool immediate_delete = false);
	void removeCOFLinksOfType(LLWearableType::EType type, LLPointer<LLInventoryCallback> cb = NULL, bool immediate_delete = false);
	void removeAllClothesFromAvatar();
	void removeAllAttachmentsFromAvatar();
	bool shouldRemoveTempAttachment(const LLUUID& item_id);
	bool isOutfitDirty() { return mOutfitIsDirty; }
	void setOutfitDirty(bool isDirty) { mOutfitIsDirty = isDirty; }
	void updateIsDirty();
	void setOutfitLocked(bool locked);
	void onFirstFullyVisible();
	void copyLibraryGestures();
	void wearBaseOutfit();
	bool updateBaseOutfit();
	void removeItemFromAvatar(const LLUUID& id_to_remove, LLPointer<LLInventoryCallback> cb = NULL, bool immediate_delete = false);
	void removeItemsFromAvatar(const uuid_vec_t& ids_to_remove, LLPointer<LLInventoryCallback> cb = NULL, bool immediate_delete = false);
	void onOutfitFolderCreated(const LLUUID& folder_id, bool show_panel);
	void onOutfitFolderCreatedAndClothingOrdered(const LLUUID& folder_id, bool show_panel);
private:
	LLUUID makeNewOutfitCore(const std::string& new_folder_name, bool show_panel, LLInventoryModel::item_array_t* items = NULL);
public:
	LLUUID makeNewOutfitLinks(const std::string& new_folder_name, bool show_panel);
	LLUUID makeNewOutfitLinks(const std::string& new_folder_name, LLInventoryModel::item_array_t& item_list);
	LLUUID makeNewOutfitLegacy(const std::string& new_folder_name, LLInventoryModel::item_array_t& items, bool use_links);
	bool moveWearable(LLViewerInventoryItem* item, bool closer_to_body);
	static void sortItemsByActualDescription(LLInventoryModel::item_array_t& items);
	static void divvyWearablesByType(const LLInventoryModel::item_array_t& items, wearables_by_type_t& items_by_type);
	typedef std::map<LLUUID,std::string> desc_map_t;
	void getWearableOrderingDescUpdates(LLInventoryModel::item_array_t& wear_items, desc_map_t& desc_map);
	bool validateClothingOrderingInfo(LLUUID cat_id = LLUUID::null);
	void updateClothingOrderingInfo(LLUUID cat_id = LLUUID::null,
									LLPointer<LLInventoryCallback> cb = NULL);
	bool isOutfitLocked() { return mOutfitLocked; }
	bool isInUpdateAppearanceFromCOF() { return mIsInUpdateAppearanceFromCOF; }
	void requestServerAppearanceUpdate();
	void setAppearanceServiceURL(const std::string& url) { mAppearanceServiceURL = url; }
	std::string getAppearanceServiceURL() const;
private:
	std::string		mAppearanceServiceURL;
protected:
	LLAppearanceMgr();
	~LLAppearanceMgr();
private:
	void filterWearableItems(LLInventoryModel::item_array_t& items, S32 max_per_type, S32 max_total);
	void getDescendentsOfAssetType(const LLUUID& category,
										  LLInventoryModel::item_array_t& items,
										  LLAssetType::EType type,
										  bool follow_folder_links = false);
	void getUserDescendents(const LLUUID& category,
								   LLInventoryModel::item_array_t& wear_items,
								   LLInventoryModel::item_array_t& obj_items,
								   LLInventoryModel::item_array_t& gest_items,
								   bool follow_folder_links = false);
	static void onOutfitRename(const LLSD& notification, const LLSD& response);
	bool mAttachmentInvLinkEnabled;
	bool mOutfitIsDirty;
	bool mIsInUpdateAppearanceFromCOF;
	boost::intrusive_ptr<RequestAgentUpdateAppearanceResponder> mAppearanceResponder;
	bool mOutfitLocked;
	std::auto_ptr<LLOutfitUnLockTimer> mUnlockOutfitTimer;
	typedef uuid_set_t doomed_temp_attachments_t;
	doomed_temp_attachments_t	mDoomedTempAttachmentIDs;
	void addDoomedTempAttachment(const LLUUID& id_to_remove);
public:
	BOOL getIsInCOF(const LLUUID& obj_id) const;
	BOOL getIsProtectedCOFItem(const LLUUID& obj_id) const;
};
class LLUpdateAppearanceOnDestroy: public LLInventoryCallback
{
public:
	LLUpdateAppearanceOnDestroy(bool enforce_item_restrictions = true,
								bool enforce_ordering = true,
								nullary_func_t post_update_func = no_op);
	virtual ~LLUpdateAppearanceOnDestroy();
	void fire(const LLUUID& inv_item);
	static U32 sActiveCallbacks;
private:
	U32 mFireCount;
	bool mEnforceItemRestrictions;
	bool mEnforceOrdering;
	nullary_func_t mPostUpdateFunc;
};
class LLUpdateAppearanceAndEditWearableOnDestroy: public LLInventoryCallback
{
public:
	LLUpdateAppearanceAndEditWearableOnDestroy(const LLUUID& item_id);
	void fire(const LLUUID& item_id) {}
	~LLUpdateAppearanceAndEditWearableOnDestroy();
private:
	LLUUID mItemID;
};
class LLRequestServerAppearanceUpdateOnDestroy: public LLInventoryCallback
{
public:
	LLRequestServerAppearanceUpdateOnDestroy() {}
	~LLRequestServerAppearanceUpdateOnDestroy();
	void fire(const LLUUID& item_id) {}
};
void edit_wearable_and_customize_avatar(LLUUID item_id);
LLUUID findDescendentCategoryIDByName(const LLUUID& parent_id,const std::string& name);
void callAfterCategoryFetch(const LLUUID& cat_id, nullary_func_t cb);
void wear_multiple(const uuid_vec_t& ids, bool replace);
void wear_on_avatar_cb(const LLUUID& inv_item, bool do_replace = false);
#endif
