/** 
 * @file llinventoryactions.cpp
 * @brief Implementation of the actions associated with menu items.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llavataractions.h"
#include "llfloaterperms.h"
#include "llfoldervieweventlistener.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llinventoryclipboard.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "llmakeoutfitdialog.h"
#include "llmarketplacefunctions.h"
#include "llnotificationsutil.h"
#include "llpanelmaininventory.h"
#include "llpanelobjectinventory.h"
#include "llpreview.h"
#include "lltrans.h"
#include "llvoavatarself.h"
#include "llnotifications.h"
#include "rlvactions.h"
#include "rlvcommon.h"
extern LLUUID gAgentID;
using namespace LLOldEvents;
bool contains_nocopy_items(const LLUUID& id);
namespace LLInventoryAction
{
	void callback_doToSelected(const LLSD& notification, const LLSD& response, LLFolderView* folder, const std::string& action);
	void callback_copySelected(const LLSD& notification, const LLSD& response, class LLFolderView* root, const std::string& action);
	bool doToSelected(LLFolderView* root, std::string action, BOOL user_confirm = TRUE);
	void buildMarketplaceFolders(LLFolderView* root);
	void updateMarketplaceFolders();
	std::list<LLUUID> sMarketplaceFolders;
}
typedef LLMemberListener<LLPanelObjectInventory> object_inventory_listener_t;
typedef LLMemberListener<LLPanelMainInventory> inventory_listener_t;
typedef LLMemberListener<LLInventoryPanel> inventory_panel_listener_t;
void LLInventoryAction::callback_doToSelected(const LLSD& notification, const LLSD& response, LLFolderView* folder, const std::string& action)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		doToSelected(folder, action, false);
	}
}
void LLInventoryAction::callback_copySelected(const LLSD& notification, const LLSD& response, class LLFolderView* root, const std::string& action)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		doToSelected(root, "copy_or_move_to_marketplace_listings", false);
	}
	else if (option == 1)
	{
		doToSelected(root, "copy_to_marketplace_listings", false);
	}
}
bool get_selection_object_uuids(LLFolderView *root, uuid_vec_t& ids)
{
	uuid_vec_t results;
	for(const auto& id : root->getSelectionList())
	{
		if(id.notNull())
		{
			results.push_back(id);
		}
		else
		{
			return false;
		}
	}
	{
		ids = results;
		return true;
	}
}
bool LLInventoryAction::doToSelected(LLFolderView* root, std::string action, BOOL user_confirm)
{
	if (!root)
		return true;
	auto selected_items = root->getSelectionList();
	if (user_confirm && (("delete" == action) || ("cut" == action) || ("rename" == action) || ("properties" == action) || ("task_properties" == action) || ("open" == action)))
	{
		auto set_iter = std::find_if(selected_items.begin(), selected_items.end(), boost::bind(&depth_nesting_in_marketplace, _1) >= 0);
		if (set_iter != selected_items.end())
		{
			if ("open" == action)
			{
				if (get_can_item_be_worn(*set_iter))
				{
					LLNotificationsUtil::add("AlertMerchantListingCannotWear");
					return true;
				}
			}
			else if (LLMarketplaceData::instance().isInActiveFolder(*set_iter) ||
					 LLMarketplaceData::instance().isListedAndActive(*set_iter))
			{
				if ((("cut" == action) || ("delete" == action)) && (LLMarketplaceData::instance().isListed(*set_iter) || LLMarketplaceData::instance().isVersionFolder(*set_iter)))
				{
					LLNotificationsUtil::add("ConfirmMerchantUnlist", LLSD(), LLSD(), boost::bind(&LLInventoryAction::callback_doToSelected, _1, _2, root, action));
					return true;
				}
				LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLInventoryAction::callback_doToSelected, _1, _2, root, action));
				return true;
			}
			else if (LLMarketplaceData::instance().isListed(*set_iter) && (("cut" == action) || ("delete" == action)))
			{
				LLNotificationsUtil::add("ConfirmListingCutOrDelete", LLSD(), LLSD(), boost::bind(&LLInventoryAction::callback_doToSelected, _1, _2, root, action));
				return true;
			}
		}
	}
	if (("copy_to_marketplace_listings" == action))
	{
		auto set_iter = selected_items.begin();
		if (contains_nocopy_items(*set_iter))
		{
			LLNotificationsUtil::add("ConfirmCopyToMarketplace", LLSD(), LLSD(), boost::bind(&LLInventoryAction::callback_copySelected, _1, _2, root, action));
			return true;
		}
	}
	buildMarketplaceFolders(root);
	LLInventoryModel* model = &gInventory;
	if ("rename" == action)
	{
		root->startRenamingSelectedItem();
		updateMarketplaceFolders();
		return true;
	}
	if ("delete" == action)
	{
		root->removeSelectedItems();
		updateMarketplaceFolders();
		return true;
	}
	if ("copy" == action || "cut" == action)
	{
		LLInventoryClipboard::instance().reset();
	}
	LLMultiFloater* multi_floaterp = NULL;
	if (("task_open" == action  || "open" == action) && selected_items.size() > 1)
	{
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		multi_floaterp = new LLMultiPreview(LLRect(left, top, left + 300, top - 100));
		gFloaterView->addChild(multi_floaterp);
		LLFloater::setFloaterHost(multi_floaterp);
	}
	else if (("task_properties" == action || "properties" == action) && selected_items.size() > 1)
	{
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		multi_floaterp = new LLMultiProperties(LLRect(left, top, left + 100, top - 100));
		gFloaterView->addChild(multi_floaterp);
		LLFloater::setFloaterHost(multi_floaterp);
	}
	uuid_vec_t object_uuids_to_remove;
	if (isRemoveAction(action) && get_selection_object_uuids(root, object_uuids_to_remove))
	{
		LLAppearanceMgr::instance().removeItemsFromAvatar(object_uuids_to_remove);
	}
	else
	{
		for (const auto& id : selected_items)
		{
			LLFolderViewItem* folder_item = root->getItemByID(id);
			if(!folder_item) continue;
			LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getListener();
			if(!bridge) continue;
			bridge->performAction(model, action);
		}
	}
	LLFloater::setFloaterHost(NULL);
	if (multi_floaterp)
	{
		multi_floaterp->open();
	}
	return true;
}
void LLInventoryAction::buildMarketplaceFolders(LLFolderView* root)
{
	sMarketplaceFolders.clear();
	const LLUUID& marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	if (marketplacelistings_id.isNull())
	{
		return;
	}
	for (const auto& item : root->getSelectionList())
	{
		const LLInventoryObject* obj(gInventory.getObject(item));
		if (!obj) continue;
		if (gInventory.isObjectDescendentOf(obj->getParentUUID(), marketplacelistings_id))
		{
			const LLUUID& parent_id = obj->getParentUUID();
			if (parent_id != marketplacelistings_id)
			{
				sMarketplaceFolders.push_back(parent_id);
			}
			const LLUUID& curr_id = obj->getUUID();
			if (curr_id != marketplacelistings_id)
			{
				sMarketplaceFolders.push_back(curr_id);
			}
		}
	}
	sMarketplaceFolders.sort();
	sMarketplaceFolders.unique();
}
void LLInventoryAction::updateMarketplaceFolders()
{
	while (!sMarketplaceFolders.empty())
	{
		update_marketplace_category(sMarketplaceFolders.back());
		sMarketplaceFolders.pop_back();
	}
}
class LLDoToSelectedPanel : public object_inventory_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLPanelObjectInventory *panel = mPtr;
		LLFolderView* folder = panel->getRootFolder();
		if(!folder) return true;
		return LLInventoryAction::doToSelected(folder, userdata.asString());
	}
};
class LLDoToSelectedFloater : public inventory_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLInventoryPanel *panel = mPtr->getPanel();
		LLFolderView* folder = panel->getRootFolder();
		if(!folder) return true;
		return LLInventoryAction::doToSelected(folder, userdata.asString());
	}
};
class LLDoToSelected : public inventory_panel_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLInventoryPanel *panel = mPtr;
		LLFolderView* folder = panel->getRootFolder();
		if(!folder) return true;
		return LLInventoryAction::doToSelected(folder, userdata.asString());
	}
};
class LLNewWindow : public inventory_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLRect rect(gSavedSettings.getRect("FloaterInventoryRect"));
		S32 left = 0 , top = 0;
		gFloaterView->getNewFloaterPosition(&left, &top);
		rect.setLeftTopAndSize(left, top, rect.getWidth(), rect.getHeight());
		LLPanelMainInventory* iv = new LLPanelMainInventory(std::string("Inventory"),
												rect,
												mPtr->getActivePanel()->getModel());
		iv->getActivePanel()->setFilterTypes(mPtr->getActivePanel()->getFilterObjectTypes());
		iv->getActivePanel()->setFilterSubString(mPtr->getActivePanel()->getFilterSubString());
		iv->open();
		gFloaterView->adjustToFitScreen(iv, FALSE);
		return true;
	}
};
void do_create(LLInventoryModel *model, LLInventoryPanel *ptr, const LLSD& sdtype, LLFolderBridge *self = NULL)
{
	const std::string& type = sdtype.asString();
	LL_INFOS() << "self=0x" << std::hex << self << std::dec << LL_ENDL;
	if ("category" == type)
	{
		LLUUID category;
		if (self)
		{
			category = model->createNewCategory(self->getUUID(), LLFolderType::FT_NONE, LLStringUtil::null);
		}
		else
		{
			category = model->createNewCategory(gInventory.getRootFolderID(),
												LLFolderType::FT_NONE, LLStringUtil::null);
		}
		model->notifyObservers();
		bool autoselected = ptr->getRootFolder()->getAutoSelectOverride();
		ptr->getRootFolder()->setAutoSelectOverride(true);
		ptr->setSelection(category, TRUE);
		ptr->getRootFolder()->setAutoSelectOverride(autoselected);
	}
	else if ("lsl" == type)
	{
		LLUUID parent_id = self ? self->getUUID() : model->findCategoryUUIDForType(LLFolderType::FT_LSL_TEXT);
		ptr->createNewItem(LLTrans::getString("New Script"),
							parent_id,
							LLAssetType::AT_LSL_TEXT,
							LLInventoryType::IT_LSL,
							LLFloaterPerms::getNextOwnerPerms("Scripts"));
	}
	else if ("notecard" == type)
	{
		LLUUID parent_id = self ? self->getUUID() : model->findCategoryUUIDForType(LLFolderType::FT_NOTECARD);
		ptr->createNewItem(LLTrans::getString("New Note"),
							parent_id,
							LLAssetType::AT_NOTECARD,
							LLInventoryType::IT_NOTECARD,
							LLFloaterPerms::getNextOwnerPerms("Notecards"));
	}
	else if ("gesture" == type)
	{
		LLUUID parent_id = self ? self->getUUID() : model->findCategoryUUIDForType(LLFolderType::FT_GESTURE);
		ptr->createNewItem(LLTrans::getString("New Gesture"),
							parent_id,
							LLAssetType::AT_GESTURE,
							LLInventoryType::IT_GESTURE,
							LLFloaterPerms::getNextOwnerPerms("Gestures"));
	}
	else if ("outfit" == type || ("update outfit" == type && !LLAppearanceMgr::getInstance()->updateBaseOutfit()))
	{
		new LLMakeOutfitDialog(false);
		return;
	}
	else
	{
		LLAgentWearables::createWearable(LLWearableType::typeNameToType(type), false, self ? self->getUUID() : LLUUID::null);
	}
	ptr->getRootFolder()->setNeedsAutoRename(TRUE);
}
struct LLSetSortBy : public inventory_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		std::string sort_field = userdata.asString();
		if (sort_field == "name")
		{
			U32 order = mPtr->getActivePanel()->getSortOrder();
			mPtr->getActivePanel()->setSortOrder( order & ~LLInventoryFilter::SO_DATE );
			mPtr->getControl("Inventory.SortByName")->setValue( TRUE );
			mPtr->getControl("Inventory.SortByDate")->setValue( FALSE );
		}
		else if (sort_field == "date")
		{
			U32 order = mPtr->getActivePanel()->getSortOrder();
			mPtr->getActivePanel()->setSortOrder( order | LLInventoryFilter::SO_DATE );
			mPtr->getControl("Inventory.SortByName")->setValue( FALSE );
			mPtr->getControl("Inventory.SortByDate")->setValue( TRUE );
		}
		else if (sort_field == "foldersalwaysbyname")
		{
			U32 order = mPtr->getActivePanel()->getSortOrder();
			if ( order & LLInventoryFilter::SO_FOLDERS_BY_NAME )
			{
				order &= ~LLInventoryFilter::SO_FOLDERS_BY_NAME;
				mPtr->getControl("Inventory.FoldersAlwaysByName")->setValue( FALSE );
			}
			else
			{
				order |= LLInventoryFilter::SO_FOLDERS_BY_NAME;
				mPtr->getControl("Inventory.FoldersAlwaysByName")->setValue( TRUE );
			}
			mPtr->getActivePanel()->setSortOrder( order );
		}
		else if (sort_field == "systemfolderstotop")
		{
			U32 order = mPtr->getActivePanel()->getSortOrder();
			if ( order & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP )
			{
				order &= ~LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;
				mPtr->getControl("Inventory.SystemFoldersToTop")->setValue( FALSE );
			}
			else
			{
				order |= LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;
				mPtr->getControl("Inventory.SystemFoldersToTop")->setValue( TRUE );
			}
			mPtr->getActivePanel()->setSortOrder( order );
		}
		return true;
	}
};
struct LLSetSearchType : public inventory_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		U32 flags = mPtr->getActivePanel()->getRootFolder()->toggleSearchType(userdata.asString());
		mPtr->getControl("Inventory.SearchName")->setValue((BOOL)(flags & 1));
		mPtr->getControl("Inventory.SearchDesc")->setValue((BOOL)(flags & 2));
		mPtr->getControl("Inventory.SearchCreator")->setValue((BOOL)(flags & 4));
		return true;
	}
};
struct LLBeginIMSession : public inventory_panel_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLInventoryPanel *panel = mPtr;
		LLInventoryModel* model = panel->getModel();
		if(!model) return true;
		std::string name;
		static int session_num = 1;
		uuid_vec_t members;
		EInstantMessage type = IM_SESSION_CONFERENCE_START;
		bool fRlvCanStartIM = true;
		for (const auto& item : panel->getRootFolder()->getSelectionList())
		{
			LLFolderViewItem* folder_item = panel->getRootFolder()->getItemByID(item);
			if(folder_item)
			{
				LLFolderViewEventListener* fve_listener = folder_item->getListener();
				if (fve_listener && (fve_listener->getInventoryType() == LLInventoryType::IT_CATEGORY))
				{
					LLFolderBridge* bridge = (LLFolderBridge*)folder_item->getListener();
					if(!bridge) return true;
					LLViewerInventoryCategory* cat = bridge->getCategory();
					if(!cat) return true;
					name = cat->getName();
					LLUniqueBuddyCollector is_buddy;
					LLInventoryModel::cat_array_t cat_array;
					LLInventoryModel::item_array_t item_array;
					model->collectDescendentsIf(bridge->getUUID(),
												cat_array,
												item_array,
												LLInventoryModel::EXCLUDE_TRASH,
												is_buddy);
					S32 count = item_array.size();
					if(count > 0)
					{
						gIMMgr->setFloaterOpen(TRUE);
						LLAvatarTracker& at = LLAvatarTracker::instance();
						LLUUID id;
						for(S32 i = 0; i < count; ++i)
						{
							id = item_array.at(i)->getCreatorUUID();
							if(at.isBuddyOnline(id))
							{
								fRlvCanStartIM &= RlvActions::canStartIM(id);
								members.push_back(id);
							}
						}
					}
				}
				else
				{
					LLFolderViewItem* folder_item = panel->getRootFolder()->getItemByID(item);
					if(!folder_item) return true;
					LLInvFVBridge* listenerp = (LLInvFVBridge*)folder_item->getListener();
					if (listenerp->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
					{
						LLInventoryItem* inv_item = gInventory.getItem(listenerp->getUUID());
						if (inv_item)
						{
							LLAvatarTracker& at = LLAvatarTracker::instance();
							LLUUID id = inv_item->getCreatorUUID();
							if(at.isBuddyOnline(id))
							{
								fRlvCanStartIM &= RlvActions::canStartIM(id);
								members.push_back(id);
							}
						}
					}
				}
			}
		}
		if (!fRlvCanStartIM)
		{
			make_ui_sound("UISndIvalidOp");
			RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTCONF);
			return true;
		}
		if (name.empty())
		{
			name = llformat("Session %d", session_num++);
		}
		gIMMgr->addSession(
			name,
			type,
			members[0],
			members);
		return true;
	}
};
struct LLAttachObject : public inventory_panel_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLInventoryPanel *panel = mPtr;
		LLFolderView* folder = panel->getRootFolder();
		if(!folder) return true;
		auto selected_items = folder->getSelectionList();
		LLUUID id = *selected_items.begin();
		std::string joint_name = userdata.asString();
		LLViewerJointAttachment* attachmentp = NULL;
		for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
			 iter != gAgentAvatarp->mAttachmentPoints.end(); )
		{
			LLVOAvatar::attachment_map_t::iterator curiter = iter++;
			LLViewerJointAttachment* attachment = curiter->second;
			if (attachment->getName() == joint_name)
			{
				attachmentp = attachment;
				break;
			}
		}
		if (!attachmentp)
		{
			return true;
		}
		if (LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getLinkedItem(id))
		{
			if(gInventory.isObjectDescendentOf(id, gInventory.getRootFolderID()))
			{
				rez_attachment(item, attachmentp);
			}
			else if(item->isFinished())
			{
				LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(&rez_attachment_cb, _1, attachmentp, false));
				copy_inventory_item(
					gAgentID,
					item->getPermissions().getOwner(),
					item->getUUID(),
					LLUUID::null,
					std::string(),
					cb);
			}
		}
		gFocusMgr.setKeyboardFocus(NULL);
		return true;
	}
};
void init_object_inventory_panel_actions(LLPanelObjectInventory *panel)
{
	(new LLBindMemberListener(panel, "Inventory.DoToSelected", boost::bind(&LLInventoryAction::doToSelected, boost::bind(&LLPanelObjectInventory::getRootFolder, panel), _2, true)));
}
void init_inventory_actions(LLPanelMainInventory *floater)
{
	(new LLBindMemberListener(floater, "Inventory.DoToSelected", boost::bind(&LLInventoryAction::doToSelected, boost::bind(&LLPanelMainInventory::getRootFolder, floater), _2, true)));
	(new LLBindMemberListener(floater, "Inventory.CloseAllFolders", boost::bind(&LLInventoryPanel::closeAllFolders, boost::bind(&LLPanelMainInventory::getPanel, floater))));
	(new LLBindMemberListener(floater, "Inventory.EmptyTrash", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "", LLFolderType::FT_TRASH)));
	(new LLBindMemberListener(floater, "Inventory.DoCreate", boost::bind(&do_create, &gInventory, boost::bind(&LLPanelMainInventory::getPanel, floater), _2, (LLFolderBridge*)0)));
	(new LLNewWindow())->registerListener(floater, "Inventory.NewWindow");
	(new LLBindMemberListener(floater, "Inventory.ShowFilters", boost::bind(&LLPanelMainInventory::toggleFindOptions, floater)));
	(new LLBindMemberListener(floater, "Inventory.ResetFilter", boost::bind(&LLPanelMainInventory::resetFilters, floater)));
	(new LLSetSortBy())->registerListener(floater, "Inventory.SetSortBy");
	(new LLSetSearchType())->registerListener(floater, "Inventory.SetSearchType");
}
void init_inventory_panel_actions(LLInventoryPanel *panel)
{
	(new LLBindMemberListener(panel, "Inventory.DoToSelected", boost::bind(&LLInventoryAction::doToSelected, boost::bind(&LLInventoryPanel::getRootFolder, panel), _2, true)));
	(new LLAttachObject())->registerListener(panel, "Inventory.AttachObject");
	(new LLBindMemberListener(panel, "Inventory.CloseAllFolders", boost::bind(&LLInventoryPanel::closeAllFolders, panel)));
	(new LLBindMemberListener(panel, "Inventory.EmptyTrash", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyTrash", LLFolderType::FT_TRASH)));
	(new LLBindMemberListener(panel, "Inventory.EmptyLostAndFound", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyLostAndFound", LLFolderType::FT_LOST_AND_FOUND)));
	(new LLBindMemberListener(panel, "Inventory.DoCreate", boost::bind(&do_create, &gInventory, panel, _2, boost::bind(&LLHandle<LLFolderBridge>::get, boost::cref(LLFolderBridge::sSelf)))));
	(new LLBeginIMSession())->registerListener(panel, "Inventory.BeginIMSession");
	(new LLBindMemberListener(panel, "Inventory.Share", boost::bind(&LLAvatarActions::shareWithAvatars, panel)));
}
