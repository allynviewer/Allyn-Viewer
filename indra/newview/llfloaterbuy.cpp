/** 
 * @file llfloaterbuy.cpp
 * @author James Cook
 * @brief LLFloaterBuy class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#include "llfloaterbuy.h"
#include "llagent.h"
#include "llinventorymodel.h"
#include "llinventorydefines.h"
#include "llinventoryicon.h"
#include "llnotificationsutil.h"
#include "llselectmgr.h"
#include "llscrolllistctrl.h"
#include "llviewerobject.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "hippogridmanager.h"
LLFloaterBuy* LLFloaterBuy::sInstance = NULL;
LLFloaterBuy::LLFloaterBuy()
:	LLFloater(std::string("floater_buy_object"), std::string("FloaterBuyRect"), LLStringUtil::null)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_buy_object.xml");
	childDisable("object_list");
	childDisable("item_list");
	childSetAction("cancel_btn", onClickCancel, this);
	childSetAction("buy_btn", onClickBuy, this);
	setDefaultBtn("cancel_btn");
}
LLFloaterBuy::~LLFloaterBuy()
{
	sInstance = NULL;
}
void LLFloaterBuy::reset()
{
	LLScrollListCtrl* object_list = getChild<LLScrollListCtrl>("object_list");
	if (object_list) object_list->deleteAllItems();
	LLScrollListCtrl* item_list = getChild<LLScrollListCtrl>("item_list");
	if (item_list) item_list->deleteAllItems();
}
void LLFloaterBuy::show(const LLSaleInfo& sale_info)
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getRootObjectCount() != 1)
	{
		LLNotificationsUtil::add("BuyOneObjectOnly");
		return;
	}
	if (sInstance)
	{
		sInstance->reset();
	}
	else
	{
		sInstance = new LLFloaterBuy();
	}
	sInstance->open();
	sInstance->setFocus(TRUE);
	sInstance->mSaleInfo = sale_info;
	sInstance->mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
	sInstance->center();
	LLSelectNode* node = selection->getFirstRootNode();
	if (!node)
		return;
	LLUIString title;
	switch (sale_info.getSaleType())
	{
	  case LLSaleInfo::FS_ORIGINAL:
		title = sInstance->getString("title_buy_text");
		break;
	  case LLSaleInfo::FS_COPY:
	  default:
		title = sInstance->getString("title_buy_copy_text");
		break;
	}
	title.setArg("[NAME]", node->mName);
	sInstance->setTitle(title);
	LLUUID owner_id;
	std::string owner_name;
	BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);
	if (!owners_identical)
	{
		LLNotificationsUtil::add("BuyObjectOneOwner");
		return;
	}
	LLCtrlListInterface *object_list = sInstance->childGetListInterface("object_list");
	if (!object_list)
	{
		return;
	}
	LLSD row;
	std::string icon_name = LLInventoryIcon::getIconName(LLAssetType::AT_OBJECT,
									 LLInventoryType::IT_OBJECT,
									 0x0, FALSE);
	row["columns"][0]["column"] = "icon";
	row["columns"][0]["type"] = "icon";
	row["columns"][0]["value"] = icon_name;
	U32 next_owner_mask = node->mPermissions->getMaskNextOwner();
	std::string text = node->mName;
	if (!(next_owner_mask & PERM_COPY))
	{
		text.append(sInstance->getString("no_copy_text"));
	}
	if (!(next_owner_mask & PERM_MODIFY))
	{
		text.append(sInstance->getString("no_modify_text"));
	}
	if (!(next_owner_mask & PERM_TRANSFER))
	{
		text.append(sInstance->getString("no_transfer_text"));
	}
	row["columns"][1]["column"] = "text";
	row["columns"][1]["value"] = text;
	row["columns"][1]["font"] = "SANSSERIF";
	object_list->addElement(row);
	sInstance->childSetTextArg("buy_text", "[CURRENCY]", gHippoGridManager->getConnectedGrid()->getCurrencySymbol());
	sInstance->childSetTextArg("buy_text", "[AMOUNT]", llformat("%d", sale_info.getSalePrice()));
	sInstance->childSetTextArg("buy_text", "[NAME]", owner_name);
	LLViewerObject* obj = selection->getFirstRootObject();
	sInstance->registerVOInventoryListener(obj,NULL);
	sInstance->requestVOInventory();
}
void LLFloaterBuy::inventoryChanged(LLViewerObject* obj,
								 LLInventoryObject::object_list_t* inv,
								 S32 serial_num,
								 void* data)
{
	if (!obj)
	{
		LL_WARNS() << "No object in LLFloaterBuy::inventoryChanged" << LL_ENDL;
		return;
	}
	if (!inv)
	{
		LL_WARNS() << "No inventory in LLFloaterBuy::inventoryChanged"
			<< LL_ENDL;
		removeVOInventoryListener();
		return;
	}
	LLCtrlListInterface *item_list = childGetListInterface("item_list");
	if (!item_list)
	{
		removeVOInventoryListener();
		return;
	}
	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it )
	{
		LLInventoryObject* obj = (LLInventoryObject*)(*it);
		if (obj->getType() == LLAssetType::AT_CATEGORY)
			continue;
		if (obj->getType() == LLAssetType::AT_NONE)
			continue;
		LLInventoryItem* inv_item = (LLInventoryItem*)(obj);
		if (!inv_item->getPermissions().allowTransferTo(gAgent.getID()))
			continue;
		LLSD row;
		BOOL item_is_multi = FALSE;
		if ( inv_item->getFlags() & LLInventoryItemFlags::II_FLAGS_LANDMARK_VISITED
			|| inv_item->getFlags() & LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS)
		{
			item_is_multi = TRUE;
		}
		std::string icon_name = LLInventoryIcon::getIconName(inv_item->getType(),
							 inv_item->getInventoryType(),
							 inv_item->getFlags(),
							 item_is_multi);
		row["columns"][0]["column"] = "icon";
		row["columns"][0]["type"] = "icon";
		row["columns"][0]["value"] = icon_name;
		U32 next_owner_mask = inv_item->getPermissions().getMaskNextOwner();
		std::string text = obj->getName();
		if (!(next_owner_mask & PERM_COPY))
		{
			text.append(" (no copy)");
		}
		if (!(next_owner_mask & PERM_MODIFY))
		{
			text.append(" (no modify)");
		}
		if (!(next_owner_mask & PERM_TRANSFER))
		{
			text.append(" (no transfer)");
		}
		row["columns"][1]["column"] = "text";
		row["columns"][1]["value"] = text;
		row["columns"][1]["font"] = "SANSSERIF";
		item_list->addElement(row);
	}
	removeVOInventoryListener();
}
void LLFloaterBuy::onClickBuy(void*)
{
	if (!sInstance)
	{
		LL_INFOS() << "LLFloaterBuy::onClickBuy no sInstance!" << LL_ENDL;
		return;
	}
	LLUUID category_id;
	category_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT);
	LLSelectMgr::getInstance()->sendBuy(gAgent.getID(), category_id, sInstance->mSaleInfo );
	sInstance->close();
}
void LLFloaterBuy::onClickCancel(void*)
{
	if (sInstance)
	{
		sInstance->close();
	}
}
void LLFloaterBuy::onClose(bool app_quitting)
{
	mObjectSelection = NULL;
	LLFloater::onClose(app_quitting);
}
