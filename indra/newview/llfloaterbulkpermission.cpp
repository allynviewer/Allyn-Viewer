/** 
 * @file llfloaterbulkpermissions.cpp
 * @author Michelle2 Zenovka
 * @brief A floater which allows task inventory item's properties to be changed on mass.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
#include "llfloaterbulkpermission.h"
#include "llfloaterperms.h"
#include "llagent.h"
#include "llchat.h"
#include "llinventorydefines.h"
#include "llviewerwindow.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llresmgr.h"
#include "llbutton.h"
#include "lldir.h"
#include "llviewerstats.h"
#include "lluictrlfactory.h"
#include "llselectmgr.h"
#include "llcheckboxctrl.h"
#include "llnotificationsutil.h"
#include "roles_constants.h"
LLFloaterBulkPermission::LLFloaterBulkPermission(const LLSD& seed)
:	LLFloater(),
	mDone(FALSE)
{
	mID.generate();
	mCommitCallbackRegistrar.add("BulkPermission.Help",	boost::bind(LLFloaterBulkPermission::onHelpBtn));
	mCommitCallbackRegistrar.add("BulkPermission.Apply",	boost::bind(&LLFloaterBulkPermission::onApplyBtn, this));
	mCommitCallbackRegistrar.add("BulkPermission.Close",	boost::bind(&LLFloaterBulkPermission::onCloseBtn, this));
	mCommitCallbackRegistrar.add("BulkPermission.CheckAll",	boost::bind(&LLFloaterBulkPermission::onCheckAll, this));
	mCommitCallbackRegistrar.add("BulkPermission.UncheckAll",	boost::bind(&LLFloaterBulkPermission::onUncheckAll, this));
	mCommitCallbackRegistrar.add("BulkPermission.CommitCopy",	boost::bind(&LLFloaterBulkPermission::onCommitCopy, this));
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_bulk_perms.xml");
	childSetEnabled("next_owner_transfer", gSavedSettings.getBOOL("BulkChangeNextOwnerCopy"));
}
void LLFloaterBulkPermission::doApply()
{
	class ModifiableGatherer : public LLSelectedNodeFunctor
	{
	public:
		ModifiableGatherer(uuid_vec_t& q) : mQueue(q) {}
		virtual bool apply(LLSelectNode* node)
		{
			if( node->allowOperationOnNode(PERM_MODIFY, GP_OBJECT_MANIPULATE) )
			{
				mQueue.push_back(node->getObject()->getID());
			}
			return true;
		}
	private:
		uuid_vec_t& mQueue;
	};
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("queue output");
	list->deleteAllItems();
	ModifiableGatherer gatherer(mObjectIDs);
	LLSelectMgr::getInstance()->getSelection()->applyToNodes(&gatherer);
	if(mObjectIDs.empty())
	{
		list->setCommentText(getString("nothing_to_modify_text"));
	}
	else
	{
		mDone = FALSE;
		if (!start())
		{
			LL_WARNS() << "Unexpected bulk permission change failure." << LL_ENDL;
		}
	}
}
void LLFloaterBulkPermission::inventoryChanged(LLViewerObject* viewer_object,
											 LLInventoryObject::object_list_t* inv,
											 S32,
											 void* q_id)
{
	removeVOInventoryListener();
	if (viewer_object && inv && (viewer_object->getID() == mCurrentObjectID) )
	{
		handleInventory(viewer_object, inv);
	}
	else
	{
		LL_WARNS() << "No inventory for " << mCurrentObjectID << LL_ENDL;
		nextObject();
	}
}
void LLFloaterBulkPermission::onApplyBtn()
{
	doApply();
}
void LLFloaterBulkPermission::onHelpBtn()
{
	LLNotificationsUtil::add("HelpBulkPermission");
}
void LLFloaterBulkPermission::onCloseBtn()
{
	onClose(false);
}
void LLFloaterBulkPermission::onCommitCopy()
{
	BOOL copyable = gSavedSettings.getBOOL("BulkChangeNextOwnerCopy");
	if(!copyable)
	{
		gSavedSettings.setBOOL("BulkChangeNextOwnerTransfer", TRUE);
	}
	LLCheckBoxCtrl* xfer = getChild<LLCheckBoxCtrl>("next_owner_transfer");
	xfer->setEnabled(copyable);
}
BOOL LLFloaterBulkPermission::start()
{
	getChild<LLScrollListCtrl>("queue output")->addSimpleElement(getString("start_text"));
	return nextObject();
}
BOOL LLFloaterBulkPermission::nextObject()
{
	S32 count;
	BOOL successful_start = FALSE;
	do
	{
		count = mObjectIDs.size();
		mCurrentObjectID.setNull();
		if(count > 0)
		{
			successful_start = popNext();
		}
	} while((mObjectIDs.size() > 0) && !successful_start);
	if(isDone() && !mDone)
	{
		getChild<LLScrollListCtrl>("queue output")->addSimpleElement(getString("done_text"));
		mDone = TRUE;
	}
	return successful_start;
}
BOOL LLFloaterBulkPermission::popNext()
{
	BOOL rv = FALSE;
	S32 count = mObjectIDs.size();
	if(mCurrentObjectID.isNull() && (count > 0))
	{
		mCurrentObjectID = mObjectIDs.at(0);
		mObjectIDs.erase(mObjectIDs.begin());
		LLViewerObject* obj = gObjectList.findObject(mCurrentObjectID);
		if(obj)
		{
			LLUUID* id = new LLUUID(mID);
			registerVOInventoryListener(obj,id);
			requestVOInventory();
			rv = TRUE;
		}
		else
		{
			LL_INFOS() <<"NULL LLViewerObject" <<LL_ENDL;
		}
	}
	return rv;
}
void LLFloaterBulkPermission::doCheckUncheckAll(BOOL check)
{
	gSavedSettings.setBOOL("BulkChangeIncludeAnimations", check);
	gSavedSettings.setBOOL("BulkChangeIncludeBodyParts" , check);
	gSavedSettings.setBOOL("BulkChangeIncludeClothing"  , check);
	gSavedSettings.setBOOL("BulkChangeIncludeGestures"  , check);
	gSavedSettings.setBOOL("BulkChangeIncludeLandmarks" , check);
	gSavedSettings.setBOOL("BulkChangeIncludeNotecards" , check);
	gSavedSettings.setBOOL("BulkChangeIncludeObjects"   , check);
	gSavedSettings.setBOOL("BulkChangeIncludeScripts"   , check);
	gSavedSettings.setBOOL("BulkChangeIncludeSounds"    , check);
	gSavedSettings.setBOOL("BulkChangeIncludeTextures"  , check);
}
void LLFloaterBulkPermission::handleInventory(LLViewerObject* viewer_obj, LLInventoryObject::object_list_t* inv)
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("queue output");
	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		LLAssetType::EType asstype = (*it)->getType();
		if(
			( asstype == LLAssetType::AT_ANIMATION && gSavedSettings.getBOOL("BulkChangeIncludeAnimations")) ||
			( asstype == LLAssetType::AT_BODYPART  && gSavedSettings.getBOOL("BulkChangeIncludeBodyParts" )) ||
			( asstype == LLAssetType::AT_CLOTHING  && gSavedSettings.getBOOL("BulkChangeIncludeClothing"  )) ||
			( asstype == LLAssetType::AT_GESTURE   && gSavedSettings.getBOOL("BulkChangeIncludeGestures"  )) ||
			( asstype == LLAssetType::AT_LANDMARK  && gSavedSettings.getBOOL("BulkChangeIncludeLandmarks" )) ||
			( asstype == LLAssetType::AT_NOTECARD  && gSavedSettings.getBOOL("BulkChangeIncludeNotecards" )) ||
			( asstype == LLAssetType::AT_OBJECT    && gSavedSettings.getBOOL("BulkChangeIncludeObjects"   )) ||
			( asstype == LLAssetType::AT_LSL_TEXT  && gSavedSettings.getBOOL("BulkChangeIncludeScripts"   )) ||
			( asstype == LLAssetType::AT_SOUND     && gSavedSettings.getBOOL("BulkChangeIncludeSounds"    )) ||
			( asstype == LLAssetType::AT_TEXTURE   && gSavedSettings.getBOOL("BulkChangeIncludeTextures"  )))
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());
			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				LLViewerInventoryItem* new_item = (LLViewerInventoryItem*)item;
				LLPermissions perm(new_item->getPermissions());
				U32 flags = new_item->getFlags();
				U32 desired_next_owner_perms = LLFloaterPerms::getNextOwnerPerms("BulkChange");
				U32 desired_everyone_perms = LLFloaterPerms::getEveryonePerms("BulkChange");
				U32 desired_group_perms = LLFloaterPerms::getGroupPerms("BulkChange");
				if((perm.getMaskNextOwner() != desired_next_owner_perms)
				   && (new_item->getType() == LLAssetType::AT_OBJECT))
				{
					flags |= LLInventoryItemFlags::II_FLAGS_OBJECT_SLAM_PERM;
				}
				if ((perm.getMaskEveryone() != desired_everyone_perms)
				    && (new_item->getType() == LLAssetType::AT_OBJECT))
				{
					flags |= LLInventoryItemFlags::II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE;
				}
				if ((perm.getMaskGroup() != desired_group_perms)
				    && (new_item->getType() == LLAssetType::AT_OBJECT))
				{
					flags |= LLInventoryItemFlags::II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP;
				}
				std::string invname;
				invname=item->getName().substr(0,item->getName().size() < 30 ? item->getName().size() : 30 );
				LLUIString status_text = getString("status_text");
				status_text.setArg("[NAME]", invname.c_str());
				if(true
					)
				{
					perm.setMaskNext(desired_next_owner_perms);
					perm.setMaskEveryone(desired_everyone_perms);
					perm.setMaskGroup(desired_group_perms);
					new_item->setPermissions(perm);
					new_item->setFlags(flags);
					updateInventory(object,new_item,TASK_INVENTORY_ITEM_KEY,FALSE);
					status_text.setArg("[STATUS]", "");
				}
#if 0
				else
				{
					status_text.setArg("[STATUS]", "");
				}
#endif
				list->addSimpleElement(status_text.getString());
			}
		}
	}
	nextObject();
}
void LLFloaterBulkPermission::updateInventory(LLViewerObject* object, LLViewerInventoryItem* item, U8 key, bool is_new)
{
	LLPointer<LLViewerInventoryItem> task_item =
		new LLViewerInventoryItem(item->getUUID(), mID, item->getPermissions(),
								  item->getAssetUUID(), item->getType(),
								  item->getInventoryType(),
								  item->getName(), item->getDescription(),
								  item->getSaleInfo(),
								  item->getFlags(),
								  item->getCreationDate());
	task_item->setTransactionID(item->getTransactionID());
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateTaskInventory);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_UpdateData);
	msg->addU32Fast(_PREHASH_LocalID, object->mLocalID);
	msg->addU8Fast(_PREHASH_Key, key);
	msg->nextBlockFast(_PREHASH_InventoryData);
	task_item->packMessage(msg);
	msg->sendReliable(object->getRegion()->getHost());
}
