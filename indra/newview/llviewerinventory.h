/** 
 * @file llviewerinventory.h
 * @brief Declaration of the inventory bits that only used on the viewer.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
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
#ifndef LL_LLVIEWERINVENTORY_H
#define LL_LLVIEWERINVENTORY_H
#include "llinventory.h"
#include "llframetimer.h"
#include "llwearable.h"
#include "llui.h"
#include <boost/signals2.hpp>
class LLFolderView;
class LLFolderBridge;
class LLViewerInventoryCategory;
class LLAvatarName;
class LLViewerInventoryItem final : public LLInventoryItem, public boost::signals2::trackable
{
public:
	typedef std::vector<LLPointer<LLViewerInventoryItem> > item_array_t;
protected:
	~LLViewerInventoryItem( void );
public:
    LLAssetType::EType getType() const override;
    const LLUUID& getAssetUUID() const override;
	virtual const LLUUID& getProtectedAssetUUID() const;
    const std::string& getName() const override;
	virtual S32 getSortField() const;
	virtual void getSLURL();
    const LLPermissions& getPermissions() const override;
	virtual const bool getIsFullPerm() const;
    const LLUUID& getCreatorUUID() const override;
    const std::string& getDescription() const override;
    const LLSaleInfo& getSaleInfo() const override;
    LLInventoryType::EType getInventoryType() const override;
	virtual bool isWearableType() const;
	virtual LLWearableType::EType getWearableType() const;
    U32 getFlags() const override;
    time_t getCreationDate() const override;
    U32 getCRC32() const override;
	static BOOL extractSortFieldAndDisplayName(const std::string& name, S32* sortField, std::string* displayName);
	LLViewerInventoryItem(const LLUUID& uuid, const LLUUID& parent_uuid,
						  const LLPermissions& permissions,
						  const LLUUID& asset_uuid,
						  LLAssetType::EType type,
						  LLInventoryType::EType inv_type,
						  const std::string& name,
						  const std::string& desc,
						  const LLSaleInfo& sale_info,
						  U32 flags,
						  time_t creation_date_utc);
	LLViewerInventoryItem(
		const LLUUID& item_id,
		const LLUUID& parent_id,
		const std::string& name,
		LLInventoryType::EType inv_type);
	LLViewerInventoryItem();
	LLViewerInventoryItem(const LLViewerInventoryItem* other);
	LLViewerInventoryItem(const LLInventoryItem* other);
	void copyViewerItem(const LLViewerInventoryItem* other);
	void copyItem(const LLInventoryItem* other) override;
	void cloneViewerItem(LLPointer<LLViewerInventoryItem>& newitem) const;
    void updateParentOnServer(BOOL restamp) const override;
    void updateServer(BOOL is_new) const override;
	void fetchFromServer(void) const;
    void packMessage(LLMessageSystem* msg) const override;
    BOOL unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0) override;
	virtual BOOL unpackMessage(const LLSD& item);
    BOOL importFile(LLFILE* fp) override;
    BOOL importLegacyStream(std::istream& input_stream) override;
	bool exportFileLocal(LLFILE* fp) const;
	bool importFileLocal(LLFILE* fp);
	BOOL isComplete() const { return mIsComplete; }
	BOOL isFinished() const { return mIsComplete; }
	void setComplete(BOOL complete) { mIsComplete = complete; }
	virtual void setTransactionID(const LLTransactionID& transaction_id);
	struct comparePointers
	{
		bool operator()(const LLPointer<LLViewerInventoryItem>& a, const LLPointer<LLViewerInventoryItem>& b)
		{
			return a->getName().compare(b->getName()) < 0;
		}
	};
	LLTransactionID getTransactionID() const { return mTransactionID; }
	bool getIsBrokenLink() const;
	LLViewerInventoryItem *getLinkedItem() const;
	LLViewerInventoryCategory *getLinkedCategory() const;
	bool checkPermissionsSet(PermissionMask mask) const;
	PermissionMask getPermissionMask() const;
	void onCallingCardNameLookup(const LLUUID& id, const LLAvatarName& name);
	BOOL regenerateLink();
	void setWearableType(LLWearableType::EType type);
public:
	BOOL mIsComplete;
	LLTransactionID mTransactionID;
};
class LLViewerInventoryCategory final : public LLInventoryCategory
{
public:
	typedef std::vector<LLPointer<LLViewerInventoryCategory> > cat_array_t;
protected:
	~LLViewerInventoryCategory();
public:
	LLViewerInventoryCategory(const LLUUID& uuid, const LLUUID& parent_uuid,
							  LLFolderType::EType preferred_type,
							  const std::string& name,
							  const LLUUID& owner_id);
	LLViewerInventoryCategory(const LLUUID& owner_id);
	LLViewerInventoryCategory(const LLViewerInventoryCategory* other);
	void copyViewerCategory(const LLViewerInventoryCategory* other);
    void updateParentOnServer(BOOL restamp_children) const override;
    void updateServer(BOOL is_new) const override;
    void packMessage(LLMessageSystem* msg) const override;
	const LLUUID& getOwnerID() const { return mOwnerID; }
	enum { VERSION_UNKNOWN = -1, VERSION_INITIAL = 1 };
	S32 getVersion() const;
	void setVersion(S32 version);
	bool fetch();
	enum { DESCENDENT_COUNT_UNKNOWN = -1 };
	S32 getDescendentCount() const { return mDescendentCount; }
	void setDescendentCount(S32 descendents) { mDescendentCount = descendents; }
	S32 getViewerDescendentCount() const;
	bool exportFileLocal(LLFILE* fp) const;
	bool importFileLocal(LLFILE* fp);
	void determineFolderType();
	void changeType(LLFolderType::EType new_folder_type);
    void unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0) override;
	virtual BOOL unpackMessage(const LLSD& category);
	bool acceptItem(LLInventoryItem* inv_item);
private:
	friend class LLInventoryModel;
	void localizeName();
protected:
	LLUUID mOwnerID;
	S32 mVersion;
	S32 mDescendentCount;
	LLFrameTimer mDescendentsRequested;
};
class LLInventoryCallback : public LLRefCount
{
public:
	virtual void fire(const LLUUID& inv_item) = 0;
};
class LLViewerJointAttachment;
void rez_attachment_cb(const LLUUID& inv_item, LLViewerJointAttachment *attachmentp, bool replace = false);
void activate_gesture_cb(const LLUUID& inv_item);
void create_gesture_cb(const LLUUID& inv_item);
class AddFavoriteLandmarkCallback final : public LLInventoryCallback
{
public:
	AddFavoriteLandmarkCallback() : mTargetLandmarkId(LLUUID::null) {}
	void setTargetLandmarkId(const LLUUID& target_uuid) { mTargetLandmarkId = target_uuid; }
private:
	void fire(const LLUUID& inv_item) override;
	LLUUID mTargetLandmarkId;
};
typedef std::function<void(const LLUUID&)> inventory_func_type;
typedef std::function<void(const LLSD&)> llsd_func_type;
typedef std::function<void()> nullary_func_type;
void no_op_inventory_func(const LLUUID&);
void no_op_llsd_func(const LLSD&);
void no_op();
class LLBoostFuncInventoryCallback : public LLInventoryCallback
{
public:
	LLBoostFuncInventoryCallback(inventory_func_type fire_func = no_op_inventory_func,
								 nullary_func_type destroy_func = no_op):
		mFireFunc(fire_func),
		mDestroyFunc(destroy_func)
	{
	}
	void fire(const LLUUID& item_id) override
	{
		mFireFunc(item_id);
	}
	~LLBoostFuncInventoryCallback()
	{
		mDestroyFunc();
	}
private:
	inventory_func_type mFireFunc;
	nullary_func_type mDestroyFunc;
};
class LLInventoryCallbackManager : public LLDestroyClass<LLInventoryCallbackManager>
{
	friend class LLDestroyClass<LLInventoryCallbackManager>;
public:
	LLInventoryCallbackManager();
	~LLInventoryCallbackManager();
	void fire(U32 callback_id, const LLUUID& item_id);
	U32 registerCB(LLPointer<LLInventoryCallback> cb);
private:
	typedef std::map<U32, LLPointer<LLInventoryCallback> > callback_map_t;
	callback_map_t mMap;
	U32 mLastCallback;
	static LLInventoryCallbackManager *sInstance;
	static void destroyClass();
public:
	static bool is_instantiated() { return sInstance != NULL; }
};
extern LLInventoryCallbackManager gInventoryCallbacks;
#define NOT_WEARABLE (LLWearableType::EType)0
void create_inventory_item(const LLUUID& agent_id, const LLUUID& session_id,
						   const LLUUID& parent, const LLTransactionID& transaction_id,
						   const std::string& name,
						   const std::string& desc, LLAssetType::EType asset_type,
						   LLInventoryType::EType inv_type, LLWearableType::EType wtype,
						   U32 next_owner_perm,
						   LLPointer<LLInventoryCallback> cb);
void create_inventory_callingcard(const LLUUID& avatar_id, const LLUUID& parent = LLUUID::null, LLPointer<LLInventoryCallback> cb=NULL);
void copy_inventory_item(
	const LLUUID& agent_id,
	const LLUUID& current_owner,
	const LLUUID& item_id,
	const LLUUID& parent_id,
	const std::string& new_name,
	LLPointer<LLInventoryCallback> cb);
void link_inventory_object(const LLUUID& category,
			 LLConstPointer<LLInventoryObject> baseobj,
			 LLPointer<LLInventoryCallback> cb);
void link_inventory_object(const LLUUID& category,
			 const LLUUID& id,
			 LLPointer<LLInventoryCallback> cb);
void link_inventory_array(const LLUUID& category,
						  LLInventoryObject::const_object_list_t& baseobj_array,
						  LLPointer<LLInventoryCallback> cb);
void move_inventory_item(
	const LLUUID& agent_id,
	const LLUUID& session_id,
	const LLUUID& item_id,
	const LLUUID& parent_id,
	const std::string& new_name,
	LLPointer<LLInventoryCallback> cb);
void update_inventory_item(
	LLViewerInventoryItem *update_item,
	LLPointer<LLInventoryCallback> cb);
void update_inventory_item(
	const LLUUID& item_id,
	const LLSD& updates,
	LLPointer<LLInventoryCallback> cb);
void update_inventory_category(
	const LLUUID& cat_id,
	const LLSD& updates,
	LLPointer<LLInventoryCallback> cb);
void remove_inventory_items(
	LLInventoryObject::object_list_t& items,
	LLPointer<LLInventoryCallback> cb);
void remove_inventory_item(
	LLPointer<LLInventoryObject> obj,
	LLPointer<LLInventoryCallback> cb,
	bool immediate_delete = false);
void remove_inventory_item(
	const LLUUID& item_id,
	LLPointer<LLInventoryCallback> cb,
	bool immediate_delete = false);
void remove_inventory_category(
	const LLUUID& cat_id,
	LLPointer<LLInventoryCallback> cb);
void remove_inventory_object(
	const LLUUID& object_id,
	LLPointer<LLInventoryCallback> cb);
void purge_descendents_of(
	const LLUUID& cat_id,
	LLPointer<LLInventoryCallback> cb);
const LLUUID get_folder_by_itemtype(const LLInventoryItem *src);
void copy_inventory_from_notecard(const LLUUID& destination_id,
								  const LLUUID& object_id,
								  const LLUUID& notecard_inv_id,
								  const LLInventoryItem *src,
								  U32 callback_id = 0);
void menu_create_inventory_item(LLFolderView* root,
								LLFolderBridge* bridge,
								const LLSD& userdata,
								const LLUUID& default_parent_uuid = LLUUID::null);
void slam_inventory_folder(const LLUUID& folder_id,
						   const LLSD& contents,
						   LLPointer<LLInventoryCallback> cb);
void remove_folder_contents(const LLUUID& folder_id, bool keep_outfit_links,
							  LLPointer<LLInventoryCallback> cb);
#endif
