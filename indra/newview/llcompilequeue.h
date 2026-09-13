/** 
 * @file llcompilequeue.h
 * @brief LLCompileQueue class header file
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
#ifndef LL_LLCOMPILEQUEUE_H
#define LL_LLCOMPILEQUEUE_H
#include "llinventory.h"
#include "llviewerobject.h"
#include "llvoinventorylistener.h"
#include "llmap.h"
#include "lluuid.h"
#include "llfloater.h"
#include "llviewerinventory.h"
class LLScrollListCtrl;
class LLFloaterScriptQueue : public LLFloater, public LLVOInventoryListener
{
public:
	static LLFloaterScriptQueue* findInstance(const LLUUID& id);
	LLFloaterScriptQueue(const std::string& name, const LLRect& rect,
						 const std::string& title, const std::string& start_string);
	virtual ~LLFloaterScriptQueue();
	BOOL postBuild();
	void setMono(bool mono) { mMono = mono; }
	void addObject(const LLUUID& id);
	BOOL start();
protected:
	void inventoryChanged(LLViewerObject* obj,
								  LLInventoryObject::object_list_t* inv,
								 S32 serial_num,
								 void* queue);
	virtual void handleInventory(LLViewerObject* viewer_obj,
								 LLInventoryObject::object_list_t* inv) = 0;
	static void onCloseBtn(void* user_data);
	BOOL isDone() const;
	virtual BOOL startQueue();
	BOOL nextObject();
	BOOL popNext();
	void setStartString(const std::string& s) { mStartString = s; }
	const LLUUID& getID() const { return mID; }
protected:
	LLScrollListCtrl* mMessages;
	LLButton* mCloseBtn;
	uuid_vec_t mObjectIDs;
	LLUUID mCurrentObjectID;
	bool mDone;
	LLUUID mID;
	static LLMap<LLUUID, LLFloaterScriptQueue*> sInstances;
	std::string mStartString;
	bool mMono;
};
struct LLCompileQueueData
{
	LLUUID mQueueID;
	LLUUID mItemId;
	LLCompileQueueData(const LLUUID& q_id, const LLUUID& item_id) :
		mQueueID(q_id), mItemId(item_id) {}
};
class LLAssetUploadQueue;
class LLFloaterCompileQueue : public LLFloaterScriptQueue
{
public:
	static LLFloaterCompileQueue* create(bool mono);
	void removeItemByItemID(const LLUUID& item_id);
	LLAssetUploadQueue* getUploadQueue() { return mUploadQueue; }
	void experienceIdsReceived(const LLSD& content);
	BOOL hasExperience(const LLUUID& id) const;
protected:
	LLFloaterCompileQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterCompileQueue();
	virtual void handleInventory(LLViewerObject* viewer_obj,
								 LLInventoryObject::object_list_t* inv);
	static void requestAsset(struct LLScriptQueueData* datap, const LLSD& experience);
	static void scriptArrived(LLVFS *vfs, const LLUUID& asset_id,
								LLAssetType::EType type,
								void* user_data, S32 status, LLExtStat ext_status);
	void removeItemByAssetID(const LLUUID& asset_id);
	void saveItemByItemID(const LLUUID& item_id);
	const LLInventoryItem* findItemByItemID(const LLUUID& item_id) const;
	virtual BOOL startQueue();
protected:
	LLViewerInventoryItem::item_array_t mCurrentScripts;
private:
	LLAssetUploadQueue* mUploadQueue;
	uuid_list_t mExperienceIds;
};
class LLFloaterResetQueue : public LLFloaterScriptQueue
{
public:
	static LLFloaterResetQueue* create();
protected:
	LLFloaterResetQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterResetQueue();
	virtual void handleInventory(LLViewerObject* viewer_obj,
								 LLInventoryObject::object_list_t* inv);
};
class LLFloaterRunQueue : public LLFloaterScriptQueue
{
public:
	static LLFloaterRunQueue* create();
protected:
	LLFloaterRunQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterRunQueue();
	virtual void handleInventory(LLViewerObject* viewer_obj,
								 LLInventoryObject::object_list_t* inv);
};
class LLFloaterNotRunQueue : public LLFloaterScriptQueue
{
public:
	static LLFloaterNotRunQueue* create();
protected:
	LLFloaterNotRunQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterNotRunQueue();
	virtual void handleInventory(LLViewerObject* viewer_obj,
								 LLInventoryObject::object_list_t* inv);
};
#endif
