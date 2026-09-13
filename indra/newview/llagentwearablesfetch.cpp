/** 
 * @file llagentwearablesfetch.cpp
 * @brief LLAgentWearblesFetch class implementation
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
#include "llviewerprecompiledheaders.h"
#include "llagentwearablesfetch.h"
#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "llstartup.h"
#include "llvoavatarself.h"
#include "llviewercontrol.h"
#include "rlvhelper.h"
void order_my_outfits_cb()
{
		if (!LLApp::isRunning())
		{
			LL_WARNS() << "called during shutdown, skipping" << LL_ENDL;
			return;
		}
		const LLUUID& my_outfits_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
		if (my_outfits_id.isNull()) return;
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(my_outfits_id, cats, items);
		if (!cats) return;
		if (cats->size() < 2)
		{
			LL_WARNS() << "My Outfits category was not populated properly" << LL_ENDL;
			return;
		}
		LL_INFOS() << "Starting updating My Outfits with wearables ordering information" << LL_ENDL;
		for (LLInventoryModel::cat_array_t::iterator outfit_iter = cats->begin();
			outfit_iter != cats->end(); ++outfit_iter)
		{
			const LLUUID& cat_id = (*outfit_iter)->getUUID();
			if (cat_id.isNull()) continue;
			if (cat_id == LLAppearanceMgr::getInstance()->getBaseOutfitUUID()) continue;
		LLAppearanceMgr::getInstance()->updateClothingOrderingInfo(cat_id);
	}
	LL_INFOS() << "Finished updating My Outfits with wearables ordering information" << LL_ENDL;
}
LLInitialWearablesFetch::LLInitialWearablesFetch(const LLUUID& cof_id) :
	LLInventoryFetchDescendentsObserver(cof_id)
{
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->startPhase("initial_wearables_fetch");
		gAgentAvatarp->outputRezTiming("Initial wearables fetch started");
	}
}
LLInitialWearablesFetch::~LLInitialWearablesFetch()
{
}
void LLInitialWearablesFetch::done()
{
	gInventory.removeObserver(this);
	F32 nDelay = gSavedSettings.getF32("ForceInitialCOFDelay");
	if (0.0f == nDelay)
		doOnIdleOneTime(boost::bind(&LLInitialWearablesFetch::processContents,this));
	else
		rlvCallbackTimerOnce(nDelay, boost::bind(&LLInitialWearablesFetch::processContents,this));
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->stopPhase("initial_wearables_fetch");
		gAgentAvatarp->outputRezTiming("Initial wearables fetch done");
	}
}
void LLInitialWearablesFetch::add(InitialWearableData &data)
{
	mAgentInitialWearables.push_back(data);
}
void LLInitialWearablesFetch::processContents()
{
	if(!gAgentAvatarp)
	{
		delete this;
		return ;
	}
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	LLFindWearables is_wearable;
	llassert(!mComplete.empty());
	if (mComplete.empty())
	{
		LL_WARNS() << "mComplete is empty." << LL_ENDL;
	}
	else
	gInventory.collectDescendentsIf(mComplete.front(), cat_array, wearable_array,
									LLInventoryModel::EXCLUDE_TRASH, is_wearable);
	bool fUpdateFromCOF = !wearable_array.empty();
	if ( (fUpdateFromCOF) && (gSavedSettings.getBOOL("VerifyInitialWearables")) )
	{
		LLAppearanceMgr::wearables_by_type_t items_by_type(LLWearableType::WT_COUNT);
		LLAppearanceMgr::sortItemsByActualDescription(wearable_array);
		LLAppearanceMgr::divvyWearablesByType(wearable_array, items_by_type);
		for (initial_wearable_data_vec_t::const_iterator itWearableData = mAgentInitialWearables.begin();
				(itWearableData != mAgentInitialWearables.end()) && (fUpdateFromCOF);  ++itWearableData)
		{
			const LLUUID& idItem = itWearableData->mItemID; bool fFound = false;
			for (S32 idxItem = 0, cntItem = items_by_type[itWearableData->mType].size(); idxItem < cntItem; idxItem++)
			{
				const LLViewerInventoryItem* pCOFItem = items_by_type[itWearableData->mType].at(idxItem);
				if (idItem == pCOFItem->getLinkedUUID())
				{
					fFound = true;
					break;
				}
			}
			if (!fFound)
				fUpdateFromCOF = false;
		}
	}
	LLAppearanceMgr::instance().setAttachmentInvLinkEnable(true);
	if (fUpdateFromCOF)
	{
		gAgentWearables.notifyLoadingStarted();
		LLAppearanceMgr::instance().updateAppearanceFromCOF();
	}
	else
	{
		LLAppearanceMgr::instance().setOutfitDirty(true);
		processWearablesMessage();
	}
	delete this;
}
class LLFetchAndLinkObserver: public LLInventoryFetchItemsObserver
{
public:
	LLFetchAndLinkObserver(uuid_vec_t& ids):
		LLInventoryFetchItemsObserver(ids)
	{
	}
	~LLFetchAndLinkObserver()
	{
	}
	virtual void done()
	{
		gInventory.removeObserver(this);
		doOnIdleOneTime(boost::bind(&LLFetchAndLinkObserver::doneIdle, this));
	}
	void doneIdle()
	{
		LLInventoryObject::const_object_list_t initial_items;
		for (uuid_vec_t::iterator itItem = mIDs.begin(); itItem != mIDs.end(); ++itItem)
		{
			LLViewerInventoryItem* pItem = gInventory.getItem(*itItem);
			if (!pItem)
			{
				LL_WARNS() << "fetch failed!" << LL_ENDL;
				continue;
			}
			initial_items.push_back(pItem);
		}
		LLAppearanceMgr::instance().updateAppearanceFromInitialWearables(initial_items);
		delete this;
	}
};
void LLInitialWearablesFetch::processWearablesMessage()
{
	if (!mAgentInitialWearables.empty())
	{
		const LLUUID current_outfit_id = LLAppearanceMgr::instance().getCOF();
		uuid_vec_t ids;
		for (U8 i = 0; i < mAgentInitialWearables.size(); ++i)
		{
			const InitialWearableData& wearable_data = mAgentInitialWearables[i];
			if (wearable_data.mAssetID.notNull())
			{
				ids.push_back(wearable_data.mItemID);
			}
			else
			{
				LL_INFOS() << "Invalid wearable, type " << wearable_data.mType << " itemID "
				<< wearable_data.mItemID << " assetID " << wearable_data.mAssetID << LL_ENDL;
			}
		}
		if (isAgentAvatarValid())
		{
			for (LLVOAvatar::attachment_map_t::const_iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
				 iter != gAgentAvatarp->mAttachmentPoints.end(); ++iter)
			{
				LLViewerJointAttachment* attachment = iter->second;
				if (!attachment) continue;
				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 ++attachment_iter)
				{
					LLViewerObject* attached_object = (*attachment_iter);
					if (!attached_object) continue;
					const LLUUID& item_id = attached_object->getAttachmentItemID();
					if (item_id.isNull()) continue;
					ids.push_back(item_id);
				}
			}
		}
		LLFetchAndLinkObserver *fetcher = new LLFetchAndLinkObserver(ids);
		fetcher->startFetch();
		if (fetcher->isFinished())
		{
			fetcher->done();
		}
		else
		{
			gInventory.addObserver(fetcher);
		}
	}
	else
	{
		LL_WARNS("Wearables") << "No current outfit folder items found and no initial wearables fallback message received." << LL_ENDL;
	}
}
LLLibraryOutfitsFetch::LLLibraryOutfitsFetch(const LLUUID& my_outfits_id) :
	LLInventoryFetchDescendentsObserver(my_outfits_id),
	mCurrFetchStep(LOFS_FOLDER),
	mOutfitsPopulated(false)
{
	LL_INFOS() << "created" << LL_ENDL;
	mMyOutfitsID = LLUUID::null;
	mClothingID = LLUUID::null;
	mLibraryClothingID = LLUUID::null;
	mImportedClothingID = LLUUID::null;
	mImportedClothingName = "Imported Library Clothing";
}
LLLibraryOutfitsFetch::~LLLibraryOutfitsFetch()
{
	LL_INFOS() << "destroyed" << LL_ENDL;
}
void LLLibraryOutfitsFetch::done()
{
	LL_INFOS() << "start" << LL_ENDL;
	doOnIdleOneTime(boost::bind(&LLLibraryOutfitsFetch::doneIdle,this));
	gInventory.removeObserver(this);
}
void LLLibraryOutfitsFetch::doneIdle()
{
	LL_INFOS() << "start" << LL_ENDL;
	gInventory.addObserver(this);
	switch (mCurrFetchStep)
	{
		case LOFS_FOLDER:
			folderDone();
			mCurrFetchStep = LOFS_OUTFITS;
			break;
		case LOFS_OUTFITS:
			outfitsDone();
			mCurrFetchStep = LOFS_LIBRARY;
			break;
		case LOFS_LIBRARY:
			libraryDone();
			mCurrFetchStep = LOFS_IMPORTED;
			break;
		case LOFS_IMPORTED:
			importedFolderDone();
			mCurrFetchStep = LOFS_CONTENTS;
			break;
		case LOFS_CONTENTS:
			contentsDone();
			break;
		default:
			LL_WARNS() << "Got invalid state for outfit fetch: " << mCurrFetchStep << LL_ENDL;
			mOutfitsPopulated = TRUE;
			break;
	}
	if (mOutfitsPopulated)
	{
		gInventory.removeObserver(this);
		delete this;
		return;
	}
}
void LLLibraryOutfitsFetch::folderDone()
{
	LL_INFOS() << "start" << LL_ENDL;
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	gInventory.collectDescendents(mMyOutfitsID, cat_array, wearable_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	if (cat_array.size() > 1)
	{
		mOutfitsPopulated = true;
		return;
	}
	mClothingID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING);
	mLibraryClothingID = gInventory.findLibraryCategoryUUIDForType(LLFolderType::FT_CLOTHING, false);
	LLNameCategoryCollector matchFolderFunctor("Initial Outfits");
	cat_array.clear();
	gInventory.collectDescendentsIf(mLibraryClothingID,
									cat_array, wearable_array,
									LLInventoryModel::EXCLUDE_TRASH,
									matchFolderFunctor);
	if (cat_array.size() > 0)
	{
		const LLViewerInventoryCategory *cat = cat_array.at(0);
		mLibraryClothingID = cat->getUUID();
	}
	mComplete.clear();
	uuid_vec_t folders;
	folders.push_back(mClothingID);
	folders.push_back(mLibraryClothingID);
	setFetchIDs(folders);
	startFetch();
	if (isFinished())
	{
		done();
	}
}
void LLLibraryOutfitsFetch::outfitsDone()
{
	LL_INFOS() << "start" << LL_ENDL;
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	uuid_vec_t folders;
	gInventory.collectDescendents(mLibraryClothingID, cat_array, wearable_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	llassert(cat_array.size() > 0);
	for (LLInventoryModel::cat_array_t::const_iterator iter = cat_array.begin();
		 iter != cat_array.end();
		 ++iter)
	{
		const LLViewerInventoryCategory *cat = iter->get();
		if (cat->getName() != "Ruth")
		{
			folders.push_back(cat->getUUID());
			mLibraryClothingFolders.push_back(cat->getUUID());
		}
	}
	cat_array.clear();
	wearable_array.clear();
	LLNameCategoryCollector matchFolderFunctor(mImportedClothingName);
	gInventory.collectDescendentsIf(mClothingID,
									cat_array, wearable_array,
									LLInventoryModel::EXCLUDE_TRASH,
									matchFolderFunctor);
	if (cat_array.size() > 0)
	{
		const LLViewerInventoryCategory *cat = cat_array.at(0);
		mImportedClothingID = cat->getUUID();
	}
	mComplete.clear();
	setFetchIDs(folders);
	startFetch();
	if (isFinished())
	{
		done();
	}
}
class LLLibraryOutfitsCopyDone: public LLInventoryCallback
{
public:
	LLLibraryOutfitsCopyDone(LLLibraryOutfitsFetch * fetcher):
	mFireCount(0), mLibraryOutfitsFetcher(fetcher)
	{
	}
	virtual ~LLLibraryOutfitsCopyDone()
	{
		if (!LLApp::isExiting() && mLibraryOutfitsFetcher)
		{
			gInventory.addObserver(mLibraryOutfitsFetcher);
			mLibraryOutfitsFetcher->done();
		}
	}
	void fire(const LLUUID& inv_item)
	{
		mFireCount++;
	}
private:
	U32 mFireCount;
	LLLibraryOutfitsFetch * mLibraryOutfitsFetcher;
};
void LLLibraryOutfitsFetch::libraryDone()
{
	LL_INFOS() << "start" << LL_ENDL;
	if (mImportedClothingID != LLUUID::null)
	{
		importedFolderFetch();
		return;
	}
	gInventory.removeObserver(this);
	LLPointer<LLInventoryCallback> copy_waiter = new LLLibraryOutfitsCopyDone(this);
	mImportedClothingID = gInventory.createNewCategory(mClothingID,
													   LLFolderType::FT_NONE,
													   mImportedClothingName);
	for (uuid_vec_t::const_iterator iter = mLibraryClothingFolders.begin();
		 iter != mLibraryClothingFolders.end();
		 ++iter)
	{
		const LLUUID& src_folder_id = (*iter);
		const LLViewerInventoryCategory *cat = gInventory.getCategory(src_folder_id);
		if (!cat)
		{
			LL_WARNS() << "Library folder import for uuid:" << src_folder_id << " failed to find folder." << LL_ENDL;
			continue;
		}
		if (!LLAppearanceMgr::getInstance()->getCanMakeFolderIntoOutfit(src_folder_id))
		{
			LL_INFOS() << "Skipping non-outfit folder name:" << cat->getName() << LL_ENDL;
			continue;
		}
		LLNameCategoryCollector matchFolderFunctor(cat->getName());
		LLInventoryModel::cat_array_t cat_array;
		LLInventoryModel::item_array_t wearable_array;
		gInventory.collectDescendentsIf(mImportedClothingID,
										cat_array, wearable_array,
										LLInventoryModel::EXCLUDE_TRASH,
										matchFolderFunctor);
		if (cat_array.size() > 0)
		{
			continue;
		}
		LLUUID dst_folder_id = gInventory.createNewCategory(mImportedClothingID,
															LLFolderType::FT_NONE,
															cat->getName());
		LLAppearanceMgr::getInstance()->shallowCopyCategoryContents(src_folder_id, dst_folder_id, copy_waiter);
	}
}
void LLLibraryOutfitsFetch::importedFolderFetch()
{
	LL_INFOS() << "start" << LL_ENDL;
	uuid_vec_t folders;
	folders.push_back(mImportedClothingID);
	mComplete.clear();
	setFetchIDs(folders);
	startFetch();
	if (isFinished())
	{
		done();
	}
}
void LLLibraryOutfitsFetch::importedFolderDone()
{
	LL_INFOS() << "start" << LL_ENDL;
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	uuid_vec_t folders;
	gInventory.collectDescendents(mImportedClothingID, cat_array, wearable_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	for (LLInventoryModel::cat_array_t::const_iterator iter = cat_array.begin();
		 iter != cat_array.end();
		 ++iter)
	{
		const LLViewerInventoryCategory *cat = iter->get();
		folders.push_back(cat->getUUID());
		mImportedClothingFolders.push_back(cat->getUUID());
	}
	mComplete.clear();
	setFetchIDs(folders);
	startFetch();
	if (isFinished())
	{
		done();
	}
}
void LLLibraryOutfitsFetch::contentsDone()
{
	LL_INFOS() << "start" << LL_ENDL;
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	LLPointer<LLInventoryCallback> order_myoutfits_on_destroy = new LLBoostFuncInventoryCallback(no_op_inventory_func, order_my_outfits_cb);
	for (uuid_vec_t::const_iterator folder_iter = mImportedClothingFolders.begin();
		 folder_iter != mImportedClothingFolders.end();
		 ++folder_iter)
	{
		const LLUUID &folder_id = (*folder_iter);
		const LLViewerInventoryCategory *cat = gInventory.getCategory(folder_id);
		if (!cat)
		{
			LL_WARNS() << "Library folder import for uuid:" << folder_id << " failed to find folder." << LL_ENDL;
			continue;
		}
		if (cat->getName() == LLStartUp::getInitialOutfitName()) continue;
		LLUUID new_outfit_folder_id = gInventory.createNewCategory(mMyOutfitsID, LLFolderType::FT_OUTFIT, cat->getName());
		cat_array.clear();
		wearable_array.clear();
		gInventory.collectDescendents(folder_id, cat_array, wearable_array,
									  LLInventoryModel::EXCLUDE_TRASH);
		for (LLInventoryModel::item_array_t::const_iterator wearable_iter = wearable_array.begin();
			 wearable_iter != wearable_array.end();
			 ++wearable_iter)
		{
			LLConstPointer<LLInventoryObject> item = wearable_iter->get();
			link_inventory_object(new_outfit_folder_id,
								item,
								order_myoutfits_on_destroy);
		}
	}
	mOutfitsPopulated = true;
}
