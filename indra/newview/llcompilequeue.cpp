/** 
 * @file llcompilequeue.cpp
 * @brief LLCompileQueueData class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "llcompilequeue.h"
#include "llagent.h"
#include "llassetuploadqueue.h"
#include "llassetuploadresponders.h"
#include "llchat.h"
#include "llviewerwindow.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llresmgr.h"
#include "llbutton.h"
#include "llscrolllistctrl.h"
#include "lldir.h"
#include "llnotificationsutil.h"
#include "llfloaterchat.h"
#include "llviewerstats.h"
#include "lluictrlfactory.h"
#include "lltrans.h"
#include "llselectmgr.h"
#include "llexperiencecache.h"
struct LLScriptQueueData
{
	LLUUID mQueueID;
	LLPointer<LLInventoryItem> mItemp;
	LLUUID mTaskId;
	LLUUID mExperienceId;
	LLHost mHost;
	LLScriptQueueData(const LLUUID& q_id, const LLUUID& task_id, LLInventoryItem* item, const LLHost& host) :
		mQueueID(q_id), mTaskId(task_id), mItemp(item), mHost(host) {}
};
LLMap<LLUUID, LLFloaterScriptQueue*> LLFloaterScriptQueue::sInstances;
LLFloaterScriptQueue::LLFloaterScriptQueue(const std::string& name,
										   const LLRect& rect,
										   const std::string& title,
										   const std::string& start_string) :
	LLFloater(name, rect, title,
			  RESIZE_YES, DEFAULT_MIN_WIDTH, DEFAULT_MIN_HEIGHT,
			  DRAG_ON_TOP, MINIMIZE_YES, CLOSE_YES),
	mMessages(nullptr),
	mCloseBtn(nullptr),
	mDone(false),
	mMono(false)
{
	mID.generate();
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_script_queue.xml");
	setTitle(title);
	LLRect curRect = getRect();
	translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	mStartString = start_string;
	sInstances.addData(mID, this);
}
LLFloaterScriptQueue::~LLFloaterScriptQueue()
{
	sInstances.removeData(mID);
}
BOOL LLFloaterScriptQueue::postBuild()
{
	childSetAction("close",onCloseBtn,this);
	getChildView("close")->setEnabled(FALSE);
	return TRUE;
}
LLFloaterScriptQueue* LLFloaterScriptQueue::findInstance(const LLUUID& id)
{
	return sInstances.checkData(id) ? sInstances.getData(id) : NULL;
}
void LLFloaterScriptQueue::inventoryChanged(LLViewerObject* viewer_object,
											  LLInventoryObject::object_list_t* inv,
											 S32,
											 void* q_id)
{
	LL_INFOS() << "LLFloaterScriptQueue::inventoryChanged() for  object "
			<< viewer_object->getID() << LL_ENDL;
	removeVOInventoryListener();
	if (viewer_object && inv && (viewer_object->getID() == mCurrentObjectID) )
	{
		handleInventory(viewer_object, inv);
	}
	else
	{
		LL_WARNS() << "No inventory for " << mCurrentObjectID
				<< LL_ENDL;
		nextObject();
	}
}
void LLFloaterScriptQueue::onCloseBtn(void* user_data)
{
	LLFloaterScriptQueue* self = (LLFloaterScriptQueue*)user_data;
	self->close();
}
void LLFloaterScriptQueue::addObject(const LLUUID& id)
{
	mObjectIDs.push_back(id);
}
BOOL LLFloaterScriptQueue::start()
{
	std::string buffer;
	LLStringUtil::format_map_t args;
	args["[START]"] = mStartString;
	args["[COUNT]"] = llformat ("%d", mObjectIDs.size());
	buffer = getString ("Starting", args);
	getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);
	return startQueue();
}
BOOL LLFloaterScriptQueue::isDone() const
{
	return (mCurrentObjectID.isNull() && (mObjectIDs.size() == 0));
}
BOOL LLFloaterScriptQueue::nextObject()
{
	U32 count;
	BOOL successful_start = FALSE;
	do
	{
		count = mObjectIDs.size();
		LL_INFOS() << "LLFloaterScriptQueue::nextObject() - " << count
				<< " objects left to process." << LL_ENDL;
		mCurrentObjectID.setNull();
		if (count > 0)
		{
			successful_start = popNext();
		}
		LL_INFOS() << "LLFloaterScriptQueue::nextObject() "
				<< (successful_start ? "successful" : "unsuccessful")
				<< LL_ENDL;
	} while((mObjectIDs.size() > 0) && !successful_start);
	if (isDone() && !mDone)
	{
		mDone = true;
		getChild<LLScrollListCtrl>("queue output")->addSimpleElement(getString("Done"), ADD_BOTTOM);
		getChildView("close")->setEnabled(TRUE);
	}
	return successful_start;
}
BOOL LLFloaterScriptQueue::popNext()
{
	BOOL rv = FALSE;
	S32 count = mObjectIDs.size();
	if (mCurrentObjectID.isNull() && (count > 0))
	{
		mCurrentObjectID = mObjectIDs.at(0);
		LL_INFOS() << "LLFloaterScriptQueue::popNext() - mCurrentID: "
				<< mCurrentObjectID << LL_ENDL;
		mObjectIDs.erase(mObjectIDs.begin());
		LLViewerObject* obj = gObjectList.findObject(mCurrentObjectID);
		if (obj)
		{
			LL_INFOS() << "LLFloaterScriptQueue::popNext() requesting inv for "
					<< mCurrentObjectID << LL_ENDL;
			LLUUID* id = new LLUUID(mID);
			registerVOInventoryListener(obj,id);
			requestVOInventory();
			rv = TRUE;
		}
	}
	return rv;
}
BOOL LLFloaterScriptQueue::startQueue()
{
	return nextObject();
}
class CompileQueueExperienceResponder final : public LLHTTPClient::ResponderWithResult
{
	const LLUUID mParent;
public:
	CompileQueueExperienceResponder(const LLUUID& parent): mParent(parent) {}
	void httpSuccess() override { sendResult(getContent()); }
	void httpFailure() override { sendResult(LLSD()); }
	void sendResult(const LLSD& content)
	{
		if (auto queue = static_cast<LLFloaterCompileQueue*>(LLFloaterCompileQueue::findInstance(mParent)))
			queue->experienceIdsReceived(content["experience_ids"]);
	}
	char const* getName() const override { return "CompileQueueExperienceResponder"; }
};
class LLCompileFloaterUploadQueueSupplier : public LLAssetUploadQueueSupplier
{
public:
	LLCompileFloaterUploadQueueSupplier(const LLUUID& queue_id) :
		mQueueId(queue_id)
	{
	}
	virtual LLAssetUploadQueue* get() const
	{
		LLFloaterCompileQueue* queue = (LLFloaterCompileQueue*) LLFloaterScriptQueue::findInstance(mQueueId);
		if (NULL == queue)
		{
			return NULL;
		}
		return queue->getUploadQueue();
	}
	virtual void log(std::string message) const
	{
		LLFloaterCompileQueue* queue = (LLFloaterCompileQueue*) LLFloaterScriptQueue::findInstance(mQueueId);
		if (NULL == queue)
		{
			return;
		}
		queue->getChild<LLScrollListCtrl>("queue output")->addSimpleElement(message, ADD_BOTTOM);
	}
private:
	LLUUID mQueueId;
};
LLFloaterCompileQueue* LLFloaterCompileQueue::create(bool mono)
{
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect = gSavedSettings.getRect("CompileOutputRect");
	rect.translate(left - rect.mLeft, top - rect.mTop);
	LLFloaterCompileQueue* new_queue = new LLFloaterCompileQueue("queue", rect);
	new_queue->mUploadQueue = new LLAssetUploadQueue(new LLCompileFloaterUploadQueueSupplier(new_queue->getID()));
	new_queue->setMono(mono);
	new_queue->open();
	return new_queue;
}
LLFloaterCompileQueue::LLFloaterCompileQueue(const std::string& name, const LLRect& rect)
: LLFloaterScriptQueue(name, rect, LLTrans::getString("CompileQueueTitle"), LLTrans::getString("CompileQueueStart"))
{ }
LLFloaterCompileQueue::~LLFloaterCompileQueue()
{
}
void LLFloaterCompileQueue::experienceIdsReceived(const LLSD& content)
{
	for(LLSD::array_const_iterator it  = content.beginArray(); it != content.endArray(); ++it)
	{
		mExperienceIds.insert(it->asUUID());
	}
	nextObject();
}
BOOL LLFloaterCompileQueue::hasExperience(const LLUUID& id) const
{
	return mExperienceIds.find(id) != mExperienceIds.end();
}
void LLFloaterCompileQueue::handleInventory(LLViewerObject *viewer_object,
											 LLInventoryObject::object_list_t* inv)
{
	typedef std::multimap<LLUUID, LLPointer<LLInventoryItem> > uuid_item_map;
	uuid_item_map asset_item_map;
	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if ((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
			if (item->getPermissions().allowModifyBy(gAgent.getID(), gAgent.getGroupID())  &&
				item->getPermissions().allowCopyBy(gAgent.getID(), gAgent.getGroupID()) )
			{
				LLPointer<LLViewerInventoryItem> script = new LLViewerInventoryItem(item);
				mCurrentScripts.push_back(script);
				asset_item_map.insert(std::make_pair(item->getAssetUUID(), item));
			}
		}
	}
	if (asset_item_map.empty())
	{
		nextObject();
	}
	else
	{
		LLViewerRegion* region = viewer_object->getRegion();
		std::string url = std::string();
		if (region)
		{
			url = region->getCapability("GetMetadata");
		}
		const auto& host = region->getHost();
		const auto& obj_id = viewer_object->getID();
		for(const auto& pair : asset_item_map)
		{
			LLInventoryItem *itemp = pair.second;
			LLScriptQueueData* datap = new LLScriptQueueData(getID(),
												 obj_id,
												 itemp,
												 host);
			LLExperienceCache::instance().fetchAssociatedExperience(itemp->getParentUUID(), itemp->getUUID(),
				boost::bind(LLFloaterCompileQueue::requestAsset, datap, _1));
		}
	}
}
void LLFloaterCompileQueue::requestAsset(LLScriptQueueData* datap, const LLSD& experience)
{
	LLFloaterCompileQueue* queue = static_cast<LLFloaterCompileQueue*>(findInstance(datap->mQueueID));
	if (!queue)
	{
		delete datap;
		return;
	}
	if (experience.has(LLExperienceCache::EXPERIENCE_ID))
	{
		datap->mExperienceId = experience[LLExperienceCache::EXPERIENCE_ID].asUUID();
		if (!queue->hasExperience(datap->mExperienceId))
		{
			std::string buffer = LLTrans::getString("CompileNoExperiencePerm", LLSD::emptyMap()
				.with("SCRIPT", datap->mItemp->getName())
				.with("EXPERIENCE", experience[LLExperienceCache::NAME].asString()));
			queue->getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);
			queue->removeItemByItemID(datap->mItemp->getUUID());
			delete datap;
			return;
		}
	}
	gAssetStorage->getInvItemAsset(datap->mHost,
		gAgent.getID(),
		gAgent.getSessionID(),
		datap->mItemp->getPermissions().getOwner(),
		datap->mTaskId,
		datap->mItemp->getUUID(),
		datap->mItemp->getAssetUUID(),
		datap->mItemp->getType(),
		LLFloaterCompileQueue::scriptArrived,
		(void*)datap);
}
void LLFloaterCompileQueue::scriptArrived(LLVFS *vfs, const LLUUID& asset_id,
										  LLAssetType::EType type,
										  void* user_data, S32 status, LLExtStat ext_status)
{
	LL_INFOS() << "LLFloaterCompileQueue::scriptArrived()" << LL_ENDL;
	LLScriptQueueData* data = (LLScriptQueueData*)user_data;
	if(!data)
	{
		return;
	}
	LLFloaterCompileQueue* queue = static_cast<LLFloaterCompileQueue*> (LLFloaterScriptQueue::findInstance(data->mQueueID));
	std::string buffer;
	if (queue && (0 == status))
	{
		std::string filename;
		LLVFile file(vfs, asset_id, type);
		std::string uuid_str;
		asset_id.toString(uuid_str);
		filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_str) + llformat(".%s",LLAssetType::lookup(type));
		const bool is_running = true;
		LLViewerObject* object = gObjectList.findObject(data->mTaskId);
		if (object)
		{
			std::string url = object->getRegion()->getCapability("UpdateScriptTask");
			if (!url.empty())
			{
				U32 script_size = file.getSize();
				U8* script_data = new U8[script_size];
				file.read(script_data, script_size);
				queue->mUploadQueue->queue(filename, data->mTaskId,
										   data->mItemp->getUUID(), is_running, queue->mMono, queue->getID(),
										   script_data, script_size, data->mItemp->getName(), data->mExperienceId);
			}
			else
			{
				std::string text = LLTrans::getString("CompileQueueProblemUploading");
				LLChat chat(text);
				LLFloaterChat::addChat(chat);
				buffer = text + LLTrans::getString(":") + ' ' + data->mItemp->getName();
				LL_WARNS() << "Problem uploading script asset." << LL_ENDL;
				if(queue) queue->removeItemByItemID(data->mItemp->getUUID());
			}
		}
	}
	else
	{
		LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
		if ( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status )
		{
			LLChat chat(LLTrans::getString("CompileQueueScriptNotFound"));
			LLFloaterChat::addChat(chat);
			buffer = LLTrans::getString("CompileQueueProblemDownloading") + LLTrans::getString(":") + ' ' + data->mItemp->getName();
		}
		else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
		{
			LLChat chat(LLTrans::getString("CompileQueueInsufficientPermDownload"));
			LLFloaterChat::addChat(chat);
			buffer = LLTrans::getString("CompileQueueInsufficientPermFor") + LLTrans::getString(":") + ' ' + data->mItemp->getName();
		}
		else
		{
			buffer = LLTrans::getString("CompileQueueUnknownFailure") + ' ' + data->mItemp->getName();
		}
		LL_WARNS() << "Problem downloading script asset." << LL_ENDL;
		if(queue) queue->removeItemByItemID(data->mItemp->getUUID());
	}
	if (queue && (buffer.size() > 0))
	{
		queue->getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);
	}
	delete data;
}
const LLInventoryItem* LLFloaterCompileQueue::findItemByItemID(const LLUUID& asset_id) const
{
	LLInventoryItem* result = NULL;
	S32 count = mCurrentScripts.size();
	for(S32 i = 0; i < count; ++i)
	{
		if(asset_id == mCurrentScripts.at(i)->getUUID())
		{
			result = mCurrentScripts.at(i);
		}
	}
	return result;
}
void LLFloaterCompileQueue::saveItemByItemID(const LLUUID& asset_id)
{
	LL_INFOS() << "LLFloaterCompileQueue::saveItemByAssetID()" << LL_ENDL;
	LLViewerObject* viewer_object = gObjectList.findObject(mCurrentObjectID);
	if (viewer_object)
	{
		S32 count = mCurrentScripts.size();
		for(S32 i = 0; i < count; ++i)
		{
			if(asset_id == mCurrentScripts.at(i)->getUUID())
			{
				viewer_object->saveScript(mCurrentScripts.at(i), TRUE, false);
			}
		}
	}
	else
	{
		LL_WARNS() << "Unable to finish save!" << LL_ENDL;
	}
}
LLFloaterResetQueue* LLFloaterResetQueue::create()
{
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect = gSavedSettings.getRect("CompileOutputRect");
	rect.translate(left - rect.mLeft, top - rect.mTop);
	LLFloaterResetQueue* new_queue = new LLFloaterResetQueue("queue", rect);
	gFloaterView->addChild(new_queue);
	new_queue->open();
	return new_queue;
}
LLFloaterResetQueue::LLFloaterResetQueue(const std::string& name, const LLRect& rect)
: LLFloaterScriptQueue(name, rect, LLTrans::getString("ResetQueueTitle"), LLTrans::getString("ResetQueueStart"))
{ }
LLFloaterResetQueue::~LLFloaterResetQueue()
{
}
void LLFloaterResetQueue::handleInventory(LLViewerObject* viewer_obj,
										   LLInventoryObject::object_list_t* inv)
{
	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if ((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());
			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				std::string buffer;
				buffer = getString("Resetting") + LLTrans::getString(":") + ' ' + item->getName();
				getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_ScriptReset);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_Script);
				msg->addUUIDFast(_PREHASH_ObjectID, viewer_obj->getID());
				msg->addUUIDFast(_PREHASH_ItemID, (*it)->getUUID());
				msg->sendReliable(object->getRegion()->getHost());
			}
		}
	}
	nextObject();
}
LLFloaterRunQueue* LLFloaterRunQueue::create()
{
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect = gSavedSettings.getRect("CompileOutputRect");
	rect.translate(left - rect.mLeft, top - rect.mTop);
	LLFloaterRunQueue* new_queue = new LLFloaterRunQueue("queue", rect);
	new_queue->open();
	return new_queue;
}
LLFloaterRunQueue::LLFloaterRunQueue(const std::string& name, const LLRect& rect)
: LLFloaterScriptQueue(name, rect, LLTrans::getString("RunQueueTitle"), LLTrans::getString("RunQueueStart"))
{ }
LLFloaterRunQueue::~LLFloaterRunQueue()
{
}
void LLFloaterRunQueue::handleInventory(LLViewerObject* viewer_obj,
										  LLInventoryObject::object_list_t* inv)
{
	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if ((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());
			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				LLScrollListCtrl* list = getChild<LLScrollListCtrl>("queue output");
				std::string buffer;
				buffer = getString("Running") + LLTrans::getString(":") + ' ' + item->getName();
				list->addSimpleElement(buffer, ADD_BOTTOM);
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_SetScriptRunning);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_Script);
				msg->addUUIDFast(_PREHASH_ObjectID, viewer_obj->getID());
				msg->addUUIDFast(_PREHASH_ItemID, (*it)->getUUID());
				msg->addBOOLFast(_PREHASH_Running, TRUE);
				msg->sendReliable(object->getRegion()->getHost());
			}
		}
	}
	nextObject();
}
LLFloaterNotRunQueue* LLFloaterNotRunQueue::create()
{
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect = gSavedSettings.getRect("CompileOutputRect");
	rect.translate(left - rect.mLeft, top - rect.mTop);
	LLFloaterNotRunQueue* new_queue = new LLFloaterNotRunQueue("queue", rect);
	new_queue->open();
	return new_queue;
}
LLFloaterNotRunQueue::LLFloaterNotRunQueue(const std::string& name, const LLRect& rect)
: LLFloaterScriptQueue(name, rect, LLTrans::getString("NotRunQueueTitle"), LLTrans::getString("NotRunQueueStart"))
{ }
LLFloaterNotRunQueue::~LLFloaterNotRunQueue()
{
}
void LLFloaterCompileQueue::removeItemByItemID(const LLUUID& asset_id)
{
	LL_INFOS() << "LLFloaterCompileQueue::removeItemByAssetID()" << LL_ENDL;
	for(size_t i = 0; i < mCurrentScripts.size(); )
	{
		if(asset_id == mCurrentScripts[i]->getUUID())
		{
			vector_replace_with_last(mCurrentScripts, mCurrentScripts.begin() + i);
		}
		else
		{
			++i;
		}
	}
	if(mCurrentScripts.empty())
	{
		nextObject();
	}
}
BOOL LLFloaterCompileQueue::startQueue()
{
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		std::string lookup_url = region->getCapability("GetCreatorExperiences");
		if (!lookup_url.empty())
		{
			LLHTTPClient::get(lookup_url, new CompileQueueExperienceResponder(mID));
			return TRUE;
		}
	}
	return nextObject();
}
void LLFloaterNotRunQueue::handleInventory(LLViewerObject* viewer_obj,
										   LLInventoryObject::object_list_t* inv)
{
	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if ((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());
			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				LLScrollListCtrl* list = getChild<LLScrollListCtrl>("queue output");
				std::string buffer;
				buffer = getString("NotRunning") + LLTrans::getString(":") + ' ' + item->getName();
				list->addSimpleElement(buffer, ADD_BOTTOM);
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_SetScriptRunning);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_Script);
				msg->addUUIDFast(_PREHASH_ObjectID, viewer_obj->getID());
				msg->addUUIDFast(_PREHASH_ItemID, (*it)->getUUID());
				msg->addBOOLFast(_PREHASH_Running, FALSE);
				msg->sendReliable(object->getRegion()->getHost());
			}
		}
	}
	nextObject();
}
