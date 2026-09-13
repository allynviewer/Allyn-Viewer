/** 
 * @file llpanelpermissions.cpp
 * @brief LLPanelPermissions class implementation
 * This class represents the panel in the build view for
 * viewing/editing object names, owners, permissions, etc.
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
#include "llviewerprecompiledheaders.h"
#include "llpanelpermissions.h"
#include "llpermissions.h"
#include "llclickaction.h"
#include "llfocusmgr.h"
#include "llnotificationsutil.h"
#include "lltrans.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llviewerobject.h"
#include "llselectmgr.h"
#include "llagent.h"
#include "llavataractions.h"
#include "llfloatergroups.h"
#include "llgroupactions.h"
#include "lllineeditor.h"
#include "llradiogroup.h"
#include "llcombobox.h"
#include "lldbstrings.h"
#include "llnamebox.h"
#include "lluictrlfactory.h"
#include "roles_constants.h"
#include "llinventoryfunctions.h"
#include "lfsimfeaturehandler.h"
#include "hippogridmanager.h"
bool can_set_export(const U32& base, const U32& own, const U32& next)
{
	return base & PERM_EXPORT && own & PERM_EXPORT && (next & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED;
}
bool perms_allow_export(const LLPermissions& perms)
{
	return perms.getMaskBase() & PERM_EXPORT && perms.getMaskEveryone() & PERM_EXPORT;
}
bool is_asset_exportable(const LLUUID& asset_id)
{
	if (asset_id.isNull()) return true;
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLAssetIDMatches asset_id_matches(asset_id);
	gInventory.collectDescendentsIf(LLUUID::null, cats, items, true, asset_id_matches, false);
	for (U32 i = 0; i < items.size(); ++i)
	{
		if (perms_allow_export(items[i]->getPermissions())) return true;
	}
	return false;
}
LLPanelPermissions::LLPanelPermissions(const std::string& title) :
	LLPanel(title)
{
	setMouseOpaque(FALSE);
}
BOOL LLPanelPermissions::postBuild()
{
	childSetCommitCallback("Object Name",LLPanelPermissions::onCommitName,this);
	getChild<LLLineEditor>("Object Name")->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
	childSetCommitCallback("Object Description",LLPanelPermissions::onCommitDesc,this);
	getChild<LLLineEditor>("Object Description")->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
	getChild<LLUICtrl>("button set group")->setCommitCallback(boost::bind(&LLPanelPermissions::onClickGroup,this));
	childSetCommitCallback("checkbox share with group",LLPanelPermissions::onCommitGroupShare,this);
	childSetAction("button deed",LLPanelPermissions::onClickDeedToGroup,this);
	getChild<LLUICtrl>("button cpy_key")->setCommitCallback(boost::bind(LLPanelPermissions::onClickCopyObjKey));
	childSetCommitCallback("checkbox allow everyone move",LLPanelPermissions::onCommitEveryoneMove,this);
	childSetCommitCallback("checkbox allow everyone copy",LLPanelPermissions::onCommitEveryoneCopy,this);
	childSetCommitCallback("checkbox for sale",LLPanelPermissions::onCommitSaleInfo,this);
	childSetCommitCallback("sale type",LLPanelPermissions::onCommitSaleType,this);
	childSetCommitCallback("Edit Cost",LLPanelPermissions::onCommitSaleInfo,this);
	getChild<LLLineEditor>("Edit Cost")->setPrevalidate(&LLLineEditor::prevalidateNonNegativeS32);
	childSetCommitCallback("checkbox next owner can modify",LLPanelPermissions::onCommitNextOwnerModify,this);
	childSetCommitCallback("checkbox next owner can copy",LLPanelPermissions::onCommitNextOwnerCopy,this);
	childSetCommitCallback("checkbox next owner can transfer",LLPanelPermissions::onCommitNextOwnerTransfer,this);
	childSetCommitCallback("clickaction",LLPanelPermissions::onCommitClickAction,this);
	childSetCommitCallback("search_check",LLPanelPermissions::onCommitIncludeInSearch,this);
	mLabelGroupName = getChild<LLNameBox>("Group Name Proxy");
	if (!gHippoGridManager->getCurrentGrid()->isSecondLife())
	{
		getChild<LLUICtrl>("checkbox allow export")->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitExport, this, _2));
		LFSimFeatureHandler::instance().setSupportsExportCallback(boost::bind(&LLPanelPermissions::refresh, this));
	}
	return TRUE;
}
LLPanelPermissions::~LLPanelPermissions()
{
}
void LLPanelPermissions::handleVisibilityChange(BOOL new_visibility)
{
	if (new_visibility)
		refresh();
	LLPanel::handleVisibilityChange(new_visibility);
}
void LLPanelPermissions::disableAll()
{
	getChildView("perm_modify")->setEnabled(FALSE);
	getChild<LLUICtrl>("perm_modify")->setValue(LLStringUtil::null);
	getChildView("pathfinding_attributes_value")->setEnabled(FALSE);
	getChild<LLUICtrl>("pathfinding_attributes_value")->setValue(LLStringUtil::null);
	getChildView("Creator:")->setEnabled(FALSE);
	if (auto view = getChildView("Creator Name"))
	{
		view->setValue(LLUUID::null);
		view->setEnabled(FALSE);
	}
	getChildView("Owner:")->setEnabled(FALSE);
	if (auto view = getChildView("Owner Name"))
	{
		view->setValue(LLUUID::null);
		view->setEnabled(FALSE);
	}
	getChildView("Last Owner:")->setEnabled(FALSE);
	if (auto view = getChildView("Last Owner Name"))
	{
		view->setValue(LLUUID::null);
		view->setEnabled(FALSE);
	}
	getChildView("Group:")->setEnabled(FALSE);
	if (mLabelGroupName)
	{
		mLabelGroupName->setNameID(LLUUID::null, LFIDBearer::GROUP);
		mLabelGroupName->setEnabled(FALSE);
	}
	getChildView("button set group")->setEnabled(FALSE);
	LLLineEditor* ed = getChild<LLLineEditor>("Object Name");
	ed->setValue(LLStringUtil::null);
	ed->setEnabled(FALSE);
	ed->setLabel(LLStringUtil::null);
	getChildView("Name:")->setEnabled(FALSE);
	getChildView("Description:")->setEnabled(FALSE);
	ed = getChild<LLLineEditor>("Object Description");
	ed->setEnabled(FALSE);
	ed->setValue(LLStringUtil::null);
	ed->setLabel(LLStringUtil::null);
	getChildView("Permissions:")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox share with group")->setValue(FALSE);
	getChildView("checkbox share with group")->setEnabled(FALSE);
	getChildView("button deed")->setEnabled(FALSE);
	getChildView("text anyone can")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox allow everyone move")->setValue(FALSE);
	getChildView("checkbox allow everyone move")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(FALSE);
	getChildView("checkbox allow everyone copy")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox allow export")->setValue(FALSE);
	getChildView("checkbox allow export")->setEnabled(FALSE);
	getChildView("Next owner can:")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox next owner can modify")->setValue(FALSE);
	getChildView("checkbox next owner can modify")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox next owner can copy")->setValue(FALSE);
	getChildView("checkbox next owner can copy")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(FALSE);
	getChildView("checkbox next owner can transfer")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox for sale")->setValue(FALSE);
	getChildView("checkbox for sale")->setEnabled(FALSE);
	getChild<LLUICtrl>("search_check")->setValue(FALSE);
	getChildView("search_check")->setEnabled(FALSE);
	LLRadioGroup*	RadioSaleType = getChild<LLRadioGroup>("sale type");
	if(RadioSaleType)
	{
		RadioSaleType->setSelectedIndex(-1);
		RadioSaleType->setEnabled(FALSE);
	}
	getChildView("Cost")->setEnabled(FALSE);
	getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default"));
	getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
	getChildView("Edit Cost")->setEnabled(FALSE);
	getChildView("label click action")->setEnabled(FALSE);
	LLComboBox*	combo_click_action = getChild<LLComboBox>("clickaction");
	if (combo_click_action)
	{
		combo_click_action->setEnabled(FALSE);
		combo_click_action->clear();
	}
	getChildView("B:")->setVisible(								FALSE);
	getChildView("G:")->setVisible(								FALSE);
	getChildView("E:")->setVisible(								FALSE);
	getChildView("N:")->setVisible(								FALSE);
	getChildView("F:")->setVisible(								FALSE);
}
void LLPanelPermissions::refresh()
{
	LLStringUtil::format_map_t argsCurrency;
	argsCurrency["[CURRENCY]"] = gHippoGridManager->getConnectedGrid()->getCurrencySymbol();
	LLButton*	BtnDeedToGroup = getChild<LLButton>("button deed");
	if(BtnDeedToGroup)
	{
		std::string deedText;
		if (gSavedSettings.getWarning("DeedObject"))
		{
			deedText = getString("text deed continued");
		}
		else
		{
			deedText = getString("text deed");
		}
		BtnDeedToGroup->setLabelSelected(deedText);
		BtnDeedToGroup->setLabelUnselected(deedText);
	}
	BOOL root_selected = TRUE;
	LLSelectNode* nodep = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
	S32 object_count = LLSelectMgr::getInstance()->getSelection()->getRootObjectCount();
	if(!nodep || 0 == object_count)
	{
		nodep = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
		object_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
		root_selected = FALSE;
	}
	LLViewerObject* objectp = NULL;
	if(nodep) objectp = nodep->getObject();
	if(!nodep || !objectp)
	{
		disableAll();
		return;
	}
	const BOOL is_one_object = (object_count == 1);
	BOOL is_perm_modify = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
							&& LLSelectMgr::getInstance()->selectGetRootsModify())
							|| LLSelectMgr::getInstance()->selectGetModify();
	BOOL is_nonpermanent_enforced = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
						   && LLSelectMgr::getInstance()->selectGetRootsNonPermanentEnforced())
		|| LLSelectMgr::getInstance()->selectGetNonPermanentEnforced();
	const LLFocusableElement* keyboard_focus_view = gFocusMgr.getKeyboardFocus();
	S32 string_index = 0;
	static std::string MODIFY_INFO_STRINGS[] =
	{
		getString("text modify info 1"),
		getString("text modify info 2"),
		getString("text modify info 3"),
		getString("text modify info 4"),
		getString("text modify info 5"),
		getString("text modify info 6")
	};
	if(!is_perm_modify)
	{
		string_index += 2;
	}
	else if (!is_nonpermanent_enforced)
	{
		string_index += 4;
	}
	if(!is_one_object)
	{
		++string_index;
	}
	getChildView("perm_modify")->setEnabled(TRUE);
	getChild<LLUICtrl>("perm_modify")->setValue(MODIFY_INFO_STRINGS[string_index]);
	std::string pfAttrName;
	if ((LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
		&& LLSelectMgr::getInstance()->selectGetRootsNonPathfinding())
		|| LLSelectMgr::getInstance()->selectGetNonPathfinding())
	{
		pfAttrName = "Pathfinding_Object_Attr_None";
	}
	else if ((LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
		&& LLSelectMgr::getInstance()->selectGetRootsPermanent())
		|| LLSelectMgr::getInstance()->selectGetPermanent())
	{
		pfAttrName = "Pathfinding_Object_Attr_Permanent";
	}
	else if ((LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
		&& LLSelectMgr::getInstance()->selectGetRootsCharacter())
		|| LLSelectMgr::getInstance()->selectGetCharacter())
	{
		pfAttrName = "Pathfinding_Object_Attr_Character";
	}
	else
	{
		pfAttrName = "Pathfinding_Object_Attr_MultiSelect";
	}
	getChildView("pathfinding_attributes_value")->setEnabled(TRUE);
	getChild<LLUICtrl>("pathfinding_attributes_value")->setValue(LLTrans::getString(pfAttrName));
	getChildView("Permissions:")->setEnabled(TRUE);
	getChildView("Creator:")->setEnabled(TRUE);
	getChildView("Owner:")->setEnabled(TRUE);
	getChildView("Last Owner:")->setEnabled(TRUE);
	static const auto none_str = LLTrans::getString("GroupNameNone");
	std::string owner_app_link(none_str);
	if (auto view = getChild<LLTextBox>("Creator Name"))
	{
		if (LLSelectMgr::getInstance()->selectGetCreator(mCreatorID, owner_app_link))
		{
			view->setValue(mCreatorID);
		}
		else
		{
			view->setValue(LLUUID::null);
			view->setText(owner_app_link);
		}
		view->setEnabled(true);
	}
	owner_app_link = none_str;
	const BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(mOwnerID, owner_app_link);
	if (auto view = getChild<LLTextBox>("Owner Name"))
	{
		if (owners_identical)
		{
			view->setValue(mOwnerID);
		}
		else
		{
			view->setValue(LLUUID::null);
			view->setText(owner_app_link);
		}
		view->setEnabled(true);
	}
	if (auto view = getChild<LLTextBox>("Last Owner Name"))
	{
		owner_app_link = none_str;
		if (LLSelectMgr::getInstance()->selectGetLastOwner(mLastOwnerID, owner_app_link))
		{
			view->setValue(mLastOwnerID);
		}
		else
		{
			view->setValue(LLUUID::null);
			view->setText(owner_app_link);
		}
		view->setEnabled(true);
	}
	getChildView("Group:")->setEnabled(TRUE);
	LLUUID group_id;
	BOOL groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
	if (groups_identical)
	{
		if(mLabelGroupName)
		{
			mLabelGroupName->setNameID(group_id, LFIDBearer::GROUP);
			mLabelGroupName->setEnabled(TRUE);
		}
	}
	else
	{
		if(mLabelGroupName)
		{
			mLabelGroupName->setNameID(LLUUID::null, LFIDBearer::GROUP);
			mLabelGroupName->setNameText();
			mLabelGroupName->setEnabled(FALSE);
		}
	}
	getChildView("button set group")->setEnabled(root_selected && owners_identical && (mOwnerID == gAgent.getID()) && is_nonpermanent_enforced);
	getChildView("Name:")->setEnabled(TRUE);
	LLLineEditor* LineEditorObjectName = getChild<LLLineEditor>("Object Name");
	getChildView("Description:")->setEnabled(TRUE);
	LLLineEditor* LineEditorObjectDesc = getChild<LLLineEditor>("Object Description");
	if(is_one_object)
	{
		if(keyboard_focus_view != LineEditorObjectName)
		{
			LineEditorObjectName->setValue(nodep->mName);
		}
		if(LineEditorObjectDesc)
		{
			if(keyboard_focus_view != LineEditorObjectDesc)
			{
				LineEditorObjectDesc->setText(nodep->mDescription);
			}
		}
	}
	else
	{
		LineEditorObjectName->setText(LLStringUtil::null);
		LineEditorObjectDesc->setText(LLStringUtil::null);
	}
	{
		const std::string& str(object_count > 1 ? getString("multiple_objects_selected") : LLStringUtil::null);
		LineEditorObjectName->setLabel(str);
		LineEditorObjectDesc->setLabel(str);
	}
	if ( objectp->permModify() && !objectp->isPermanentEnforced())
	{
		LineEditorObjectName->setEnabled(TRUE);
		LineEditorObjectDesc->setEnabled(TRUE);
	}
	else
	{
		LineEditorObjectName->setEnabled(FALSE);
		LineEditorObjectDesc->setEnabled(FALSE);
	}
	S32 total_sale_price = 0;
	S32 individual_sale_price = 0;
	BOOL is_for_sale_mixed = FALSE;
	BOOL is_sale_price_mixed = FALSE;
	U32 num_for_sale = FALSE;
    LLSelectMgr::getInstance()->selectGetAggregateSaleInfo(num_for_sale,
										   is_for_sale_mixed,
										   is_sale_price_mixed,
										   total_sale_price,
										   individual_sale_price);
	const BOOL self_owned = (gAgent.getID() == mOwnerID);
	const BOOL group_owned = LLSelectMgr::getInstance()->selectIsGroupOwned() ;
	const BOOL public_owned = (mOwnerID.isNull() && !LLSelectMgr::getInstance()->selectIsGroupOwned());
	const BOOL can_transfer = LLSelectMgr::getInstance()->selectGetRootsTransfer();
	const BOOL can_copy = LLSelectMgr::getInstance()->selectGetRootsCopy();
	if (!owners_identical)
	{
		getChildView("Cost")->setEnabled(FALSE);
		getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
		getChildView("Edit Cost")->setEnabled(FALSE);
	}
	else if(self_owned || (group_owned && gAgent.hasPowerInGroup(group_id,GP_OBJECT_SET_SALE)))
	{
		if (num_for_sale > 1)
		{
			getChild<LLUICtrl>("Cost")->setValue(getString("Cost Per Unit"));
		}
		else
		{
			getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default"));
		}
		LLLineEditor *edit_price = getChild<LLLineEditor>("Edit Cost");
		if(keyboard_focus_view != edit_price)
		{
			if (num_for_sale > 0 && is_for_sale_mixed)
			{
				edit_price->setValue(getString("Sale Mixed"));
			}
			else if (num_for_sale > 0 && is_sale_price_mixed)
			{
				edit_price->setValue(getString("Cost Mixed"));
			}
			else
			{
				edit_price->setValue(individual_sale_price);
			}
		}
		BOOL enable_edit = (num_for_sale && can_transfer) ? !is_for_sale_mixed : FALSE;
		getChildView("Cost")->setEnabled(enable_edit);
		getChildView("Edit Cost")->setEnabled(enable_edit);
	}
	else if(!public_owned)
	{
		getChildView("Cost")->setEnabled(FALSE);
		getChildView("Edit Cost")->setEnabled(FALSE);
		if (num_for_sale)
			getChild<LLUICtrl>("Edit Cost")->setValue(llformat("%d",total_sale_price));
		else
			getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
		if (num_for_sale > 1)
			getChild<LLUICtrl>("Cost")->setValue(getString("Cost Total", argsCurrency));
		else
			getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default", argsCurrency));
	}
	else
	{
		getChildView("Cost")->setEnabled(FALSE);
		getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default", argsCurrency));
		getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
		getChildView("Edit Cost")->setEnabled(FALSE);
	}
	U32 base_mask_on = 0;
	U32 base_mask_off = 0;
	U32 owner_mask_off = 0;
	U32 owner_mask_on = 0;
	U32 group_mask_on = 0;
	U32 group_mask_off = 0;
	U32 everyone_mask_on = 0;
	U32 everyone_mask_off = 0;
	U32 next_owner_mask_on = 0;
	U32 next_owner_mask_off = 0;
	BOOL valid_base_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_BASE,
									  &base_mask_on,
									  &base_mask_off);
	LLSelectMgr::getInstance()->selectGetPerm(PERM_OWNER,
									  &owner_mask_on,
									  &owner_mask_off);
	BOOL valid_group_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_GROUP,
									  &group_mask_on,
									  &group_mask_off);
	BOOL valid_everyone_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_EVERYONE,
									  &everyone_mask_on,
									  &everyone_mask_off);
	BOOL valid_next_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_NEXT_OWNER,
									  &next_owner_mask_on,
									  &next_owner_mask_off);
	bool supports_export = LFSimFeatureHandler::instance().simSupportsExport();
	if( gSavedSettings.getBOOL("DebugPermissions") )
	{
		childSetVisible("perm_modify", false);
		if (valid_base_perms)
		{
			std::string perm_string = mask_to_string(base_mask_on);
			if (!supports_export && base_mask_on & PERM_EXPORT)
				perm_string.erase(perm_string.find_last_of("E"));
			if (U32 diff_mask = base_mask_on ^ owner_mask_on)
			{
				if (diff_mask & PERM_MOVE)
					LLStringUtil::replaceChar(perm_string, 'V', 'v');
				if (diff_mask & PERM_MODIFY)
					LLStringUtil::replaceChar(perm_string, 'M', 'm');
				if (diff_mask & PERM_COPY)
					LLStringUtil::replaceChar(perm_string, 'C', 'c');
				if (diff_mask & PERM_TRANSFER)
					LLStringUtil::replaceChar(perm_string, 'T', 't');
				if (diff_mask & PERM_EXPORT)
					LLStringUtil::replaceChar(perm_string, 'E', 'e');
			}
			getChild<LLUICtrl>("B:")->setValue("B: " + perm_string);
			getChildView("B:")->setVisible(							TRUE);
			getChild<LLUICtrl>("G:")->setValue("G: " + mask_to_string(group_mask_on));
			getChildView("G:")->setVisible(							TRUE);
			perm_string = mask_to_string(everyone_mask_on);
			if (!supports_export && everyone_mask_on & PERM_EXPORT)
				perm_string.erase(perm_string.find_last_of("E"));
			getChild<LLUICtrl>("E:")->setValue("E: " + perm_string);
			getChildView("E:")->setVisible(							TRUE);
			getChild<LLUICtrl>("N:")->setValue("N: " + mask_to_string(next_owner_mask_on));
			getChildView("N:")->setVisible(							TRUE);
		}
		U32 flag_mask = 0x0;
		if (objectp->permMove()) 		flag_mask |= PERM_MOVE;
		if (objectp->permModify()) 		flag_mask |= PERM_MODIFY;
		if (objectp->permCopy()) 		flag_mask |= PERM_COPY;
		if (objectp->permTransfer()) 	flag_mask |= PERM_TRANSFER;
		getChild<LLUICtrl>("F:")->setValue("F:" + mask_to_string(flag_mask));
		getChildView("F:")->setVisible(								TRUE);
	}
	else
	{
		childSetVisible("perm_modify", true);
		getChildView("B:")->setVisible(								FALSE);
		getChildView("G:")->setVisible(								FALSE);
		getChildView("E:")->setVisible(								FALSE);
		getChildView("N:")->setVisible(								FALSE);
		getChildView("F:")->setVisible(								FALSE);
	}
	BOOL has_change_perm_ability = FALSE;
	BOOL has_change_sale_ability = FALSE;
	if (valid_base_perms && is_nonpermanent_enforced &&
		(self_owned || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_MANIPULATE))))
	{
		has_change_perm_ability = TRUE;
	}
	if (valid_base_perms && is_nonpermanent_enforced &&
	   (self_owned || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_SET_SALE))))
	{
		has_change_sale_ability = TRUE;
	}
	if (!has_change_perm_ability && !has_change_sale_ability && !root_selected)
	{
		getChild<LLUICtrl>("perm_modify")->setValue(getString("text modify warning"));
	}
	if (has_change_perm_ability)
	{
		getChildView("checkbox share with group")->setEnabled(TRUE);
		getChildView("text anyone can")->setEnabled(true);
		getChildView("checkbox allow everyone move")->setEnabled(owner_mask_on & PERM_MOVE);
		getChildView("checkbox allow everyone copy")->setEnabled(owner_mask_on & PERM_COPY && owner_mask_on & PERM_TRANSFER);
	}
	else
	{
		getChildView("checkbox share with group")->setEnabled(FALSE);
		getChildView("text anyone can")->setEnabled(false);
		getChildView("checkbox allow everyone move")->setEnabled(FALSE);
		getChildView("checkbox allow everyone copy")->setEnabled(FALSE);
	}
	if (supports_export && self_owned && mCreatorID == mOwnerID && can_set_export(base_mask_on, owner_mask_on, next_owner_mask_on))
	{
		bool can_export = true;
		LLInventoryObject::object_list_t objects;
		objectp->getInventoryContents(objects);
		for (LLInventoryObject::object_list_t::iterator i = objects.begin(); can_export && i != objects.end() ; ++i)
		{
			LLViewerInventoryItem* item = static_cast<LLViewerInventoryItem*>(i->get());
			can_export = perms_allow_export(item->getPermissions());
		}
		for (U8 i = 0; can_export && i < objectp->getNumTEs(); ++i)
			if (LLTextureEntry* texture = objectp->getTE(i))
				can_export = is_asset_exportable(texture->getID());
		getChildView("checkbox allow export")->setEnabled(can_export);
	}
	else
	{
		getChildView("checkbox allow export")->setEnabled(false);
		if (!gHippoGridManager->getCurrentGrid()->isSecondLife())
			getChildView("checkbox allow everyone copy")->setVisible(true);
	}
	if (has_change_sale_ability && (owner_mask_on & PERM_TRANSFER))
	{
		getChildView("checkbox for sale")->setEnabled(can_transfer || (!can_transfer && num_for_sale));
		getChild<LLUICtrl>("checkbox for sale")->setTentative( 				is_for_sale_mixed);
		getChildView("sale type")->setEnabled(num_for_sale && can_transfer && !is_sale_price_mixed);
		bool no_export = everyone_mask_off & PERM_EXPORT;
		getChildView("Next owner can:")->setEnabled(no_export);
		getChildView("checkbox next owner can modify")->setEnabled(no_export && base_mask_on & PERM_MODIFY);
		getChildView("checkbox next owner can copy")->setEnabled(no_export && base_mask_on & PERM_COPY);
		getChildView("checkbox next owner can transfer")->setEnabled(no_export && next_owner_mask_on & PERM_COPY);
	}
	else
	{
		getChildView("checkbox for sale")->setEnabled(FALSE);
		getChildView("sale type")->setEnabled(FALSE);
		getChildView("Next owner can:")->setEnabled(FALSE);
		getChildView("checkbox next owner can modify")->setEnabled(FALSE);
		getChildView("checkbox next owner can copy")->setEnabled(FALSE);
		getChildView("checkbox next owner can transfer")->setEnabled(FALSE);
	}
	if (valid_group_perms)
	{
		if ((group_mask_on & PERM_COPY) && (group_mask_on & PERM_MODIFY) && (group_mask_on & PERM_MOVE))
		{
			getChild<LLUICtrl>("checkbox share with group")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox share with group")->setTentative(	FALSE);
			getChildView("button deed")->setEnabled(gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
		}
		else if ((group_mask_off & PERM_COPY) && (group_mask_off & PERM_MODIFY) && (group_mask_off & PERM_MOVE))
		{
			getChild<LLUICtrl>("checkbox share with group")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox share with group")->setTentative(	FALSE);
			getChildView("button deed")->setEnabled(FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox share with group")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox share with group")->setTentative(	TRUE);
			getChildView("button deed")->setEnabled(gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (group_mask_on & PERM_MOVE) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
		}
	}
	if (valid_everyone_perms)
	{
		if (everyone_mask_on & PERM_MOVE)
		{
			getChild<LLUICtrl>("checkbox allow everyone move")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox allow everyone move")->setTentative( 	FALSE);
		}
		else if (everyone_mask_off & PERM_MOVE)
		{
			getChild<LLUICtrl>("checkbox allow everyone move")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox allow everyone move")->setTentative( 	FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox allow everyone move")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox allow everyone move")->setTentative( 	TRUE);
		}
		if (everyone_mask_on & PERM_COPY)
		{
			getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative( 	!can_copy || !can_transfer);
		}
		else if (everyone_mask_off & PERM_COPY)
		{
			getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative(	FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative(	TRUE);
		}
		if (supports_export)
		{
			if(everyone_mask_on & PERM_EXPORT)
			{
				getChild<LLUICtrl>("checkbox allow export")->setValue(TRUE);
				getChild<LLUICtrl>("checkbox allow export")->setTentative(	FALSE);
			}
			else if(everyone_mask_off & PERM_EXPORT)
			{
				getChild<LLUICtrl>("checkbox allow export")->setValue(FALSE);
				getChild<LLUICtrl>("checkbox allow export")->setTentative(	FALSE);
			}
			else
			{
				getChild<LLUICtrl>("checkbox allow export")->setValue(TRUE);
				getChild<LLUICtrl>("checkbox allow export")->setValue(	TRUE);
			}
		}
		else
		{
			childSetValue("checkbox allow export", false);
			childSetTentative("checkbox allow export", false);
		}
	}
	if (valid_next_perms)
	{
		if (next_owner_mask_on & PERM_MODIFY)
		{
			getChild<LLUICtrl>("checkbox next owner can modify")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can modify")->setTentative(	FALSE);
		}
		else if (next_owner_mask_off & PERM_MODIFY)
		{
			getChild<LLUICtrl>("checkbox next owner can modify")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox next owner can modify")->setTentative(	FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox next owner can modify")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can modify")->setTentative(	TRUE);
		}
		if (next_owner_mask_on & PERM_COPY)
		{
			getChild<LLUICtrl>("checkbox next owner can copy")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(	!can_copy);
		}
		else if (next_owner_mask_off & PERM_COPY)
		{
			getChild<LLUICtrl>("checkbox next owner can copy")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(	FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox next owner can copy")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(	TRUE);
		}
		if (next_owner_mask_on & PERM_TRANSFER)
		{
			getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can transfer")->setTentative( !can_transfer);
		}
		else if (next_owner_mask_off & PERM_TRANSFER)
		{
			getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox next owner can transfer")->setTentative( FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can transfer")->setTentative( TRUE);
		}
	}
	LLSaleInfo sale_info;
	BOOL valid_sale_info = LLSelectMgr::getInstance()->selectGetSaleInfo(sale_info);
	LLSaleInfo::EForSale sale_type = sale_info.getSaleType();
	LLRadioGroup* RadioSaleType = getChild<LLRadioGroup>("sale type");
	if(RadioSaleType)
	{
		if (valid_sale_info)
		{
			RadioSaleType->setSelectedIndex((S32)sale_type - 1);
			RadioSaleType->setTentative(FALSE);
		}
		else
		{
			RadioSaleType->setSelectedIndex((S32)LLSaleInfo::FS_COPY - 1);
			RadioSaleType->setTentative(TRUE);
		}
	}
	getChild<LLUICtrl>("checkbox for sale")->setValue((num_for_sale != 0));
	bool cannot_actually_sell = !can_transfer || (!can_copy && sale_type == LLSaleInfo::FS_COPY);
	if (cannot_actually_sell)
	{
		if (num_for_sale && has_change_sale_ability)
		{
			getChildView("checkbox for sale")->setEnabled(true);
		}
	}
	const BOOL all_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME );
	bool include_in_search;
	const BOOL all_include_in_search = LLSelectMgr::getInstance()->selectionGetIncludeInSearch(&include_in_search);
	getChildView("search_check")->setEnabled(has_change_sale_ability && all_volume);
	getChild<LLUICtrl>("search_check")->setValue(include_in_search);
	getChild<LLUICtrl>("search_check")->setTentative( 				!all_include_in_search);
	U8 click_action = 0;
	if (LLSelectMgr::getInstance()->selectionGetClickAction(&click_action))
	{
		LLComboBox*	combo_click_action = getChild<LLComboBox>("clickaction");
		if(combo_click_action)
		{
			combo_click_action->setCurrentByIndex((S32)click_action);
		}
	}
	getChildView("label click action")->setEnabled(is_perm_modify && is_nonpermanent_enforced  && all_volume);
	getChildView("clickaction")->setEnabled(is_perm_modify && is_nonpermanent_enforced && all_volume);
}
void LLPanelPermissions::onClickClaim(void*)
{
	LLSelectMgr::getInstance()->sendOwner(gAgent.getID(), gAgent.getGroupID());
}
void LLPanelPermissions::onClickRelease(void*)
{
	LLSelectMgr::getInstance()->sendOwner(LLUUID::null, LLUUID::null);
}
void LLPanelPermissions::onClickGroup()
{
	LLUUID owner_id;
	std::string name;
	BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, name);
	LLFloater* parent_floater = gFloaterView->getParentFloater(this);
	if(owners_identical && (owner_id == gAgent.getID()))
	{
		LLFloaterGroupPicker* fg = LLFloaterGroupPicker::showInstance(LLSD(gAgent.getID()));
		if (fg)
		{
			fg->setSelectGroupCallback( boost::bind(&LLPanelPermissions::cbGroupID, this, _1) );
			if (parent_floater)
			{
				LLRect new_rect = gFloaterView->findNeighboringPosition(parent_floater, fg);
				fg->setOrigin(new_rect.mLeft, new_rect.mBottom);
				parent_floater->addDependentFloater(fg);
			}
		}
	}
}
void LLPanelPermissions::cbGroupID(LLUUID group_id)
{
	if(mLabelGroupName)
	{
		mLabelGroupName->setNameID(group_id, LFIDBearer::GROUP);
	}
	LLSelectMgr::getInstance()->sendGroup(group_id);
}
bool callback_deed_to_group(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLUUID group_id;
		BOOL groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
		if(group_id.notNull() && groups_identical && (gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED)))
		{
			LLSelectMgr::getInstance()->sendOwner(LLUUID::null, group_id, FALSE);
		}
	}
	return false;
}
void LLPanelPermissions::onClickDeedToGroup(void* data)
{
	LLNotificationsUtil::add( "DeedObjectToGroup", LLSD(), LLSD(), callback_deed_to_group);
}
template <typename iterator>
std::string gather_keys(iterator iter, iterator end)
{
	std::string output;
	std::string separator = gSavedSettings.getString("AscentDataSeparator");
	for (; iter != end; ++iter)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject* object = selectNode->getObject();
		if (object)
		{
			if (!output.empty()) output.append(separator);
			output.append(object->getID().asString());
		}
	}
	return output;
}
void LLPanelPermissions::onClickCopyObjKey()
{
	bool parts(gSavedSettings.getBOOL("EditLinkedParts"));
	LLObjectSelectionHandle selection(LLSelectMgr::getInstance()->getSelection());
	std::string output = parts ? gather_keys(selection->begin(), selection->end()) : gather_keys(selection->root_begin(), selection->root_end());
	if (!output.empty()) gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(output));
}
void LLPanelPermissions::onCommitPerm(LLUICtrl *ctrl, void *data, U8 field, U32 perm)
{
	LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject();
	if(!object) return;
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	BOOL new_state = check->get();
	LLSelectMgr::getInstance()->selectionSetObjectPermissions(field, new_state, perm);
}
void LLPanelPermissions::onCommitGroupShare(LLUICtrl *ctrl, void *data)
{
	onCommitPerm(ctrl, data, PERM_GROUP, PERM_MODIFY | PERM_MOVE | PERM_COPY);
}
void LLPanelPermissions::onCommitEveryoneMove(LLUICtrl *ctrl, void *data)
{
	onCommitPerm(ctrl, data, PERM_EVERYONE, PERM_MOVE);
}
void LLPanelPermissions::onCommitEveryoneCopy(LLUICtrl *ctrl, void *data)
{
	onCommitPerm(ctrl, data, PERM_EVERYONE, PERM_COPY);
}
void LLPanelPermissions::onCommitExport(const LLSD& param)
{
	LLSelectMgr::getInstance()->selectionSetObjectPermissions(PERM_EVERYONE, param.asBoolean(), PERM_EXPORT);
}
void LLPanelPermissions::onCommitNextOwnerModify(LLUICtrl* ctrl, void* data)
{
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_MODIFY);
}
void LLPanelPermissions::onCommitNextOwnerCopy(LLUICtrl* ctrl, void* data)
{
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_COPY);
}
void LLPanelPermissions::onCommitNextOwnerTransfer(LLUICtrl* ctrl, void* data)
{
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_TRANSFER);
}
void LLPanelPermissions::onCommitName(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	LLLineEditor*	tb = self->getChild<LLLineEditor>("Object Name");
	if(tb)
	{
		LLSelectMgr::getInstance()->selectionSetObjectName(tb->getText());
	}
}
void LLPanelPermissions::onCommitDesc(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	LLLineEditor*	le = self->getChild<LLLineEditor>("Object Description");
	if(le)
	{
		LLSelectMgr::getInstance()->selectionSetObjectDescription(le->getText());
	}
}
void LLPanelPermissions::onCommitSaleInfo(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	self->setAllSaleInfo();
}
void LLPanelPermissions::onCommitSaleType(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	self->setAllSaleInfo();
}
void LLPanelPermissions::setAllSaleInfo()
{
	LL_INFOS() << "LLPanelPermissions::setAllSaleInfo()" << LL_ENDL;
	LLSaleInfo::EForSale sale_type = LLSaleInfo::FS_NOT;
	LLStringUtil::format_map_t argsCurrency;
	argsCurrency["[CURRENCY]"] = gHippoGridManager->getConnectedGrid()->getCurrencySymbol();
	LLCheckBoxCtrl *checkPurchase = getChild<LLCheckBoxCtrl>("checkbox for sale");
	if(checkPurchase && checkPurchase->get())
	{
		LLRadioGroup* RadioSaleType = getChild<LLRadioGroup>("sale type");
		if(RadioSaleType)
		{
			switch(RadioSaleType->getSelectedIndex())
			{
			case 0:
				sale_type = LLSaleInfo::FS_ORIGINAL;
				break;
			case 1:
				sale_type = LLSaleInfo::FS_COPY;
				break;
			case 2:
				sale_type = LLSaleInfo::FS_CONTENTS;
				break;
			default:
				sale_type = LLSaleInfo::FS_COPY;
				break;
			}
		}
	}
	S32 price = -1;
	LLLineEditor *editPrice = getChild<LLLineEditor>("Edit Cost");
	if (editPrice)
	{
		const std::string& editPriceString = editPrice->getText();
		if (editPriceString != getString("Cost Mixed", argsCurrency) &&
			editPriceString != getString("Sale Mixed", argsCurrency) &&
			!editPriceString.empty())
		{
			price = atoi(editPriceString.c_str());
		}
		else
		{
			price = DEFAULT_PRICE;
		}
	}
	if (price < 0)
		sale_type = LLSaleInfo::FS_NOT;
	if (sale_type == LLSaleInfo::FS_NOT)
	{
		price = DEFAULT_PRICE;
	}
	LLSaleInfo sale_info(sale_type, price);
	LLSelectMgr::getInstance()->selectionSetObjectSaleInfo(sale_info);
	if (sale_type == LLSaleInfo::FS_NOT)
	{
		U8 click_action = 0;
		LLSelectMgr::getInstance()->selectionGetClickAction(&click_action);
		if (click_action == CLICK_ACTION_BUY)
		{
			LLSelectMgr::getInstance()->selectionSetClickAction(CLICK_ACTION_TOUCH);
		}
	}
}
struct LLSelectionPayable : public LLSelectedObjectFunctor
{
	virtual bool apply(LLViewerObject* obj)
	{
		LLViewerObject* parent = (LLViewerObject*)obj->getParent();
		return (obj->flagTakesMoney()
			   || (parent && parent->flagTakesMoney()));
	}
};
void LLPanelPermissions::onCommitClickAction(LLUICtrl* ctrl, void*)
{
	LLComboBox* box = (LLComboBox*)ctrl;
	if (!box) return;
	U8 click_action = (U8)box->getCurrentIndex();
	if (click_action == CLICK_ACTION_BUY)
	{
		LLSaleInfo sale_info;
		LLSelectMgr::getInstance()->selectGetSaleInfo(sale_info);
		if (!sale_info.isForSale())
		{
			LLNotificationsUtil::add("CantSetBuyObject");
			U8 click_action = 0;
			LLSelectMgr::getInstance()->selectionGetClickAction(&click_action);
			box->setCurrentByIndex((S32)click_action);
			return;
		}
	}
	else if (click_action == CLICK_ACTION_PAY)
	{
		LLSelectionPayable payable;
		bool can_pay = LLSelectMgr::getInstance()->getSelection()->applyToObjects(&payable);
		if (!can_pay)
		{
			LLNotificationsUtil::add("ClickActionNotPayable");
		}
	}
	LLSelectMgr::getInstance()->selectionSetClickAction(click_action);
}
void LLPanelPermissions::onCommitIncludeInSearch(LLUICtrl* ctrl, void*)
{
	LLCheckBoxCtrl* box = (LLCheckBoxCtrl*)ctrl;
	llassert(box);
	LLSelectMgr::getInstance()->selectionSetIncludeInSearch(box->get());
}
