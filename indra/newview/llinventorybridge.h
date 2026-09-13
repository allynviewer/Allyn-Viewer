/** 
 * @file llinventorybridge.h
 * @brief Implementation of the Inventory-Folder-View-Bridge classes.
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
#ifndef LL_LLINVENTORYBRIDGE_H
#define LL_LLINVENTORYBRIDGE_H
#include "llcallingcard.h"
#include "llfloaterproperties.h"
#include "llfolderview.h"
#include "llfoldervieweventlistener.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llviewercontrol.h"
#include "llviewerwearable.h"
class LLInventoryPanel;
class LLInventoryModel;
class LLMenuGL;
class LLCallingCardObserver;
class LLViewerJointAttachment;
typedef std::vector<std::string> menuentry_vec_t;
class LLInvFVBridge : public LLFolderViewEventListener
{
public:
	static LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
									   LLAssetType::EType actual_asset_type,
									   LLInventoryType::EType inv_type,
									   LLInventoryPanel* inventory,
									   LLFolderView* root,
									   const LLUUID& uuid,
									   U32 flags = 0x00);
	virtual ~LLInvFVBridge() {}
	bool canShare() const;
	bool canListOnMarketplace() const;
	bool canListOnMarketplaceNow() const;
	virtual const LLUUID& getUUID() const { return mUUID; }
	virtual void clearDisplayName() { mDisplayName.clear(); }
	virtual const std::string& getPrefix() { return LLStringUtil::null; }
	virtual void restoreItem() {}
	virtual void restoreToWorld() {}
	virtual const std::string& getName() const;
	virtual const std::string& getDisplayName() const;
	virtual PermissionMask getPermissionMask() const;
	virtual LLFolderType::EType getPreferredType() const;
	virtual time_t getCreationDate() const;
        virtual void setCreationDate(time_t creation_date_utc);
	virtual LLFontGL::StyleFlags getLabelStyle() const { return LLFontGL::NORMAL; }
	virtual std::string getLabelSuffix() const { return LLStringUtil::null; }
	virtual void openItem() {}
	virtual void closeItem() {}
	virtual void previewItem() {openItem();}
	virtual void showProperties();
	virtual BOOL isItemRenameable() const { return TRUE; }
	virtual BOOL isItemRemovable() const;
	virtual BOOL isItemMovable() const;
	virtual BOOL isItemInTrash() const;
	virtual BOOL isLink() const;
	virtual BOOL isLibraryItem() const;
	virtual void removeBatch(std::vector<LLFolderViewEventListener*>& batch);
	virtual void move(LLFolderViewEventListener* new_parent_bridge) {}
	virtual BOOL isItemCopyable() const { return FALSE; }
	virtual BOOL copyToClipboard() const;
	virtual BOOL cutToClipboard();
	virtual BOOL isClipboardPasteable() const;
	bool isClipboardPasteableAsCopy() const;
	virtual BOOL isClipboardPasteableAsLink() const;
	virtual void pasteFromClipboard(bool only_copies = false) {}
	virtual void pasteLinkFromClipboard() {}
	void getClipboardEntries(bool show_asset_id, menuentry_vec_t &items,
							 menuentry_vec_t &disabled_items, U32 flags);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) const;
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data) { return FALSE; }
	virtual LLInventoryType::EType getInventoryType() const { return mInvType; }
	virtual LLWearableType::EType getWearableType() const { return LLWearableType::WT_NONE; }
	virtual LLInventoryObject* getInventoryObject() const;
protected:
	virtual void addTrashContextMenuOptions(menuentry_vec_t &items,
											menuentry_vec_t &disabled_items);
	virtual void addDeleteContextMenuOptions(menuentry_vec_t &items,
											 menuentry_vec_t &disabled_items);
	virtual void addOpenRightClickMenuOption(menuentry_vec_t &items);
	virtual void addMarketplaceContextMenuOptions(U32 flags,
											 menuentry_vec_t &items,
											 menuentry_vec_t &disabled_items);
protected:
	LLInvFVBridge(LLInventoryPanel* inventory, LLFolderView* root, const LLUUID& uuid);
	LLInventoryModel* getInventoryModel() const;
	LLInventoryFilter* getInventoryFilter() const;
	BOOL isLinkedObjectInTrash() const;
	BOOL isLinkedObjectMissing() const;
	BOOL isAgentInventory() const;
	BOOL isCOFFolder() const;
	BOOL isInboxFolder() const;
	BOOL isMarketplaceListingsFolder() const;
	virtual BOOL isItemPermissive() const;
	static void changeItemParent(LLInventoryModel* model,
								 LLViewerInventoryItem* item,
								 const LLUUID& new_parent,
								 BOOL restamp);
	static void changeCategoryParent(LLInventoryModel* model,
									 LLViewerInventoryCategory* item,
									 const LLUUID& new_parent,
									 BOOL restamp);
	void removeBatchNoCheck(std::vector<LLFolderViewEventListener*>& batch);
public:
	BOOL callback_cutToClipboard(const LLSD& notification, const LLSD& response);
	BOOL perform_cutToClipboard();
protected:
	LLHandle<LLInventoryPanel> mInventoryPanel;
	LLFolderView* mRoot;
	const LLUUID mUUID;
	LLInventoryType::EType mInvType;
	bool mIsLink;
	mutable std::string			mDisplayName;
	void purgeItem(LLInventoryModel *model, const LLUUID &uuid);
	virtual void buildDisplayName() const {}
	void removeObject(LLInventoryModel* model, const LLUUID& uuid);
};
class AIFilePicker;
class LLInventoryFolderViewModelBuilder
{
public:
	LLInventoryFolderViewModelBuilder() {}
 	virtual ~LLInventoryFolderViewModelBuilder() {}
	virtual LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
										LLAssetType::EType actual_asset_type,
										LLInventoryType::EType inv_type,
										LLInventoryPanel* inventory,
										LLFolderView* root,
										const LLUUID& uuid,
										U32 flags = 0x00) const;
};
class LLItemBridge : public LLInvFVBridge
{
public:
	LLItemBridge(LLInventoryPanel* inventory,
				 LLFolderView* root,
				 const LLUUID& uuid) :
		LLInvFVBridge(inventory, root, uuid) {}
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void selectItem();
	virtual void restoreItem();
	virtual void restoreToWorld();
	virtual void gotoItem();
	virtual LLUIImagePtr getIcon() const;
	virtual std::string getLabelSuffix() const;
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual PermissionMask getPermissionMask() const;
	virtual time_t getCreationDate() const;
	virtual BOOL isItemRenameable() const;
	virtual BOOL renameItem(const std::string& new_name);
	virtual BOOL removeItem();
	virtual BOOL isItemCopyable() const;
	virtual bool hasChildren() const { return FALSE; }
	virtual BOOL isUpToDate() const { return TRUE; }
	virtual LLUIImagePtr getIconOverlay() const;
	static void showFloaterImagePreview(LLInventoryItem* item, AIFilePicker* filepicker);
	LLViewerInventoryItem* getItem() const;
protected:
	BOOL confirmRemoveItem(const LLSD& notification, const LLSD& response);
	virtual BOOL isItemPermissive() const;
	virtual void buildDisplayName() const;
};
class LLFolderBridge : public LLInvFVBridge
{
public:
	LLFolderBridge(LLInventoryPanel* inventory,
				   LLFolderView* root,
				   const LLUUID& uuid)
	:	LLInvFVBridge(inventory, root, uuid),
		mCallingCards(FALSE),
		mWearables(FALSE)
	{}
	BOOL dragItemIntoFolder(LLInventoryItem* inv_item, BOOL drop, BOOL user_confirm = TRUE);
	BOOL dragCategoryIntoFolder(LLInventoryCategory* inv_category, BOOL drop, BOOL user_confirm = TRUE);
	void callback_dropItemIntoFolder(const LLSD& notification, const LLSD& response, LLInventoryItem* inv_item);
	void callback_dropCategoryIntoFolder(const LLSD& notification, const LLSD& response, LLInventoryCategory* inv_item);
    virtual void buildDisplayName() const;
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual void closeItem();
	virtual BOOL isItemRenameable() const;
	virtual void selectItem();
	virtual void restoreItem();
	virtual LLFolderType::EType getPreferredType() const;
	virtual LLUIImagePtr getIcon() const;
	virtual LLUIImagePtr getIconOpen() const;
	virtual LLUIImagePtr getIconOverlay() const;
	static LLUIImagePtr getIcon(LLFolderType::EType preferred_type);
	virtual std::string getLabelSuffix() const;
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual BOOL renameItem(const std::string& new_name);
	virtual BOOL removeItem();
	BOOL removeSystemFolder();
	bool removeItemResponse(const LLSD& notification, const LLSD& response);
	virtual void pasteFromClipboard(bool only_copies = false);
	virtual void pasteLinkFromClipboard();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual bool hasChildren() const;
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data);
	virtual BOOL isItemRemovable() const;
	virtual BOOL isItemMovable() const ;
	virtual BOOL isUpToDate() const;
	virtual BOOL isItemCopyable() const;
	virtual BOOL isClipboardPasteable() const;
	virtual BOOL isClipboardPasteableAsLink() const;
	static void createWearable(LLFolderBridge* bridge, LLWearableType::EType type);
	LLViewerInventoryCategory* getCategory() const;
	LLHandle<LLFolderBridge> getHandle() { mHandle.bind(this); return mHandle; }
protected:
	void buildContextMenuOptions(U32 flags, menuentry_vec_t& items,   menuentry_vec_t& disabled_items);
	void buildContextMenuFolderOptions(U32 flags, menuentry_vec_t& items,   menuentry_vec_t& disabled_items);
	static void pasteClipboard(void* user_data);
	static void createNewCategory(void* user_data);
	static void createNewShirt(void* user_data);
	static void createNewPants(void* user_data);
	static void createNewShoes(void* user_data);
	static void createNewSocks(void* user_data);
	static void createNewJacket(void* user_data);
	static void createNewSkirt(void* user_data);
	static void createNewGloves(void* user_data);
	static void createNewUndershirt(void* user_data);
	static void createNewUnderpants(void* user_data);
	static void createNewAlpha(void* user_data);
	static void createNewTattoo(void* user_data);
	static void createNewShape(void* user_data);
	static void createNewSkin(void* user_data);
	static void createNewHair(void* user_data);
	static void createNewEyes(void* user_data);
	static void createNewPhysics(void* user_data);
	BOOL checkFolderForContentsOfType(LLInventoryModel* model, LLInventoryCollectFunctor& typeToCheck);
	void modifyOutfit(BOOL append);
	void determineFolderType();
	void dropToFavorites(LLInventoryItem* inv_item);
	void dropToOutfit(LLInventoryItem* inv_item, BOOL move_is_into_current_outfit);
public:
	static LLHandle<LLFolderBridge> sSelf;
	static void staticFolderOptionsMenu();
protected:
	void callback_pasteFromClipboard(const LLSD& notification, const LLSD& response, bool only_copies);
	void perform_pasteFromClipboard(bool only_copies);
	void gatherMessage(std::string& message, S32 depth, LLError::ELevel log_level);
	LLUIImagePtr getFolderIcon(BOOL is_open) const;
	bool				mCallingCards;
	bool				mWearables;
	std::string		mMessage;
	LLRootHandle<LLFolderBridge> mHandle;
};
class LLTextureBridge : public LLItemBridge
{
public:
	virtual const std::string& getPrefix() { static std::string ret("Texture: ");return ret; }
	LLTextureBridge(LLInventoryPanel* inventory,
					LLFolderView* root,
					const LLUUID& uuid,
					LLInventoryType::EType type) :
		LLItemBridge(inventory, root, uuid)
	{
		mInvType = type;
	}
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void performAction(LLInventoryModel* model, std::string action);
	bool canSaveTexture(void);
};
class LLSoundBridge : public LLItemBridge
{
public:
 	LLSoundBridge(LLInventoryPanel* inventory,
				  LLFolderView* root,
				  const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual void openItem();
	virtual void previewItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void performAction(LLInventoryModel* model, std::string action);
	static void openSoundPreview(void*);
};
class LLLandmarkBridge : public LLItemBridge
{
public:
	static const std::string& prefix() { static std::string ret("Landmark: ");return ret; }
	virtual const std::string& getPrefix() { return prefix(); }
 	LLLandmarkBridge(LLInventoryPanel* inventory,
					 LLFolderView* root,
					 const LLUUID& uuid,
					 U32 flags = 0x00);
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
protected:
	BOOL mVisited;
};
class LLCallingCardBridge : public LLItemBridge
{
public:
	virtual const std::string& getPrefix() { static std::string ret("Calling Card: ");return ret; }
	LLCallingCardBridge(LLInventoryPanel* inventory,
						LLFolderView* folder,
						const LLUUID& uuid );
	~LLCallingCardBridge();
	virtual std::string getLabelSuffix() const;
	virtual LLUIImagePtr getIcon() const;
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data);
	void refreshFolderViewItem();
protected:
	LLCallingCardObserver* mObserver;
};
class LLNotecardBridge : public LLItemBridge
{
public:
	virtual const std::string& getPrefix() { static std::string ret("Notecard: ");return ret; }
	LLNotecardBridge(LLInventoryPanel* inventory,
					 LLFolderView* root,
					 const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
};
class LLGestureBridge : public LLItemBridge
{
public:
	virtual const std::string& getPrefix() { static std::string ret("Gesture: ");return ret; }
	LLGestureBridge(LLInventoryPanel* inventory,
					LLFolderView* root,
					const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual std::string getLabelSuffix() const;
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual BOOL removeItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	static void playGesture(const LLUUID& item_id);
};
class LLAnimationBridge : public LLItemBridge
{
public:
	virtual const std::string& getPrefix() { static std::string ret("Animation: ");return ret; }
	LLAnimationBridge(LLInventoryPanel* inventory,
					  LLFolderView* root,
					  const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void openItem();
};
class LLObjectBridge : public LLItemBridge
{
public:
	virtual const std::string& getPrefix() { static std::string ret("Object: ");return ret; }
	LLObjectBridge(LLInventoryPanel* inventory,
				   LLFolderView* root,
				   const LLUUID& uuid,
				   LLInventoryType::EType type,
				   U32 flags);
	virtual LLUIImagePtr	getIcon() const;
	virtual void			performAction(LLInventoryModel* model, std::string action);
	virtual void			openItem();
	virtual BOOL isItemWearable() const { return TRUE; }
	virtual std::string getLabelSuffix() const;
	virtual void			buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL renameItem(const std::string& new_name);
	LLInventoryObject* getObject() const;
protected:
	static LLUUID sContextMenuItemID;
	U32 mAttachPt;
	BOOL mIsMultiObject;
};
class LLLSLTextBridge : public LLItemBridge
{
public:
	virtual const std::string& getPrefix() { static std::string ret("Script: ");return ret; }
	LLLSLTextBridge(LLInventoryPanel* inventory,
					LLFolderView* root,
					const LLUUID& uuid ) :
		LLItemBridge(inventory, root, uuid) {}
	virtual void openItem();
};
class LLWearableBridge : public LLItemBridge
{
public:
	LLWearableBridge(LLInventoryPanel* inventory,
					 LLFolderView* root,
					 const LLUUID& uuid,
					 LLAssetType::EType asset_type,
					 LLInventoryType::EType inv_type,
					 LLWearableType::EType wearable_type);
	virtual LLUIImagePtr getIcon() const;
	virtual void	performAction(LLInventoryModel* model, std::string action);
	virtual void	openItem();
	virtual BOOL isItemWearable() const { return TRUE; }
	virtual void	buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual std::string getLabelSuffix() const;
	virtual BOOL renameItem(const std::string& new_name);
	virtual void nameOrDescriptionChanged(void) const;
	virtual LLWearableType::EType getWearableType() const { return mWearableType; }
	static void		onWearOnAvatar( void* userdata );
	static BOOL		canWearOnAvatar( void* userdata );
	static void		onWearOnAvatarArrived( LLViewerWearable* wearable, void* userdata );
	void			wearOnAvatar();
	static void		onWearAddOnAvatarArrived( LLViewerWearable* wearable, void* userdata );
	void			wearAddOnAvatar();
	static BOOL		canEditOnAvatar( void* userdata );
	static void		onEditOnAvatar( void* userdata );
	void			editOnAvatar();
	static BOOL		canRemoveFromAvatar( void* userdata );
	void			removeFromAvatar();
protected:
	LLAssetType::EType mAssetType;
	LLWearableType::EType  mWearableType;
};
class LLLinkItemBridge : public LLItemBridge
{
public:
	LLLinkItemBridge(LLInventoryPanel* inventory,
					 LLFolderView* root,
					 const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual const std::string& getPrefix() { return sPrefix; }
	virtual LLUIImagePtr getIcon() const;
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
protected:
	static std::string sPrefix;
};
class LLLinkFolderBridge : public LLItemBridge
{
public:
	LLLinkFolderBridge(LLInventoryPanel* inventory,
					   LLFolderView* root,
					   const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual const std::string& getPrefix() { return sPrefix; }
	virtual LLUIImagePtr getIcon() const;
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void gotoItem();
protected:
	const LLUUID &getFolderID() const;
	static std::string sPrefix;
};
class LLMeshBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
protected:
	LLMeshBridge(LLInventoryPanel* inventory,
		     LLFolderView* root,
		     const LLUUID& uuid) :
                       LLItemBridge(inventory, root, uuid) {}
};
class LLInvFVBridgeAction
{
public:
	static LLInvFVBridgeAction* createAction(LLAssetType::EType asset_type,
											 const LLUUID& uuid,
											 LLInventoryModel* model);
	static void doAction(LLAssetType::EType asset_type,
						 const LLUUID& uuid, LLInventoryModel* model);
	static void doAction(const LLUUID& uuid, LLInventoryModel* model);
	virtual void doIt() {};
	virtual ~LLInvFVBridgeAction() {}
protected:
	LLInvFVBridgeAction(const LLUUID& id, LLInventoryModel* model) :
		mUUID(id), mModel(model) {}
	LLViewerInventoryItem* getItem() const;
protected:
	const LLUUID& mUUID;
	LLInventoryModel* mModel;
};
class LLRecentItemsFolderBridge : public LLFolderBridge
{
	friend class LLInvFVBridgeAction;
public:
	LLRecentItemsFolderBridge(LLInventoryType::EType type,
							  LLInventoryPanel* inventory,
							  LLFolderView* root,
							  const LLUUID& uuid) :
		LLFolderBridge(inventory, root, uuid)
	{
		mInvType = type;
	}
	void buildContextMenu(LLMenuGL& menu, U32 flags);
};
class LLRecentInventoryBridgeBuilder : public LLInventoryFolderViewModelBuilder
{
public:
	LLRecentInventoryBridgeBuilder() {}
	virtual LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
		LLAssetType::EType actual_asset_type,
		LLInventoryType::EType inv_type,
		LLInventoryPanel* inventory,
		LLFolderView* root,
		const LLUUID& uuid,
		U32 flags = 0x00) const;
};
class LLMarketplaceFolderBridge : public LLFolderBridge
{
public:
	LLMarketplaceFolderBridge(LLInventoryPanel* inventory,
							  LLFolderView* root,
							  const LLUUID& uuid);
	virtual LLUIImagePtr getIcon() const;
	virtual LLUIImagePtr getIconOpen() const;
	virtual std::string getLabelSuffix() const;
	virtual LLFontGL::StyleFlags getLabelStyle() const;
private:
	LLUIImagePtr getMarketplaceFolderIcon(BOOL is_open) const;
	mutable S32 m_depth;
	mutable S32 m_stockCountCache;
};
void rez_attachment(LLViewerInventoryItem* item,
					LLViewerJointAttachment* attachment,
					bool replace = false);
BOOL move_inv_category_world_to_agent(const LLUUID& object_id,
									  const LLUUID& category_id,
									  BOOL drop,
									  void (*callback)(S32, void*) = NULL,
									  void* user_data = NULL);
void hide_context_entries(LLMenuGL& menu,
						  const menuentry_vec_t &entries_to_show,
						  const menuentry_vec_t &disabled_entries);
class LLFolderViewGroupedItemBridge: public LLFolderViewGroupedItemModel
{
public:
    LLFolderViewGroupedItemBridge();
    virtual void groupFilterContextMenu(folder_view_item_deque& selected_items, LLMenuGL& menu);
};
bool isAddAction(const std::string& action);
bool isRemoveAction(const std::string& action);
#endif
