/* Copyright (c) 2009
 * Modifications Copyright (C) 2026 - Allyn Viewer
 *
 * Modular Systems Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems Ltd nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS LTD AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Modified, debugged, optimized and improved by Henri Beauchamp Feb 2010.
 */
#include "llviewerprecompiledheaders.h"
#include "jcfloaterareasearch.h"
#include "llfiltereditor.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lluictrlfactory.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "lltracker.h"
#include "llviewerobjectlist.h"
#include "llviewermessage.h"
#include "llviewerregion.h"
#include "llworld.h"
const std::string request_string = "JCFloaterAreaSearch::Requested_\xF8\xA7\xB5";
const F32 min_refresh_interval = 0.25f;
const S32 max_property_requests = 30;
JCFloaterAreaSearch::JCFloaterAreaSearch(const LLSD& data) :
	LLFloater(),
	mCounterText(0),
	mResultList(0),
	mLastRegion(0),
	mCoverRegionHandle(0),
	mStopped(false),
	mResultsDirty(false),
	mCovering(false),
	mSeenObjects(-1),
	mRequestBudget(0)
{
	mLastUpdateTimer.reset();
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_area_search.xml");
}
JCFloaterAreaSearch::~JCFloaterAreaSearch()
{
	mStopped = true;
	mCovering = false;
	if (!LLApp::isExiting() && mCoverRegionHandle && LLWorld::instanceExists())
	{
		if (LLViewerRegion* region = LLWorld::getInstance()->getRegionFromHandle(mCoverRegionHandle))
			region->setInterestListMode(IL_MODE_DEFAULT);
		mCoverRegionHandle = 0;
	}
	set_agent_update_cover_region(false);
}
void JCFloaterAreaSearch::close(bool app)
{
	if (app || mStopped)
	{
		LLFloater::close(app);
	}
	else
	{
		setVisible(FALSE);
	}
}
BOOL JCFloaterAreaSearch::postBuild()
{
	mResultList = getChild<LLScrollListCtrl>("result_list");
	mResultList->setDoubleClickCallback(boost::bind(&JCFloaterAreaSearch::onDoubleClick,this));
	mResultList->sortByColumn("Name", TRUE);
	auto tp = getChild<LLButton>("TP");
	auto look = getChild<LLButton>("Look");
	mResultList->setCommitOnSelectionChange(true);
	mResultList->setCommitCallback([=](LLUICtrl* ctrl, const LLSD& param){
		bool enabled = mResultList->getNumSelected() == 1;
		tp->setEnabled(enabled);
		look->setEnabled(enabled);
	});
	mCounterText = getChild<LLTextBox>("counter");
	getChild<LLButton>("Refresh")->setClickedCallback(boost::bind(&JCFloaterAreaSearch::onRefresh,this));
	getChild<LLButton>("Stop")->setClickedCallback(boost::bind(&JCFloaterAreaSearch::onStop,this));
	tp->setClickedCallback(boost::bind(&JCFloaterAreaSearch::teleportToSelected, this));
	look->setClickedCallback(boost::bind(&JCFloaterAreaSearch::lookAtSelected, this));
	getChild<LLFilterEditor>("Name query chunk")->setCommitCallback(boost::bind(&JCFloaterAreaSearch::onCommitLine,this,_1,_2,LIST_OBJECT_NAME));
	getChild<LLFilterEditor>("Description query chunk")->setCommitCallback(boost::bind(&JCFloaterAreaSearch::onCommitLine,this,_1,_2,LIST_OBJECT_DESC));
	getChild<LLFilterEditor>("Owner query chunk")->setCommitCallback(boost::bind(&JCFloaterAreaSearch::onCommitLine,this,_1,_2,LIST_OBJECT_OWNER));
	getChild<LLFilterEditor>("Group query chunk")->setCommitCallback(boost::bind(&JCFloaterAreaSearch::onCommitLine,this,_1,_2,LIST_OBJECT_GROUP));
	return TRUE;
}
void JCFloaterAreaSearch::onOpen()
{
	mStopped = false;
	checkRegion();
	updateCover();
	mResultsDirty = true;
	results();
}
void JCFloaterAreaSearch::draw()
{
	if (getVisible())
	{
		updateCover();
		if (!mStopped)
		{
			if (checkRegion())
				mResultsDirty = true;
			if (gObjectList.getNumObjects() != mSeenObjects)
				mResultsDirty = true;
			if (mResultsDirty && mLastUpdateTimer.getElapsedTimeF32() >= min_refresh_interval)
				results();
			if (mCoverPulse.getElapsedTimeF32() >= 2.f)
			{
				send_agent_update(TRUE);
				mCoverPulse.reset();
			}
		}
	}
	LLFloater::draw();
}
void JCFloaterAreaSearch::setVisible(BOOL visible)
{
	LLFloater::setVisible(visible);
	updateCover();
}
bool JCFloaterAreaSearch::checkRegion(bool force_clear)
{
	LLViewerRegion* region = gAgent.getRegion();
	if (force_clear || region != mLastRegion)
	{
		mLastRegion = region;
		mPendingObjects.clear();
		mCachedObjects.clear();
		mSeenObjects = -1;
		mResultList->deleteAllItems();
		mCounterText->setText(std::string("Listed/Pending/Total"));
		return true;
	}
	return false;
}
void JCFloaterAreaSearch::updateCover()
{
	const bool cover = getVisible() && !mStopped;
	LLViewerRegion* previous = NULL;
	if (mCoverRegionHandle && LLWorld::instanceExists())
		previous = LLWorld::getInstance()->getRegionFromHandle(mCoverRegionHandle);
	LLViewerRegion* region = (cover && LLWorld::instanceExists()) ? gAgent.getRegion() : NULL;
	if (previous && previous != region)
		previous->setInterestListMode(IL_MODE_DEFAULT);
	if (region)
		region->setInterestListMode(IL_MODE_360);
	mCoverRegionHandle = region ? region->getHandle() : 0;
	const bool enabling = cover && !mCovering;
	mCovering = cover;
	set_agent_update_cover_region(cover);
	if (enabling)
		mCoverPulse.reset();
}
bool JCFloaterAreaSearch::cachedPosition(const LLUUID& id, LLVector3d& pos) const
{
	boost::unordered_map<LLUUID, ObjectData>::const_iterator it = mCachedObjects.find(id);
	if (it == mCachedObjects.end() || !it->second.has_position)
		return false;
	pos = LLVector3d(it->second.pos_global[0], it->second.pos_global[1], it->second.pos_global[2]);
	return true;
}
void JCFloaterAreaSearch::onDoubleClick()
{
	if (LLScrollListItem* item = mResultList->getFirstSelected())
	{
		LLVector3d pos;
		std::string name;
		boost::unordered_map<LLUUID, ObjectData>::const_iterator it = mCachedObjects.find(item->getUUID());
		if (it != mCachedObjects.end())
			name = it->second.name;
		if (LLViewerObject* objectp = gObjectList.findObject(item->getUUID()))
			pos = objectp->getPositionGlobal();
		else if (!cachedPosition(item->getUUID(), pos))
			return;
		LLTracker::trackLocation(pos, name, "", LLTracker::LOCATION_ITEM);
	}
}
void JCFloaterAreaSearch::teleportToSelected()
{
	if (LLScrollListItem* item = mResultList->getFirstSelected())
	{
		LLVector3d pos;
		if (LLViewerObject* objectp = gObjectList.findObject(item->getUUID()))
			gAgent.teleportViaLocation(objectp->getPositionGlobal());
		else if (cachedPosition(item->getUUID(), pos))
			gAgent.teleportViaLocation(pos);
	}
}
void JCFloaterAreaSearch::lookAtSelected()
{
	if (LLScrollListItem* item = mResultList->getFirstSelected())
	{
		if (gAgentCamera.lookAtObject(item->getUUID(), false))
			return;
		LLVector3d pos;
		std::string name;
		boost::unordered_map<LLUUID, ObjectData>::const_iterator it = mCachedObjects.find(item->getUUID());
		if (it != mCachedObjects.end())
			name = it->second.name;
		if (cachedPosition(item->getUUID(), pos))
			LLTracker::trackLocation(pos, name, "", LLTracker::LOCATION_ITEM);
	}
}
void JCFloaterAreaSearch::onStop()
{
	mStopped = true;
	mPendingObjects.clear();
	mResultsDirty = false;
	results();
	updateCover();
	mCounterText->setText(std::string("Stopped"));
}
void JCFloaterAreaSearch::onRefresh()
{
	mStopped = false;
	checkRegion(true);
	updateCover();
	mResultsDirty = true;
	results();
}
void JCFloaterAreaSearch::onCommitLine(LLUICtrl* caller, const LLSD& value, OBJECT_COLUMN_ORDER type)
{
	std::string text = value.asString();
	LLStringUtil::toLower(text);
	caller->setValue(text);
 	mFilterStrings[type] = text;
	checkRegion();
	results();
}
bool JCFloaterAreaSearch::requestIfNeeded(LLUUID object_id)
{
	if (!mCachedObjects.count(object_id) && !mPendingObjects.count(object_id))
	{
		if (mStopped || mRequestBudget <= 0)
			return true;
		--mRequestBudget;
		mPendingObjects.insert(object_id);
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_RequestObjectPropertiesFamily);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_RequestFlags, 0 );
		msg->addUUIDFast(_PREHASH_ObjectID, object_id);
		gAgent.sendReliableMessage();
		return true;
	}
	return false;
}
void JCFloaterAreaSearch::results()
{
	if (!getVisible()) return;
	if (mPendingObjects.size() > 0 && mLastUpdateTimer.getElapsedTimeF32() < min_refresh_interval)
	{
		mResultsDirty = true;
		return;
	}
	mResultsDirty = false;
	mRequestBudget = max_property_requests;
	uuid_vec_t selected = mResultList->getSelectedIDs();
	S32 scrollpos = mResultList->getScrollPos();
	mResultList->deleteAllItems();
	S32 i;
	S32 total = gObjectList.getNumObjects();
	LLViewerRegion* our_region = gAgent.getRegion();
	for (i = 0; i < total; i++)
	{
		LLViewerObject *objectp = gObjectList.getObject(i);
		if (objectp)
		{
			if (our_region && objectp->getRegion() == our_region && !objectp->isAvatar() && objectp->isRoot() &&
				!objectp->flagTemporary() && !objectp->flagTemporaryOnRez())
			{
				LLUUID object_id = objectp->getID();
				boost::unordered_map<LLUUID, ObjectData>::iterator cached = mCachedObjects.find(object_id);
				if (cached != mCachedObjects.end())
				{
					const LLVector3d pos = objectp->getPositionGlobal();
					cached->second.pos_global[0] = pos.mdV[0];
					cached->second.pos_global[1] = pos.mdV[1];
					cached->second.pos_global[2] = pos.mdV[2];
					cached->second.has_position = true;
				}
				requestIfNeeded(object_id);
			}
		}
	}
	for (boost::unordered_map<LLUUID, ObjectData>::const_iterator it = mCachedObjects.begin(); it != mCachedObjects.end(); ++it)
	{
		if (!mStopped)
		{
			LLViewerObject* objectp = gObjectList.findObject(it->first);
			if (!objectp || objectp->getRegion() != our_region || objectp->isAvatar() || !objectp->isRoot()
				|| objectp->flagTemporary() || objectp->flagTemporaryOnRez())
				continue;
		}
		std::string object_name = it->second.name;
		std::string object_desc = it->second.desc;
		std::string object_owner;
		std::string object_group;
		gCacheName->getFullName(it->second.owner_id, object_owner);
		gCacheName->getGroupName(it->second.group_id, object_group);
		std::string onU = object_owner;
		std::string cnU = object_group;
		LLStringUtil::toLower(object_name);
		LLStringUtil::toLower(object_desc);
		LLStringUtil::toLower(object_owner);
		LLStringUtil::toLower(object_group);
		if ((mFilterStrings[LIST_OBJECT_NAME].empty() || object_name.find(mFilterStrings[LIST_OBJECT_NAME]) != std::string::npos) &&
			(mFilterStrings[LIST_OBJECT_DESC].empty() || object_desc.find(mFilterStrings[LIST_OBJECT_DESC]) != std::string::npos) &&
			(mFilterStrings[LIST_OBJECT_OWNER].empty() || object_owner.find(mFilterStrings[LIST_OBJECT_OWNER]) != std::string::npos) &&
			(mFilterStrings[LIST_OBJECT_GROUP].empty() || object_group.find(mFilterStrings[LIST_OBJECT_GROUP]) != std::string::npos))
		{
			LLSD element;
			element["id"] = it->first;
			element["columns"][LIST_OBJECT_NAME]["column"] = "Name";
			element["columns"][LIST_OBJECT_NAME]["type"] = "text";
			element["columns"][LIST_OBJECT_NAME]["value"] = it->second.name;
			element["columns"][LIST_OBJECT_DESC]["column"] = "Description";
			element["columns"][LIST_OBJECT_DESC]["type"] = "text";
			element["columns"][LIST_OBJECT_DESC]["value"] = it->second.desc;
			element["columns"][LIST_OBJECT_OWNER]["column"] = "Owner";
			element["columns"][LIST_OBJECT_OWNER]["type"] = "text";
			element["columns"][LIST_OBJECT_OWNER]["value"] = onU;
			element["columns"][LIST_OBJECT_GROUP]["column"] = "Group";
			element["columns"][LIST_OBJECT_GROUP]["type"] = "text";
			element["columns"][LIST_OBJECT_GROUP]["value"] = cnU;
			mResultList->addElement(element, ADD_BOTTOM);
		}
	}
	mResultList->updateSort();
	mResultList->selectMultiple(selected);
	mResultList->setScrollPos(scrollpos);
	mCounterText->setText(llformat("%d listed/%d pending/%d total", mResultList->getItemCount(), mPendingObjects.size(), mPendingObjects.size()+mCachedObjects.size()));
	mSeenObjects = total;
	mLastUpdateTimer.reset();
}
void JCFloaterAreaSearch::processObjectPropertiesFamily(LLMessageSystem* msg, void** user_data)
{
	JCFloaterAreaSearch* floater = findInstance();
	if(!floater)
		return;
	floater->checkRegion();
	LLUUID object_id;
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, object_id);
	auto it = floater->mPendingObjects.find(object_id);
	if(it != floater->mPendingObjects.end())
		floater->mPendingObjects.erase(it);
	ObjectData* data = &floater->mCachedObjects[object_id];
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_OwnerID, data->owner_id);
	if (LLViewerObject* obj = gObjectList.findObject(object_id))
	{
		obj->mOwnerID = data->owner_id;
		const LLVector3d pos = obj->getPositionGlobal();
		data->pos_global[0] = pos.mdV[0];
		data->pos_global[1] = pos.mdV[1];
		data->pos_global[2] = pos.mdV[2];
		data->has_position = true;
	}
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_GroupID, data->group_id);
	msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, data->name);
	msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, data->desc);
	gCacheName->get(data->owner_id, false, boost::bind(&JCFloaterAreaSearch::results,floater));
	gCacheName->get(data->group_id, true, boost::bind(&JCFloaterAreaSearch::results,floater));
}
