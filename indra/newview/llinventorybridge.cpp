/**
 * @file llinventorybridge.cpp
 * @brief Implementation of the Inventory-Folder-View-Bridge classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"
#include "llinventorybridge.h"
#include "lluictrlfactory.h"
#include "lffloaterinvpanel.h"
#include "llagent.h"
#include "llagentbenefits.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llattachmentsmgr.h"
#include "llavataractions.h"
#include "llcallingcard.h"
#include "llfavoritesbar.h"
#include "llfirstuse.h"
#include "llfloatercustomize.h"
#include "llfloatermarketplacelistings.h"
#include "llfloateropenobject.h"
#include "llfloaterproperties.h"
#include "llfloaterworldmap.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "llgiveinventory.h"
#include "llimview.h"
#include "llinventoryclipboard.h"
#include "llinventorydefines.h"
#include "llinventoryfunctions.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "llmarketplacefunctions.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpanelmaininventory.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewlandmark.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llselectmgr.h"
#include "lltooldraganddrop.h"
#include "lltrans.h"
#include "llurlaction.h"
#include "llviewerassettype.h"
#include "llviewerfoldertype.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llworldmap.h"
#include "llwearable.h"
#include "llwearablelist.h"
#include "hippogridmanager.h"
#include "rlvactions.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
typedef std::pair<LLUUID, LLUUID> two_uuids_t;
typedef std::list<two_uuids_t> two_uuids_list_t;
const F32 SOUND_GAIN = 1.0f;
struct LLMoveInv
{
	LLUUID mObjectID;
	LLUUID mCategoryID;
	two_uuids_list_t mMoveList;
	void (*mCallback)(S32, void*);
	void* mUserData;
};
using namespace LLOldEvents;
void remove_inventory_category_from_avatar(LLInventoryCategory* category);
void remove_inventory_category_from_avatar_step2( BOOL proceed, LLUUID category_id);
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, LLMoveInv*);
bool confirm_attachment_rez(const LLSD& notification, const LLSD& response);
static BOOL can_move_to_outfit(LLInventoryItem* inv_item, BOOL move_is_into_current_outfit);
void build_context_menu_folder_options(LLInventoryModel* mModel, const LLUUID& mUUID, menuentry_vec_t& items, menuentry_vec_t& disabled_items);
bool isAddAction(const std::string& action)
{
	return ("wear" == action || "attach" == action || "activate" == action);
}
bool isRemoveAction(const std::string& action)
{
	return ("take_off" == action || "detach" == action);
}
class LLRightClickInventoryFetchDescendentsObserver : public LLInventoryFetchDescendentsObserver
{
public:
	LLRightClickInventoryFetchDescendentsObserver(const uuid_vec_t& ids) : LLInventoryFetchDescendentsObserver(ids) {}
	~LLRightClickInventoryFetchDescendentsObserver() {}
	virtual void execute(bool clear_observer = false);
	virtual void done()
	{
		execute(true);
	}
};
class LLRightClickInventoryFetchObserver : public LLInventoryFetchItemsObserver
{
public:
	LLRightClickInventoryFetchObserver(const uuid_vec_t& ids) : LLInventoryFetchItemsObserver(ids) { };
	~LLRightClickInventoryFetchObserver() {}
	void execute(bool clear_observer = false)
	{
		if (clear_observer)
		{
			gInventory.removeObserver(this);
			delete this;
		}
		LLFolderBridge::staticFolderOptionsMenu();
	}
	virtual void done()
	{
		execute(true);
	}
};
LLInvFVBridge::LLInvFVBridge(LLInventoryPanel* inventory,
							 LLFolderView* root,
							 const LLUUID& uuid) :
	mUUID(uuid),
	mRoot(root),
	mInvType(LLInventoryType::IT_NONE),
	mIsLink(FALSE)
{
	mInventoryPanel = inventory->getInventoryPanelHandle();
	const LLInventoryObject* obj = getInventoryObject();
	mIsLink = obj && obj->getIsLinkType();
}
const std::string& LLInvFVBridge::getName() const
{
	const LLInventoryObject* obj = getInventoryObject();
	if(obj)
	{
		return obj->getName();
	}
	return LLStringUtil::null;
}
const std::string& LLInvFVBridge::getDisplayName() const
{
	if(mDisplayName.empty())
	{
		buildDisplayName();
	}
	return mDisplayName;
}
PermissionMask LLInvFVBridge::getPermissionMask() const
{
	return PERM_ALL;
}
LLFolderType::EType LLInvFVBridge::getPreferredType() const
{
	return LLFolderType::FT_NONE;
}
time_t LLInvFVBridge::getCreationDate() const
{
	LLInventoryObject* objectp = getInventoryObject();
	if (objectp)
	{
		return objectp->getCreationDate();
	}
	return (time_t)0;
}
void LLInvFVBridge::setCreationDate(time_t creation_date_utc)
{
	LLInventoryObject* objectp = getInventoryObject();
	if (objectp)
	{
		objectp->setCreationDate(creation_date_utc);
	}
}
BOOL LLInvFVBridge::isItemRemovable() const
{
	return get_is_item_removable(getInventoryModel(), mUUID);
}
BOOL LLInvFVBridge::isItemMovable() const
{
	return TRUE;
}
BOOL LLInvFVBridge::isLink() const
{
	return mIsLink;
}
BOOL LLInvFVBridge::isLibraryItem() const
{
	return gInventory.isObjectDescendentOf(getUUID(),gInventory.getLibraryRootFolderID());
}
BOOL LLInvFVBridge::cutToClipboard()
{
	if(isItemMovable())
	{
		const LLUUID& marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
		const BOOL cut_from_marketplacelistings = gInventory.isObjectDescendentOf(mUUID, marketplacelistings_id);
		if (cut_from_marketplacelistings && (LLMarketplaceData::instance().isInActiveFolder(mUUID) ||
											 LLMarketplaceData::instance().isListedAndActive(mUUID)))
		{
			LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLInvFVBridge::callback_cutToClipboard, this, _1, _2));
		}
		else
		{
			return perform_cutToClipboard();
		}
	}
	return FALSE;
}
BOOL LLInvFVBridge::callback_cutToClipboard(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		return perform_cutToClipboard();
	}
	return FALSE;
}
BOOL LLInvFVBridge::perform_cutToClipboard()
{
	if (isItemMovable())
	{
		LLInventoryClipboard::instance().cut(mUUID);
		LLFolderView::removeCutItems();
		return true;
	}
	return false;
}
BOOL LLInvFVBridge::copyToClipboard() const
{
	const LLInventoryObject* obj = gInventory.getObject(mUUID);
	if (obj && isItemCopyable())
	{
		LLInventoryClipboard::instance().add(mUUID);
		return true;
	}
	return FALSE;
}
void LLInvFVBridge::showProperties()
{
	if (isMarketplaceListingsFolder())
	{
		show_item_profile(mUUID);
		LLFloater* floater_properties = LLFloaterProperties::find(mUUID, LLUUID::null);
		if (floater_properties)
		{
			floater_properties->setVisible(true);
			floater_properties->setFrontmost(true);
		}
	}
	else
	{
		show_item_profile(mUUID);
	}
}
void LLInvFVBridge::removeBatch(std::vector<LLFolderViewEventListener*>& batch)
{
	LLInvFVBridge* bridge;
	LLInventoryModel* model = getInventoryModel();
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	U32 count = batch.size();
	U32 i,j;
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch[i]);
		if(!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if (item)
		{
			if(LLAssetType::AT_GESTURE == item->getType())
			{
				LLGestureMgr::instance().deactivateGesture(item->getUUID());
			}
		}
	}
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch[i]);
		if(!bridge || !bridge->isItemRemovable()) continue;
		cat = (LLViewerInventoryCategory*)model->getCategory(bridge->getUUID());
		if (cat)
		{
			gInventory.collectDescendents( cat->getUUID(), descendent_categories, descendent_items, FALSE );
			for (j=0; j<descendent_items.size(); j++)
			{
				if(LLAssetType::AT_GESTURE == descendent_items[j]->getType())
				{
					LLGestureMgr::instance().deactivateGesture(descendent_items[j]->getUUID());
				}
			}
		}
	}
	removeBatchNoCheck(batch);
}
void LLInvFVBridge::removeBatchNoCheck(std::vector<LLFolderViewEventListener*>& batch)
{
	LLInvFVBridge* bridge;
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	LLMessageSystem* msg = gMessageSystem;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLViewerInventoryItem* item = NULL;
	uuid_vec_t move_ids;
	LLInventoryModel::update_map_t update;
	bool start_new_message = true;
	S32 count = batch.size();
	S32 i;
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch[i]);
		if(!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if(item)
		{
			LLPreview::hide(item->getUUID());
		}
	}
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch[i]);
		if(!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if(item)
		{
			if(item->getParentUUID() == trash_id) continue;
			move_ids.push_back(item->getUUID());
			--update[item->getParentUUID()];
			++update[trash_id];
			if(start_new_message)
			{
				start_new_message = false;
				msg->newMessageFast(_PREHASH_MoveInventoryItem);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addBOOLFast(_PREHASH_Stamp, TRUE);
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_ItemID, item->getUUID());
			msg->addUUIDFast(_PREHASH_FolderID, trash_id);
			msg->addString("NewName", NULL);
			if(msg->isSendFullFast(_PREHASH_InventoryData))
			{
				start_new_message = true;
				gAgent.sendReliableMessage();
				gInventory.accountForUpdate(update);
				update.clear();
			}
		}
	}
	if(!start_new_message)
	{
		start_new_message = true;
		gAgent.sendReliableMessage();
		gInventory.accountForUpdate(update);
		update.clear();
	}
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch[i]);
		if(!bridge || !bridge->isItemRemovable()) continue;
		LLViewerInventoryCategory* cat = (LLViewerInventoryCategory*)model->getCategory(bridge->getUUID());
		if(cat)
		{
			if(cat->getParentUUID() == trash_id) continue;
			move_ids.push_back(cat->getUUID());
			--update[cat->getParentUUID()];
			++update[trash_id];
			if(start_new_message)
			{
				start_new_message = false;
				msg->newMessageFast(_PREHASH_MoveInventoryFolder);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addBOOL("Stamp", TRUE);
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_FolderID, cat->getUUID());
			msg->addUUIDFast(_PREHASH_ParentID, trash_id);
			if(msg->isSendFullFast(_PREHASH_InventoryData))
			{
				start_new_message = true;
				gAgent.sendReliableMessage();
				gInventory.accountForUpdate(update);
				update.clear();
			}
		}
	}
	if(!start_new_message)
	{
		gAgent.sendReliableMessage();
		gInventory.accountForUpdate(update);
	}
	uuid_vec_t::iterator it = move_ids.begin();
	uuid_vec_t::iterator end = move_ids.end();
	for(; it != end; ++it)
	{
		gInventory.moveObject((*it), trash_id);
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		if (item)
		{
			model->updateItem(item);
		}
	}
	model->notifyObservers();
}
BOOL LLInvFVBridge::isClipboardPasteable() const
{
	if (!LLInventoryClipboard::instance().hasContents() || !isAgentInventory())
	{
		return FALSE;
	}
	LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}
	if (LLInventoryClipboard::instance().isCutMode())
	{
		return TRUE;
	}
	LLInventoryPanel* panel = dynamic_cast<LLInventoryPanel*>(mInventoryPanel.get());
	uuid_vec_t objects;
	LLInventoryClipboard::instance().retrieve(objects);
	S32 count = objects.size();
	for(S32 i = 0; i < count; i++)
	{
		const LLUUID &item_id = objects.at(i);
		const LLInventoryCategory *cat = model->getCategory(item_id);
		if (cat)
		{
			LLFolderBridge cat_br(panel, mRoot, item_id);
			if (!cat_br.isItemCopyable())
				return FALSE;
			continue;
		}
		LLItemBridge item_br(panel, mRoot, item_id);
		if (!item_br.isItemCopyable())
			return FALSE;
	}
	return TRUE;
}
bool LLInvFVBridge::isClipboardPasteableAsCopy() const
{
	if (LLInventoryClipboard::instance().isCutMode())
	{
		return false;
	}
	LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return false;
	}
	LLInventoryPanel* panel = dynamic_cast<LLInventoryPanel*>(mInventoryPanel.get());
	uuid_vec_t objects;
	LLInventoryClipboard::instance().retrieve(objects);
	const S32 count = objects.size();
	for(S32 i = 0; i < count; i++)
	{
		const LLUUID &item_id = objects.at(i);
		const LLInventoryCategory *cat = model->getCategory(item_id);
		if (cat)
		{
			const LLFolderBridge cat_br(panel, mRoot, item_id);
			if (cat_br.isLink())
				return true;
			continue;
		}
		const LLItemBridge item_br(panel, mRoot, item_id);
		if (item_br.isLink())
			return true;
	}
	return false;
}
BOOL LLInvFVBridge::isClipboardPasteableAsLink() const
{
	if (!LLInventoryClipboard::instance().hasContents() || !isAgentInventory())
	{
		return FALSE;
	}
	const LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}
	uuid_vec_t objects;
	LLInventoryClipboard::instance().retrieve(objects);
	S32 count = objects.size();
	for(S32 i = 0; i < count; i++)
	{
		const LLInventoryItem *item = model->getItem(objects.at(i));
		if (item)
		{
			if (!LLAssetType::lookupCanLink(item->getActualType()))
			{
				return FALSE;
			}
		}
		const LLViewerInventoryCategory *cat = model->getCategory(objects.at(i));
		if (cat && LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
		{
			return FALSE;
		}
	}
	return TRUE;
}
void disable_context_entries_if_present(LLMenuGL& menu,
                                        const menuentry_vec_t &disabled_entries)
{
	const LLView::child_list_t *list = menu.getChildList();
	for (LLView::child_list_t::const_iterator itor = list->begin();
		 itor != list->end();
		 ++itor)
	{
		LLView *menu_item = (*itor);
		std::string name = menu_item->getName();
		LLMenuItemBranchGL* branchp = dynamic_cast<LLMenuItemBranchGL*>(menu_item);
		if ((name == "More") && branchp)
		{
			disable_context_entries_if_present(*branchp->getBranch(), disabled_entries);
		}
		bool found = false;
		menuentry_vec_t::const_iterator itor2;
		for (itor2 = disabled_entries.begin(); itor2 != disabled_entries.end(); ++itor2)
		{
			if (*itor2 == name)
			{
				found = true;
				break;
			}
		}
        if (found)
        {
			menu_item->setEnabled(FALSE);
        }
    }
}
void hide_context_entries(LLMenuGL& menu,
						  const menuentry_vec_t &entries_to_show,
						  const menuentry_vec_t &disabled_entries)
{
	const LLView::child_list_t *list = menu.getChildList();
	bool is_previous_entry_separator = true;
	for (LLView::child_list_t::const_iterator itor = list->begin();
		 itor != list->end();
		 ++itor)
	{
		LLView *menu_item = (*itor);
		std::string name = menu_item->getName();
		LLMenuItemBranchGL* branchp = dynamic_cast<LLMenuItemBranchGL*>(menu_item);
		if ((name == "More") && branchp)
		{
			hide_context_entries(*branchp->getBranch(), entries_to_show, disabled_entries);
		}
		bool found = false;
		menuentry_vec_t::const_iterator itor2;
		for (itor2 = entries_to_show.begin(); itor2 != entries_to_show.end(); ++itor2)
		{
			if (*itor2 == name)
			{
				found = true;
				break;
			}
		}
		if (found)
		{
			const bool is_entry_separator = !branchp && (dynamic_cast<LLMenuItemSeparatorGL *>(menu_item) != NULL);
			found = !(is_entry_separator && is_previous_entry_separator);
			is_previous_entry_separator = is_entry_separator;
		}
		if (!found)
		{
			if (!menu_item->getLastVisible())
			{
				menu_item->setVisible(FALSE);
			}
			menu_item->setEnabled(FALSE);
		}
		else
		{
			menu_item->setVisible(TRUE);
			menu_item->pushVisible(TRUE);
			bool enabled = (menu_item->getEnabled() == TRUE);
			for (itor2 = disabled_entries.begin(); enabled && (itor2 != disabled_entries.end()); ++itor2)
			{
				enabled &= (*itor2 != name);
			}
			menu_item->setEnabled(enabled);
		}
	}
}
void LLInvFVBridge::getClipboardEntries(bool show_asset_id,
										menuentry_vec_t &items,
										menuentry_vec_t &disabled_items, U32 flags)
{
	const LLInventoryObject *obj = getInventoryObject();
	if (obj)
	{
		if (obj->getIsLinkType())
		{
			items.push_back(std::string("Copy Separator"));
			items.push_back(std::string("Copy"));
			if (!isItemCopyable())
			{
				disabled_items.push_back(std::string("Copy"));
			}
			items.push_back(std::string("Cut"));
			if (!isItemMovable() || !isItemRemovable())
			{
				disabled_items.push_back(std::string("Cut"));
			}
			items.push_back(std::string("Find Original"));
			if (isLinkedObjectMissing())
			{
				disabled_items.push_back(std::string("Find Original"));
			}
		}
		else
		{
			if (LLAssetType::lookupCanLink(obj->getType()))
			{
				items.push_back(std::string("Find Links"));
			}
			if (!isInboxFolder())
			{
				items.push_back(std::string("Rename"));
				if (!isItemRenameable() || ((flags & FIRST_SELECTED_ITEM) == 0))
				{
					disabled_items.push_back(std::string("Rename"));
				}
			}
			if (show_asset_id)
			{
				items.push_back(std::string("Copy Asset UUID"));
				bool is_asset_knowable = false;
				LLViewerInventoryItem* inv_item = gInventory.getItem(mUUID);
				if (inv_item)
				{
					is_asset_knowable = LLAssetType::lookupIsAssetIDKnowable(inv_item->getType());
				}
				if ( !is_asset_knowable
					 || (! ( isItemPermissive() || gAgent.isGodlike() ) )
					 || (flags & FIRST_SELECTED_ITEM) == 0)
				{
					disabled_items.push_back(std::string("Copy Asset UUID"));
				}
			}
			items.push_back(std::string("Copy Separator"));
			items.push_back(std::string("Copy"));
			if (!isItemCopyable())
			{
				disabled_items.push_back(std::string("Copy"));
			}
			items.push_back(std::string("Cut"));
			if (!isItemMovable() || !isItemRemovable())
			{
				disabled_items.push_back(std::string("Cut"));
			}
			if (canListOnMarketplace() && !isMarketplaceListingsFolder() && !isInboxFolder())
			{
				items.push_back(std::string("Marketplace Separator"));
				if (gMenuHolder->getChild<LLView>("MarketplaceListings")->getVisible())
				{
					items.push_back(std::string("Marketplace Copy"));
					items.push_back(std::string("Marketplace Move"));
					if (!canListOnMarketplaceNow())
					{
						disabled_items.push_back(std::string("Marketplace Copy"));
						disabled_items.push_back(std::string("Marketplace Move"));
					}
				}
			}
		}
	}
	bool paste_as_copy = false;
	if (!isCOFFolder() && !isInboxFolder())
	{
		items.push_back(std::string("Paste"));
		if (isClipboardPasteableAsCopy())
		{
			items.push_back(std::string("Paste As Copy"));
			paste_as_copy = true;
		}
	}
	if (!isClipboardPasteable() || ((flags & FIRST_SELECTED_ITEM) == 0))
	{
		disabled_items.push_back(std::string("Paste"));
		disabled_items.push_back(std::string("Paste As Copy"));
		paste_as_copy = false;
	}
	if (!paste_as_copy)
	{
		items.push_back(std::string("Paste As Link"));
		if (!isClipboardPasteableAsLink() || (flags & FIRST_SELECTED_ITEM) == 0)
		{
			disabled_items.push_back(std::string("Paste As Link"));
		}
	}
	items.push_back(std::string("Paste Separator"));
	addDeleteContextMenuOptions(items, disabled_items);
}
void LLInvFVBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLInvFVBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
		addOpenRightClickMenuOption(items);
		items.push_back(std::string("Properties"));
		if (rlv_handler_t::isEnabled())
		{
			const LLInventoryObject* pItem = getInventoryObject();
			if ( (pItem) &&
				 ( ((LLAssetType::AT_NOTECARD == pItem->getType()) && (gRlvHandler.hasBehaviour(RLV_BHVR_VIEWNOTE))) ||
				   ((LLAssetType::AT_LSL_TEXT == pItem->getType()) && (gRlvHandler.hasBehaviour(RLV_BHVR_VIEWSCRIPT))) ||
				   ((LLAssetType::AT_TEXTURE == pItem->getType()) && (gRlvHandler.hasBehaviour(RLV_BHVR_VIEWTEXTURE))) ) )
			{
				disabled_items.push_back(std::string("Open"));
			}
		}
		getClipboardEntries(true, items, disabled_items, flags);
	}
	hide_context_entries(menu, items, disabled_items);
}
bool get_selection_item_uuids(LLFolderView::selected_items_t& selected_items, uuid_vec_t& ids)
{
	uuid_vec_t results;
    S32 non_item = 0;
	for(LLFolderView::selected_items_t::iterator it = selected_items.begin(); it != selected_items.end(); ++it)
	{
		LLItemBridge *view_model = dynamic_cast<LLItemBridge *>((*it)->getListener());
		if(view_model && view_model->getUUID().notNull())
		{
			results.push_back(view_model->getUUID());
		}
        else
        {
            non_item++;
        }
	}
	if (non_item == 0)
	{
		ids = results;
		return true;
	}
	return false;
}
void LLInvFVBridge::addTrashContextMenuOptions(menuentry_vec_t &items,
											   menuentry_vec_t &disabled_items)
{
	const LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType())
	{
		items.push_back(std::string("Find Original"));
		if (isLinkedObjectMissing())
		{
			disabled_items.push_back(std::string("Find Original"));
		}
	}
	items.push_back(std::string("Purge Item"));
	if (!isItemRemovable())
	{
		disabled_items.push_back(std::string("Purge Item"));
	}
	items.push_back(std::string("Restore Item"));
}
void LLInvFVBridge::addDeleteContextMenuOptions(menuentry_vec_t &items,
												menuentry_vec_t &disabled_items)
{
	const LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType() && isCOFFolder() && get_is_item_worn(mUUID))
	{
		return;
	}
	if (obj && obj->getIsLinkType() && !get_is_item_worn(mUUID))
	{
		items.push_back(std::string("Remove Link"));
	}
	else
	{
		items.push_back(std::string("Delete"));
	}
	if (!isItemRemovable())
	{
		disabled_items.push_back(std::string("Delete"));
	}
}
void LLInvFVBridge::addOpenRightClickMenuOption(menuentry_vec_t &items)
{
	const LLInventoryObject *obj = getInventoryObject();
	const BOOL is_link = (obj && obj->getIsLinkType());
	if (is_link)
		items.push_back(std::string("Open Original"));
	else
		items.push_back(std::string("Open"));
}
void LLInvFVBridge::addMarketplaceContextMenuOptions(U32 flags,
												menuentry_vec_t &items,
												menuentry_vec_t &disabled_items)
{
	S32 depth = depth_nesting_in_marketplace(mUUID);
	if (depth == 1)
	{
		items.push_back(std::string("Marketplace Create Listing"));
		items.push_back(std::string("Marketplace Associate Listing"));
		items.push_back(std::string("Marketplace Check Listing"));
		items.push_back(std::string("Marketplace List"));
		items.push_back(std::string("Marketplace Unlist"));
		if (LLMarketplaceData::instance().isUpdating(mUUID,depth) || ((flags & FIRST_SELECTED_ITEM) == 0))
		{
			disabled_items.push_back(std::string("Marketplace Create Listing"));
			disabled_items.push_back(std::string("Marketplace Associate Listing"));
			disabled_items.push_back(std::string("Marketplace Check Listing"));
			disabled_items.push_back(std::string("Marketplace List"));
			disabled_items.push_back(std::string("Marketplace Unlist"));
		}
		else
		{
			if (gSavedSettings.getBOOL("MarketplaceListingsLogging"))
			{
				items.push_back(std::string("Marketplace Get Listing"));
			}
			if (LLMarketplaceData::instance().isListed(mUUID))
			{
				disabled_items.push_back(std::string("Marketplace Create Listing"));
				disabled_items.push_back(std::string("Marketplace Associate Listing"));
				items.push_back(std::string("Marketplace Copy ID"));
				if (LLMarketplaceData::instance().getVersionFolder(mUUID).isNull())
				{
					disabled_items.push_back(std::string("Marketplace List"));
					disabled_items.push_back(std::string("Marketplace Unlist"));
				}
				else
				{
					if (LLMarketplaceData::instance().getActivationState(mUUID))
					{
						disabled_items.push_back(std::string("Marketplace List"));
					}
					else
					{
						disabled_items.push_back(std::string("Marketplace Unlist"));
					}
				}
			}
			else
			{
				disabled_items.push_back(std::string("Marketplace List"));
				disabled_items.push_back(std::string("Marketplace Unlist"));
				if (gSavedSettings.getBOOL("MarketplaceListingsLogging"))
				{
					disabled_items.push_back(std::string("Marketplace Get Listing"));
				}
			}
		}
	}
	if (depth == 2)
	{
		LLInventoryCategory* cat = gInventory.getCategory(mUUID);
		if (cat && LLMarketplaceData::instance().isListed(cat->getParentUUID()))
		{
			items.push_back(std::string("Marketplace Activate"));
			items.push_back(std::string("Marketplace Deactivate"));
			if (LLMarketplaceData::instance().isUpdating(mUUID,depth) || ((flags & FIRST_SELECTED_ITEM) == 0))
			{
				disabled_items.push_back(std::string("Marketplace Activate"));
				disabled_items.push_back(std::string("Marketplace Deactivate"));
			}
			else
			{
				if (LLMarketplaceData::instance().isVersionFolder(mUUID))
				{
					disabled_items.push_back(std::string("Marketplace Activate"));
					if (LLMarketplaceData::instance().getActivationState(mUUID))
					{
						disabled_items.push_back(std::string("Marketplace Deactivate"));
					}
				}
				else
				{
					disabled_items.push_back(std::string("Marketplace Deactivate"));
				}
			}
		}
	}
	items.push_back(std::string("Marketplace Edit Listing"));
	LLUUID listing_folder_id = nested_parent_id(mUUID,depth);
	LLUUID version_folder_id = LLMarketplaceData::instance().getVersionFolder(listing_folder_id);
	if (depth >= 2)
	{
		LLUUID local_version_folder_id = nested_parent_id(mUUID,depth-1);
		LLInventoryModel::cat_array_t categories;
		LLInventoryModel::item_array_t items;
		gInventory.collectDescendents(local_version_folder_id, categories, items, FALSE);
		if (categories.size() >= gSavedSettings.getU32("InventoryOutboxMaxFolderCount"))
		{
			disabled_items.push_back(std::string("New Folder"));
		}
	}
	if (!LLMarketplaceData::instance().isListed(listing_folder_id) || version_folder_id.isNull())
	{
		disabled_items.push_back(std::string("Marketplace Edit Listing"));
	}
	items.push_back(std::string("Marketplace Listings Separator"));
}
BOOL LLInvFVBridge::startDrag(EDragAndDropType* type, LLUUID* id) const
{
	BOOL rv = FALSE;
	const LLInventoryObject* obj = getInventoryObject();
	if(obj)
	{
		*type = LLViewerAssetType::lookupDragAndDropType(obj->getActualType());
		if(*type == DAD_NONE)
		{
			return FALSE;
		}
		*id = obj->getUUID();
		if (*type == DAD_CATEGORY)
		{
			LLInventoryModelBackgroundFetch::instance().start(obj->getUUID());
		}
		rv = TRUE;
	}
	return rv;
}
LLInventoryObject* LLInvFVBridge::getInventoryObject() const
{
	LLInventoryObject* obj = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		obj = (LLInventoryObject*)model->getObject(mUUID);
	}
	return obj;
}
LLInventoryModel* LLInvFVBridge::getInventoryModel() const
{
	LLInventoryPanel* panel = mInventoryPanel.get();
	return panel ? panel->getModel() : nullptr;
}
LLInventoryFilter* LLInvFVBridge::getInventoryFilter() const
{
	LLInventoryPanel* panel = mInventoryPanel.get();
	return panel ? &panel->getFilter() : nullptr;
}
BOOL LLInvFVBridge::isItemInTrash() const
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	return model->isObjectDescendentOf(mUUID, trash_id);
}
BOOL LLInvFVBridge::isLinkedObjectInTrash() const
{
	if (isItemInTrash()) return TRUE;
	const LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType())
	{
		LLInventoryModel* model = getInventoryModel();
		if(!model) return FALSE;
		const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
		return model->isObjectDescendentOf(obj->getLinkedUUID(), trash_id);
	}
	return FALSE;
}
BOOL LLInvFVBridge::isLinkedObjectMissing() const
{
	const LLInventoryObject *obj = getInventoryObject();
	if (!obj)
	{
		return TRUE;
	}
	if (obj->getIsLinkType() && LLAssetType::lookupIsLinkType(obj->getType()))
	{
		return TRUE;
	}
	return FALSE;
}
BOOL LLInvFVBridge::isAgentInventory() const
{
	const LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	if(gInventory.getRootFolderID() == mUUID) return TRUE;
	return model->isObjectDescendentOf(mUUID, gInventory.getRootFolderID());
}
BOOL LLInvFVBridge::isCOFFolder() const
{
	return LLAppearanceMgr::instance().getIsInCOF(mUUID);
}
BOOL LLInvFVBridge::isInboxFolder() const
{
	const LLUUID inbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, false);
	if (inbox_id.isNull())
	{
		return FALSE;
	}
	return gInventory.isObjectDescendentOf(mUUID, inbox_id);
}
BOOL LLInvFVBridge::isMarketplaceListingsFolder() const
{
	const LLUUID folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	if (folder_id.isNull())
	{
		return FALSE;
	}
	return gInventory.isObjectDescendentOf(mUUID, folder_id);
}
BOOL LLInvFVBridge::isItemPermissive() const
{
	return FALSE;
}
void LLInvFVBridge::changeItemParent(LLInventoryModel* model,
									 LLViewerInventoryItem* item,
									 const LLUUID& new_parent_id,
									 BOOL restamp)
{
	model->changeItemParent(item, new_parent_id, restamp);
}
void LLInvFVBridge::changeCategoryParent(LLInventoryModel* model,
										 LLViewerInventoryCategory* cat,
										 const LLUUID& new_parent_id,
										 BOOL restamp)
{
	model->changeCategoryParent(cat, new_parent_id, restamp);
}
LLInvFVBridge* LLInvFVBridge::createBridge(LLAssetType::EType asset_type,
										   LLAssetType::EType actual_asset_type,
										   LLInventoryType::EType inv_type,
										   LLInventoryPanel* inventory,
										   LLFolderView* root,
										   const LLUUID& uuid,
										   U32 flags)
{
	LLInvFVBridge* new_listener = NULL;
	switch(asset_type)
	{
		case LLAssetType::AT_TEXTURE:
			if(!(inv_type == LLInventoryType::IT_TEXTURE || inv_type == LLInventoryType::IT_SNAPSHOT))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLTextureBridge(inventory, root, uuid, inv_type);
			break;
		case LLAssetType::AT_SOUND:
			if(!(inv_type == LLInventoryType::IT_SOUND))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLSoundBridge(inventory, root, uuid);
			break;
		case LLAssetType::AT_LANDMARK:
			if(!(inv_type == LLInventoryType::IT_LANDMARK))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLLandmarkBridge(inventory, root, uuid, flags);
			break;
		case LLAssetType::AT_CALLINGCARD:
			if(!(inv_type == LLInventoryType::IT_CALLINGCARD))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLCallingCardBridge(inventory, root, uuid);
			break;
		case LLAssetType::AT_SCRIPT:
			if(!(inv_type == LLInventoryType::IT_LSL))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLItemBridge(inventory, root, uuid);
			break;
		case LLAssetType::AT_OBJECT:
			if(!(inv_type == LLInventoryType::IT_OBJECT || inv_type == LLInventoryType::IT_ATTACHMENT)
			|| inv_type == LLInventoryType::IT_TEXTURE )
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLObjectBridge(inventory, root, uuid, inv_type, flags);
			break;
		case LLAssetType::AT_NOTECARD:
			if(!(inv_type == LLInventoryType::IT_NOTECARD))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLNotecardBridge(inventory, root, uuid);
			break;
		case LLAssetType::AT_ANIMATION:
			if(!(inv_type == LLInventoryType::IT_ANIMATION))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLAnimationBridge(inventory, root, uuid);
			break;
		case LLAssetType::AT_GESTURE:
			if(!(inv_type == LLInventoryType::IT_GESTURE))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLGestureBridge(inventory, root, uuid);
			break;
		case LLAssetType::AT_LSL_TEXT:
			if(!(inv_type == LLInventoryType::IT_LSL))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLLSLTextBridge(inventory, root, uuid);
			break;
		case LLAssetType::AT_CLOTHING:
		case LLAssetType::AT_BODYPART:
			if(!(inv_type == LLInventoryType::IT_WEARABLE))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLWearableBridge(inventory, root, uuid, asset_type, inv_type, LLWearableType::inventoryFlagsToWearableType(flags));
			break;
		case LLAssetType::AT_CATEGORY:
			if (actual_asset_type == LLAssetType::AT_LINK_FOLDER)
			{
				new_listener = new LLLinkFolderBridge(inventory, root, uuid);
			}
			else if (actual_asset_type == LLAssetType::AT_MARKETPLACE_FOLDER)
			{
				new_listener = new LLMarketplaceFolderBridge(inventory, root, uuid);
			}
			else
			{
				new_listener = new LLFolderBridge(inventory, root, uuid);
			}
			break;
		case LLAssetType::AT_LINK:
		case LLAssetType::AT_LINK_FOLDER:
			new_listener = new LLLinkItemBridge(inventory, root, uuid);
			break;
	    case LLAssetType::AT_MESH:
			if(!(inv_type == LLInventoryType::IT_MESH))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLMeshBridge(inventory, root, uuid);
			break;
		case LLAssetType::AT_IMAGE_TGA:
		case LLAssetType::AT_IMAGE_JPEG:
			break;
		case LLAssetType::AT_SETTINGS:
			break;
		default:
			LL_INFOS() << "Unhandled asset type (llassetstorage.h): "
					<< (S32)asset_type << " (" << LLAssetType::lookup(asset_type) << ")" << LL_ENDL;
			break;
	}
	if (new_listener)
	{
		new_listener->mInvType = inv_type;
	}
	return new_listener;
}
void LLInvFVBridge::purgeItem(LLInventoryModel *model, const LLUUID &uuid)
{
	LLInventoryObject* obj = model->getObject(uuid);
	if (obj)
	{
		remove_inventory_object(uuid, NULL);
	}
}
void LLInvFVBridge::removeObject(LLInventoryModel *model, const LLUUID &uuid)
{
	LLInventoryItem* itemp = model->getItem(uuid);
	LLUUID parent_id = (itemp ? itemp->getParentUUID() : LLUUID::null);
	model->removeObject(uuid);
	if (parent_id.notNull())
	{
		LLViewerInventoryCategory* parent_cat = model->getCategory(parent_id);
		model->updateCategory(parent_cat);
		model->notifyObservers();
	}
}
bool LLInvFVBridge::canShare() const
{
	bool can_share = false;
	if (!isItemInTrash() && isAgentInventory())
	{
		const LLInventoryModel* model = getInventoryModel();
		if (model)
		{
			const LLViewerInventoryItem *item = model->getItem(mUUID);
			if (item)
			{
				if (LLInventoryCollectFunctor::itemTransferCommonlyAllowed(item))
				{
					can_share = LLGiveInventory::isInventoryGiveAcceptable(item);
				}
			}
			else
			{
				can_share = (model->getCategory(mUUID) != NULL);
			}
		}
	}
	return can_share;
}
bool LLInvFVBridge::canListOnMarketplace() const
{
	LLInventoryModel * model = getInventoryModel();
	LLViewerInventoryCategory * cat = model->getCategory(mUUID);
	if (cat && LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
	{
		return false;
	}
	if (!isAgentInventory())
	{
		return false;
	}
	LLViewerInventoryItem * item = model->getItem(mUUID);
	if (item)
	{
		if (!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
		{
			return false;
		}
		if (LLAssetType::AT_CALLINGCARD == item->getType())
		{
			return false;
		}
	}
	return true;
}
bool LLInvFVBridge::canListOnMarketplaceNow() const
{
	bool can_list = true;
	const LLInventoryObject* obj = getInventoryObject();
	can_list &= (obj != NULL);
	if (can_list)
	{
		const LLUUID& object_id = obj->getLinkedUUID();
		can_list = object_id.notNull();
		if (can_list)
		{
			LLFolderViewFolder * object_folderp = mRoot->getFolderByID(object_id);
			if (object_folderp)
			{
				can_list = !object_folderp->isLoading();
			}
		}
		if (can_list)
		{
			std::string error_msg;
			LLInventoryModel* model = getInventoryModel();
			const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
			if (marketplacelistings_id.notNull())
			{
				LLViewerInventoryCategory * master_folder = model->getCategory(marketplacelistings_id);
				LLInventoryCategory *cat = model->getCategory(mUUID);
				if (cat)
				{
					can_list = can_move_folder_to_marketplace(master_folder, master_folder, cat, error_msg);
				}
				else
				{
					LLInventoryItem *item = model->getItem(mUUID);
					can_list = (item ? can_move_item_to_marketplace(master_folder, master_folder, item, error_msg) : false);
				}
			}
			else
			{
				can_list = false;
			}
		}
	}
	return can_list;
}
LLInvFVBridge* LLInventoryFolderViewModelBuilder::createBridge(LLAssetType::EType asset_type,
														LLAssetType::EType actual_asset_type,
														LLInventoryType::EType inv_type,
														LLInventoryPanel* inventory,
														LLFolderView* root,
														const LLUUID& uuid,
														U32 flags ) const
{
	return LLInvFVBridge::createBridge(asset_type,
									   actual_asset_type,
									   inv_type,
									   inventory,
									   root,
									   uuid,
									   flags);
}
void LLItemBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("goto" == action)
	{
		gotoItem();
	}
	if ("open" == action || "open_original" == action)
	{
		openItem();
		return;
	}
	else if ("properties" == action)
	{
		showProperties();
		return;
	}
	else if ("purge" == action)
	{
		purgeItem(model, mUUID);
		return;
	}
	else if ("restoreToWorld" == action)
	{
		restoreToWorld();
		return;
	}
	else if ("restore" == action)
	{
		restoreItem();
		return;
	}
	else if ("copy_uuid" == action)
	{
		LLViewerInventoryItem* item = static_cast<LLViewerInventoryItem*>(getItem());
		if(!item) return;
		LLUUID asset_id = item->getProtectedAssetUUID();
		std::string buffer;
		asset_id.toString(buffer);
		gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(buffer));
		return;
	}
	else if ("cut" == action)
	{
		cutToClipboard();
		return;
	}
	else if ("copy" == action)
	{
		copyToClipboard();
		return;
	}
	else if ("paste" == action)
	{
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;
		LLFolderViewItem* folder_view_itemp = mRoot->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;
		folder_view_itemp->getListener()->pasteFromClipboard();
		return;
	}
	else if ("paste_copies" == action)
	{
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;
		LLFolderViewItem* folder_view_itemp = mRoot->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;
		folder_view_itemp->getListener()->pasteFromClipboard(true);
		return;
	}
	else if ("paste_link" == action)
	{
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;
		LLFolderViewItem* folder_view_itemp = mRoot->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;
		folder_view_itemp->getListener()->pasteLinkFromClipboard();
		return;
	}
	else if (("move_to_marketplace_listings" == action) || ("copy_to_marketplace_listings" == action) || ("copy_or_move_to_marketplace_listings" == action))
	{
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;
		const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
		move_item_to_marketplacelistings(itemp, marketplacelistings_id, ("copy_to_marketplace_listings" == action));
	}
	else if ("marketplace_edit_listing" == action)
	{
		std::string url = LLMarketplaceData::instance().getListingURL(mUUID);
		LLUrlAction::openURL(url);
	}
}
void LLItemBridge::selectItem()
{
	LLViewerInventoryItem* item = static_cast<LLViewerInventoryItem*>(getItem());
	if(item && !item->isFinished())
	{
		LLInventoryModelBackgroundFetch::instance().start(item->getUUID(), false);
	}
}
void LLItemBridge::restoreItem()
{
	LLViewerInventoryItem* item = static_cast<LLViewerInventoryItem*>(getItem());
	if(item)
	{
		LLInventoryModel* model = getInventoryModel();
		const LLUUID new_parent = model->findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(item->getType()));
		LLInvFVBridge::changeItemParent(model, item, new_parent, FALSE);
	}
}
void restore_to_world(LLViewerInventoryItem* itemp, bool no_copy, bool response = true)
{
	if (!response) return;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("RezRestoreToWorld");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
	msg->nextBlockFast(_PREHASH_InventoryData);
	itemp->packMessage(msg);
	msg->sendReliable(gAgent.getRegion()->getHost());
	if (no_copy || gInventory.isObjectDescendentOf(itemp->getUUID(), gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH)))
	{
		gInventory.deleteObject(itemp->getUUID());
		gInventory.notifyObservers();
	}
}
void LLItemBridge::restoreToWorld()
{
	if (LLViewerInventoryItem* itemp = static_cast<LLViewerInventoryItem*>(getItem()))
	{
		bool no_copy = !itemp->getPermissions().allowCopyBy(gAgentID);
		if (no_copy && gHippoGridManager->getCurrentGrid()->isSecondLife())
			LLNotificationsUtil::add("RezRestoreToWorld", LLSD(), LLSD(), boost::bind(restore_to_world, itemp, true, !boost::bind(LLNotification::getSelectedOption, _1, _2)));
		else
			restore_to_world(itemp, no_copy);
	}
}
void LLItemBridge::gotoItem()
{
	LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType())
	{
		LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();
		if (active_panel)
		{
			active_panel->setSelection(obj->getLinkedUUID(), TAKE_FOCUS_NO);
		}
	}
}
LLUIImagePtr LLItemBridge::getIcon() const
{
	LLInventoryObject *obj = getInventoryObject();
	if (obj)
	{
		return LLInventoryIcon::getIcon(obj->getType(),
										LLInventoryType::IT_NONE,
										mIsLink);
	}
	return LLInventoryIcon::getIcon(LLInventoryType::ICONNAME_OBJECT);
}
LLUIImagePtr LLItemBridge::getIconOverlay() const
{
	if (getItem() && getItem()->getIsLinkType())
	{
		return LLUI::getUIImage("inv_link_overlay.tga");
	}
	return NULL;
}
PermissionMask LLItemBridge::getPermissionMask() const
{
	LLViewerInventoryItem* item = getItem();
	PermissionMask perm_mask = 0;
	if (item) perm_mask = item->getPermissionMask();
	return perm_mask;
}
void LLItemBridge::buildDisplayName() const
{
	if(getItem())
	{
		mDisplayName.assign(getItem()->getName());
	}
	else
	{
		mDisplayName.assign(LLStringUtil::null);
	}
}
LLFontGL::StyleFlags LLItemBridge::getLabelStyle() const
{
	U8 font = LLFontGL::NORMAL;
	const LLViewerInventoryItem* item = getItem();
	if (get_is_item_worn(mUUID))
	{
		font |= LLFontGL::BOLD;
	}
	else if(item && item->getIsLinkType())
	{
		font |= LLFontGL::ITALIC;
	}
	return (LLFontGL::StyleFlags)font;
}
std::string LLItemBridge::getLabelSuffix() const
{
	static std::string NO_COPY = LLTrans::getString("no_copy");
	static std::string NO_MOD = LLTrans::getString("no_modify");
	static std::string NO_XFER = LLTrans::getString("no_transfer");
	static std::string TEMPO = LLTrans::getString("temporary");
	static std::string LINK = LLTrans::getString("link");
	static std::string BROKEN_LINK = LLTrans::getString("broken_link");
	std::string suffix;
	LLInventoryItem* item = getItem();
	if(item)
	{
		BOOL broken_link = LLAssetType::lookupIsLinkType(item->getType());
		if (broken_link) return BROKEN_LINK;
		BOOL link = item->getIsLinkType();
		if (link) return LINK;
		if(LLAssetType::AT_CALLINGCARD != item->getType()
		   && item->getPermissions().getOwner() == gAgent.getID())
		{
			BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
			if (!copy)
			{
				suffix += NO_COPY;
			}
			BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
			if (!mod)
			{
				suffix += NO_MOD;
			}
			BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
																gAgent.getID());
			if (!xfer)
			{
				suffix += NO_XFER;
			}
			if (item->getPermissions().getGroup() == gAgent.getID())
			{
				suffix += TEMPO;
			}
		}
	}
	return suffix;
}
time_t LLItemBridge::getCreationDate() const
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		return item->getCreationDate();
	}
	return 0;
}
BOOL LLItemBridge::isItemRenameable() const
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		if (item->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
		{
			return FALSE;
		}
		if (!item->isFinished())
		{
			return FALSE;
		}
		if (isInboxFolder())
		{
			return FALSE;
		}
		if ( (rlv_handler_t::isEnabled()) && (!RlvFolderLocks::instance().canRenameItem(mUUID)) )
		{
			return FALSE;
		}
		return (item->getPermissions().allowModifyBy(gAgent.getID()));
	}
	return FALSE;
}
BOOL LLItemBridge::renameItem(const std::string& new_name)
{
	if(!isItemRenameable())
		return FALSE;
	LLPreview::rename(mUUID, getPrefix() + new_name);
	LLInventoryModel* model = getInventoryModel();
	if(!model)
		return FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item && (item->getName() != new_name))
	{
		LLSD updates;
		updates["name"] = new_name;
		update_inventory_item(item->getUUID(),updates, NULL);
	}
	return FALSE;
}
BOOL LLItemBridge::removeItem()
{
	if(!isItemRemovable())
	{
		return FALSE;
	}
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	const LLUUID& trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLViewerInventoryItem* item = getItem();
	if (!item) return FALSE;
	if (item->getType() != LLAssetType::AT_LSL_TEXT)
	{
		LLPreview::hide(mUUID, TRUE);
	}
	if (model->isObjectDescendentOf(mUUID, trash_id)) return FALSE;
	LLNotification::Params params("ConfirmItemDeleteHasLinks");
	params.functor(boost::bind(&LLItemBridge::confirmRemoveItem, this, _1, _2));
	if (!item->getIsLinkType())
	{
		LLInventoryModel::item_array_t item_array = gInventory.collectLinksTo(mUUID);
		const U32 num_links = item_array.size();
		if (num_links > 0)
		{
			LLNotifications::instance().add(params);
			return FALSE;
		}
	}
	LLNotifications::instance().forceResponse(params, 0);
	return TRUE;
}
BOOL LLItemBridge::confirmRemoveItem(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0) return FALSE;
	LLInventoryModel* model = getInventoryModel();
	if (!model) return FALSE;
	LLViewerInventoryItem* item = getItem();
	if (!item) return FALSE;
	const LLUUID& trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if(item && !model->isObjectDescendentOf(mUUID, trash_id))
	{
		LLInvFVBridge::changeItemParent(model, item, trash_id, TRUE);
		return TRUE;
	}
	return FALSE;
}
BOOL LLItemBridge::isItemCopyable() const
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		if (item->getIsLinkType() && !item->getLinkedItem())
		{
			return FALSE;
		}
		return (item->getPermissions().allowCopyBy(gAgent.getID())) || (LLAssetType::lookupCanLink(item->getType()));
	}
	return FALSE;
}
LLViewerInventoryItem* LLItemBridge::getItem() const
{
	LLViewerInventoryItem* item = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		item = (LLViewerInventoryItem*)model->getItem(mUUID);
	}
	return item;
}
BOOL LLItemBridge::isItemPermissive() const
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		return item->getIsFullPerm();
	}
	return FALSE;
}
LLHandle<LLFolderBridge> LLFolderBridge::sSelf;
BOOL LLFolderBridge::isItemMovable() const
{
	LLInventoryObject* obj = getInventoryObject();
	if(obj)
	{
		if (LLFolderType::lookupIsProtectedType(((LLInventoryCategory*)obj)->getPreferredType()))
			return FALSE;
		return TRUE;
	}
	return FALSE;
}
void LLFolderBridge::selectItem()
{
	LLInventoryModelBackgroundFetch::instance().start(getUUID(), true);
}
void LLFolderBridge::buildDisplayName() const
{
	mDisplayName.assign(getName());
}
std::string LLFolderBridge::getLabelSuffix() const
{
	return LLInvFVBridge::getLabelSuffix();
}
LLFontGL::StyleFlags LLFolderBridge::getLabelStyle() const
{
	return LLFontGL::NORMAL;
}
class LLIsItemRemovable : public LLFolderViewFunctor
{
public:
	LLIsItemRemovable() : mPassed(TRUE) {}
	virtual void doFolder(LLFolderViewFolder* folder)
	{
		mPassed &= folder->getListener()->isItemRemovable();
	}
	virtual void doItem(LLFolderViewItem* item)
	{
		mPassed &= item->getListener()->isItemRemovable();
	}
	BOOL mPassed;
};
BOOL LLFolderBridge::isItemRemovable() const
{
	if (!get_is_category_removable(getInventoryModel(), mUUID))
	{
		return FALSE;
	}
	LLInventoryPanel* panel = mInventoryPanel.get();
	LLFolderViewFolder* folderp = dynamic_cast<LLFolderViewFolder*>(panel ? panel->getRootFolder()->getItemByID(mUUID) : NULL);
	if (folderp)
	{
		LLIsItemRemovable folder_test;
		folderp->applyFunctorToChildren(folder_test);
		if (!folder_test.mPassed)
		{
			return FALSE;
		}
	}
	if (isMarketplaceListingsFolder() && LLMarketplaceData::instance().getActivationState(mUUID))
	{
		return FALSE;
	}
	return TRUE;
}
BOOL LLFolderBridge::isUpToDate() const
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	LLViewerInventoryCategory* category = (LLViewerInventoryCategory*)model->getCategory(mUUID);
	if( !category )
	{
		return FALSE;
	}
	return category->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN;
}
BOOL LLFolderBridge::isItemCopyable() const
{
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(mUUID,cat_array,item_array);
	LLInventoryPanel* panel = dynamic_cast<LLInventoryPanel*>(mInventoryPanel.get());
	LLInventoryModel::item_array_t item_array_copy = *item_array;
	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
		LLItemBridge item_br(panel, mRoot, item->getUUID());
		if (!item_br.isItemCopyable())
			return FALSE;
	}
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLViewerInventoryCategory* category = *iter;
		LLFolderBridge cat_br(panel, mRoot, category->getUUID());
		if (!cat_br.isItemCopyable())
			return FALSE;
	}
	return TRUE;
}
BOOL LLFolderBridge::isClipboardPasteable() const
{
	if ( ! LLInvFVBridge::isClipboardPasteable() )
		return FALSE;
	return TRUE;
}
BOOL LLFolderBridge::isClipboardPasteableAsLink() const
{
	if (!LLInvFVBridge::isClipboardPasteableAsLink())
	{
		return FALSE;
	}
	const LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}
	const LLViewerInventoryCategory *current_cat = getCategory();
	if (current_cat)
	{
		const LLUUID &current_cat_id = current_cat->getUUID();
		uuid_vec_t objects;
		LLInventoryClipboard::instance().retrieve(objects);
		S32 count = objects.size();
		for (S32 i = 0; i < count; i++)
		{
			const LLUUID &obj_id = objects.at(i);
			const LLInventoryCategory *cat = model->getCategory(obj_id);
			if (cat)
			{
				const LLUUID &cat_id = cat->getUUID();
				if ((cat_id == current_cat_id) ||
					model->isObjectDescendentOf(current_cat_id, cat_id))
				{
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}
BOOL LLFolderBridge::dragCategoryIntoFolder(LLInventoryCategory* inv_cat,
											BOOL drop,
											BOOL user_confirm)
{
	LLInventoryModel* model = getInventoryModel();
	if (!inv_cat) return FALSE;
	if (!model) return FALSE;
	if (!isAgentAvatarValid()) return FALSE;
	if (!isAgentInventory()) return FALSE;
	const LLUUID &cat_id = inv_cat->getUUID();
	const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
	const LLUUID& marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	const LLUUID from_folder_uuid = inv_cat->getParentUUID();
	const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
	const BOOL move_is_into_marketplacelistings = model->isObjectDescendentOf(mUUID, marketplacelistings_id);
	const BOOL move_is_from_marketplacelistings = model->isObjectDescendentOf(cat_id, marketplacelistings_id);
	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	const BOOL is_agent_inventory = (model->getCategory(cat_id) != NULL)
		&& (LLToolDragAndDrop::SOURCE_AGENT == source);
	BOOL accept = FALSE;
	if (is_agent_inventory)
	{
		const LLUUID &trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH, false);
		const LLUUID &landmarks_id = model->findCategoryUUIDForType(LLFolderType::FT_LANDMARK, false);
		const BOOL move_is_into_trash = (mUUID == trash_id) || model->isObjectDescendentOf(mUUID, trash_id);
		const BOOL move_is_into_outfit = (getCategory() && getCategory()->getPreferredType() == LLFolderType::FT_OUTFIT);
		const BOOL move_is_into_landmarks = (mUUID == landmarks_id) || model->isObjectDescendentOf(mUUID, landmarks_id);
		BOOL is_movable = TRUE;
		std::string tooltip_msg;
		if (is_movable && (marketplacelistings_id == cat_id))
		{
			is_movable = FALSE;
			tooltip_msg = LLTrans::getString("TooltipOutboxCannotMoveRoot");
		}
		if (is_movable && move_is_from_marketplacelistings && LLMarketplaceData::instance().getActivationState(cat_id))
		{
			is_movable = FALSE;
			tooltip_msg = LLTrans::getString("TooltipOutboxDragActive");
		}
		if (is_movable && (mUUID == cat_id))
		{
			is_movable = FALSE;
		}
		if (is_movable && (model->isObjectDescendentOf(mUUID, cat_id)))
		{
			is_movable = FALSE;
		}
		if (is_movable && LLFolderType::lookupIsProtectedType(inv_cat->getPreferredType()))
		{
			is_movable = FALSE;
		}
		if (is_movable && move_is_into_outfit)
		{
			is_movable = FALSE;
		}
		if (is_movable && (mUUID == model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE)))
		{
			is_movable = FALSE;
		}
		if (is_movable && (getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK))
		{
			is_movable = FALSE;
		}
		LLInventoryModel::cat_array_t descendent_categories;
		LLInventoryModel::item_array_t descendent_items;
		if (is_movable)
		{
			model->collectDescendents(cat_id, descendent_categories, descendent_items, FALSE);
			for (U32 i=0; i < descendent_categories.size(); ++i)
			{
				LLInventoryCategory* category = descendent_categories[i];
				if(LLFolderType::lookupIsProtectedType(category->getPreferredType()))
				{
					is_movable = FALSE;
					break;
				}
			}
		}
		U32 max_items_to_wear = gSavedSettings.getU32("WearFolderLimit");
		if (is_movable
			&& move_is_into_current_outfit
			&& descendent_items.size() > max_items_to_wear)
		{
			LLInventoryModel::cat_array_t cats;
			LLInventoryModel::item_array_t items;
			LLFindWearablesEx not_worn( false, false);
			gInventory.collectDescendentsIf(cat_id,
				cats,
				items,
				LLInventoryModel::EXCLUDE_TRASH,
				not_worn);
			if (items.size() > max_items_to_wear)
			{
				is_movable = FALSE;
				LLStringUtil::format_map_t args;
				args["AMOUNT"] = llformat("%d", max_items_to_wear);
			}
		}
		if (is_movable && move_is_into_trash)
		{
			for (U32 i=0; i < descendent_items.size(); ++i)
			{
				LLInventoryItem* item = descendent_items[i];
				if (get_is_item_worn(item->getUUID()))
				{
					is_movable = FALSE;
					break;
				}
			}
		}
		if (is_movable && move_is_into_landmarks)
		{
			for (U32 i=0; i < descendent_items.size(); ++i)
			{
				LLViewerInventoryItem* item = descendent_items[i];
				if (LLAssetType::AT_LANDMARK != item->getType() && LLAssetType::AT_CATEGORY != item->getType())
				{
					is_movable = FALSE;
					break;
				}
			}
		}
		if ( (is_movable) && (rlv_handler_t::isEnabled()) && (RlvFolderLocks::instance().hasLockedFolder(RLV_LOCK_ANY)) )
		{
			is_movable = RlvFolderLocks::instance().canMoveFolder(cat_id, mUUID);
		}
		if (is_movable && move_is_into_marketplacelistings)
		{
			const LLViewerInventoryCategory* master_folder = model->getFirstDescendantOf(marketplacelistings_id, mUUID);
			LLViewerInventoryCategory * dest_folder = getCategory();
			S32 bundle_size = (drop ? 1 : LLToolDragAndDrop::instance().getCargoCount());
			is_movable = can_move_folder_to_marketplace(master_folder, dest_folder, inv_cat, tooltip_msg, bundle_size);
		}
		accept = is_movable;
		if (accept && drop)
		{
			if (user_confirm && (move_is_from_marketplacelistings || move_is_into_marketplacelistings))
			{
				if (move_is_from_marketplacelistings && (LLMarketplaceData::instance().isInActiveFolder(cat_id) ||
													 LLMarketplaceData::instance().isListedAndActive(cat_id)))
				{
					if (LLMarketplaceData::instance().isListed(cat_id) || LLMarketplaceData::instance().isVersionFolder(cat_id))
					{
						LLNotificationsUtil::add("ConfirmMerchantUnlist", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
					}
					else
					{
						LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
					}
					return true;
				}
				if (move_is_from_marketplacelistings && LLMarketplaceData::instance().isVersionFolder(cat_id))
				{
					LLNotificationsUtil::add("ConfirmMerchantClearVersion", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
					return true;
				}
				if (move_is_into_marketplacelistings && LLMarketplaceData::instance().isInActiveFolder(mUUID))
				{
					LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
					return true;
				}
				if (move_is_from_marketplacelistings && LLMarketplaceData::instance().isListed(cat_id))
				{
					LLNotificationsUtil::add("ConfirmListingCutOrDelete", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
					return true;
				}
				if (move_is_into_marketplacelistings && !move_is_from_marketplacelistings)
				{
					LLNotificationsUtil::add("ConfirmMerchantMoveInventory", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
					return true;
				}
			}
			if (move_is_into_trash)
			{
				for (U32 i=0; i < descendent_items.size(); i++)
				{
					LLInventoryItem* item = descendent_items[i];
					if (item->getType() == LLAssetType::AT_GESTURE
						&& LLGestureMgr::instance().isGestureActive(item->getUUID()))
					{
						LLGestureMgr::instance().deactivateGesture(item->getUUID());
					}
				}
			}
			if (move_is_into_current_outfit || move_is_into_outfit)
			{
				if (inv_cat->getPreferredType() == LLFolderType::FT_NONE)
				{
					if (move_is_into_current_outfit)
					{
						BOOL append = true;
						LLAppearanceMgr::instance().wearInventoryCategory(inv_cat, false, append);
					}
					else
					{
						LLInventoryModel::cat_array_t cats;
						LLInventoryModel::item_array_t items;
						model->collectDescendents(cat_id, cats, items, LLInventoryModel::EXCLUDE_TRASH);
						LLInventoryObject::const_object_list_t citems;
						for (LLPointer<LLViewerInventoryItem> item : items)
						{
							citems.push_back(item.get());
						}
						link_inventory_array(mUUID, citems, NULL);
					}
				}
			}
			else if (move_is_into_marketplacelistings)
			{
				move_folder_to_marketplacelistings(inv_cat, mUUID);
			}
			else
			{
				if (model->isObjectDescendentOf(cat_id, model->findCategoryUUIDForType(LLFolderType::FT_INBOX, false)))
				{
					set_dad_inbox_object(cat_id);
				}
				LLInvFVBridge::changeCategoryParent(
					model,
					(LLViewerInventoryCategory*)inv_cat,
					mUUID,
					move_is_into_trash);
			}
			if (move_is_from_marketplacelistings)
			{
				if (from_folder_uuid == marketplacelistings_id)
				{
					if (LLMarketplaceData::instance().isListed(cat_id))
					{
						LLMarketplaceData::instance().clearListing(cat_id);
					}
				}
				else
				{
					LLUUID version_folder_id = LLMarketplaceData::instance().getActiveFolder(from_folder_uuid);
					if (version_folder_id.notNull())
					{
						LLViewerInventoryCategory* cat = gInventory.getCategory(version_folder_id);
						if (!validate_marketplacelistings(cat, NULL))
						{
							LLMarketplaceData::instance().activateListing(version_folder_id,false);
						}
					}
					update_marketplace_category(from_folder_uuid);
				}
			}
		}
	}
	else if (LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		if (move_is_into_marketplacelistings)
		{
			accept = FALSE;
		}
		else
		{
			accept = move_inv_category_world_to_agent(cat_id, mUUID, drop);
		}
	}
	else if (LLToolDragAndDrop::SOURCE_LIBRARY == source)
	{
		if (move_is_into_marketplacelistings)
		{
			accept = FALSE;
		}
		else
		{
			accept = move_is_into_current_outfit && LLAppearanceMgr::instance().getCanMakeFolderIntoOutfit(cat_id);
		}
		if (accept && drop)
		{
			LLAppearanceMgr::instance().wearInventoryCategory(inv_cat, true, false);
		}
	}
	return accept;
}
void warn_move_inventory(LLViewerObject* object, LLMoveInv* move_inv)
{
	const char* dialog = NULL;
	if (object->flagScripted())
	{
		dialog = "MoveInventoryFromScriptedObject";
	}
	else
	{
		dialog = "MoveInventoryFromObject";
	}
	LLNotificationsUtil::add(dialog, LLSD(), LLSD(), boost::bind(move_task_inventory_callback, _1, _2, move_inv));
}
BOOL move_inv_category_world_to_agent(const LLUUID& object_id,
									  const LLUUID& category_id,
									  BOOL drop,
									  void (*callback)(S32, void*),
									  void* user_data)
{
	LLViewerObject* object = gObjectList.findObject(object_id);
	if(!object)
	{
		LL_INFOS() << "Object not found for drop." << LL_ENDL;
		return FALSE;
	}
	LLInventoryObject::object_list_t inventory_objects;
	object->getInventoryContents(inventory_objects);
	if (inventory_objects.empty())
	{
		LL_INFOS() << "Object contents not found for drop." << LL_ENDL;
		return FALSE;
	}
	BOOL accept = FALSE;
	BOOL is_move = FALSE;
	LLInventoryObject::object_list_t::iterator it = inventory_objects.begin();
	LLInventoryObject::object_list_t::iterator end = inventory_objects.end();
	for ( ; it != end; ++it)
	{
		LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(it->get());
		if (!item)
		{
			LL_WARNS() << "Invalid inventory item for drop" << LL_ENDL;
			continue;
		}
		LLPermissions perm(item->getPermissions());
		if((perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID())
			&& perm.allowTransferTo(gAgent.getID())))
		{
			accept = TRUE;
		}
		else if(object->permYouOwner())
		{
			is_move = TRUE;
			accept = TRUE;
		}
		if (!accept)
		{
			break;
		}
	}
	if(drop && accept)
	{
		it = inventory_objects.begin();
		LLMoveInv* move_inv = new LLMoveInv;
		move_inv->mObjectID = object_id;
		move_inv->mCategoryID = category_id;
		move_inv->mCallback = callback;
		move_inv->mUserData = user_data;
		for ( ; it != end; ++it)
		{
			two_uuids_t two(category_id, (*it)->getUUID());
			move_inv->mMoveList.push_back(two);
		}
		if(is_move)
		{
			warn_move_inventory(object, move_inv);
		}
		else
		{
			LLNotification::Params params("MoveInventoryFromObject");
			params.functor(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
			LLNotifications::instance().forceResponse(params, 0);
		}
	}
	return accept;
}
void LLRightClickInventoryFetchDescendentsObserver::execute(bool clear_observer)
{
	if( mComplete.empty() )
	{
		LL_WARNS() << "LLRightClickInventoryFetchDescendentsObserver::done with empty mCompleteFolders" << LL_ENDL;
		if (clear_observer)
		{
		gInventory.removeObserver(this);
		delete this;
		}
		return;
	}
	uuid_vec_t completed_folder = mComplete;
	if (clear_observer)
	{
		gInventory.removeObserver(this);
		delete this;
	}
	for (uuid_vec_t::iterator current_folder = completed_folder.begin(); current_folder != completed_folder.end(); ++current_folder)
	{
		LLInventoryModel::cat_array_t* cat_array;
		LLInventoryModel::item_array_t* item_array;
		gInventory.getDirectDescendentsOf(*current_folder, cat_array, item_array);
		S32 item_count(0);
		if (item_array)
		{
			item_count = item_array->size();
		}
		S32 cat_count(0);
		if (cat_array)
		{
			cat_count = cat_array->size();
		}
		if ((item_count == 0) && (cat_count == 0))
		{
			continue;
		}
		uuid_vec_t ids;
		LLRightClickInventoryFetchObserver* outfit = NULL;
		LLRightClickInventoryFetchDescendentsObserver* categories = NULL;
		if (item_count)
		{
			for (S32 i = 0; i < item_count; ++i)
			{
				ids.push_back(item_array->at(i)->getUUID());
			}
			outfit = new LLRightClickInventoryFetchObserver(ids);
		}
		if (cat_count)
		{
			for (S32 i = 0; i < cat_count; ++i)
			{
				ids.push_back(cat_array->at(i)->getUUID());
			}
			categories = new LLRightClickInventoryFetchDescendentsObserver(ids);
		}
		if (outfit)
		{
	outfit->startFetch();
			outfit->execute();
			delete outfit;
		}
		if (categories)
		{
			categories->startFetch();
			if (categories->isFinished())
			{
				categories->execute();
				delete categories;
			}
			else
			{
				gInventory.addObserver(categories);
			}
		}
	}
}
class LLInventoryCopyAndWearObserver : public LLInventoryObserver
{
public:
	LLInventoryCopyAndWearObserver(const LLUUID& cat_id, int count, bool folder_added=false, bool replace=false) :
		mCatID(cat_id), mContentsCount(count), mFolderAdded(folder_added), mReplace(replace){}
	virtual ~LLInventoryCopyAndWearObserver() {}
	virtual void changed(U32 mask);
protected:
	LLUUID mCatID;
	int    mContentsCount;
	bool   mFolderAdded;
	bool   mReplace;
};
void LLInventoryCopyAndWearObserver::changed(U32 mask)
{
	if((mask & (LLInventoryObserver::ADD)) != 0)
	{
		if (!mFolderAdded)
		{
			const uuid_set_t& changed_items = gInventory.getChangedIDs();
			auto id_it = changed_items.begin();
			auto id_end = changed_items.end();
			for (;id_it != id_end; ++id_it)
			{
				if ((*id_it) == mCatID)
				{
					mFolderAdded = TRUE;
					break;
				}
			}
		}
		if (mFolderAdded)
		{
			LLViewerInventoryCategory* category = gInventory.getCategory(mCatID);
			if (NULL == category)
			{
				LL_WARNS() << "gInventory.getCategory(" << mCatID
						<< ") was NULL" << LL_ENDL;
			}
			else
			{
				if (category->getDescendentCount() ==
				    mContentsCount)
				{
					gInventory.removeObserver(this);
					LLAppearanceMgr::instance().wearInventoryCategory(category, FALSE, !mReplace);
					delete this;
				}
			}
		}
	}
}
void LLFolderBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("open" == action)
	{
		LLFolderViewFolder *f = dynamic_cast<LLFolderViewFolder *>(mRoot->getItemByID(mUUID));
		if (f)
		{
			f->setOpen(TRUE);
		}
		return;
	}
	else if ("open_in_new_window" == action)
	{
		LLInventoryModel* model = getInventoryModel();
		LLViewerInventoryCategory* cat = getCategory();
		if (!model || !cat) return;
		LFFloaterInvPanel::show(LLSD().with("id", mUUID), cat->getName(), model);
		return;
	}
	else if ("copy_folder_uuid" == action)
	{
		gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(mUUID.asString()));
		return;
	}
	else if ("paste" == action)
	{
		pasteFromClipboard();
		return;
	}
	else if ("paste_copies" == action)
	{
		pasteFromClipboard(true);
		return;
	}
	else if ("paste_link" == action)
	{
		pasteLinkFromClipboard();
		return;
	}
	else if ("properties" == action)
	{
		showProperties();
		return;
	}
	else if ("replaceoutfit" == action)
	{
		modifyOutfit(FALSE);
		return;
	}
	else if ("addtooutfit" == action)
	{
		modifyOutfit(TRUE);
		return;
	}
	else if ("cut" == action)
	{
		cutToClipboard();
		return;
	}
	else if ("copy" == action)
	{
		copyToClipboard();
		return;
	}
	else if ("wearitems" == action)
	{
		modifyOutfit(TRUE);
	}
	else if ("removefromoutfit" == action)
	{
		LLInventoryModel* model = getInventoryModel();
		if(!model) return;
		LLViewerInventoryCategory* cat = getCategory();
		if(!cat) return;
		LLAppearanceMgr::instance().takeOffOutfit( cat->getLinkedUUID() );
		return;
	}
	else if ("purge" == action)
	{
		purgeItem(model, mUUID);
		return;
	}
	else if ("restore" == action)
	{
		restoreItem();
		return;
	}
	else if ("marketplace_list" == action)
	{
		if (depth_nesting_in_marketplace(mUUID) == 1)
		{
			LLUUID version_folder_id = LLMarketplaceData::instance().getVersionFolder(mUUID);
			LLViewerInventoryCategory* cat = gInventory.getCategory(version_folder_id);
			mMessage = "";
			if (!validate_marketplacelistings(cat, boost::bind(&LLFolderBridge::gatherMessage, this, _1, _2, _3)))
			{
				LLSD subs;
				subs["[ERROR_CODE]"] = mMessage;
				LLNotificationsUtil::add("MerchantListingFailed", subs);
			}
			else
			{
				LLMarketplaceData::instance().activateListing(mUUID,true);
			}
		}
		return;
	}
	else if ("marketplace_activate" == action)
	{
		if (depth_nesting_in_marketplace(mUUID) == 2)
		{
			LLInventoryCategory* category = gInventory.getCategory(mUUID);
            mMessage = "";
            if (!validate_marketplacelistings(category,boost::bind(&LLFolderBridge::gatherMessage, this, _1, _2, _3),false,2))
            {
                LLSD subs;
                subs["[ERROR_CODE]"] = mMessage;
                LLNotificationsUtil::add("MerchantFolderActivationFailed", subs);
            }
            else
            {
				LLMarketplaceData::instance().setVersionFolder(category->getParentUUID(), mUUID);
			}
		}
		return;
	}
	else if ("marketplace_unlist" == action)
	{
		if (depth_nesting_in_marketplace(mUUID) == 1)
		{
			LLMarketplaceData::instance().activateListing(mUUID, false,1);
		}
		return;
	}
	else if ("marketplace_deactivate" == action)
	{
		if (depth_nesting_in_marketplace(mUUID) == 2)
		{
			LLInventoryCategory* category = gInventory.getCategory(mUUID);
			LLMarketplaceData::instance().setVersionFolder(category->getParentUUID(), LLUUID::null, 1);
		}
		return;
	}
	else if ("marketplace_create_listing" == action)
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(mUUID);
		mMessage = "";
		bool validates = validate_marketplacelistings(cat,boost::bind(&LLFolderBridge::gatherMessage, this, _1, _2, _3),false);
		if (!validates)
		{
			mMessage = "";
			validates = validate_marketplacelistings(cat,boost::bind(&LLFolderBridge::gatherMessage, this, _1, _2, _3),true);
			if (validates)
			{
				LLNotificationsUtil::add("MerchantForceValidateListing");
			}
		}
		if (!validates)
		{
			LLSD subs;
			subs["[ERROR_CODE]"] = mMessage;
			LLNotificationsUtil::add("MerchantListingFailed", subs);
		}
		else
		{
			LLMarketplaceData::instance().createListing(mUUID);
		}
		return;
	}
	else if ("marketplace_disassociate_listing" == action)
	{
		LLMarketplaceData::instance().clearListing(mUUID);
		return;
	}
	else if ("marketplace_get_listing" == action)
	{
		LLMarketplaceData::instance().getListing(mUUID);
		return;
	}
	else if ("marketplace_associate_listing" == action)
	{
		LLFloaterAssociateListing::showInstance(mUUID);
		return;
	}
	else if ("marketplace_check_listing" == action)
	{
		LLSD data(mUUID);
		new LLFloaterMarketplaceValidation(data);
		return;
	}
	else if ("marketplace_edit_listing" == action)
	{
		std::string url = LLMarketplaceData::instance().getListingURL(mUUID);
		if (!url.empty())
		{
			LLUrlAction::openURL(url);
		}
		return;
	}
	else if ("marketplace_copy_id" == action)
	{
		auto id = LLMarketplaceData::instance().getListingID(mUUID);
		gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(fmt::to_string(id)));
	}
	else if ("move_to_lost_and_found" == action)
	{
		gInventory.changeCategoryParent(getCategory(), gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND), TRUE);
		gInventory.addChangedMask(LLInventoryObserver::REBUILD, mUUID);
		gInventory.notifyObservers();
	}
#ifdef DELETE_SYSTEM_FOLDERS
	else if ("delete_system_folder" == action)
	{
		removeSystemFolder();
	}
#endif
	else if (("move_to_marketplace_listings" == action) || ("copy_to_marketplace_listings" == action) || ("copy_or_move_to_marketplace_listings" == action))
	{
		LLInventoryCategory * cat = gInventory.getCategory(mUUID);
		if (!cat) return;
		const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
        move_folder_to_marketplacelistings(cat, marketplacelistings_id, ("move_to_marketplace_listings" != action), (("copy_or_move_to_marketplace_listings" == action)));
	}
}
void LLFolderBridge::gatherMessage(std::string& message, S32 depth, LLError::ELevel log_level)
{
	if (log_level >= LLError::LEVEL_ERROR)
	{
		if (!mMessage.empty())
		{
			return;
		}
		std::string::size_type start = message.find_first_not_of(" ");
		mMessage += message.substr(start, message.length() - start);
	}
}
void LLFolderBridge::openItem()
{
	LL_DEBUGS() << "LLFolderBridge::openItem()" << LL_ENDL;
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	if(mUUID.isNull()) return;
	bool fetching_inventory = model->fetchDescendentsOf(mUUID);
	if (!fetching_inventory)
	{
	}
}
void LLFolderBridge::closeItem()
{
	determineFolderType();
}
void LLFolderBridge::determineFolderType()
{
	if (isUpToDate())
	{
		LLInventoryModel* model = getInventoryModel();
		LLViewerInventoryCategory* category = model->getCategory(mUUID);
		if (category)
		{
			category->determineFolderType();
		}
	}
}
BOOL LLFolderBridge::isItemRenameable() const
{
	return get_is_category_renameable(getInventoryModel(), mUUID);
}
void LLFolderBridge::restoreItem()
{
	LLViewerInventoryCategory* cat;
	cat = (LLViewerInventoryCategory*)getCategory();
	if(cat)
	{
		LLInventoryModel* model = getInventoryModel();
		const LLUUID new_parent = model->findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(cat->getType()));
		LLInvFVBridge::changeCategoryParent(model, cat, new_parent, FALSE);
	}
}
LLFolderType::EType LLFolderBridge::getPreferredType() const
{
	LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
	LLViewerInventoryCategory* cat = getCategory();
	if(cat)
	{
		preferred_type = cat->getPreferredType();
	}
	return preferred_type;
}
LLUIImagePtr LLFolderBridge::getIcon() const
{
	return getFolderIcon(FALSE);
}
LLUIImagePtr LLFolderBridge::getIconOpen() const
{
	return getFolderIcon(TRUE);
}
LLUIImagePtr LLFolderBridge::getFolderIcon(BOOL is_open) const
{
	LLFolderType::EType preferred_type = getPreferredType();
	S32 depth = depth_nesting_in_marketplace(mUUID);
	if ((preferred_type == LLFolderType::FT_NONE) && (depth == 2))
	{
		preferred_type = LLFolderType::FT_MARKETPLACE_VERSION;
	}
	return LLUI::getUIImage(LLViewerFolderType::lookupIconName(preferred_type, is_open));
}
LLUIImagePtr LLFolderBridge::getIcon(LLFolderType::EType preferred_type)
{
	return LLUI::getUIImage(LLViewerFolderType::lookupIconName(preferred_type, FALSE));
}
LLUIImagePtr LLFolderBridge::getIconOverlay() const
{
	if (getInventoryObject() && getInventoryObject()->getIsLinkType())
	{
		return LLUI::getUIImage("inv_link_overlay.tga");
	}
	return NULL;
}
BOOL LLFolderBridge::renameItem(const std::string& new_name)
{
	LLScrollOnRenameObserver *observer = new LLScrollOnRenameObserver(mUUID, mRoot);
	gInventory.addObserver(observer);
	rename_category(getInventoryModel(), mUUID, new_name);
	return FALSE;
}
BOOL LLFolderBridge::removeItem()
{
	if(!isItemRemovable())
	{
		return FALSE;
	}
	const LLViewerInventoryCategory *cat = getCategory();
	LLSD payload;
	LLSD args;
	args["FOLDERNAME"] = cat->getName();
	LLNotification::Params params("ConfirmDeleteProtectedCategory");
	params.payload(payload).substitutions(args).functor(boost::bind(&LLFolderBridge::removeItemResponse, this, _1, _2));
	LLNotifications::instance().forceResponse(params, 0);
	return TRUE;
}
BOOL LLFolderBridge::removeSystemFolder()
{
	const LLViewerInventoryCategory *cat = getCategory();
	if (!LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
	{
		return FALSE;
	}
	LLSD payload;
	LLSD args;
	args["FOLDERNAME"] = cat->getName();
	LLNotification::Params params("ConfirmDeleteProtectedCategory");
	params.payload(payload).substitutions(args).functor(boost::bind(&LLFolderBridge::removeItemResponse, this, _1, _2));
	{
		LLNotifications::instance().add(params);
	}
	return TRUE;
}
bool LLFolderBridge::removeItemResponse(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if(option == 0)
	{
		LLPreview::hide(mUUID);
		getInventoryModel()->removeCategory(mUUID);
		return TRUE;
	}
	return FALSE;
}
void LLFolderBridge::pasteFromClipboard(bool only_copies)
{
	LLInventoryModel* model = getInventoryModel();
	if (model && isClipboardPasteable())
	{
		const LLUUID& marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
		const BOOL paste_into_marketplacelistings = model->isObjectDescendentOf(mUUID, marketplacelistings_id);
        BOOL cut_from_marketplacelistings = FALSE;
		const auto& clipboard(LLInventoryClipboard::instance());
        if (clipboard.isCutMode())
        {
            uuid_vec_t objects;
            clipboard.retrieve(objects);
            for (auto iter = objects.begin(); iter != objects.end(); ++iter)
            {
                const LLUUID& item_id = (*iter);
                if(gInventory.isObjectDescendentOf(item_id, marketplacelistings_id) && (LLMarketplaceData::instance().isInActiveFolder(item_id) ||
                    LLMarketplaceData::instance().isListedAndActive(item_id)))
                {
                    cut_from_marketplacelistings = TRUE;
                    break;
                }
            }
        }
        if (cut_from_marketplacelistings || (paste_into_marketplacelistings && !LLMarketplaceData::instance().isListed(mUUID) && LLMarketplaceData::instance().isInActiveFolder(mUUID)))
		{
			LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_pasteFromClipboard, this, _1, _2, only_copies));
		}
		else
		{
			perform_pasteFromClipboard(only_copies);
		}
	}
}
void LLFolderBridge::callback_pasteFromClipboard(const LLSD& notification, const LLSD& response, bool only_copies)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		perform_pasteFromClipboard(only_copies);
	}
}
void LLFolderBridge::perform_pasteFromClipboard(bool only_copies)
{
	LLInventoryModel* model = getInventoryModel();
	if (model && isClipboardPasteable())
	{
		const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
		const LLUUID& marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
		const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS, false);
		const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
		const BOOL move_is_into_my_outfits = (mUUID == my_outifts_id) || model->isObjectDescendentOf(mUUID, my_outifts_id);
		const BOOL move_is_into_outfit = move_is_into_my_outfits || (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
		const BOOL move_is_into_marketplacelistings = model->isObjectDescendentOf(mUUID, marketplacelistings_id);
		uuid_vec_t objects;
		LLInventoryClipboard::instance().retrieve(objects);
		LLViewerInventoryCategory* dest_folder = getCategory();
		if (move_is_into_marketplacelistings)
		{
			std::string error_msg;
			const LLViewerInventoryCategory* master_folder = model->getFirstDescendantOf(marketplacelistings_id, mUUID);
			int index = 0;
			for (auto iter = objects.begin(); iter != objects.end(); ++iter)
			{
				const LLUUID& item_id = (*iter);
				LLInventoryItem *item = model->getItem(item_id);
				LLInventoryCategory *cat = model->getCategory(item_id);
				if (item && !can_move_item_to_marketplace(master_folder, dest_folder, item, error_msg, objects.size() - index, true))
				{
					break;
				}
				if (cat && !can_move_folder_to_marketplace(master_folder, dest_folder, cat, error_msg, objects.size() - index, true, true))
				{
					break;
				}
				++index;
			}
			if (!error_msg.empty())
			{
				LLSD subs;
				subs["[ERROR_CODE]"] = error_msg;
				LLNotificationsUtil::add("MerchantPasteFailed", subs);
				return;
			}
		}
		else
		{
			for (auto iter = objects.begin(); iter != objects.end(); ++iter)
			{
				const LLUUID& item_id = (*iter);
				LLInventoryItem *item = model->getItem(item_id);
				LLInventoryCategory *cat = model->getCategory(item_id);
				if ((item && !dest_folder->acceptItem(item)) || (cat && (dest_folder->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)))
				{
					std::string error_msg = LLTrans::getString("TooltipOutboxMixedStock");
					LLSD subs;
					subs["[ERROR_CODE]"] = error_msg;
					LLNotificationsUtil::add("StockPasteFailed", subs);
					return;
				}
			}
		}
		const LLUUID parent_id(mUUID);
		for (auto iter = objects.begin();
			 iter != objects.end();
			 ++iter)
		{
			const LLUUID& item_id = (*iter);
			LLInventoryItem *item = model->getItem(item_id);
			LLInventoryObject *obj = model->getObject(item_id);
			if (obj)
			{
				if (move_is_into_current_outfit || move_is_into_outfit)
				{
					if (item && can_move_to_outfit(item, move_is_into_current_outfit))
					{
						dropToOutfit(item, move_is_into_current_outfit);
					}
				}
				else if (!only_copies && LLInventoryClipboard::instance().isCutMode())
				{
					if (LLAssetType::AT_CATEGORY == obj->getType())
					{
						LLViewerInventoryCategory* vicat = (LLViewerInventoryCategory *)model->getCategory(item_id);
						llassert(vicat);
						if (vicat)
						{
							if (LLMarketplaceData::instance().isListed(item_id))
							{
								LLMarketplaceData::instance().clearListing(item_id);
							}
							if (move_is_into_marketplacelistings)
							{
								move_folder_to_marketplacelistings(vicat, parent_id);
							}
							else
							{
								changeCategoryParent(model, vicat, parent_id, FALSE);
							}
						}
					}
					else
					{
						LLViewerInventoryItem* viitem = dynamic_cast<LLViewerInventoryItem*>(item);
						llassert(viitem);
						if (viitem)
						{
							if (move_is_into_marketplacelistings)
							{
								if (!move_item_to_marketplacelistings(viitem, parent_id))
								{
									break;
								}
							}
							else
							{
								changeItemParent(model, viitem, parent_id, FALSE);
							}
						}
					}
				}
				else
				{
					if (only_copies)
					{
						item = model->getLinkedItem(item_id);
						obj = model->getObject(item->getUUID());
					}
					if (LLAssetType::AT_CATEGORY == obj->getType())
					{
						LLViewerInventoryCategory* vicat = (LLViewerInventoryCategory *)model->getCategory(item_id);
						llassert(vicat);
						if (vicat)
						{
							if (move_is_into_marketplacelistings)
							{
								move_folder_to_marketplacelistings(vicat, parent_id, true);
							}
							else
							{
								copy_inventory_category(model, vicat, parent_id);
							}
						}
					}
					else if (!move_is_into_marketplacelistings && !only_copies && LLAssetType::lookupIsLinkType(item->getActualType()))
					{
						link_inventory_object(
							parent_id,
							item,
							NULL);
					}
					else
					{
						LLViewerInventoryItem* viitem = dynamic_cast<LLViewerInventoryItem*>(item);
						llassert(viitem);
						if (viitem)
						{
							if (move_is_into_marketplacelistings)
							{
								if (!move_item_to_marketplacelistings(viitem, parent_id, true))
								{
									break;
								}
							}
							else
							{
								copy_inventory_item(
									gAgent.getID(),
									item->getPermissions().getOwner(),
									item->getUUID(),
									parent_id,
									std::string(),
									LLPointer<LLInventoryCallback>(NULL));
							}
						}
					}
				}
			}
		}
		LLInventoryClipboard::instance().setCutMode(false);
	}
}
void LLFolderBridge::pasteLinkFromClipboard()
{
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
		const LLUUID& marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
		const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS, false);
		const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
		const BOOL move_is_into_my_outfits = (mUUID == my_outifts_id) || model->isObjectDescendentOf(mUUID, my_outifts_id);
		const BOOL move_is_into_outfit = move_is_into_my_outfits || (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
		const BOOL move_is_into_marketplacelistings = model->isObjectDescendentOf(mUUID, marketplacelistings_id);
		if (move_is_into_marketplacelistings)
		{
			return;
		}
		const LLUUID parent_id(mUUID);
		uuid_vec_t objects;
		LLInventoryClipboard::instance().retrieve(objects);
		for (auto iter = objects.begin();
			 iter != objects.end();
			 ++iter)
		{
			const LLUUID &object_id = (*iter);
			if (move_is_into_current_outfit || move_is_into_outfit)
			{
				LLInventoryItem *item = model->getItem(object_id);
				if (item && can_move_to_outfit(item, move_is_into_current_outfit))
				{
					dropToOutfit(item, move_is_into_current_outfit);
				}
			}
			else if (LLConstPointer<LLInventoryObject> obj = model->getObject(object_id))
			{
				link_inventory_object(parent_id, obj, LLPointer<LLInventoryCallback>(NULL));
			}
		}
	}
}
void LLFolderBridge::staticFolderOptionsMenu()
{
	LLFolderBridge* selfp = sSelf.get();
	if (selfp && selfp->mRoot)
	{
		selfp->mRoot->updateMenu();
	}
}
BOOL checkFolderForContentsOfType(LLInventoryModel* model, LLInventoryCollectFunctor& is_type, const LLUUID& mUUID)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	model->collectDescendentsIf(mUUID,
								cat_array,
								item_array,
								LLInventoryModel::EXCLUDE_TRASH,
								is_type,
								true);
	return ((item_array.size() > 0) ? TRUE : FALSE );
}
BOOL LLFolderBridge::checkFolderForContentsOfType(LLInventoryModel* model, LLInventoryCollectFunctor& is_type)
{
	return ::checkFolderForContentsOfType(model, is_type, mUUID);
}
void LLFolderBridge::buildContextMenuOptions(U32 flags, menuentry_vec_t&   items, menuentry_vec_t& disabled_items)
{
	LLInventoryModel* model = getInventoryModel();
	llassert(model != NULL);
	const LLUUID& trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	const LLUUID& lost_and_found_id = model->findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
	const LLUUID& marketplace_listings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	if (lost_and_found_id == mUUID)
	{
		items.push_back(std::string("Empty Lost And Found"));
		disabled_items.push_back(std::string("New Folder"));
		disabled_items.push_back(std::string("New Script"));
		disabled_items.push_back(std::string("New Note"));
		disabled_items.push_back(std::string("New Gesture"));
		disabled_items.push_back(std::string("New Clothes"));
		disabled_items.push_back(std::string("New Body Parts"));
	}
	if (isMarketplaceListingsFolder())
	{
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		if (LLMarketplaceData::instance().isUpdating(mUUID))
		{
			disabled_items.push_back(std::string("New Folder"));
			disabled_items.push_back(std::string("Rename"));
			disabled_items.push_back(std::string("Cut"));
			disabled_items.push_back(std::string("Copy"));
			disabled_items.push_back(std::string("Paste"));
			disabled_items.push_back(std::string("Delete"));
		}
	}
	if (getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
	{
		disabled_items.push_back(std::string("New Folder"));
		disabled_items.push_back(std::string("New Script"));
		disabled_items.push_back(std::string("New Note"));
		disabled_items.push_back(std::string("New Gesture"));
		disabled_items.push_back(std::string("New Clothes"));
		disabled_items.push_back(std::string("New Body Parts"));
	}
	if (marketplace_listings_id == mUUID)
	{
		disabled_items.push_back(std::string("New Folder"));
		disabled_items.push_back(std::string("Rename"));
		disabled_items.push_back(std::string("Cut"));
		disabled_items.push_back(std::string("Delete"));
	}
	if(trash_id == mUUID)
	{
		items.push_back(std::string("Empty Trash"));
	}
	else if(isItemInTrash())
	{
		items.clear();
		addTrashContextMenuOptions(items, disabled_items);
	}
	else if(isAgentInventory())
	{
		LLViewerInventoryCategory *cat = getCategory();
		const bool is_cof(isCOFFolder());
		if (!is_cof && cat && (cat->getPreferredType() != LLFolderType::FT_OUTFIT))
		{
			if (!isInboxFolder())
			{
				{
					items.push_back(std::string("New Folder"));
				}
				if (!isMarketplaceListingsFolder())
				{
					items.push_back(std::string("New Folder"));
					items.push_back(std::string("New Script"));
					items.push_back(std::string("New Note"));
					items.push_back(std::string("New Gesture"));
					items.push_back(std::string("New Clothes"));
					items.push_back(std::string("New Body Parts"));
				}
			}
			getClipboardEntries(false, items, disabled_items, flags);
		}
		else
		{
			if (cat && (cat->getPreferredType() == LLFolderType::FT_OUTFIT))
			{
				items.push_back(std::string("Rename"));
				addDeleteContextMenuOptions(items, disabled_items);
				const LLViewerInventoryItem *base_outfit_link = LLAppearanceMgr::instance().getBaseOutfitLink();
				if (base_outfit_link && (cat == base_outfit_link->getLinkedCategory()))
				{
					disabled_items.push_back(std::string("Delete"));
				}
			}
		}
		if (!is_cof) getClipboardEntries(false, items, disabled_items, flags);
		mCallingCards = mWearables = FALSE;
		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if (checkFolderForContentsOfType(model, is_callingcard))
		{
			mCallingCards=TRUE;
		}
		LLFindWearables is_wearable;
		LLIsType is_object( LLAssetType::AT_OBJECT );
		LLIsType is_gesture( LLAssetType::AT_GESTURE );
		if (checkFolderForContentsOfType(model, is_wearable)  ||
			checkFolderForContentsOfType(model, is_object) ||
			checkFolderForContentsOfType(model, is_gesture) )
		{
			mWearables=TRUE;
		}
	}
	else if (!isAgentInventory())
	{
		const LLUUID& library(gInventory.getLibraryRootFolderID());
		if (library != mUUID && !gInventory.isObjectDescendentOf(mUUID, library))
			 items.push_back(std::string("Move to Lost And Found"));
	}
	if ((flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Delete System Folder"));
	}
	if (!isMarketplaceListingsFolder() && !isItemInTrash())
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
	}
	LLViewerInventoryCategory* category = (LLViewerInventoryCategory *) model->getCategory(mUUID);
	if (category && (marketplace_listings_id != mUUID))
	{
		uuid_vec_t folders;
		folders.push_back(category->getUUID());
		sSelf = getHandle();
		LLRightClickInventoryFetchDescendentsObserver* fetch = new LLRightClickInventoryFetchDescendentsObserver(folders);
		fetch->startFetch();
		if (fetch->isFinished())
		{
			delete fetch;
			buildContextMenuFolderOptions(flags, items, disabled_items);
		}
		else
		{
			gInventory.addObserver(fetch);
	}
}
}
void LLFolderBridge::buildContextMenuFolderOptions(U32 flags,   menuentry_vec_t& items, menuentry_vec_t& disabled_items)
{
	LLInventoryModel* model = getInventoryModel();
	if (!isAgentInventory()) return;
	build_context_menu_folder_options(model, mUUID, items, disabled_items);
	if (!isItemRemovable())
	{
		disabled_items.push_back(std::string("Delete"));
	}
#ifdef DELETE_SYSTEM_FOLDERS
	if (LLFolderType::lookupIsProtectedType(type))
	{
		mItems.push_back(std::string("Delete System Folder"));
	}
#endif
}
void build_context_menu_folder_options(LLInventoryModel* model, const LLUUID& mUUID, menuentry_vec_t& items, menuentry_vec_t& disabled_items)
{
	if(!model) return;
	const LLInventoryCategory* category = model->getCategory(mUUID);
	if(!category) return;
	items.push_back(std::string("Open Folder In New Window"));
	items.push_back(std::string("Copy Folder UUID"));
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if (trash_id == mUUID) return;
	if (model->isObjectDescendentOf(mUUID, trash_id)) return;
	if (!get_is_item_removable(model, mUUID))
	{
		disabled_items.push_back(std::string("Delete"));
	}
	const LLUUID listings_folder = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	if (listings_folder.notNull() && gInventory.isObjectDescendentOf(mUUID, listings_folder)) return;
	LLFolderType::EType type = category->getPreferredType();
	const bool is_system_folder = LLFolderType::lookupIsProtectedType(type);
	const bool is_outfit = (type == LLFolderType::FT_OUTFIT);
	if (!is_system_folder)
	{
		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if ( checkFolderForContentsOfType(model, is_callingcard, mUUID))
		{
			items.push_back(std::string("Calling Card Separator"));
			items.push_back(std::string("Conference Chat Folder"));
			items.push_back(std::string("IM All Contacts In Folder"));
		}
	}
	LLFindWearables is_wearable;
	LLIsType is_object( LLAssetType::AT_OBJECT );
	LLIsType is_gesture( LLAssetType::AT_GESTURE );
	if (
		checkFolderForContentsOfType(model, is_wearable, mUUID)  ||
		checkFolderForContentsOfType(model, is_object, mUUID) ||
		checkFolderForContentsOfType(model, is_gesture, mUUID) )
	{
		items.push_back(std::string("Folder Wearables Separator"));
		if (!is_system_folder)
		{
			{
				items.push_back(std::string("Add To Outfit"));
			}
			items.push_back(std::string("Replace Outfit"));
		}
		items.push_back(std::string("Replace Remove Separator"));
		items.push_back(std::string("Remove From Outfit"));
		if (!LLAppearanceMgr::getCanRemoveFromCOF(mUUID))
		{
			disabled_items.push_back(std::string("Remove From Outfit"));
		}
		if ( ((is_outfit) && (!LLAppearanceMgr::instance().getCanReplaceCOF(mUUID))) ||
			 ((!is_outfit) && (gAgentWearables.isCOFChangeInProgress())) )
		{
			disabled_items.push_back(std::string("Replace Outfit"));
		}
		if (!LLAppearanceMgr::instance().getCanAddToCOF(mUUID))
		{
			disabled_items.push_back(std::string("Add To Outfit"));
		}
		items.push_back(std::string("Outfit Separator"));
	}
}
void LLFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	sSelf.markDead();
	gInventory.fetchDescendentsOf(getUUID());
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	LL_DEBUGS() << "LLFolderBridge::buildContextMenu()" << LL_ENDL;
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	buildContextMenuOptions(flags, items, disabled_items);
    hide_context_entries(menu, items, disabled_items);
	menu.needsArrange();
	menu.arrangeAndClear();
}
bool LLFolderBridge::hasChildren() const
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	LLInventoryModel::EHasChildren has_children;
	has_children = gInventory.categoryHasChildren(mUUID);
	return has_children != LLInventoryModel::CHILDREN_NO;
}
BOOL LLFolderBridge::dragOrDrop(MASK mask, BOOL drop,
								EDragAndDropType cargo_type,
								void* cargo_data)
{
	LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;
	BOOL accept = FALSE;
	switch(cargo_type)
	{
		case DAD_TEXTURE:
		case DAD_SOUND:
		case DAD_CALLINGCARD:
		case DAD_LANDMARK:
		case DAD_SCRIPT:
		case DAD_CLOTHING:
		case DAD_OBJECT:
		case DAD_NOTECARD:
		case DAD_BODYPART:
		case DAD_ANIMATION:
		case DAD_GESTURE:
		case DAD_MESH:
			accept = dragItemIntoFolder(inv_item, drop);
			break;
		case DAD_LINK:
			{
				accept = dragItemIntoFolder(inv_item, drop);
			}
			break;
		case DAD_CATEGORY:
			accept = dragCategoryIntoFolder((LLInventoryCategory*)cargo_data, drop);
		case DAD_ROOT_CATEGORY:
		case DAD_NONE:
			break;
		default:
			LL_WARNS() << "Unhandled cargo type for drag&drop " << cargo_type << LL_ENDL;
			break;
	}
	return accept;
}
LLViewerInventoryCategory* LLFolderBridge::getCategory() const
{
	LLViewerInventoryCategory* cat = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		cat = (LLViewerInventoryCategory*)model->getCategory(mUUID);
	}
	return cat;
}
void LLFolderBridge::pasteClipboard(void* user_data)
{
	LLFolderBridge* self = (LLFolderBridge*)user_data;
	if(self) self->pasteFromClipboard();
}
void LLFolderBridge::createNewCategory(void* user_data)
{
	LLFolderBridge* bridge = (LLFolderBridge*)user_data;
	if(!bridge) return;
	LLInventoryPanel* panel = bridge->mInventoryPanel.get();
	if (!panel) return;
	LLInventoryModel* model = panel->getModel();
	if(!model) return;
	LLUUID id;
	id = model->createNewCategory(bridge->getUUID(),
								  LLFolderType::FT_NONE,
								  LLStringUtil::null);
	model->notifyObservers();
	panel->setSelection(id, TAKE_FOCUS_YES);
}
void LLFolderBridge::createNewShirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SHIRT);
}
void LLFolderBridge::createNewPants(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_PANTS);
}
void LLFolderBridge::createNewShoes(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SHOES);
}
void LLFolderBridge::createNewSocks(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SOCKS);
}
void LLFolderBridge::createNewJacket(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_JACKET);
}
void LLFolderBridge::createNewSkirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SKIRT);
}
void LLFolderBridge::createNewGloves(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_GLOVES);
}
void LLFolderBridge::createNewUndershirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_UNDERSHIRT);
}
void LLFolderBridge::createNewUnderpants(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_UNDERPANTS);
}
void LLFolderBridge::createNewAlpha(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_ALPHA);
}
void LLFolderBridge::createNewTattoo(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_TATTOO);
}
void LLFolderBridge::createNewShape(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SHAPE);
}
void LLFolderBridge::createNewSkin(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SKIN);
}
void LLFolderBridge::createNewHair(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_HAIR);
}
void LLFolderBridge::createNewEyes(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_EYES);
}
void LLFolderBridge::createNewPhysics(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_PHYSICS);
}
void LLFolderBridge::createWearable(LLFolderBridge* bridge, LLWearableType::EType type)
{
	if(!bridge) return;
	LLUUID parent_id = bridge->getUUID();
	LLAgentWearables::createWearable(type, false, parent_id);
}
void LLFolderBridge::modifyOutfit(BOOL append)
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	LLViewerInventoryCategory* cat = getCategory();
	if(!cat) return;
	LLAppearanceMgr::instance().wearInventoryCategory( cat, FALSE, append );
}
LLMarketplaceFolderBridge::LLMarketplaceFolderBridge(LLInventoryPanel* inventory,
						  LLFolderView* root,
						  const LLUUID& uuid) :
LLFolderBridge(inventory, root, uuid)
{
	m_depth = depth_nesting_in_marketplace(mUUID);
	m_stockCountCache = COMPUTE_STOCK_NOT_EVALUATED;
}
LLUIImagePtr LLMarketplaceFolderBridge::getIcon() const
{
	return getMarketplaceFolderIcon(FALSE);
}
LLUIImagePtr LLMarketplaceFolderBridge::getIconOpen() const
{
	return getMarketplaceFolderIcon(TRUE);
}
LLUIImagePtr LLMarketplaceFolderBridge::getMarketplaceFolderIcon(BOOL is_open) const
{
	LLFolderType::EType preferred_type = getPreferredType();
	if (!LLMarketplaceData::instance().isUpdating(getUUID()))
	{
		m_depth = depth_nesting_in_marketplace(mUUID);
	}
	if ((preferred_type == LLFolderType::FT_NONE) && (m_depth == 2))
	{
		preferred_type = LLFolderType::FT_MARKETPLACE_VERSION;
	}
	return LLUI::getUIImage(LLViewerFolderType::lookupIconName(preferred_type, is_open));
}
std::string LLMarketplaceFolderBridge::getLabelSuffix() const
{
	std::string suffix = "";
	if (LLMarketplaceData::instance().isListed(getUUID()))
	{
		suffix = llformat("%d",LLMarketplaceData::instance().getListingID(getUUID()));
		if (suffix.empty())
		{
			suffix = LLTrans::getString("MarketplaceNoID");
		}
		suffix = " (" +  suffix + ")";
		if (LLMarketplaceData::instance().getActivationState(getUUID()))
		{
			suffix += " (" +  LLTrans::getString("MarketplaceLive") + ")";
		}
	}
	else if (LLMarketplaceData::instance().isVersionFolder(getUUID()))
	{
		suffix += " (" +  LLTrans::getString("MarketplaceActive") + ")";
	}
	bool updating = LLMarketplaceData::instance().isUpdating(getUUID());
	if (!updating)
	{
		m_stockCountCache = compute_stock_count(getUUID());
	}
	if (m_stockCountCache == 0)
	{
		suffix += " (" +  LLTrans::getString("MarketplaceNoStock") + ")";
	}
	else if (m_stockCountCache != COMPUTE_STOCK_INFINITE)
	{
		if (getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
		{
			suffix += " (" +  LLTrans::getString("MarketplaceStock");
		}
		else
		{
			suffix += " (" +  LLTrans::getString("MarketplaceMax");
		}
		if (m_stockCountCache == COMPUTE_STOCK_NOT_EVALUATED)
		{
			suffix += "=" + LLTrans::getString("MarketplaceUpdating") + ")";
		}
		else
		{
			suffix +=  "=" + llformat("%d", m_stockCountCache) + ")";
		}
	}
	if (updating)
	{
		suffix += " (" +  LLTrans::getString("MarketplaceUpdating") + ")";
	}
	return LLInvFVBridge::getLabelSuffix() + suffix;
}
LLFontGL::StyleFlags LLMarketplaceFolderBridge::getLabelStyle() const
{
	return (LLMarketplaceData::instance().getActivationState(getUUID()) ? LLFontGL::BOLD :  LLFontGL::NORMAL);
}
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, LLMoveInv* move_inv)
{
	LLFloaterOpenObject::LLCatAndWear* cat_and_wear = (LLFloaterOpenObject::LLCatAndWear* )move_inv->mUserData;
	LLViewerObject* object = gObjectList.findObject(move_inv->mObjectID);
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(option == 0 && object)
	{
		if (cat_and_wear && cat_and_wear->mWear)
		{
			LLInventoryObject::object_list_t inventory_objects;
			object->getInventoryContents(inventory_objects);
			int contents_count = inventory_objects.size()-1;
			LLInventoryCopyAndWearObserver* inventoryObserver = new LLInventoryCopyAndWearObserver(cat_and_wear->mCatID, contents_count, cat_and_wear->mFolderResponded,
																									cat_and_wear->mReplace);
			gInventory.addObserver(inventoryObserver);
		}
		two_uuids_list_t::iterator move_it;
		for (move_it = move_inv->mMoveList.begin();
			 move_it != move_inv->mMoveList.end();
			 ++move_it)
		{
			object->moveInventory(move_it->first, move_it->second);
		}
		dialog_refresh_all();
	}
	if (move_inv->mCallback)
	{
		move_inv->mCallback(option, move_inv->mUserData);
	}
	delete move_inv;
	return false;
}
static BOOL can_move_to_outfit(LLInventoryItem* inv_item, BOOL move_is_into_current_outfit)
{
	if ((inv_item->getInventoryType() != LLInventoryType::IT_WEARABLE) &&
		(inv_item->getInventoryType() != LLInventoryType::IT_GESTURE) &&
		(inv_item->getInventoryType() != LLInventoryType::IT_ATTACHMENT) &&
		(inv_item->getInventoryType() != LLInventoryType::IT_OBJECT))
	{
		return FALSE;
	}
	U32 flags = inv_item->getFlags();
	if(flags & LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS)
	{
		return FALSE;
	}
	if (move_is_into_current_outfit && get_is_item_worn(inv_item->getUUID()))
	{
		return FALSE;
	}
	return TRUE;
}
static BOOL can_move_to_landmarks(LLInventoryItem* inv_item)
{
	if (LLAssetType::AT_LINK == inv_item->getType())
	{
		LLInventoryItem* linked_item = gInventory.getItem(inv_item->getLinkedUUID());
		if (linked_item)
		{
			return LLAssetType::AT_LANDMARK == linked_item->getType();
		}
	}
	return LLAssetType::AT_LANDMARK == inv_item->getType();
}
void LLFolderBridge::dropToFavorites(LLInventoryItem* inv_item)
{
	LLPointer<AddFavoriteLandmarkCallback> cb = new AddFavoriteLandmarkCallback();
	LLInventoryPanel* panel = mInventoryPanel.get();
	LLFolderViewItem* drag_over_item = panel ? panel->getRootFolder()->getDraggingOverItem() : NULL;
	if (drag_over_item && drag_over_item->getListener())
	{
		cb.get()->setTargetLandmarkId(drag_over_item->getListener()->getUUID());
	}
	copy_inventory_item(
		gAgent.getID(),
		inv_item->getPermissions().getOwner(),
		inv_item->getUUID(),
		mUUID,
		std::string(),
		cb);
}
void LLFolderBridge::dropToOutfit(LLInventoryItem* inv_item, BOOL move_is_into_current_outfit)
{
	if (move_is_into_current_outfit)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(inv_item->getUUID(), true, true);
	}
	else
	{
		LLPointer<LLInventoryCallback> cb = nullptr;
		link_inventory_object(mUUID, LLConstPointer<LLInventoryObject>(inv_item), cb);
	}
}
void LLFolderBridge::callback_dropItemIntoFolder(const LLSD& notification, const LLSD& response, LLInventoryItem* inv_item)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		dragItemIntoFolder(inv_item, TRUE, FALSE);
	}
}
void LLFolderBridge::callback_dropCategoryIntoFolder(const LLSD& notification, const LLSD& response, LLInventoryCategory* inv_category)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		dragCategoryIntoFolder(inv_category, TRUE, FALSE);
	}
}
BOOL LLFolderBridge::dragItemIntoFolder(LLInventoryItem* inv_item,
										BOOL drop,
										BOOL user_confirm)
{
	LLInventoryModel* model = getInventoryModel();
	if (!model || !inv_item) return FALSE;
	if (!isAgentInventory()) return FALSE;
	if (!isAgentAvatarValid()) return FALSE;
	LLInventoryPanel* destination_panel = mInventoryPanel.get();
	if (!destination_panel) return false;
	LLInventoryFilter* filter = getInventoryFilter();
	if (!filter) return false;
	const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
	const LLUUID &favorites_id = model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE, false);
	const LLUUID &landmarks_id = model->findCategoryUUIDForType(LLFolderType::FT_LANDMARK, false);
	const LLUUID& marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS, false);
	const LLUUID from_folder_uuid = inv_item->getParentUUID();
	const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
	const BOOL move_is_into_favorites = (mUUID == favorites_id);
	const BOOL move_is_into_my_outfits = (mUUID == my_outifts_id) || model->isObjectDescendentOf(mUUID, my_outifts_id);
	const BOOL move_is_into_outfit = move_is_into_my_outfits || (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
	const BOOL move_is_into_landmarks = (mUUID == landmarks_id) || model->isObjectDescendentOf(mUUID, landmarks_id);
	const BOOL move_is_into_marketplacelistings = model->isObjectDescendentOf(mUUID, marketplacelistings_id);
	const BOOL move_is_from_marketplacelistings = model->isObjectDescendentOf(inv_item->getUUID(), marketplacelistings_id);
	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	BOOL accept = FALSE;
	LLViewerObject* object = NULL;
	if(LLToolDragAndDrop::SOURCE_AGENT == source)
	{
		const LLUUID &trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH, false);
		const BOOL move_is_into_trash = (mUUID == trash_id) || model->isObjectDescendentOf(mUUID, trash_id);
		const BOOL move_is_outof_current_outfit = LLAppearanceMgr::instance().getIsInCOF(inv_item->getUUID());
		BOOL is_movable = TRUE;
		switch (inv_item->getActualType())
		{
			case LLAssetType::AT_CATEGORY:
				is_movable = !LLFolderType::lookupIsProtectedType(((LLInventoryCategory*)inv_item)->getPreferredType());
				break;
			default:
				break;
		}
		if (move_is_outof_current_outfit)
		{
			is_movable = FALSE;
		}
		if ( (rlv_handler_t::isEnabled()) && (is_movable) )
		{
			if (move_is_into_current_outfit)
			{
				const LLViewerInventoryItem* pItem = dynamic_cast<const LLViewerInventoryItem*>(inv_item);
				is_movable = rlvPredCanWearItem(pItem, RLV_WEAR_REPLACE);
			}
			if (is_movable)
			{
				is_movable = (!RlvFolderLocks::instance().hasLockedFolder(RLV_LOCK_ANY)) ||
					(RlvFolderLocks::instance().canMoveItem(inv_item->getUUID(), mUUID));
			}
		}
		if (move_is_into_trash)
		{
			is_movable &= inv_item->getIsLinkType() || !get_is_item_worn(inv_item->getUUID());
		}
		accept = TRUE;
		if (user_confirm && !is_movable)
		{
			accept = FALSE;
		}
		else if (user_confirm && (mUUID == inv_item->getParentUUID()) && !move_is_into_favorites)
		{
			accept = FALSE;
		}
		else if (user_confirm && (move_is_into_current_outfit || move_is_into_outfit))
		{
			accept = can_move_to_outfit(inv_item, move_is_into_current_outfit);
		}
		else if (user_confirm && ( move_is_into_landmarks))
		{
			accept = can_move_to_landmarks(inv_item);
		}
		else if (user_confirm && move_is_into_marketplacelistings)
		{
			const LLViewerInventoryCategory* master_folder = model->getFirstDescendantOf(marketplacelistings_id, mUUID);
			LLViewerInventoryCategory* dest_folder = getCategory();
			std::string tooltip_msg;
			accept = can_move_item_to_marketplace(master_folder, dest_folder, inv_item, tooltip_msg, LLToolDragAndDrop::instance().getCargoCount() - LLToolDragAndDrop::instance().getCargoIndex());
		}
		if (user_confirm && accept)
		{
			LLViewerInventoryCategory * dest_folder = getCategory();
			accept = dest_folder->acceptItem(inv_item);
		}
		LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);
		if (user_confirm && accept && active_panel)
		{
			LLFolderView* active_folder_view = active_panel->getRootFolder();
			if (!active_folder_view) return false;
		}
		if (accept && drop)
		{
			if (inv_item->getType() == LLAssetType::AT_GESTURE
				&& LLGestureMgr::instance().isGestureActive(inv_item->getUUID()) && move_is_into_trash)
			{
				LLGestureMgr::instance().deactivateGesture(inv_item->getUUID());
			}
			if (active_panel && (destination_panel != active_panel))
			{
				active_panel->unSelectAll();
			}
			if (user_confirm && (move_is_from_marketplacelistings || move_is_into_marketplacelistings))
			{
				if ((move_is_from_marketplacelistings && (LLMarketplaceData::instance().isInActiveFolder(inv_item->getUUID())
														|| LLMarketplaceData::instance().isListedAndActive(inv_item->getUUID()))) ||
					(move_is_into_marketplacelistings && LLMarketplaceData::instance().isInActiveFolder(mUUID)))
				{
					LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropItemIntoFolder, this, _1, _2, inv_item));
					return true;
				}
				if (move_is_into_marketplacelistings && !move_is_from_marketplacelistings)
				{
					LLNotificationsUtil::add("ConfirmMerchantMoveInventory", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropItemIntoFolder, this, _1, _2, inv_item));
					return true;
				}
			}
			if ((mUUID == inv_item->getParentUUID()) && move_is_into_favorites)
			{
				LLFolderViewItem* itemp = destination_panel->getRootFolder()->getDraggingOverItem();
				if (itemp)
				{
					LLUUID srcItemId = inv_item->getUUID();
					LLUUID destItemId = itemp->getListener()->getUUID();
					LLFavoritesOrderStorage::instance().rearrangeFavoriteLandmarks(srcItemId, destItemId);
				}
			}
			else if (move_is_into_favorites)
			{
				dropToFavorites(inv_item);
			}
			else if (move_is_into_current_outfit || move_is_into_outfit)
			{
				dropToOutfit(inv_item, move_is_into_current_outfit);
			}
			else if (move_is_into_marketplacelistings)
			{
				move_item_to_marketplacelistings(inv_item, mUUID);
			}
			else
			{
				if (gInventory.isObjectDescendentOf(inv_item->getUUID(), gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, false)))
				{
					set_dad_inbox_object(inv_item->getUUID());
				}
				LLInvFVBridge::changeItemParent(
					model,
					(LLViewerInventoryItem*)inv_item,
					mUUID,
					move_is_into_trash);
			}
			if (move_is_from_marketplacelistings)
			{
				LLUUID version_folder_id = LLMarketplaceData::instance().getActiveFolder(from_folder_uuid);
				if (version_folder_id.notNull())
				{
					LLViewerInventoryCategory* cat = gInventory.getCategory(version_folder_id);
					if (!validate_marketplacelistings(cat, NULL))
					{
						LLMarketplaceData::instance().activateListing(version_folder_id, false);
					}
				}
			}
		}
	}
	else if (LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		object = gObjectList.findObject(inv_item->getParentUUID());
		if (!object)
		{
			LL_INFOS() << "Object not found for drop." << LL_ENDL;
			return FALSE;
		}
		LLPermissions perm(inv_item->getPermissions());
		BOOL is_move = FALSE;
		if ((perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID())
			&& perm.allowTransferTo(gAgent.getID())))
		{
			accept = TRUE;
		}
		else if(object->permYouOwner())
		{
			is_move = TRUE;
			accept = TRUE;
		}
		if (move_is_into_current_outfit || move_is_into_outfit)
		{
			accept = FALSE;
		}
		else if (( move_is_into_landmarks)
				 && !can_move_to_landmarks(inv_item))
		{
			accept = FALSE;
		}
		else if (move_is_into_marketplacelistings)
		{
			accept = FALSE;
		}
		if (accept && drop)
		{
			LLMoveInv* move_inv = new LLMoveInv;
			move_inv->mObjectID = inv_item->getParentUUID();
			two_uuids_t item_pair(mUUID, inv_item->getUUID());
			move_inv->mMoveList.push_back(item_pair);
			move_inv->mCallback = NULL;
			move_inv->mUserData = NULL;
			if(is_move)
			{
				warn_move_inventory(object, move_inv);
			}
			else
			{
				set_dad_inventory_item(inv_item, mUUID);
				LLNotification::Params params("MoveInventoryFromObject");
				params.functor(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
				LLNotifications::instance().forceResponse(params, 0);
			}
		}
	}
	else if(LLToolDragAndDrop::SOURCE_NOTECARD == source)
	{
		if (move_is_into_marketplacelistings)
		{
			accept = FALSE;
		}
		else
		{
			accept = !(move_is_into_current_outfit || move_is_into_outfit);
		}
		if (accept && drop)
		{
			copy_inventory_from_notecard(mUUID,
										 LLToolDragAndDrop::getInstance()->getObjectID(),
										 LLToolDragAndDrop::getInstance()->getSourceID(),
										 inv_item);
		}
	}
	else if(LLToolDragAndDrop::SOURCE_LIBRARY == source)
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)inv_item;
		if(item && item->isFinished())
		{
			accept = TRUE;
			if (move_is_into_marketplacelistings)
			{
				accept = FALSE;
			}
			else if (move_is_into_current_outfit || move_is_into_outfit)
			{
				accept = can_move_to_outfit(inv_item, move_is_into_current_outfit);
			}
			else if ( move_is_into_landmarks)
			{
				accept = can_move_to_landmarks(inv_item);
			}
			LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);
			if (accept && active_panel)
			{
				LLFolderView* active_folder_view = active_panel->getRootFolder();
				if (!active_folder_view) return false;
			}
			if (accept && drop)
			{
				if (move_is_into_favorites)
				{
					dropToFavorites(inv_item);
				}
				else if (move_is_into_current_outfit || move_is_into_outfit)
				{
					dropToOutfit(inv_item, move_is_into_current_outfit);
				}
				else
				{
					copy_inventory_item(
						gAgent.getID(),
						inv_item->getPermissions().getOwner(),
						inv_item->getUUID(),
						mUUID,
						std::string(),
						LLPointer<LLInventoryCallback>(NULL));
				}
			}
		}
	}
	else
	{
		LL_WARNS() << "unhandled drag source" << LL_ENDL;
	}
	return accept;
}
void open_texture(const LLUUID& item_id,
				   const std::string& title,
				   BOOL show_keep_discard,
				   const LLUUID& source_id,
				   BOOL take_focus)
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_VIEWTEXTURE))
	{
		RlvUtil::notifyBlockedViewXXX(LLAssetType::AT_TEXTURE);
		return;
	}
	if( !LLPreview::show( item_id, take_focus ) )
	{
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewTextureRect");
		rect.translate( left - rect.mLeft, top - rect.mTop );
		LLPreviewTexture* preview;
		preview = new LLPreviewTexture("preview texture",
										  rect,
										  title,
										  item_id,
										  LLUUID::null,
										  show_keep_discard);
		preview->setSourceID(source_id);
		if(take_focus) preview->setFocus(TRUE);
		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}
LLUIImagePtr LLTextureBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_TEXTURE, mInvType);
}
void LLTextureBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}
bool LLTextureBridge::canSaveTexture()
{
	const LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return false;
	}
	const LLViewerInventoryItem* item = model->getItem(mUUID);
	if (item)
	{
		return item->checkPermissionsSet(PERM_ITEM_UNRESTRICTED);
	}
	return false;
}
void LLTextureBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLTextureBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else if (isMarketplaceListingsFolder())
	{
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
	}
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
		addOpenRightClickMenuOption(items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(true, items, disabled_items, flags);
		items.push_back(std::string("Texture Separator"));
		items.push_back(std::string("Save As"));
		if (!canSaveTexture())
		{
			disabled_items.push_back(std::string("Save As"));
		}
	}
	hide_context_entries(menu, items, disabled_items);
}
void LLTextureBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("save_as" == action)
	{
		const LLViewerInventoryItem* item(getItem());
		if (!item) return;
		open_texture(mUUID, std::string("Texture: ") + item->getName(), FALSE);
		LLPreview* preview_texture = LLPreview::find(mUUID);
		if (preview_texture)
		{
			preview_texture->saveAs();
		}
	}
	else LLItemBridge::performAction(model, action);
}
void LLSoundBridge::openItem()
{
	const LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}
void LLSoundBridge::previewItem()
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		send_sound_trigger(item->getAssetUUID(), SOUND_GAIN);
	}
}
void LLSoundBridge::openSoundPreview(void* which)
{
	LLSoundBridge *me = (LLSoundBridge *)which;
	if(!LLPreview::show(me->mUUID))
	{
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewSoundRect");
		rect.translate(left - rect.mLeft, top - rect.mTop);
		LLPreviewSound* preview = new LLPreviewSound("preview sound",
										   rect,
										   LLTrans::getString("Sound: ") + me->getName(),
										   me->mUUID);
		preview->setFocus(TRUE);
		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}
void LLSoundBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLSoundBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if (isMarketplaceListingsFolder())
	{
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
	}
	else
	{
		if (isItemInTrash())
		{
			addTrashContextMenuOptions(items, disabled_items);
		}
		else
		{
			items.push_back(std::string("Share"));
			if (!canShare())
			{
				disabled_items.push_back(std::string("Share"));
			}
			items.push_back(std::string("Sound Open"));
			items.push_back(std::string("Properties"));
			getClipboardEntries(true, items, disabled_items, flags);
		}
		items.push_back(std::string("Sound Separator"));
		items.push_back(std::string("Sound Play"));
	}
	hide_context_entries(menu, items, disabled_items);
}
void LLSoundBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("sound_play" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if(item)
		{
			send_sound_trigger(item->getAssetUUID(), SOUND_GAIN);
		}
	}
	else if ("open" == action)
	{
		openSoundPreview((void*)this);
	}
	else LLItemBridge::performAction(model, action);
}
LLLandmarkBridge::LLLandmarkBridge(LLInventoryPanel* inventory,
								   LLFolderView* root,
								   const LLUUID& uuid,
								   U32 flags) :
	LLItemBridge(inventory, root, uuid)
{
	mVisited = FALSE;
	if (flags & LLInventoryItemFlags::II_FLAGS_LANDMARK_VISITED)
	{
		mVisited = TRUE;
	}
}
LLUIImagePtr LLLandmarkBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_LANDMARK, LLInventoryType::IT_LANDMARK, mVisited, FALSE);
}
void LLLandmarkBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	LL_DEBUGS() << "LLLandmarkBridge::buildContextMenu()" << LL_ENDL;
	if (isMarketplaceListingsFolder())
	{
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
	}
	else
	{
		if(isItemInTrash())
		{
			addTrashContextMenuOptions(items, disabled_items);
		}
		else
		{
			items.push_back(std::string("Share"));
			if (!canShare())
			{
				disabled_items.push_back(std::string("Share"));
			}
			items.push_back(std::string("Landmark Open"));
			items.push_back(std::string("Properties"));
			getClipboardEntries(true, items, disabled_items, flags);
		}
		items.push_back(std::string("Landmark Separator"));
		items.push_back(std::string("Teleport To Landmark"));
		items.push_back(std::string("Show On Map"));
	}
	if ((flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Teleport To Landmark"));
		disabled_items.push_back(std::string("Show On Map"));
	}
	hide_context_entries(menu, items, disabled_items);
}
void teleport_via_landmark(const LLUUID& asset_id, const LLUUID& item_id)
{
	gAgent.teleportViaLandmark( asset_id );
	LLFloaterWorldMap* floater_world_map = gFloaterWorldMap;
	if( floater_world_map )
	{
		floater_world_map->trackLandmark( item_id );
	}
}
void LLLandmarkBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("teleport" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if(item)
		{
			teleport_via_landmark(item->getAssetUUID(), item->getUUID());
		}
	}
	else if ("about" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if(item)
		{
			open_landmark(item, std::string("  Landmark: ") + item->getName(), FALSE);
		}
	}
	else if ("show_on_map" == action)
	{
		if (const LLViewerInventoryItem* item = getItem())
		{
			gFloaterWorldMap->trackLandmark(item->getUUID());
			LLFloaterWorldMap::show(true);
		}
	}
	else
	{
		LLItemBridge::performAction(model, action);
	}
}
void open_landmark(LLViewerInventoryItem* inv_item,
				   const std::string& title,
				   BOOL show_keep_discard,
				   const LLUUID& source_id,
				   BOOL take_focus)
{
	if( !LLPreview::show( inv_item->getUUID(), take_focus ) )
	{
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewLandmarkRect");
		rect.translate( left - rect.mLeft, top - rect.mTop );
		LLPreviewLandmark* preview = new LLPreviewLandmark(title,
								  rect,
								  title,
								  inv_item->getUUID(),
								  show_keep_discard,
								  inv_item);
		preview->setSourceID(source_id);
		if(take_focus) preview->setFocus(TRUE);
		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}
static bool open_landmark_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLUUID asset_id = notification["payload"]["asset_id"].asUUID();
	LLUUID item_id = notification["payload"]["item_id"].asUUID();
	if (option == 0)
	{
		teleport_via_landmark( asset_id, item_id );
	}
	return false;
}
static LLNotificationFunctorRegistration open_landmark_callback_reg("TeleportFromLandmark", open_landmark_callback);
void LLLandmarkBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}
class LLCallingCardObserver : public LLFriendObserver
{
public:
	LLCallingCardObserver(LLCallingCardBridge* bridge) : mBridgep(bridge) {}
	virtual ~LLCallingCardObserver() { mBridgep = NULL; }
	virtual void changed(U32 mask)
	{
		mBridgep->refreshFolderViewItem();
	}
protected:
	LLCallingCardBridge* mBridgep;
};
LLCallingCardBridge::LLCallingCardBridge(LLInventoryPanel* inventory,
										 LLFolderView* root,
										 const LLUUID& uuid ) :
	LLItemBridge(inventory, root, uuid)
{
	mObserver = new LLCallingCardObserver(this);
	LLAvatarTracker::instance().addObserver(mObserver);
}
LLCallingCardBridge::~LLCallingCardBridge()
{
	LLAvatarTracker::instance().removeObserver(mObserver);
	delete mObserver;
}
void LLCallingCardBridge::refreshFolderViewItem()
{
	LLInventoryPanel* panel = mInventoryPanel.get();
	LLFolderViewItem* itemp = panel ? panel->getRootFolder()->getItemByID(mUUID) : NULL;
	if (itemp)
	{
		itemp->refresh();
	}
}
void LLCallingCardBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("begin_im" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgent.getID()) &&
			(!item->getCreatorUUID().isNull()))
		{
			LLAvatarActions::startIM(item->getCreatorUUID());
		}
	}
	else if ("lure" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgent.getID()) &&
			(!item->getCreatorUUID().isNull()))
		{
			LLAvatarActions::offerTeleport(item->getCreatorUUID());
		}
	}
	else if ("request_lure" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgent.getID()) &&
			(!item->getCreatorUUID().isNull()))
		{
			LLAvatarActions::teleportRequest(item->getCreatorUUID());
		}
	}
	else if ("web_profile" == action)
	{
		if (LLViewerInventoryItem* item = getItem())
		{
			LLAvatarActions::showProfile(item->getCreatorUUID(), true);
		}
	}
	else LLItemBridge::performAction(model, action);
}
LLUIImagePtr LLCallingCardBridge::getIcon() const
{
	BOOL online = FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		online = LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID());
	}
	return LLInventoryIcon::getIcon(LLAssetType::AT_CALLINGCARD, LLInventoryType::IT_CALLINGCARD, online, FALSE);
}
std::string LLCallingCardBridge::getLabelSuffix() const
{
	LLViewerInventoryItem* item = getItem();
	if( item && LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID()) )
	{
		return LLItemBridge::getLabelSuffix() + " (online)";
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}
void LLCallingCardBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}
void LLCallingCardBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLCallingCardBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else if (isMarketplaceListingsFolder())
	{
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
	}
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
		addOpenRightClickMenuOption(items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(true, items, disabled_items, flags);
		LLInventoryItem* item = getItem();
		BOOL good_card = (item
						  && (LLUUID::null != item->getCreatorUUID())
						  && (item->getCreatorUUID() != gAgent.getID()));
		BOOL user_online = FALSE;
		if (item)
		{
			user_online = (LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID()));
		}
		items.push_back(std::string("Send Instant Message Separator"));
		if (!gSavedSettings.getString("WebProfileURL").empty() && !gSavedSettings.getBOOL("UseWebProfiles"))
			items.push_back(std::string("Web Profile"));
		items.push_back(std::string("Send Instant Message"));
		items.push_back(std::string("Offer Teleport..."));
		items.push_back(std::string("Request Teleport..."));
		items.push_back(std::string("Conference Chat"));
		if (!good_card)
		{
			disabled_items.push_back(std::string("Send Instant Message"));
		}
		if (!good_card || !user_online)
		{
			disabled_items.push_back(std::string("Offer Teleport..."));
			disabled_items.push_back(std::string("Request Teleport..."));
			disabled_items.push_back(std::string("Conference Chat"));
		}
	}
	hide_context_entries(menu, items, disabled_items);
}
BOOL LLCallingCardBridge::dragOrDrop(MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data)
{
	LLViewerInventoryItem* item = getItem();
	BOOL rv = FALSE;
	if(item)
	{
		switch(cargo_type)
		{
			case DAD_TEXTURE:
			case DAD_SOUND:
			case DAD_LANDMARK:
			case DAD_SCRIPT:
			case DAD_CLOTHING:
			case DAD_OBJECT:
			case DAD_NOTECARD:
			case DAD_BODYPART:
			case DAD_ANIMATION:
			case DAD_GESTURE:
			case DAD_MESH:
			{
				LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;
				const LLPermissions& perm = inv_item->getPermissions();
				if(gInventory.getItem(inv_item->getUUID())
				   && perm.allowOperationBy(PERM_TRANSFER, gAgent.getID()))
				{
					rv = TRUE;
					if(drop)
					{
						LLGiveInventory::doGiveInventoryItem(item->getCreatorUUID(),
														 (LLInventoryItem*)cargo_data);
					}
				}
				else
				{
					rv = FALSE;
				}
				break;
			}
			case DAD_CATEGORY:
			{
				LLInventoryCategory* inv_cat = (LLInventoryCategory*)cargo_data;
				if( gInventory.getCategory( inv_cat->getUUID() ) )
				{
					rv = TRUE;
					if(drop)
					{
						LLGiveInventory::doGiveInventoryCategory(
							item->getCreatorUUID(),
							inv_cat);
					}
				}
				else
				{
					rv = FALSE;
				}
				break;
			}
			default:
				break;
		}
	}
	return rv;
}
void open_notecard(LLViewerInventoryItem* inv_item,
				   const std::string& title,
				   const LLUUID& object_id,
				   BOOL show_keep_discard,
				   const LLUUID& source_id,
				   BOOL take_focus)
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_VIEWNOTE))
	{
		RlvUtil::notifyBlockedViewXXX(LLAssetType::AT_NOTECARD);
		return;
	}
	if(!LLPreview::show(inv_item->getUUID(), take_focus))
	{
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("NotecardEditorRect");
		rect.translate(left - rect.mLeft, top - rect.mTop);
		LLPreviewNotecard* preview;
		preview = new LLPreviewNotecard("preview notecard", rect, title,
						inv_item->getUUID(), object_id, inv_item->getAssetUUID(),
						show_keep_discard, inv_item);
		preview->setSourceID(source_id);
		if(take_focus) preview->setFocus(TRUE);
		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}
void LLNotecardBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}
void LLNotecardBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLNotecardBridge::buildContextMenu()" << LL_ENDL;
	if (isMarketplaceListingsFolder())
	{
		menuentry_vec_t items;
		menuentry_vec_t disabled_items;
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
		hide_context_entries(menu, items, disabled_items);
	}
	else
	{
		LLItemBridge::buildContextMenu(menu, flags);
	}
}
LLFontGL::StyleFlags LLGestureBridge::getLabelStyle() const
{
	U8 font = LLFontGL::NORMAL;
	if (LLGestureMgr::instance().isGestureActive(mUUID))
	{
		font |= LLFontGL::BOLD;
	}
	const LLViewerInventoryItem* item = getItem();
	if (item && item->getIsLinkType())
	{
		font |= LLFontGL::ITALIC;
	}
	return (LLFontGL::StyleFlags)font;
}
std::string LLGestureBridge::getLabelSuffix() const
{
	if( LLGestureMgr::instance().isGestureActive(mUUID) )
	{
		LLStringUtil::format_map_t args;
		args["[GESLABEL]"] =  LLItemBridge::getLabelSuffix();
		return  LLTrans::getString("ActiveGesture", args);
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}
void LLGestureBridge::performAction(LLInventoryModel* model, std::string action)
{
	if (isAddAction(action))
	{
		LLGestureMgr::instance().activateGesture(mUUID);
		LLViewerInventoryItem* item = gInventory.getItem(mUUID);
		if (!item) return;
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else if ("deactivate" == action || isRemoveAction(action))
	{
		LLGestureMgr::instance().deactivateGesture(mUUID);
		LLViewerInventoryItem* item = gInventory.getItem(mUUID);
		if (!item) return;
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else if("play" == action)
	{
		if(!LLGestureMgr::instance().isGestureActive(mUUID))
		{
			BOOL inform_server = TRUE;
			BOOL deactivate_similar = FALSE;
			LLGestureMgr::instance().setGestureLoadedCallback(mUUID, std::bind(&LLGestureBridge::playGesture, mUUID));
			LLViewerInventoryItem* item = gInventory.getItem(mUUID);
			llassert(item);
			if (item)
			{
				LLGestureMgr::instance().activateGestureWithAsset(mUUID, item->getAssetUUID(), inform_server, deactivate_similar);
			}
		}
		else
		{
			playGesture(mUUID);
		}
	}
	else LLItemBridge::performAction(model, action);
}
void LLGestureBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}
BOOL LLGestureBridge::removeItem()
{
	const LLInventoryModel* model = getInventoryModel();
	if(!model)
	{
		return FALSE;
	}
	const LLUUID item_id = mUUID;
	LLGestureMgr::instance().deactivateGesture(item_id);
	if (!model->getObject(item_id))
	{
		return TRUE;
	}
	return LLItemBridge::removeItem();
}
void LLGestureBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLGestureBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else if (isMarketplaceListingsFolder())
	{
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
	}
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
		addOpenRightClickMenuOption(items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(true, items, disabled_items, flags);
		items.push_back(std::string("Gesture Separator"));
		if (LLGestureMgr::instance().isGestureActive(getUUID()))
		{
			items.push_back(std::string("Deactivate"));
		}
		else
		{
			items.push_back(std::string("Activate"));
		}
	}
	hide_context_entries(menu, items, disabled_items);
}
void LLGestureBridge::playGesture(const LLUUID& item_id)
{
	if (LLGestureMgr::instance().isGesturePlaying(item_id))
	{
		LLGestureMgr::instance().stopGesture(item_id);
	}
	else
	{
		LLGestureMgr::instance().playGesture(item_id);
	}
}
void LLAnimationBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	LL_DEBUGS() << "LLAnimationBridge::buildContextMenu()" << LL_ENDL;
	if (isMarketplaceListingsFolder())
	{
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
	}
	else
	{
		if(isItemInTrash())
		{
			addTrashContextMenuOptions(items, disabled_items);
		}
		else
		{
			items.push_back(std::string("Share"));
			if (!canShare())
			{
				disabled_items.push_back(std::string("Share"));
			}
			items.push_back(std::string("Animation Open"));
			items.push_back(std::string("Properties"));
			getClipboardEntries(true, items, disabled_items, flags);
		}
		items.push_back(std::string("Animation Separator"));
		items.push_back(std::string("Animation Play"));
		items.push_back(std::string("Animation Audition"));
	}
	hide_context_entries(menu, items, disabled_items);
}
void LLAnimationBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ((action == "playworld") || (action == "playlocal"))
	{
		if (LLPreview::show(mUUID)) return;
		if (getItem())
		{
			LLPreviewAnim::e_activation_type activate = LLPreviewAnim::NONE;
			if ("playworld" == action) activate = LLPreviewAnim::PLAY;
			if ("playlocal" == action) activate = LLPreviewAnim::AUDITION;
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLRect rect = gSavedSettings.getRect("PreviewAnimRect");
			rect.translate( left - rect.mLeft, top - rect.mTop );
			LLPreviewAnim* preview = new LLPreviewAnim("preview anim",
									rect,
									"Animations: " + getItem()->getName(),
									mUUID,
									activate);
			gFloaterView->adjustToFitScreen(preview, FALSE);
		}
	}
	else
	{
		LLItemBridge::performAction(model, action);
	}
}
void LLAnimationBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}
LLUUID LLObjectBridge::sContextMenuItemID;
LLObjectBridge::LLObjectBridge(LLInventoryPanel* inventory,
							   LLFolderView* root,
							   const LLUUID& uuid,
							   LLInventoryType::EType type,
							   U32 flags) :
	LLItemBridge(inventory, root, uuid)
{
	mAttachPt = (flags & 0xff);
	mIsMultiObject = ( flags & LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS ) ?  TRUE: FALSE;
	mInvType = type;
}
LLUIImagePtr LLObjectBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_OBJECT, mInvType, mAttachPt, mIsMultiObject);
}
LLInventoryObject* LLObjectBridge::getObject() const
{
	LLInventoryObject* object = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		object = (LLInventoryObject*)model->getObject(mUUID);
	}
	return object;
}
void LLObjectBridge::performAction(LLInventoryModel* model, std::string action)
{
	if (isAddAction(action))
	{
		LLUUID object_id = mUUID;
		LLViewerInventoryItem* item;
		item = (LLViewerInventoryItem*)gInventory.getItem(object_id);
		if(item && gInventory.isObjectDescendentOf(object_id, gInventory.getRootFolderID()))
		{
			rez_attachment(item, NULL, true);
		}
		else if(item && item->isFinished())
		{
			LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(&rez_attachment_cb, _1, (LLViewerJointAttachment*)0, false));
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		gFocusMgr.setKeyboardFocus(NULL);
	}
	else if ("wear_add" == action)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(mUUID, true, false);
	}
	else if (isRemoveAction(action))
	{
		LLAppearanceMgr::instance().removeItemFromAvatar(mUUID);
	}
	else if ("edit" == action)
	{
		handle_attachment_edit(mUUID);
	}
	else LLItemBridge::performAction(model, action);
}
void LLObjectBridge::openItem()
{
	static LLCachedControl<bool> add(gSavedSettings, "LiruAddNotReplace");
	performAction(getInventoryModel(),
		      get_is_item_worn(mUUID) ? "detach" : (add ? "wear_add" : "attach"));
}
std::string LLObjectBridge::getLabelSuffix() const
{
	if (get_is_item_worn(mUUID))
	{
		if (!isAgentAvatarValid())
		{
			return LLItemBridge::getLabelSuffix() + LLTrans::getString("worn");
		}
		std::string attachment_point_name;
		if (gAgentAvatarp->getAttachedPointName(mUUID, attachment_point_name))
		{
			LLStringUtil::format_map_t args;
			args["[ATTACHMENT_POINT]"] =  LLTrans::getString(attachment_point_name);
			if (gRlvAttachmentLocks.canDetach(getItem()))
				return LLItemBridge::getLabelSuffix() + LLTrans::getString("WornOnAttachmentPoint", args);
			else
				return LLItemBridge::getLabelSuffix() + LLTrans::getString("LockedOnAttachmentPoint", args);
		}
		else
		{
			LLStringUtil::format_map_t args;
			args["[ATTACHMENT_ERROR]"] =  LLTrans::getString(attachment_point_name);
			return LLItemBridge::getLabelSuffix() + LLTrans::getString("AttachmentErrorMessage", args);
		}
	}
	return LLItemBridge::getLabelSuffix();
}
void rez_attachment(LLViewerInventoryItem* item, LLViewerJointAttachment* attachment, bool replace)
{
	static LLCachedControl<bool> fRlvDeprecateAttachPt(gSavedSettings, "RLVaDebugDeprecateExplicitPoint", false);
	if ( (rlv_handler_t::isEnabled()) && (!fRlvDeprecateAttachPt) &&
	     (!attachment) && (gRlvAttachmentLocks.hasLockedAttachmentPoint(RLV_LOCK_ANY)) )
	{
		attachment = RlvAttachPtLookup::getAttachPoint(item);
	}
	const LLUUID& item_id = item->getLinkedUUID();
	if (isAgentAvatarValid() &&
		 gAgentAvatarp->isWearingAttachment(item_id))
	{
		LL_WARNS() << "ATT duplicate attachment request, ignoring" << LL_ENDL;
		return;
	}
	S32 attach_pt = 0;
	if (isAgentAvatarValid() && attachment)
	{
		for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
			 iter != gAgentAvatarp->mAttachmentPoints.end(); ++iter)
		{
			if (iter->second == attachment)
			{
				attach_pt = iter->first;
				break;
			}
		}
	}
	LLSD payload;
	payload["item_id"] = item_id;
	payload["attachment_point"] = attach_pt;
	payload["is_add"] = !replace;
	if (replace &&
		(attachment && attachment->getNumObjects() > 0))
	{
		if ( (rlv_handler_t::isEnabled()) && ((gRlvAttachmentLocks.canAttach(attachment) & RLV_WEAR_REPLACE) == 0)  )
		{
			return;
		}
		LLNotificationsUtil::add("ReplaceAttachment", LLSD(), payload, confirm_attachment_rez);
	}
	else
	{
		if ( (rlv_handler_t::isEnabled()) && (gRlvAttachmentLocks.isLockedAttachmentPoint(attach_pt, RLV_LOCK_ADD)) )
		{
			return;
		}
		LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0);
	}
}
bool confirm_attachment_rez(const LLSD& notification, const LLSD& response)
{
	if (!gAgentAvatarp->canAttachMoreObjects())
	{
		LLSD args;
		args["MAX_ATTACHMENTS"] = llformat("%d", LLAgentBenefitsMgr::current().getAttachmentLimit());
		LLNotificationsUtil::add("MaxAttachmentsOnOutfit", args);
		return false;
	}
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLUUID item_id = notification["payload"]["item_id"].asUUID();
		LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
		if (itemp)
		{
			U8 attachment_pt = notification["payload"]["attachment_point"].asInteger();
			BOOL is_add = notification["payload"]["is_add"].asBoolean();
			LL_DEBUGS("Avatar") << "ATT calling addAttachmentRequest " << (itemp ? itemp->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
			LLAttachmentsMgr::instance().addAttachmentRequest(item_id, attachment_pt, is_add);
		}
	}
	return false;
}
static LLNotificationFunctorRegistration confirm_replace_attachment_rez_reg("ReplaceAttachment", confirm_attachment_rez);
BOOL rlvAttachToEnabler(void* pParam)
{
	return (pParam != NULL) &&
		(!gRlvAttachmentLocks.isLockedAttachmentPoint((LLViewerJointAttachment*)pParam, RLV_LOCK_ADD));
}
void LLObjectBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else if (isMarketplaceListingsFolder())
	{
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
	}
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
		items.push_back(std::string("Properties"));
		getClipboardEntries(true, items, disabled_items, flags);
		LLObjectBridge::sContextMenuItemID = mUUID;
		LLInventoryItem* item = getItem();
		if(item)
		{
			if (!isAgentAvatarValid()) return;
			if( get_is_item_worn( mUUID ) )
			{
				items.push_back(std::string("Wearable And Object Separator"));
				items.push_back(std::string("Detach From Yourself"));
				items.push_back(std::string("Wearable Edit"));
				if ( (rlv_handler_t::isEnabled()) && (!gRlvAttachmentLocks.canDetach(item)) )
					disabled_items.push_back(std::string("Detach From Yourself"));
			}
			else if (!isItemInTrash() && !isLinkedObjectInTrash() && !isLinkedObjectMissing() && !isCOFFolder())
			{
				items.push_back(std::string("Wearable And Object Separator"));
				items.push_back(std::string("Wearable And Object Wear"));
				items.push_back(std::string("Wearable Add"));
				items.push_back(std::string("Attach To"));
				items.push_back(std::string("Attach To HUD"));
				items.push_back(std::string("Restore to Last Position"));
				if (!gAgentAvatarp->canAttachMoreObjects())
				{
					disabled_items.push_back(std::string("Wearable And Object Wear"));
					disabled_items.push_back(std::string("Wearable Add"));
					disabled_items.push_back(std::string("Attach To"));
					disabled_items.push_back(std::string("Attach To HUD"));
				}
				else if (rlv_handler_t::isEnabled())
				{
					ERlvWearMask eWearMask = gRlvAttachmentLocks.canAttach(item);
					if ((eWearMask & RLV_WEAR_REPLACE) == 0)
						disabled_items.push_back(std::string("Wearable And Object Wear"));
					if ((eWearMask & RLV_WEAR_ADD) == 0)
						disabled_items.push_back(std::string("Wearable Add"));
				}
				LLMenuGL* attach_menu = menu.getChildMenuByName("Attach To", TRUE);
				LLMenuGL* attach_hud_menu = menu.getChildMenuByName("Attach To HUD", TRUE);
				if (attach_menu
					&& (attach_menu->getChildCount() == 0)
					&& attach_hud_menu
					&& (attach_hud_menu->getChildCount() == 0)
					&& isAgentAvatarValid())
				{
					for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
						 iter != gAgentAvatarp->mAttachmentPoints.end(); )
					{
						LLVOAvatar::attachment_map_t::iterator curiter = iter++;
						LLViewerJointAttachment* attachment = curiter->second;
						LLMenuItemCallGL *new_item;
						LLMenuGL* target_menu = attachment->getIsHUDAttachment() ? attach_hud_menu : attach_menu;
						target_menu->addChild(new_item = new LLMenuItemCallGL(attachment->getName(),
								NULL,
								(rlv_handler_t::isEnabled()) ? &rlvAttachToEnabler : NULL,
								&attach_label, (void*)attachment));
						LLSimpleListener* callback = mInventoryPanel.get()->getListenerByName("Inventory.AttachObject");
						if (callback)
						{
							new_item->addListener(callback, "on_click", LLSD(attachment->getName()));
						}
					}
					LLMenuItemCallGL *new_item = new LLMenuItemCallGL("Custom...", NULL, NULL);
					attach_menu->addChild(new_item);
					LLSimpleListener* callback = mInventoryPanel.get()->getListenerByName("Inventory.AttachCustom");
					new_item->addListener(callback, "on_click", LLSD());
				}
			}
		}
	}
	hide_context_entries(menu, items, disabled_items);
}
BOOL LLObjectBridge::renameItem(const std::string& new_name)
{
	if(!isItemRenameable())
		return FALSE;
	LLPreview::rename(mUUID, std::string("Object: ") + new_name);
	LLInventoryModel* model = getInventoryModel();
	if(!model)
		return FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item && (item->getName() != new_name))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->rename(new_name);
		new_item->updateServer(FALSE);
		model->updateItem(new_item);
		model->notifyObservers();
		buildDisplayName();
		if (isAgentAvatarValid())
		{
			LLViewerObject* obj = gAgentAvatarp->getWornAttachment( item->getUUID() );
			if(obj)
			{
				LLSelectMgr::getInstance()->deselectAll();
				LLSelectMgr::getInstance()->addAsIndividual( obj, SELECT_ALL_TES, FALSE );
				LLSelectMgr::getInstance()->selectionSetObjectName( new_name );
				LLSelectMgr::getInstance()->deselectAll();
			}
		}
	}
	return FALSE;
}
void LLLSLTextBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}
LLWearableBridge::LLWearableBridge(LLInventoryPanel* inventory,
								   LLFolderView* root,
								   const LLUUID& uuid,
								   LLAssetType::EType asset_type,
								   LLInventoryType::EType inv_type,
								   LLWearableType::EType  wearable_type) :
	LLItemBridge(inventory, root, uuid),
	mAssetType( asset_type ),
	mWearableType(wearable_type)
{
	mInvType = inv_type;
}
BOOL LLWearableBridge::renameItem(const std::string& new_name)
{
	if (get_is_item_worn(mUUID))
	{
		gAgentWearables.setWearableName( mUUID, new_name );
	}
	return LLItemBridge::renameItem(new_name);
}
void LLWearableBridge::nameOrDescriptionChanged(void) const
{
	if (get_is_item_worn(mUUID))
	{
		gAgentWearables.nameOrDescriptionChanged(mUUID);
	}
}
std::string LLWearableBridge::getLabelSuffix() const
{
	if (get_is_item_worn(mUUID))
	{
		if ( (rlv_handler_t::isEnabled()) && (!gRlvWearableLocks.canRemove(getItem())) )
		{
			return LLItemBridge::getLabelSuffix() + LLTrans::getString("locked");
		}
		return LLItemBridge::getLabelSuffix() + LLTrans::getString("worn");
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}
LLUIImagePtr LLWearableBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(mAssetType, mInvType, mWearableType, FALSE);
}
void move_wearable_item(LLViewerInventoryItem* item, bool closer_to_body)
{
	if(!item)
		return;
	if(item->getIsLinkType())
	{
		if(LLAppearanceMgr::instance().moveWearable(item, closer_to_body))
			gAgentAvatarp->wearableUpdated(item->getWearableType(),TRUE);
	}
	else
	{
		LLInventoryModel::item_array_t items;
		LLInventoryModel::cat_array_t cats;
		LLLinkedItemIDMatches is_linked_item_match(item->getUUID());
		gInventory.collectDescendentsIf(LLAppearanceMgr::instance().getCOF(),
										cats,
										items,
										LLInventoryModel::EXCLUDE_TRASH,
										is_linked_item_match);
		if(!items.empty())
		{
			if(LLAppearanceMgr::instance().moveWearable(gInventory.getItem(items.front()->getUUID()),closer_to_body))
				gAgentAvatarp->wearableUpdated(item->getWearableType(),TRUE);
		}
	}
}
void LLWearableBridge::performAction(LLInventoryModel* model, std::string action)
{
	if (isAddAction(action))
	{
		wearOnAvatar();
	}
	else if ("wear_add" == action)
	{
		wearAddOnAvatar();
	}
	else if ("edit" == action)
	{
		editOnAvatar();
		return;
	}
	else if (isRemoveAction(action))
	{
		removeFromAvatar();
		return;
	}
	else if("move_forward" == action)
	{
		move_wearable_item(getItem(),false);
	}
	else if("move_back" == action)
	{
		move_wearable_item(getItem(),true);
	}
	else LLItemBridge::performAction(model, action);
}
void LLWearableBridge::openItem()
{
	if (LLViewerInventoryItem* item = getItem())
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
}
void LLWearableBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLWearableBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else if (isMarketplaceListingsFolder())
	{
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
	}
	else
	{
		BOOL can_open = ((flags & SUPPRESS_OPEN_ITEM) != SUPPRESS_OPEN_ITEM);
		LLViewerInventoryItem* item = getItem();
		if (can_open && item)
		{
			can_open = (item->getType() != LLAssetType::AT_CLOTHING) &&
				(item->getType() != LLAssetType::AT_BODYPART);
		}
		if (isLinkedObjectMissing())
		{
			can_open = FALSE;
		}
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
		if (can_open)
		{
			addOpenRightClickMenuOption(items);
		}
		else
		{
			disabled_items.push_back(std::string("Open"));
			disabled_items.push_back(std::string("Open Original"));
		}
		items.push_back(std::string("Properties"));
		getClipboardEntries(true, items, disabled_items, flags);
		items.push_back(std::string("Wearable And Object Separator"));
		items.push_back(std::string("Wearable Edit"));
		bool not_modifiable = !item || !gAgentWearables.isWearableModifiable(item->getUUID());
		if (((flags & FIRST_SELECTED_ITEM) == 0) || not_modifiable)
		{
			disabled_items.push_back(std::string("Wearable Edit"));
		}
		if (isLinkedObjectInTrash() || isLinkedObjectMissing() || isCOFFolder())
		{
			disabled_items.push_back(std::string("Wearable And Object Wear"));
			disabled_items.push_back(std::string("Wearable Add"));
			disabled_items.push_back(std::string("Wearable Edit"));
		}
		if(item)
		{
			switch (item->getType())
			{
				case LLAssetType::AT_CLOTHING:
					items.push_back(std::string("Take Off"));
				case LLAssetType::AT_BODYPART:
					items.push_back(std::string("Wearable And Object Wear"));
					if (LLUpdateAppearanceOnDestroy::sActiveCallbacks)
					{
						disabled_items.push_back(std::string("Wearable And Object Wear"));
						disabled_items.push_back(std::string("Wearable Add"));
						disabled_items.push_back(std::string("Take Off"));
						disabled_items.push_back(std::string("Wearable Move Forward"));
						disabled_items.push_back(std::string("Wearable Move Back"));
					}
					else if (get_is_item_worn(item->getUUID()))
					{
						disabled_items.push_back(std::string("Wearable And Object Wear"));
						disabled_items.push_back(std::string("Wearable Add"));
						if ((rlv_handler_t::isEnabled()) && (!gRlvWearableLocks.canRemove(item)))
							disabled_items.push_back(std::string("Take Off"));
					}
					else
					{
						disabled_items.push_back(std::string("Take Off"));
						disabled_items.push_back(std::string("Wearable Edit"));
						if (gAgentWearables.getWearableFromAssetID(item->getAssetUUID()))
						{
							disabled_items.push_back(std::string("Wearable Add"));
							LLViewerWearable* wearable = gAgentWearables.getWearableFromAssetID(item->getAssetUUID());
							if (wearable && wearable != gAgentWearables.getTopWearable(mWearableType))
								disabled_items.push_back(std::string("Wearable And Object Wear"));
						}
						if (rlv_handler_t::isEnabled())
						{
							ERlvWearMask eWearMask = gRlvWearableLocks.canWear(item);
							if ((eWearMask & RLV_WEAR_REPLACE) == 0)
								disabled_items.push_back(std::string("Wearable And Object Wear"));
							if ((eWearMask & RLV_WEAR_ADD) == 0)
								disabled_items.push_back(std::string("Wearable Add"));
						}
					}
					if (LLWearableType::getAllowMultiwear(mWearableType))
					{
						items.push_back(std::string("Wearable Add"));
						items.push_back(std::string("Wearable Move Forward"));
						items.push_back(std::string("Wearable Move Back"));
						bool is_worn = get_is_item_worn(item->getUUID());
						if (!is_worn || !gAgentWearables.canMoveWearable(item->getUUID(), false))
							disabled_items.push_back(std::string("Wearable Move Forward"));
						if (!is_worn || !gAgentWearables.canMoveWearable(item->getUUID(), true))
							disabled_items.push_back(std::string("Wearable Move Back"));
						if (item->getType() != LLAssetType::AT_BODYPART && !gAgentWearables.canAddWearable(mWearableType))
						{
							disabled_items.push_back(std::string("Wearable Add"));
						}
					}
					break;
				default:
					break;
			}
		}
	}
	hide_context_entries(menu, items, disabled_items);
}
BOOL LLWearableBridge::canWearOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return FALSE;
	if(!self->isAgentInventory())
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)self->getItem();
		if(!item || !item->isFinished()) return FALSE;
	}
	return (!get_is_item_worn(self->mUUID));
}
void LLWearableBridge::onWearOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return;
	self->wearOnAvatar();
}
void LLWearableBridge::wearOnAvatar()
{
#if 0
	if (!gAgentWearables.areWearablesLoaded())
	{
		LLNotificationsUtil::add("CanNotChangeAppearanceUntilLoaded");
		return;
	}
#endif
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		if ((item->getType() != LLAssetType::AT_BODYPART && !gAgentWearables.areWearablesLoaded()))
		{
			LLNotificationsUtil::add("CanNotChangeAppearanceUntilLoaded");
			return;
		}
		LLAppearanceMgr::instance().wearItemOnAvatar(item->getUUID(), true, true);
	}
}
void LLWearableBridge::wearAddOnAvatar()
{
	if (!gAgentWearables.areWearablesLoaded())
	{
		LLNotificationsUtil::add("CanNotChangeAppearanceUntilLoaded");
		return;
	}
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(item->getUUID(), true, false);
	}
}
BOOL LLWearableBridge::canEditOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return FALSE;
	return (get_is_item_worn(self->mUUID));
}
void LLWearableBridge::onEditOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(self)
	{
		self->editOnAvatar();
	}
}
void LLWearableBridge::editOnAvatar()
{
	LLAgentWearables::editWearable(mUUID);
}
BOOL LLWearableBridge::canRemoveFromAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if( self && (LLAssetType::AT_BODYPART != self->mAssetType) )
	{
		return get_is_item_worn( self->mUUID );
	}
	return FALSE;
}
void LLWearableBridge::removeFromAvatar()
{
	if (get_is_item_worn(mUUID))
	{
		LLAppearanceMgr::instance().removeItemFromAvatar(mUUID);
	}
}
std::string LLLinkItemBridge::sPrefix("Link: ");
LLUIImagePtr LLLinkItemBridge::getIcon() const
{
	if (LLViewerInventoryItem *item = getItem())
	{
		U32 attachment_point = (item->getFlags() & 0xff);
		bool is_multi =  LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS & item->getFlags();
		return LLInventoryIcon::getIcon(item->getActualType(), item->getInventoryType(), attachment_point, is_multi);
	}
	return LLInventoryIcon::getIcon(LLAssetType::AT_LINK, LLInventoryType::IT_NONE, 0, FALSE);
}
void LLLinkItemBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLLink::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	items.push_back(std::string("Find Original"));
	disabled_items.push_back(std::string("Find Original"));
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else
	{
		items.push_back(std::string("Properties"));
		addDeleteContextMenuOptions(items, disabled_items);
	}
	hide_context_entries(menu, items, disabled_items);
}
LLUIImagePtr LLMeshBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_MESH, LLInventoryType::IT_MESH, 0, FALSE);
}
void LLMeshBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
	}
}
void LLMeshBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLMeshBridge::buildContextMenu()" << LL_ENDL;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if(isItemInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}
		items.push_back(std::string("Restore Item"));
	}
	else if (isMarketplaceListingsFolder())
	{
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
	}
	else
	{
		items.push_back(std::string("Properties"));
		getClipboardEntries(true, items, disabled_items, flags);
	}
	hide_context_entries(menu, items, disabled_items);
}
std::string LLLinkFolderBridge::sPrefix("Link: ");
LLUIImagePtr LLLinkFolderBridge::getIcon() const
{
	LLFolderType::EType folder_type = LLFolderType::FT_NONE;
	const LLInventoryObject *obj = getInventoryObject();
	if (obj)
	{
		LLViewerInventoryCategory* cat = NULL;
		LLInventoryModel* model = getInventoryModel();
		if(model)
		{
			cat = (LLViewerInventoryCategory*)model->getCategory(obj->getLinkedUUID());
			if (cat)
			{
				folder_type = cat->getPreferredType();
			}
		}
	}
	return LLFolderBridge::getIcon(folder_type);
}
void LLLinkFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLLink::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if (isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else
	{
		getClipboardEntries(false, items, disabled_items, flags);
		if (LLPanelMainInventory::getActiveInventory() && isAgentInventory())
			build_context_menu_folder_options(getInventoryModel(), getFolderID(), items, disabled_items);
		addDeleteContextMenuOptions(items, disabled_items);
	}
	hide_context_entries(menu, items, disabled_items);
}
void LLLinkFolderBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("goto" == action)
	{
		gotoItem();
		return;
	}
	if ("purge" == action)
	{
		purgeItem(model, mUUID);
		return;
	}
	if ("restore" == action)
	{
		restoreItem();
		return;
	}
	if ("cut" == action)
	{
		cutToClipboard();
		LLFolderView::removeCutItems();
		return;
	}
	if ("copy" == action)
	{
		copyToClipboard();
		return;
	}
	if (LLPanelMainInventory* iv = LLPanelMainInventory::getActiveInventory())
	{
		if (LLFolderViewItem* folder_item = iv->getActivePanel()->getRootFolder()->getItemByID(getFolderID()))
		{
			folder_item->getListener()->performAction(model, action);
			return;
		}
	}
	LLItemBridge::performAction(model,action);
}
void LLLinkFolderBridge::gotoItem()
{
	const LLUUID &cat_uuid = getFolderID();
	if (!cat_uuid.isNull())
	{
		if (LLFolderViewItem *base_folder = mRoot->getItemByID(cat_uuid))
		{
			if (LLInventoryModel* model = getInventoryModel())
			{
				model->fetchDescendentsOf(cat_uuid);
			}
			base_folder->setOpen(TRUE);
			mRoot->setSelectionFromRoot(base_folder,TRUE);
			mRoot->scrollToShowSelection();
		}
	}
}
const LLUUID &LLLinkFolderBridge::getFolderID() const
{
	if (LLViewerInventoryItem *link_item = getItem())
	{
		if (const LLViewerInventoryCategory *cat = link_item->getLinkedCategory())
		{
			const LLUUID& cat_uuid = cat->getUUID();
			return cat_uuid;
		}
	}
	return LLUUID::null;
}
void LLInvFVBridgeAction::doAction(LLAssetType::EType asset_type,
								   const LLUUID& uuid,
								   LLInventoryModel* model)
{
	const LLUUID& linked_uuid = gInventory.getLinkedItemID(uuid);
	LLInvFVBridgeAction* action = createAction(asset_type,linked_uuid,model);
	if(action)
	{
		action->doIt();
		delete action;
	}
}
void LLInvFVBridgeAction::doAction(const LLUUID& uuid, LLInventoryModel* model)
{
	llassert(model);
	LLViewerInventoryItem* item = model->getItem(uuid);
	llassert(item);
	if (item)
	{
		LLAssetType::EType asset_type = item->getType();
		LLInvFVBridgeAction* action = createAction(asset_type,uuid,model);
		if(action)
		{
			action->doIt();
			delete action;
		}
	}
}
LLViewerInventoryItem* LLInvFVBridgeAction::getItem() const
{
	if (mModel)
		return (LLViewerInventoryItem*)mModel->getItem(mUUID);
	return NULL;
}
class LLTextureBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		if (const LLViewerInventoryItem* item = getItem())
		{
			open_texture(mUUID, std::string("Texture: ") + item->getName(), FALSE);
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLTextureBridgeAction(){}
protected:
	LLTextureBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};
class LLSoundBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			if(!LLPreview::show(mUUID))
			{
				S32 left, top;
				gFloaterView->getNewFloaterPosition(&left, &top);
				LLRect rect = gSavedSettings.getRect("PreviewSoundRect");
				rect.translate(left - rect.mLeft, top - rect.mTop);
				LLPreviewSound* preview = new LLPreviewSound("preview sound",
												   rect,
												   LLTrans::getString("Sound: ") + item->getName(),
												   mUUID);
				preview->setFocus(TRUE);
				gFloaterView->adjustToFitScreen(preview, FALSE);
			}
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLSoundBridgeAction(){}
protected:
	LLSoundBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};
class LLLandmarkBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			LLSD payload;
			payload["asset_id"] = item->getAssetUUID();
			payload["item_id"] = item->getUUID();
			LLSD args;
			args["LOCATION"] = item->getName();
			LLNotificationsUtil::add("TeleportFromLandmark", args, payload);
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLLandmarkBridgeAction(){}
protected:
	LLLandmarkBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};
class LLCallingCardBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item && item->getCreatorUUID().notNull())
		{
			LLAvatarActions::showProfile(item->getCreatorUUID());
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLCallingCardBridgeAction(){}
protected:
	LLCallingCardBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};
class LLNotecardBridgeAction
: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			open_notecard(item, std::string("Notecard: ") + item->getName(), LLUUID::null, FALSE);
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLNotecardBridgeAction(){}
protected:
	LLNotecardBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};
class LLGestureBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			if(!LLPreview::show(mUUID))
			{
				LLPreviewGesture* preview = LLPreviewGesture::show(std::string("Gesture: ") + item->getName(), mUUID, LLUUID::null);
				preview->setFocus(TRUE);
				gFloaterView->adjustToFitScreen(preview, FALSE);
			}
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLGestureBridgeAction(){}
protected:
	LLGestureBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};
class LLAnimationBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			if(!LLPreview::show( mUUID ))
			{
				S32 left, top;
				gFloaterView->getNewFloaterPosition(&left, &top);
				LLRect rect = gSavedSettings.getRect("PreviewAnimRect");
				rect.translate( left - rect.mLeft, top - rect.mTop );
				LLPreviewAnim* preview = new LLPreviewAnim("preview anim",
										rect,
										std::string("Animation: ") + item->getName(),
										mUUID,
										LLPreviewAnim::NONE);
				preview->setFocus(TRUE);
				gFloaterView->adjustToFitScreen(preview, FALSE);
			}
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLAnimationBridgeAction(){}
protected:
	LLAnimationBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};
class LLObjectBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLObjectBridgeAction(){}
protected:
	LLObjectBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};
class LLLSLTextBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_VIEWSCRIPT))
	{
		RlvUtil::notifyBlockedViewXXX(LLAssetType::AT_NOTECARD);
		return;
	}
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			if(!LLPreview::show(mUUID))
			{
				LLViewerInventoryItem* item = getItem();
				if (item)
				{
					S32 left, top;
					gFloaterView->getNewFloaterPosition(&left, &top);
					LLRect rect = gSavedSettings.getRect("PreviewScriptRect");
					rect.translate(left - rect.mLeft, top - rect.mTop);
					LLPreviewLSL* preview =	new LLPreviewLSL("preview lsl text",
													 rect,
													 std::string("Script: ") + item->getName(),
													 mUUID);
					preview->setFocus(TRUE);
					gFloaterView->adjustToFitScreen(preview, FALSE);
				}
			}
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLLSLTextBridgeAction(){}
protected:
	LLLSLTextBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};
class LLWearableBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		wearOnAvatar();
	}
	virtual ~LLWearableBridgeAction(){}
protected:
	LLWearableBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
	BOOL isItemInTrash() const;
	BOOL isAgentInventory() const;
	void wearOnAvatar();
};
BOOL LLWearableBridgeAction::isItemInTrash() const
{
	if(!mModel) return FALSE;
	const LLUUID trash_id = mModel->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	return mModel->isObjectDescendentOf(mUUID, trash_id);
}
BOOL LLWearableBridgeAction::isAgentInventory() const
{
	if(!mModel) return FALSE;
	if(gInventory.getRootFolderID() == mUUID) return TRUE;
	return mModel->isObjectDescendentOf(mUUID, gInventory.getRootFolderID());
}
void LLWearableBridgeAction::wearOnAvatar()
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		if (get_is_item_worn(item->getUUID()))
		{
			LLAppearanceMgr::instance().removeItemFromAvatar(item->getUUID());
		}
		else
		{
			static LLCachedControl<bool> add(gSavedSettings, "LiruAddNotReplace");
			LLAppearanceMgr::instance().wearItemOnAvatar(item->getUUID(), true, !add);
		}
	}
}
LLInvFVBridgeAction* LLInvFVBridgeAction::createAction(LLAssetType::EType asset_type,
													   const LLUUID& uuid,
													   LLInventoryModel* model)
{
	LLInvFVBridgeAction* action = NULL;
	switch(asset_type)
	{
		case LLAssetType::AT_TEXTURE:
			action = new LLTextureBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_SOUND:
			action = new LLSoundBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_LANDMARK:
			action = new LLLandmarkBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_CALLINGCARD:
			action = new LLCallingCardBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_OBJECT:
			action = new LLObjectBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_NOTECARD:
			action = new LLNotecardBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_ANIMATION:
			action = new LLAnimationBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_GESTURE:
			action = new LLGestureBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_LSL_TEXT:
			action = new LLLSLTextBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_CLOTHING:
		case LLAssetType::AT_BODYPART:
			action = new LLWearableBridgeAction(uuid,model);
			break;
		default:
			break;
	}
	return action;
}
void LLRecentItemsFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	menuentry_vec_t disabled_items, items;
        buildContextMenuOptions(flags, items, disabled_items);
	items.erase(std::remove(items.begin(), items.end(), std::string("New Folder")), items.end());
	hide_context_entries(menu, items, disabled_items);
}
LLInvFVBridge* LLRecentInventoryBridgeBuilder::createBridge(
	LLAssetType::EType asset_type,
	LLAssetType::EType actual_asset_type,
	LLInventoryType::EType inv_type,
	LLInventoryPanel* inventory,
	LLFolderView* root,
	const LLUUID& uuid,
	U32 flags ) const
{
	LLInvFVBridge* new_listener = NULL;
	if (asset_type == LLAssetType::AT_CATEGORY
		&& actual_asset_type != LLAssetType::AT_LINK_FOLDER)
	{
		new_listener = new LLRecentItemsFolderBridge(inv_type, inventory, root, uuid);
	}
	else
	{
		new_listener = LLInventoryFolderViewModelBuilder::createBridge(asset_type,
			actual_asset_type,
			inv_type,
			inventory,
			root,
			uuid,
			flags);
	}
	return new_listener;
}
LLFolderViewGroupedItemBridge::LLFolderViewGroupedItemBridge()
{
}
void LLFolderViewGroupedItemBridge::groupFilterContextMenu(folder_view_item_deque& selected_items, LLMenuGL& menu)
{
    uuid_vec_t ids;
	menuentry_vec_t disabled_items;
    if (get_selection_item_uuids(selected_items, ids))
    {
        if (!LLAppearanceMgr::instance().canAddWearables(ids))
        {
			disabled_items.push_back(std::string("Wearable Add"));
        }
    }
	disable_context_entries_if_present(menu, disabled_items);
}
