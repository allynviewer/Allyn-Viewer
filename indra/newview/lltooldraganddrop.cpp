/** 
 * @file lltooldraganddrop.cpp
 * @brief LLToolDragAndDrop class implementation
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
#include "lltooldraganddrop.h"
#include "llnotificationsutil.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "lldictionary.h"
#include "llfirstuse.h"
#include "llfloateravatarpicker.h"
#include "llfloatertools.h"
#include "llgesturemgr.h"
#include "llgiveinventory.h"
#include "llhudmanager.h"
#include "llhudeffecttrail.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llinventorydefines.h"
#include "llinventoryfunctions.h"
#include "llparcel.h"
#include "llpreviewnotecard.h"
#include "llrootview.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"
#include "lltrans.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "llpanelface.h"
#include "rlvhandler.h"
#include "rlvlocks.h"
#define callMemberFunction(object,ptrToMember)  ((object).*(ptrToMember))
class LLNoPreferredType : public LLInventoryCollectFunctor
{
public:
	LLNoPreferredType() {}
	virtual ~LLNoPreferredType() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if(cat && (cat->getPreferredType() == LLFolderType::FT_NONE))
		{
			return true;
		}
		return false;
	}
};
class LLNoPreferredTypeOrItem : public LLInventoryCollectFunctor
{
public:
	LLNoPreferredTypeOrItem() {}
	virtual ~LLNoPreferredTypeOrItem() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if(item) return true;
		if(cat && (cat->getPreferredType() == LLFolderType::FT_NONE))
		{
			return true;
		}
		return false;
	}
};
class LLDroppableItem : public LLInventoryCollectFunctor
{
public:
	LLDroppableItem(BOOL is_transfer) :
		mCountLosing(0), mIsTransfer(is_transfer) {}
	virtual ~LLDroppableItem() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
	S32 countNoCopy() const { return mCountLosing; }
protected:
	S32 mCountLosing;
	BOOL mIsTransfer;
};
bool LLDroppableItem::operator()(LLInventoryCategory* cat,
				 LLInventoryItem* item)
{
	bool allowed = false;
	if (item)
	{
		allowed = itemTransferCommonlyAllowed(item);
		if (allowed
		   && mIsTransfer
		   && !item->getPermissions().allowOperationBy(PERM_TRANSFER,
							       gAgent.getID()))
		{
			allowed = false;
		}
		if (allowed && !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			++mCountLosing;
		}
	}
	return allowed;
}
class LLDropCopyableItems : public LLInventoryCollectFunctor
{
public:
	LLDropCopyableItems() {}
	virtual ~LLDropCopyableItems() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
};
bool LLDropCopyableItems::operator()(
	LLInventoryCategory* cat,
	LLInventoryItem* item)
{
	bool allowed = false;
	if(item)
	{
		allowed = itemTransferCommonlyAllowed(item);
		if(allowed &&
		   !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			allowed = false;
		}
	}
	return allowed;
}
class LLCategoryFireAndForget : public LLInventoryFetchComboObserver
{
public:
	LLCategoryFireAndForget(const uuid_vec_t& folder_ids,
							const uuid_vec_t& item_ids) :
		LLInventoryFetchComboObserver(folder_ids, item_ids)
	{}
	~LLCategoryFireAndForget() {}
	virtual void done()
	{
		LL_DEBUGS() << "LLCategoryFireAndForget::done()" << LL_ENDL;
	}
};
class LLCategoryDropObserver : public LLInventoryFetchItemsObserver
{
public:
	LLCategoryDropObserver(
		const uuid_vec_t& ids,
		const LLUUID& obj_id, LLToolDragAndDrop::ESource src) :
		LLInventoryFetchItemsObserver(ids),
		mObjectID(obj_id),
		mSource(src)
	{}
	~LLCategoryDropObserver() {}
	virtual void done();
protected:
	LLUUID mObjectID;
	LLToolDragAndDrop::ESource mSource;
};
void LLCategoryDropObserver::done()
{
	gInventory.removeObserver(this);
	LLViewerObject* dst_obj = gObjectList.findObject(mObjectID);
	if(dst_obj)
	{
 		LLInventoryItem* item = NULL;
  		uuid_vec_t::iterator it = mComplete.begin();
  		uuid_vec_t::iterator end = mComplete.end();
  		for(; it < end; ++it)
  		{
 			item = gInventory.getItem(*it);
 			if(item)
 			{
 				LLToolDragAndDrop::dropInventory(
 					dst_obj,
 					item,
 					mSource,
 					LLUUID::null);
 			}
  		}
	}
	delete this;
}
S32 LLToolDragAndDrop::sOperationId = 0;
LLToolDragAndDrop::DragAndDropEntry::DragAndDropEntry(dragOrDrop3dImpl f_none,
													  dragOrDrop3dImpl f_self,
													  dragOrDrop3dImpl f_avatar,
													  dragOrDrop3dImpl f_object,
													  dragOrDrop3dImpl f_land) :
	LLDictionaryEntry("")
{
	mFunctions[DT_NONE] = f_none;
	mFunctions[DT_SELF] = f_self;
	mFunctions[DT_AVATAR] = f_avatar;
	mFunctions[DT_OBJECT] = f_object;
	mFunctions[DT_LAND] = f_land;
}
LLToolDragAndDrop::dragOrDrop3dImpl LLToolDragAndDrop::LLDragAndDropDictionary::get(EDragAndDropType dad_type, LLToolDragAndDrop::EDropTarget drop_target)
{
	const DragAndDropEntry *entry = lookup(dad_type);
	if (entry)
	{
		return (entry->mFunctions[(U8)drop_target]);
	}
	return &LLToolDragAndDrop::dad3dNULL;
}
LLToolDragAndDrop::LLDragAndDropDictionary::LLDragAndDropDictionary()
{
	addEntry(DAD_NONE, 			new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,						&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_TEXTURE, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dTextureObject,				&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_SOUND, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_CALLINGCARD, 	new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_LANDMARK, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dNULL, 					&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_SCRIPT, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dNULL, 					&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dRezScript, 				&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_CLOTHING, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dWearItem, 				&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_OBJECT, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dRezAttachmentFromInv,	&LLToolDragAndDrop::dad3dGiveInventoryObject,	&LLToolDragAndDrop::dad3dRezObjectOnObject, 		&LLToolDragAndDrop::dad3dRezObjectOnLand));
	addEntry(DAD_NOTECARD, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dNULL, 					&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_CATEGORY, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dWearCategory,			&LLToolDragAndDrop::dad3dGiveInventoryCategory,	&LLToolDragAndDrop::dad3dUpdateInventoryCategory,	&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_ROOT_CATEGORY, new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,						&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_BODYPART, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dWearItem,				&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_ANIMATION, 	new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_GESTURE, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dActivateGesture,		&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_LINK, 			new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,						&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_MESH, 			new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dMeshObject,				&LLToolDragAndDrop::dad3dNULL));
};
LLToolDragAndDrop::LLToolDragAndDrop()
:	LLTool(std::string("draganddrop"), NULL),
	mCargoCount(0),
	mDragStartX(0),
	mDragStartY(0),
	mSource(SOURCE_AGENT),
	mCursor(UI_CURSOR_NO),
	mLastAccept(ACCEPT_NO),
	mDrop(FALSE),
	mCurItemIndex(0)
{
}
void LLToolDragAndDrop::setDragStart(S32 x, S32 y)
{
	mDragStartX = x;
	mDragStartY = y;
}
BOOL LLToolDragAndDrop::isOverThreshold(S32 x,S32 y)
{
	static LLCachedControl<S32> drag_and_drop_threshold(gSavedSettings,"DragAndDropDistanceThreshold", 3);
	S32 mouse_delta_x = x - mDragStartX;
	S32 mouse_delta_y = y - mDragStartY;
	return (mouse_delta_x * mouse_delta_x) + (mouse_delta_y * mouse_delta_y) > drag_and_drop_threshold * drag_and_drop_threshold;
}
void LLToolDragAndDrop::beginDrag(EDragAndDropType type,
								  const LLUUID& cargo_id,
								  ESource source,
								  const LLUUID& source_id,
								  const LLUUID& object_id)
{
	if(type == DAD_NONE)
	{
		LL_WARNS() << "Attempted to start drag without a cargo type" << LL_ENDL;
		return;
	}
	mCargoTypes.clear();
	mCargoTypes.push_back(type);
	mCargoIDs.clear();
	mCargoIDs.push_back(cargo_id);
	mSource = source;
	mSourceID = source_id;
	mObjectID = object_id;
	setMouseCapture( TRUE );
	LLToolMgr::getInstance()->setTransientTool( this );
	mCursor = UI_CURSOR_NO;
	if((mCargoTypes[0] == DAD_CATEGORY)
	   && ((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY)))
	{
		LLInventoryCategory* cat = gInventory.getCategory(cargo_id);
		if(cat)
		{
			LLViewerInventoryCategory::cat_array_t cats;
			LLViewerInventoryItem::item_array_t items;
			LLNoPreferredTypeOrItem is_not_preferred;
			uuid_vec_t folder_ids;
			uuid_vec_t item_ids;
			if(is_not_preferred(cat, NULL))
			{
				folder_ids.push_back(cargo_id);
			}
			gInventory.collectDescendentsIf(
				cargo_id,
				cats,
				items,
				LLInventoryModel::EXCLUDE_TRASH,
				is_not_preferred);
			S32 count = cats.size();
			S32 i;
			for(i = 0; i < count; ++i)
			{
				folder_ids.push_back(cats.at(i)->getUUID());
			}
			count = items.size();
			for(i = 0; i < count; ++i)
			{
				item_ids.push_back(items.at(i)->getUUID());
			}
			if(!folder_ids.empty() || !item_ids.empty())
			{
				LLCategoryFireAndForget *fetcher = new LLCategoryFireAndForget(folder_ids, item_ids);
				fetcher->startFetch();
				delete fetcher;
			}
		}
	}
}
void LLToolDragAndDrop::beginMultiDrag(
	const std::vector<EDragAndDropType> types,
	const uuid_vec_t& cargo_ids,
	ESource source,
	const LLUUID& source_id)
{
	std::vector<EDragAndDropType>::const_iterator types_it;
	for (types_it = types.begin(); types_it != types.end(); ++types_it)
	{
		if(DAD_NONE == *types_it)
		{
			LL_WARNS() << "Attempted to start drag without a cargo type" << LL_ENDL;
			return;
		}
	}
	mCargoTypes = types;
	mCargoIDs = cargo_ids;
	mSource = source;
	mSourceID = source_id;
	setMouseCapture( TRUE );
	LLToolMgr::getInstance()->setTransientTool( this );
	mCursor = UI_CURSOR_NO;
	if((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY))
	{
		LLInventoryCategory* cat = NULL;
		S32 count = llmin(cargo_ids.size(), types.size());
		uuid_set_t cat_ids;
		for(S32 i = 0; i < count; ++i)
		{
			cat = gInventory.getCategory(cargo_ids[i]);
			if(cat)
			{
				LLViewerInventoryCategory::cat_array_t cats;
				LLViewerInventoryItem::item_array_t items;
				LLNoPreferredType is_not_preferred;
				if(is_not_preferred(cat, NULL))
				{
					cat_ids.insert(cat->getUUID());
				}
				gInventory.collectDescendentsIf(
					cat->getUUID(),
					cats,
					items,
					LLInventoryModel::EXCLUDE_TRASH,
					is_not_preferred);
				S32 cat_count = cats.size();
				for(S32 i = 0; i < cat_count; ++i)
				{
					cat_ids.insert(cat->getUUID());
				}
			}
		}
		if(!cat_ids.empty())
		{
			uuid_vec_t folder_ids;
			uuid_vec_t item_ids;
			std::back_insert_iterator<uuid_vec_t> copier(folder_ids);
			std::copy(cat_ids.begin(), cat_ids.end(), copier);
			LLCategoryFireAndForget fetcher(folder_ids, item_ids);
		}
	}
}
void LLToolDragAndDrop::endDrag()
{
	mEndDragSignal();
	LLSelectMgr::getInstance()->unhighlightAll();
	setMouseCapture(FALSE);
}
void LLToolDragAndDrop::onMouseCaptureLost()
{
	LLToolMgr::getInstance()->clearTransientTool();
	mCargoTypes.clear();
	mCargoIDs.clear();
	mSource = SOURCE_AGENT;
	mSourceID.setNull();
	mObjectID.setNull();
}
BOOL LLToolDragAndDrop::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if( hasMouseCapture() )
	{
		EAcceptance acceptance = ACCEPT_NO;
		dragOrDrop( x, y, mask, TRUE, &acceptance );
		endDrag();
	}
	return TRUE;
}
ECursorType LLToolDragAndDrop::acceptanceToCursor( EAcceptance acceptance )
{
	switch( acceptance )
	{
	case ACCEPT_YES_MULTI:
		if (mCargoIDs.size() > 1)
		{
			mCursor = UI_CURSOR_ARROWDRAGMULTI;
		}
		else
		{
			mCursor = UI_CURSOR_ARROWDRAG;
		}
		break;
	case ACCEPT_YES_SINGLE:
		if (mCargoIDs.size() > 1)
		{
			mToolTipMsg = LLTrans::getString("TooltipMustSingleDrop");
			mCursor = UI_CURSOR_NO;
		}
		else
		{
			mCursor = UI_CURSOR_ARROWDRAG;
		}
		break;
	case ACCEPT_NO_LOCKED:
		mCursor = UI_CURSOR_NOLOCKED;
		break;
	case ACCEPT_NO:
		mCursor = UI_CURSOR_NO;
		break;
	case ACCEPT_YES_COPY_MULTI:
		if (mCargoIDs.size() > 1)
		{
			mCursor = UI_CURSOR_ARROWCOPYMULTI;
		}
		else
		{
			mCursor = UI_CURSOR_ARROWCOPY;
		}
		break;
	case ACCEPT_YES_COPY_SINGLE:
		if (mCargoIDs.size() > 1)
		{
			mToolTipMsg = LLTrans::getString("TooltipMustSingleDrop");
			mCursor = UI_CURSOR_NO;
		}
		else
		{
			mCursor = UI_CURSOR_ARROWCOPY;
		}
		break;
	case ACCEPT_POSTPONED:
		break;
	default:
		llassert( FALSE );
	}
	return mCursor;
}
BOOL LLToolDragAndDrop::handleHover( S32 x, S32 y, MASK mask )
{
	EAcceptance acceptance = ACCEPT_NO;
	dragOrDrop( x, y, mask, FALSE, &acceptance );
	ECursorType cursor = acceptanceToCursor(acceptance);
	gViewerWindow->getWindow()->setCursor( cursor );
	LL_DEBUGS("UserInput") << "hover handled by LLToolDragAndDrop" << LL_ENDL;
	return TRUE;
}
BOOL LLToolDragAndDrop::handleKey(KEY key, MASK mask)
{
	if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		endDrag();
		return TRUE;
	}
	return FALSE;
}
BOOL LLToolDragAndDrop::handleToolTip(S32 x, S32 y, std::string& msg, LLRect *sticky_rect_screen)
{
	if (!mToolTipMsg.empty())
	{
		msg = mToolTipMsg;
		return TRUE;
	}
	return FALSE;
}
void LLToolDragAndDrop::handleDeselect()
{
	mToolTipMsg.clear();
}
void LLToolDragAndDrop::dragOrDrop( S32 x, S32 y, MASK mask, BOOL drop,
								   EAcceptance* acceptance)
{
	*acceptance = ACCEPT_YES_MULTI;
	BOOL handled = FALSE;
	LLView* top_view = gFocusMgr.getTopCtrl();
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	mToolTipMsg.clear();
	if (drop)
	{
		sOperationId++;
	}
	bool is_uuid_dragged = (mSource == SOURCE_PEOPLE);
	if(top_view)
	{
		handled = TRUE;
		for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
		{
			S32 local_x, local_y;
			top_view->screenPointToLocal( x, y, &local_x, &local_y );
			EAcceptance item_acceptance = ACCEPT_NO;
			LLInventoryObject* cargo = locateInventory(item, cat);
			if (cargo)
			{
				handled = handled && top_view->handleDragAndDrop(local_x, local_y, mask, FALSE,
													mCargoTypes[mCurItemIndex],
													(void*)cargo,
													&item_acceptance,
													mToolTipMsg);
			}
			else if (is_uuid_dragged)
			{
				handled = handled && top_view->handleDragAndDrop(local_x, local_y, mask, FALSE,
													mCargoTypes[mCurItemIndex],
													(void*)&mCargoIDs[mCurItemIndex],
													&item_acceptance,
													mToolTipMsg);
			}
			if (handled)
			{
				*acceptance = (EAcceptance)llmin((U32)item_acceptance, (U32)*acceptance);
			}
		}
		if (handled && drop && (U32)*acceptance >= ACCEPT_YES_COPY_SINGLE)
		{
			if ((U32)*acceptance < ACCEPT_YES_COPY_MULTI &&
			    mCargoIDs.size() > 1)
			{
				*acceptance = ACCEPT_NO;
				return;
			}
			for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
			{
				S32 local_x, local_y;
				EAcceptance item_acceptance;
				top_view->screenPointToLocal( x, y, &local_x, &local_y );
				LLInventoryObject* cargo = locateInventory(item, cat);
				if (cargo)
				{
					handled = handled && top_view->handleDragAndDrop(local_x, local_y, mask, TRUE,
														mCargoTypes[mCurItemIndex],
														(void*)cargo,
														&item_acceptance,
														mToolTipMsg);
				}
				else if (is_uuid_dragged)
				{
					handled = handled && top_view->handleDragAndDrop(local_x, local_y, mask, FALSE,
														mCargoTypes[mCurItemIndex],
														(void*)&mCargoIDs[mCurItemIndex],
														&item_acceptance,
														mToolTipMsg);
				}
			}
		}
		if (handled)
		{
			mLastAccept = (EAcceptance)*acceptance;
		}
	}
	if(!handled)
	{
		handled = TRUE;
		LLView* root_view = gViewerWindow->getRootView();
		for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
		{
			EAcceptance item_acceptance = ACCEPT_NO;
			LLInventoryObject* cargo = locateInventory(item, cat);
			if (cargo)
			{
				handled = handled && root_view->handleDragAndDrop(x, y, mask, FALSE,
													mCargoTypes[mCurItemIndex],
													(void*)cargo,
													&item_acceptance,
													mToolTipMsg);
			}
			else if (is_uuid_dragged)
			{
				handled = handled && root_view->handleDragAndDrop(x, y, mask, FALSE,
													mCargoTypes[mCurItemIndex],
													(void*)&mCargoIDs[mCurItemIndex],
													&item_acceptance,
													mToolTipMsg);
			}
			if (handled)
			{
				*acceptance = (EAcceptance)llmin((U32)item_acceptance, (U32)*acceptance);
			}
		}
		if (handled && drop && (U32)*acceptance > ACCEPT_NO_LOCKED)
		{
			if ((U32)*acceptance < ACCEPT_YES_COPY_MULTI &&
			    mCargoIDs.size() > 1)
			{
				*acceptance = ACCEPT_NO;
				return;
			}
			for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
			{
				EAcceptance item_acceptance;
				LLInventoryObject* cargo = locateInventory(item, cat);
				if (cargo)
				{
					handled = handled && root_view->handleDragAndDrop(x, y, mask, TRUE,
											  mCargoTypes[mCurItemIndex],
											  (void*)cargo,
											  &item_acceptance,
											  mToolTipMsg);
				}
				else if (is_uuid_dragged)
				{
					handled = handled && root_view->handleDragAndDrop(x, y, mask, TRUE,
											  mCargoTypes[mCurItemIndex],
											  (void*)&mCargoIDs[mCurItemIndex],
											  &item_acceptance,
											  mToolTipMsg);
				}
			}
		}
		if (handled)
		{
			mLastAccept = (EAcceptance)*acceptance;
		}
	}
	if ( !handled )
	{
		const LLUUID outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);
		if (outbox_id.notNull())
		{
			for (S32 item_index = 0; item_index < (S32)mCargoIDs.size(); item_index++)
			{
				if (gInventory.isObjectDescendentOf(mCargoIDs[item_index], outbox_id))
				{
					*acceptance = ACCEPT_NO;
					mToolTipMsg = LLTrans::getString("TooltipOutboxDragToWorld");
					return;
				}
			}
		}
		const LLUUID marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
		if (marketplacelistings_id.notNull())
		{
			for (S32 item_index = 0; item_index < (S32)mCargoIDs.size(); item_index++)
			{
				if (gInventory.isObjectDescendentOf(mCargoIDs[item_index], outbox_id))
				{
					*acceptance = ACCEPT_NO;
					mToolTipMsg = LLTrans::getString("TooltipOutboxDragToWorld");
					return;
				}
			}
		}
		dragOrDrop3D( x, y, mask, drop, acceptance );
	}
}
void LLToolDragAndDrop::dragOrDrop3D( S32 x, S32 y, MASK mask, BOOL drop, EAcceptance* acceptance )
{
	mDrop = drop;
	if (mDrop)
	{
		pick(gViewerWindow->pickImmediate(x, y, FALSE, FALSE));
	}
	else
	{
		gViewerWindow->pickAsync(x, y, mask, pickCallback, FALSE, FALSE);
	}
	*acceptance = mLastAccept;
}
void LLToolDragAndDrop::pickCallback(const LLPickInfo& pick_info)
{
	if (getInstance() != NULL)
	{
		getInstance()->pick(pick_info);
	}
}
void LLToolDragAndDrop::pick(const LLPickInfo& pick_info)
{
	EDropTarget target = DT_NONE;
	S32	hit_face = -1;
	LLViewerObject* hit_obj = pick_info.getObject();
	LLSelectMgr::getInstance()->unhighlightAll();
	bool highlight_object = false;
	if (hit_obj != NULL)
	{
		if (pick_info.mPickType == LLPickInfo::PICK_FLORA)
		{
			mCursor = UI_CURSOR_NO;
			gViewerWindow->getWindow()->setCursor( mCursor );
			return;
		}
		if (hit_obj->isAttachment() && !hit_obj->isHUDAttachment())
		{
			LLVOAvatar* avatar = LLVOAvatar::findAvatarFromAttachment( hit_obj );
			if (!avatar)
			{
				mLastAccept = ACCEPT_NO;
				mCursor = UI_CURSOR_NO;
				gViewerWindow->getWindow()->setCursor( mCursor );
				return;
			}
			hit_obj = avatar;
		}
		if(hit_obj->isAvatar())
		{
			if(((LLVOAvatar*) hit_obj)->isSelf())
			{
				target = DT_SELF;
				hit_face = -1;
			}
			else
			{
				target = DT_AVATAR;
				hit_face = -1;
			}
		}
		else
		{
			target = DT_OBJECT;
			hit_face = pick_info.mObjectFace;
			highlight_object = true;
		}
	}
	else if(pick_info.mPickType == LLPickInfo::PICK_LAND)
	{
		target = DT_LAND;
		hit_face = -1;
	}
	mLastAccept = ACCEPT_YES_MULTI;
	for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
	{
		const S32 item_index = mCurItemIndex;
		const EDragAndDropType dad_type = mCargoTypes[item_index];
		mLastAccept = (EAcceptance)llmin(
			(U32)mLastAccept,
			(U32)callMemberFunction(*this,
									LLDragAndDropDictionary::instance().get(dad_type, target))
				(hit_obj, hit_face, pick_info.mKeyMask, FALSE));
	}
	if (mDrop && ((U32)mLastAccept >= ACCEPT_YES_COPY_SINGLE))
	{
		if ((mLastAccept >= ACCEPT_YES_COPY_MULTI) || (mCargoIDs.size() == 1))
		{
			for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
			{
				const S32 item_index = mCurItemIndex;
				const EDragAndDropType dad_type = mCargoTypes[item_index];
				callMemberFunction(*this, LLDragAndDropDictionary::instance().get(dad_type, target))
					(hit_obj, hit_face, pick_info.mKeyMask, TRUE);
			}
		}
		else
		{
			mLastAccept = ACCEPT_NO;
		}
	}
	if (highlight_object && mLastAccept > ACCEPT_NO_LOCKED)
	{
		for (S32 i = 0; i < (S32)mCargoIDs.size(); i++)
		{
			if (mCargoTypes[i] != DAD_OBJECT || (pick_info.mKeyMask & MASK_CONTROL))
			{
				LLSelectMgr::getInstance()->highlightObjectAndFamily(hit_obj);
				break;
			}
		}
	}
	ECursorType cursor = acceptanceToCursor( mLastAccept );
	gViewerWindow->getWindow()->setCursor( cursor );
	mLastHitPos = pick_info.mPosGlobal;
	mLastCameraPos = gAgentCamera.getCameraPositionGlobal();
}
BOOL LLToolDragAndDrop::handleDropTextureProtections(LLViewerObject* hit_obj,
													 LLInventoryItem* item,
													 LLToolDragAndDrop::ESource source,
													 const LLUUID& src_id)
{
	if(SOURCE_LIBRARY == source)
	{
		return TRUE;
	}
	if (hit_obj->isInventoryDirty() && hit_obj->hasInventoryListeners())
	{
		hit_obj->requestInventory();
		LLSD args;
		args["ERROR_MESSAGE"] = "Unable to add texture.\nPlease wait a few seconds and try again.";
		LLNotificationsUtil::add("ErrorMessage", args);
		return FALSE;
	}
	if (hit_obj->getInventoryItemByAsset(item->getAssetUUID()))
	{
		return TRUE;
	}
	if (!item) return FALSE;
	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
	if(!item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID()))
	{
		if (willObjectAcceptInventory(hit_obj,item) < ACCEPT_YES_COPY_SINGLE )
		{
			return FALSE;
		}
		if(SOURCE_AGENT == source)
		{
			gInventory.deleteObject(item->getUUID());
			gInventory.notifyObservers();
		}
		else if(SOURCE_WORLD == source)
		{
			LLViewerObject* src_obj = gObjectList.findObject(src_id);
			if(src_obj)
			{
				src_obj->removeInventory(item->getUUID());
			}
			else
			{
				LL_WARNS() << "Unable to find source object." << LL_ENDL;
				return FALSE;
			}
		}
		if (LLAssetType::AT_TEXTURE == new_item->getType())
		{
			hit_obj->updateTextureInventory(new_item, TASK_INVENTORY_ITEM_KEY, true);
		}
		else
		{
			hit_obj->updateInventory(new_item, TASK_INVENTORY_ITEM_KEY, true);
		}
	}
	else if(!item->getPermissions().allowOperationBy(PERM_TRANSFER,
													 gAgent.getID()))
	{
		if (willObjectAcceptInventory(hit_obj,item) < ACCEPT_YES_COPY_SINGLE )
		{
			return FALSE;
		}
		if (LLAssetType::AT_TEXTURE == new_item->getType())
		{
			hit_obj->updateTextureInventory(new_item, TASK_INVENTORY_ITEM_KEY, true);
		}
		else
		{
			hit_obj->updateInventory(new_item, TASK_INVENTORY_ITEM_KEY, true);
		}
		hit_obj->dirtyInventory();
		hit_obj->requestInventory();
	}
	return TRUE;
}
void LLToolDragAndDrop::dropTextureAllFaces(LLViewerObject* hit_obj,
											LLInventoryItem* item,
											LLToolDragAndDrop::ESource source,
											const LLUUID& src_id)
{
	if (!item)
	{
		LL_WARNS() << "LLToolDragAndDrop::dropTextureAllFaces no texture item." << LL_ENDL;
		return;
	}
	LLUUID asset_id = item->getAssetUUID();
	BOOL success = handleDropTextureProtections(hit_obj, item, source, src_id);
	if(!success)
	{
		return;
	}
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_EDIT_TEXTURE_COUNT );
	S32 num_faces = hit_obj->getNumTEs();
	for( S32 face = 0; face < num_faces; face++ )
	{
		hit_obj->setTETexture(face, asset_id);
		dialog_refresh_all();
	}
	hit_obj->sendTEUpdate();
}
void LLToolDragAndDrop::dropMesh(LLViewerObject* hit_obj,
								 LLInventoryItem* item,
								 LLToolDragAndDrop::ESource source,
								 const LLUUID& src_id)
{
	if (!item)
	{
		LL_WARNS() << "no inventory item." << LL_ENDL;
		return;
	}
	LLUUID asset_id = item->getAssetUUID();
	BOOL success = handleDropTextureProtections(hit_obj, item, source, src_id);
	if(!success)
	{
		return;
	}
	LLSculptParams sculpt_params;
	sculpt_params.setSculptTexture(asset_id, LL_SCULPT_TYPE_MESH);
	hit_obj->setParameterEntry(LLNetworkData::PARAMS_SCULPT, sculpt_params, TRUE);
	dialog_refresh_all();
}
void LLToolDragAndDrop::dropTextureOneFace(LLViewerObject* hit_obj,
										   S32 hit_face,
										   LLInventoryItem* item,
										   LLToolDragAndDrop::ESource source,
										   const LLUUID& src_id)
{
	if (hit_face == -1) return;
	if (!item)
	{
		LL_WARNS() << "LLToolDragAndDrop::dropTextureOneFace no texture item." << LL_ENDL;
		return;
	}
	LLUUID asset_id = item->getAssetUUID();
	BOOL success = handleDropTextureProtections(hit_obj, item, source, src_id);
	if(!success)
	{
		return;
	}
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_EDIT_TEXTURE_COUNT );
	LLTextureEntry* tep = hit_obj ? (hit_obj->getTE(hit_face)) : NULL;
	LLPanelFace* panel_face = gFloaterTools->getPanelFace();
	if (gFloaterTools->getVisible() && panel_face)
	{
		switch (LLSelectMgr::getInstance()->getTextureChannel())
		{
		case 0:
		default:
			{
				hit_obj->setTETexture(hit_face, asset_id);
			}
		break;
		case 1:
			{
				LLMaterialPtr old_mat = tep->getMaterialParams();
				LLMaterialPtr new_mat = panel_face->createDefaultMaterial(old_mat);
				new_mat->setNormalID(asset_id);
				tep->setMaterialParams(new_mat);
				hit_obj->setTENormalMap(hit_face, asset_id);
				LLMaterialMgr::getInstance()->put(hit_obj->getID(), hit_face, *new_mat);
			}
			break;
		case 2:
			{
				LLMaterialPtr old_mat = tep->getMaterialParams();
				LLMaterialPtr new_mat = panel_face->createDefaultMaterial(old_mat);
				new_mat->setSpecularID(asset_id);
				tep->setMaterialParams(new_mat);
				hit_obj->setTESpecularMap(hit_face, asset_id);
				LLMaterialMgr::getInstance()->put(hit_obj->getID(), hit_face, *new_mat);
			}
			break;
		}
	}
	else if (hit_obj)
	{
		hit_obj->setTETexture(hit_face, asset_id);
	}
	dialog_refresh_all();
	hit_obj->sendTEUpdate();
}
void LLToolDragAndDrop::dropScript(LLViewerObject* hit_obj,
								   LLInventoryItem* item,
								   BOOL active,
								   ESource source,
								   const LLUUID& src_id)
{
	if((SOURCE_WORLD == LLToolDragAndDrop::getInstance()->mSource)
	   || (SOURCE_NOTECARD == LLToolDragAndDrop::getInstance()->mSource))
	{
		LL_WARNS() << "Call to LLToolDragAndDrop::dropScript() from world"
			<< " or notecard." << LL_ENDL;
		return;
	}
	if(hit_obj && item)
	{
		LLPointer<LLViewerInventoryItem> new_script = new LLViewerInventoryItem(item);
		if(!item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			if(SOURCE_AGENT == source)
			{
				gInventory.deleteObject(item->getUUID());
				gInventory.notifyObservers();
			}
			else if(SOURCE_WORLD == source)
			{
				LLViewerObject* src_obj = gObjectList.findObject(src_id);
				if(src_obj)
				{
					src_obj->removeInventory(item->getUUID());
				}
				else
				{
					LL_WARNS() << "Unable to find source object." << LL_ENDL;
					return;
				}
			}
		}
		hit_obj->saveScript(new_script, active, true);
		gFloaterTools->dirty();
		if (gSavedSettings.getBOOL("BroadcastViewerEffects"))
		{
			LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
			effectp->setSourceObject(gAgentAvatarp);
			effectp->setTargetObject(hit_obj);
			effectp->setDuration(LL_HUD_DUR_SHORT);
			effectp->setColor(LLColor4U(gAgent.getEffectColor()));
		}
	}
}
void LLToolDragAndDrop::dropObject(LLViewerObject* raycast_target,
								   BOOL bypass_sim_raycast,
								   BOOL from_task_inventory,
								   BOOL remove_from_inventory)
{
	LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromPosGlobal(mLastHitPos);
	if (!regionp)
	{
		LL_WARNS() << "Couldn't find region to rez object" << LL_ENDL;
		return;
	}
	if ( (rlv_handler_t::isEnabled()) && ((gRlvHandler.hasBehaviour(RLV_BHVR_REZ)) || (gRlvHandler.hasBehaviour(RLV_BHVR_INTERACT))) )
	{
		return;
	}
	make_ui_sound("UISndObjectRezIn");
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return;
	if (regionp
		&& regionp->getRegionFlag(REGION_FLAGS_SANDBOX))
	{
		LLFirstUse::useSandbox();
	}
	if(!remove_from_inventory
		&& !item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		remove_from_inventory = TRUE;
	}
	LLUUID ray_target_id;
	if( raycast_target )
	{
		ray_target_id = raycast_target->getID();
	}
	else
	{
		ray_target_id.setNull();
	}
	bool is_in_trash = false;
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if (gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
	{
		is_in_trash = true;
		remove_from_inventory = TRUE;
	}
	LLUUID source_id = from_task_inventory ? mSourceID : LLUUID::null;
	BOOL rez_selected = LLToolMgr::getInstance()->inEdit();
	LLVector3 ray_start = regionp->getPosRegionFromGlobal(mLastCameraPos);
	LLVector3 ray_end   = regionp->getPosRegionFromGlobal(mLastHitPos);
	if (bypass_sim_raycast == FALSE)
	{
		LLVector3 ray_direction = ray_start - ray_end;
		ray_end = ray_end - ray_direction;
	}
	LLMessageSystem* msg = gMessageSystem;
	if (mSource == SOURCE_NOTECARD)
	{
		msg->newMessageFast(_PREHASH_RezObjectFromNotecard);
	}
	else
	{
		msg->newMessageFast(_PREHASH_RezObject);
	}
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,  gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,  gAgent.getSessionID());
	static LLCachedControl<bool> AlchemyRezUnderLandGroup(gSavedSettings, "AscentAlwaysRezInGroup");
	LLUUID group_id = gAgent.getGroupID();
	if (AlchemyRezUnderLandGroup)
	{
		LLParcel* land_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
		if (gAgent.isInGroup(land_parcel->getGroupID()))
			group_id = land_parcel->getGroupID();
		else if (gAgent.isInGroup(land_parcel->getOwnerID()))
			group_id = land_parcel->getOwnerID();
	}
	msg->addUUIDFast(_PREHASH_GroupID, group_id);
	msg->nextBlock("RezData");
	msg->addUUIDFast(_PREHASH_FromTaskID, source_id);
	msg->addU8Fast(_PREHASH_BypassRaycast, (U8) bypass_sim_raycast);
	msg->addVector3Fast(_PREHASH_RayStart, ray_start);
	msg->addVector3Fast(_PREHASH_RayEnd, ray_end);
	msg->addUUIDFast(_PREHASH_RayTargetID, ray_target_id );
	msg->addBOOLFast(_PREHASH_RayEndIsIntersection, FALSE);
	msg->addBOOLFast(_PREHASH_RezSelected, rez_selected);
	msg->addBOOLFast(_PREHASH_RemoveItem, remove_from_inventory);
	pack_permissions_slam(msg, item->getFlags(), item->getPermissions());
	LLUUID folder_id = item->getParentUUID();
	if((SOURCE_LIBRARY == mSource) || (is_in_trash))
	{
		item->setParent(LLUUID::null);
	}
	if (mSource == SOURCE_NOTECARD)
	{
		msg->nextBlockFast(_PREHASH_NotecardData);
		msg->addUUIDFast(_PREHASH_NotecardItemID, mSourceID);
		msg->addUUIDFast(_PREHASH_ObjectID, mObjectID);
		msg->nextBlockFast(_PREHASH_InventoryData);
		msg->addUUIDFast(_PREHASH_ItemID, item->getUUID());
	}
	else
	{
		msg->nextBlockFast(_PREHASH_InventoryData);
		item->packMessage(msg);
	}
	msg->sendReliable(regionp->getHost());
	item->setParent(folder_id);
	if (rez_selected)
	{
		LLSelectMgr::getInstance()->deselectAll();
		gViewerWindow->getWindow()->incBusyCount();
	}
	if(remove_from_inventory)
	{
		gInventory.deleteObject(item->getUUID());
		gInventory.notifyObservers();
	}
	if (gSavedSettings.getBOOL("BroadcastViewerEffects"))
	{
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
		effectp->setSourceObject(gAgentAvatarp);
		effectp->setPositionGlobal(mLastHitPos);
		effectp->setDuration(LL_HUD_DUR_SHORT);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	}
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_REZ_COUNT);
}
void LLToolDragAndDrop::dropInventory(LLViewerObject* hit_obj,
									  LLInventoryItem* item,
									  LLToolDragAndDrop::ESource source,
									  const LLUUID& src_id)
{
	if((SOURCE_WORLD == LLToolDragAndDrop::getInstance()->mSource)
	   || (SOURCE_NOTECARD == LLToolDragAndDrop::getInstance()->mSource))
	{
		LL_WARNS() << "Call to LLToolDragAndDrop::dropInventory() from world"
			<< " or notecard." << LL_ENDL;
		return;
	}
	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
	time_t creation_date = time_corrected();
	new_item->setCreationDate(creation_date);
	if(!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		if(SOURCE_AGENT == source)
		{
			gInventory.deleteObject(item->getUUID());
			gInventory.notifyObservers();
		}
		else if(SOURCE_WORLD == source)
		{
			LLViewerObject* src_obj = gObjectList.findObject(src_id);
			if(src_obj)
			{
				src_obj->removeInventory(item->getUUID());
			}
			else
			{
				LL_WARNS() << "Unable to find source object." << LL_ENDL;
				return;
			}
		}
	}
	hit_obj->updateInventory(new_item, TASK_INVENTORY_ITEM_KEY, true);
	if (gFloaterTools->getVisible())
	{
		gFloaterTools->showPanel(LLFloaterTools::PANEL_CONTENTS);
	}
	if (gSavedSettings.getBOOL("BroadcastViewerEffects"))
	{
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
		effectp->setSourceObject(gAgentAvatarp);
		effectp->setTargetObject(hit_obj);
		effectp->setDuration(LL_HUD_DUR_SHORT);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	}
	gFloaterTools->dirty();
}
EAcceptance LLToolDragAndDrop::willObjectAcceptInventory(LLViewerObject* obj, LLInventoryItem* item)
{
	if(!item || !obj) return ACCEPT_NO;
	LLViewerInventoryItem* vitem = (LLViewerInventoryItem*)item;
	if(!vitem->isFinished()) return ACCEPT_NO;
	if (vitem->getIsLinkType()) return ACCEPT_NO;
	if(obj->getID() == item->getParentUUID())
	{
		return ACCEPT_NO;
	}
	BOOL worn = FALSE;
	LLVOAvatarSelf* my_avatar = NULL;
	switch(item->getType())
	{
	case LLAssetType::AT_OBJECT:
		my_avatar = gAgentAvatarp;
		if(my_avatar && my_avatar->isWearingAttachment(item->getUUID()))
		{
				worn = TRUE;
		}
		break;
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_CLOTHING:
		if(gAgentWearables.isWearingItem(item->getUUID()))
		{
			worn = TRUE;
		}
		break;
	case LLAssetType::AT_CALLINGCARD:
	default:
			break;
	}
	const LLPermissions& perm = item->getPermissions();
	BOOL modify = (obj->permModify() || obj->flagAllowInventoryAdd());
	BOOL transfer = FALSE;
	if((obj->permYouOwner() && (perm.getOwner() == gAgent.getID()))
	   || perm.allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		transfer = TRUE;
	}
	BOOL volume = (LL_PCODE_VOLUME == obj->getPCode());
	BOOL attached = false;
	BOOL unrestricted = ((perm.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED) ? TRUE : FALSE;
	if (rlv_handler_t::isEnabled())
	{
		const LLViewerObject* pObjRoot = obj->getRootEdit();
		if (gRlvAttachmentLocks.isLockedAttachment(pObjRoot))
		{
			return ACCEPT_NO_LOCKED;
		}
		else if ( (gRlvHandler.hasBehaviour(RLV_BHVR_UNSIT)) || (gRlvHandler.hasBehaviour(RLV_BHVR_SITTP)) )
		{
			if ( (isAgentAvatarValid()) && (gAgentAvatarp->isSitting()) && (gAgentAvatarp->getRoot() == pObjRoot) )
				return ACCEPT_NO_LOCKED;
		}
	}
	if(attached && !unrestricted)
	{
		return ACCEPT_NO_LOCKED;
	}
	else if(modify && transfer && volume && !worn)
	{
		return ACCEPT_YES_MULTI;
	}
	else if(!modify)
	{
		return ACCEPT_NO_LOCKED;
	}
	return ACCEPT_NO;
}
static void give_inventory_cb(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 1)
	{
		return;
	}
	LLSD payload = notification["payload"];
	const LLUUID& session_id = payload["session_id"];
	const LLUUID& agent_id = payload["agent_id"];
	LLViewerInventoryItem * inv_item =  gInventory.getItem(payload["item_id"]);
	LLViewerInventoryCategory * inv_cat =  gInventory.getCategory(payload["item_id"]);
	if (NULL == inv_item && NULL == inv_cat)
	{
		llassert( FALSE );
		return;
	}
	bool successfully_shared;
	if (inv_item)
	{
		successfully_shared = LLGiveInventory::doGiveInventoryItem(agent_id, inv_item, session_id);
	}
	else
	{
		successfully_shared = LLGiveInventory::doGiveInventoryCategory(agent_id, inv_cat, session_id);
	}
	if (successfully_shared)
	{
		if ("avatarpicker" == payload["d&d_dest"].asString())
		{
			if (LLFloaterAvatarPicker::instanceExists())
				LLFloaterAvatarPicker::getInstance()->close();
		}
		LLNotificationsUtil::add("ItemsShared");
	}
}
static void show_object_sharing_confirmation(const std::string name,
					   LLInventoryObject* inv_item,
					   const LLSD& dest,
					   const LLUUID& dest_agent,
					   const LLUUID& session_id = LLUUID::null)
{
	if (!inv_item)
	{
		llassert(NULL != inv_item);
		return;
	}
	LLSD substitutions;
	substitutions["RESIDENTS"] = name;
	substitutions["ITEMS"] = inv_item->getName();
	LLSD payload;
	payload["agent_id"] = dest_agent;
	payload["item_id"] = inv_item->getUUID();
	payload["session_id"] = session_id;
	payload["d&d_dest"] = dest.asString();
	LLNotificationsUtil::add("ShareItemsConfirmation", substitutions, payload, &give_inventory_cb);
}
static void get_name_cb(const LLUUID& id,
						const std::string& full_name,
						LLInventoryObject* inv_obj,
						const LLSD& dest,
						const LLUUID& dest_agent)
{
	show_object_sharing_confirmation(full_name,
								     inv_obj,
								     dest,
								     id,
								     LLUUID::null);
}
bool LLToolDragAndDrop::handleGiveDragAndDrop(LLUUID dest_agent, LLUUID session_id, BOOL drop,
											  EDragAndDropType cargo_type,
											  void* cargo_data,
											  EAcceptance* accept,
											  const LLSD& dest)
{
	switch(cargo_type)
	{
	case DAD_TEXTURE:
	case DAD_SOUND:
	case DAD_LANDMARK:
	case DAD_SCRIPT:
	case DAD_OBJECT:
	case DAD_NOTECARD:
	case DAD_CLOTHING:
	case DAD_BODYPART:
	case DAD_ANIMATION:
	case DAD_GESTURE:
	case DAD_CALLINGCARD:
	case DAD_MESH:
	{
		LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
		if(gInventory.getItem(inv_item->getUUID())
		   && LLGiveInventory::isInventoryGiveAcceptable(inv_item))
		{
			*accept = ACCEPT_YES_COPY_SINGLE;
			if(drop)
			{
				LLGiveInventory::doGiveInventoryItem(dest_agent, inv_item, session_id);
			}
		}
		else
		{
			*accept = ACCEPT_NO;
		}
		break;
	}
	case DAD_CATEGORY:
	{
		LLViewerInventoryCategory* inv_cat = (LLViewerInventoryCategory*)cargo_data;
		if( gInventory.getCategory( inv_cat->getUUID() ) )
		{
			*accept = ACCEPT_YES_COPY_SINGLE;
			if(drop)
			{
				LLGiveInventory::doGiveInventoryCategory(dest_agent, inv_cat, session_id);
			}
		}
		else
		{
			*accept = ACCEPT_NO;
		}
		break;
	}
	default:
		*accept = ACCEPT_NO;
		break;
	}
	return TRUE;
}
EAcceptance LLToolDragAndDrop::dad3dNULL(
	LLViewerObject*, S32, MASK, BOOL)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dNULL()" << LL_ENDL;
	return ACCEPT_NO;
}
EAcceptance LLToolDragAndDrop::dad3dRezAttachmentFromInv(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dRezAttachmentFromInv()" << LL_ENDL;
	if(mSource != SOURCE_AGENT && mSource != SOURCE_LIBRARY)
	{
		return ACCEPT_NO;
	}
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if( gInventory.isObjectDescendentOf( item->getUUID(), trash_id ) )
	{
		return ACCEPT_NO;
	}
	const LLUUID &outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);
	if(gInventory.isObjectDescendentOf(item->getUUID(), outbox_id))
	{
		return ACCEPT_NO;
	}
	bool fReplace = !(mask & MASK_CONTROL);
	if ( (rlv_handler_t::isEnabled()) && (!rlvPredCanWearItem(item, (fReplace) ? RLV_WEAR_REPLACE : RLV_WEAR_ADD)) )
	{
		return ACCEPT_NO_LOCKED;
	}
	if( drop )
	{
		if(mSource == SOURCE_LIBRARY)
		{
			LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(&rez_attachment_cb, _1, (LLViewerJointAttachment*)0, fReplace));
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		else
		{
			rez_attachment(item, 0, !(mask & MASK_CONTROL));
		}
	}
	return ACCEPT_YES_SINGLE;
}
EAcceptance LLToolDragAndDrop::dad3dRezObjectOnLand(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	if ( (rlv_handler_t::isEnabled()) && ((gRlvHandler.hasBehaviour(RLV_BHVR_REZ)) || (gRlvHandler.hasBehaviour(RLV_BHVR_INTERACT))) )
	{
		return ACCEPT_NO_LOCKED;
	}
	if (mSource == SOURCE_WORLD)
	{
		return dad3dRezFromObjectOnLand(obj, face, mask, drop);
	}
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dRezObjectOnLand()" << LL_ENDL;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	EAcceptance accept;
	BOOL remove_inventory;
	if (mask & MASK_SHIFT)
	{
		accept = ACCEPT_YES_COPY_SINGLE;
		remove_inventory = FALSE;
	}
	else
	{
		accept = ACCEPT_YES_COPY_SINGLE;
		remove_inventory = FALSE;
	}
	if(!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if(gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}
	if(drop)
	{
		dropObject(obj, TRUE, FALSE, remove_inventory);
	}
	return accept;
}
EAcceptance LLToolDragAndDrop::dad3dRezObjectOnObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	if ( (rlv_handler_t::isEnabled()) &&
		 ( ( (gRlvHandler.hasBehaviour(RLV_BHVR_REZ)) && ((mask & MASK_CONTROL) == 0) ) ||
		   ( (gRlvHandler.hasBehaviour(RLV_BHVR_INTERACT)) && (((mask & MASK_CONTROL) == 0) || (!obj->isHUDAttachment())) ) ) )
	{
		return ACCEPT_NO_LOCKED;
	}
	if (mSource == SOURCE_WORLD)
	{
		return dad3dRezFromObjectOnObject(obj, face, mask, drop);
	}
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dRezObjectOnObject()" << LL_ENDL;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if(!item || !item->isFinished()) return ACCEPT_NO;
	if((mask & MASK_CONTROL))
	{
		if(mSource == SOURCE_NOTECARD)
		{
			return ACCEPT_NO;
		}
		EAcceptance rv = willObjectAcceptInventory(obj, item);
		if(drop && (ACCEPT_YES_SINGLE <= rv))
		{
			dropInventory(obj, item, mSource, mSourceID);
		}
		return rv;
	}
	EAcceptance accept;
	BOOL remove_inventory;
	if (mask & MASK_SHIFT)
	{
		accept = ACCEPT_YES_COPY_SINGLE;
		remove_inventory = FALSE;
	}
	else
	{
		accept = ACCEPT_YES_COPY_SINGLE;
		remove_inventory = FALSE;
	}
	if(!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if(gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}
	if(drop)
	{
		dropObject(obj, FALSE, FALSE, remove_inventory);
	}
	return accept;
}
EAcceptance LLToolDragAndDrop::dad3dRezScript(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dRezScript()" << LL_ENDL;
	if((SOURCE_WORLD == mSource) || (SOURCE_NOTECARD == mSource))
	{
		return ACCEPT_NO;
	}
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	EAcceptance rv = willObjectAcceptInventory(obj, item);
	if(drop && (ACCEPT_YES_SINGLE <= rv))
	{
		BOOL active = ((mask & MASK_CONTROL) == 0);
		LLViewerObject* root_object = obj;
		if (obj && obj->getParent())
		{
			LLViewerObject* parent_obj = (LLViewerObject*)obj->getParent();
			if (!parent_obj->isAvatar())
			{
				root_object = parent_obj;
			}
		}
		dropScript(root_object, item, active, mSource, mSourceID);
	}
	return rv;
}
EAcceptance LLToolDragAndDrop::dad3dApplyToObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop, EDragAndDropType cargo_type)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dApplyToObject()" << LL_ENDL;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if(!item || !item->isFinished()) return ACCEPT_NO;
	EAcceptance rv = willObjectAcceptInventory(obj, item);
	if((mask & MASK_CONTROL))
	{
		if((ACCEPT_YES_SINGLE <= rv) && drop)
		{
			dropInventory(obj, item, mSource, mSourceID);
		}
		return rv;
	}
	if(!obj->permModify())
	{
		return ACCEPT_NO_LOCKED;
	}
	if(!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		return ACCEPT_NO;
	}
	if(drop && (ACCEPT_YES_SINGLE <= rv))
	{
		if (cargo_type == DAD_TEXTURE)
		{
			if((mask & MASK_SHIFT))
			{
				dropTextureAllFaces(obj, item, mSource, mSourceID);
			}
			else
			{
				dropTextureOneFace(obj, face, item, mSource, mSourceID);
			}
		}
		else if (cargo_type == DAD_MESH)
		{
			dropMesh(obj, item, mSource, mSourceID);
		}
		else
		{
			LL_WARNS() << "unsupported asset type" << LL_ENDL;
		}
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
		effectp->setSourceObject(gAgentAvatarp);
		effectp->setTargetObject(obj);
		effectp->setDuration(LL_HUD_DUR_SHORT);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	}
	return ACCEPT_YES_MULTI;
}
EAcceptance LLToolDragAndDrop::dad3dTextureObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	return dad3dApplyToObject(obj, face, mask, drop, DAD_TEXTURE);
}
EAcceptance LLToolDragAndDrop::dad3dMeshObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	return dad3dApplyToObject(obj, face, mask, drop, DAD_MESH);
}
EAcceptance LLToolDragAndDrop::dad3dWearItem(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dWearItem()" << LL_ENDL;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if(!item || !item->isFinished()) return ACCEPT_NO;
	if(mSource == SOURCE_AGENT || mSource == SOURCE_LIBRARY)
	{
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if( gInventory.isObjectDescendentOf( item->getUUID(), trash_id ) )
		{
			return ACCEPT_NO;
		}
		bool fReplace = (!(mask & MASK_CONTROL)) || (LLAssetType::AT_BODYPART == item->getType());
		if ( (rlv_handler_t::isEnabled()) && (!rlvPredCanWearItem(item, (fReplace) ? RLV_WEAR_REPLACE : RLV_WEAR_ADD)) )
		{
			return ACCEPT_NO_LOCKED;
		}
		if( drop )
		{
			if (!gAgentWearables.areWearablesLoaded())
			{
				LLNotificationsUtil::add("CanNotChangeAppearanceUntilLoaded");
				return ACCEPT_NO;
			}
			LLAppearanceMgr::instance().wearItemOnAvatar(item->getUUID(), true, fReplace);
		}
		return ACCEPT_YES_MULTI;
	}
	else
	{
		return ACCEPT_NO;
	}
}
EAcceptance LLToolDragAndDrop::dad3dActivateGesture(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dActivateGesture()" << LL_ENDL;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	if(mSource == SOURCE_AGENT || mSource == SOURCE_LIBRARY)
	{
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if( gInventory.isObjectDescendentOf( item->getUUID(), trash_id ) )
		{
			return ACCEPT_NO;
		}
		if( drop )
		{
			LLUUID item_id;
			if(mSource == SOURCE_LIBRARY)
			{
				LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(activate_gesture_cb);
				copy_inventory_item(
					gAgent.getID(),
					item->getPermissions().getOwner(),
					item->getUUID(),
					LLUUID::null,
					std::string(),
					cb);
			}
			else
			{
				LLGestureMgr::instance().activateGesture(item->getUUID());
				gInventory.updateItem(item);
				gInventory.notifyObservers();
			}
		}
		return ACCEPT_YES_MULTI;
	}
	else
	{
		return ACCEPT_NO;
	}
}
EAcceptance LLToolDragAndDrop::dad3dWearCategory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dWearCategory()" << LL_ENDL;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* category;
	locateInventory(item, category);
	if(!category) return ACCEPT_NO;
	if (drop)
	{
		if (!gAgentWearables.areWearablesLoaded())
		{
			LLNotificationsUtil::add("CanNotChangeAppearanceUntilLoaded");
			return ACCEPT_NO;
		}
	}
	if(mSource == SOURCE_AGENT)
	{
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if( gInventory.isObjectDescendentOf( category->getUUID(), trash_id ) )
		{
			return ACCEPT_NO;
		}
		const LLUUID &outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);
		if(gInventory.isObjectDescendentOf(category->getUUID(), outbox_id))
		{
			return ACCEPT_NO;
		}
		if(drop)
		{
			BOOL append = ( (mask & MASK_SHIFT) ? TRUE : FALSE );
			LLAppearanceMgr::instance().wearInventoryCategory(category, false, append);
		}
		return ACCEPT_YES_MULTI;
	}
	else if(mSource == SOURCE_LIBRARY)
	{
		if(drop)
		{
			LLAppearanceMgr::instance().wearInventoryCategory(category, true, false);
		}
		return ACCEPT_YES_MULTI;
	}
	else
	{
		return ACCEPT_NO;
	}
}
EAcceptance LLToolDragAndDrop::dad3dUpdateInventory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dadUpdateInventory()" << LL_ENDL;
	if((SOURCE_WORLD == mSource) || (SOURCE_NOTECARD == mSource))
	{
		return ACCEPT_NO;
	}
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	LLViewerObject* root_object = obj;
	if (obj && obj->getParent())
	{
		LLViewerObject* parent_obj = (LLViewerObject*)obj->getParent();
		if (!parent_obj->isAvatar())
		{
			root_object = parent_obj;
		}
	}
	EAcceptance rv = willObjectAcceptInventory(root_object, item);
	if(root_object && drop && (ACCEPT_YES_COPY_SINGLE <= rv))
	{
		dropInventory(root_object, item, mSource, mSourceID);
	}
	return rv;
}
BOOL LLToolDragAndDrop::dadUpdateInventory(LLViewerObject* obj, BOOL drop)
{
	EAcceptance rv = dad3dUpdateInventory(obj, -1, MASK_NONE, drop);
	return (rv >= ACCEPT_YES_COPY_SINGLE);
}
EAcceptance LLToolDragAndDrop::dad3dUpdateInventoryCategory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dUpdateInventoryCategory()" << LL_ENDL;
	if (obj == NULL)
	{
		LL_WARNS() << "obj is NULL; aborting func with ACCEPT_NO" << LL_ENDL;
		return ACCEPT_NO;
	}
	if ((mSource != SOURCE_AGENT) && (mSource != SOURCE_LIBRARY))
	{
		return ACCEPT_NO;
	}
	if (obj->isAttachment())
	{
		return ACCEPT_NO_LOCKED;
	}
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	locateInventory(item, cat);
	if (!cat)
	{
		return ACCEPT_NO;
	}
	LLDroppableItem droppable(!obj->permYouOwner());
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendentsIf(cat->getUUID(),
					cats,
					items,
					LLInventoryModel::EXCLUDE_TRASH,
					droppable);
	cats.push_back(cat);
 	if(droppable.countNoCopy() > 0)
 	{
 		LL_WARNS() << "*** Need to confirm this step" << LL_ENDL;
 	}
	LLViewerObject* root_object = obj;
	if (obj->getParent())
	{
		LLViewerObject* parent_obj = (LLViewerObject*)obj->getParent();
		if (!parent_obj->isAvatar())
		{
			root_object = parent_obj;
		}
	}
	EAcceptance rv = ACCEPT_NO;
	for (LLInventoryModel::cat_array_t::const_iterator cat_iter = cats.begin();
		 cat_iter != cats.end();
		 ++cat_iter)
	{
		const LLViewerInventoryCategory *cat = (*cat_iter);
		rv = gInventory.isCategoryComplete(cat->getUUID()) ? ACCEPT_YES_MULTI : ACCEPT_NO;
		if(rv < ACCEPT_YES_SINGLE)
		{
			LL_DEBUGS() << "Category " << cat->getUUID() << "is not complete." << LL_ENDL;
			break;
		}
	}
	if (ACCEPT_YES_COPY_SINGLE <= rv)
	{
		for (LLInventoryModel::item_array_t::const_iterator item_iter = items.begin();
			 item_iter != items.end();
			 ++item_iter)
		{
			LLViewerInventoryItem *item = (*item_iter);
			rv = willObjectAcceptInventory(root_object, item);
			if (rv < ACCEPT_YES_COPY_SINGLE)
			{
				LL_DEBUGS() << "Object will not accept " << item->getUUID() << LL_ENDL;
				break;
			}
		}
	}
	if(drop && (ACCEPT_YES_COPY_SINGLE <= rv))
	{
		uuid_vec_t ids;
		for (LLInventoryModel::item_array_t::const_iterator item_iter = items.begin();
			 item_iter != items.end();
			 ++item_iter)
		{
			const LLViewerInventoryItem *item = (*item_iter);
			ids.push_back(item->getUUID());
		}
		LLCategoryDropObserver* dropper = new LLCategoryDropObserver(ids, obj->getID(), mSource);
		dropper->startFetch();
		if (dropper->isFinished())
		{
			dropper->done();
		}
		else
		{
			gInventory.addObserver(dropper);
		}
	}
	return rv;
}
BOOL LLToolDragAndDrop::dadUpdateInventoryCategory(LLViewerObject* obj,
												   BOOL drop)
{
	EAcceptance rv = dad3dUpdateInventoryCategory(obj, -1, MASK_NONE, drop);
	return (rv >= ACCEPT_YES_COPY_SINGLE);
}
EAcceptance LLToolDragAndDrop::dad3dGiveInventoryObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dGiveInventoryObject()" << LL_ENDL;
	if(mSource != SOURCE_AGENT) return ACCEPT_NO;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	if(!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		return ACCEPT_NO;
	}
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	if( obj && avatar )
	{
		if(drop)
		{
			LLGiveInventory::doGiveInventoryItem(obj->getID(), item );
		}
		return ACCEPT_YES_SINGLE;
	}
	return ACCEPT_NO;
}
EAcceptance LLToolDragAndDrop::dad3dGiveInventory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dGiveInventory()" << LL_ENDL;
	if(mSource != SOURCE_AGENT) return ACCEPT_NO;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	if(!LLGiveInventory::isInventoryGiveAcceptable(item))
	{
		return ACCEPT_NO;
	}
	if(drop && obj)
	{
		LLGiveInventory::doGiveInventoryItem(obj->getID(), item);
	}
	return ACCEPT_YES_SINGLE;
}
EAcceptance LLToolDragAndDrop::dad3dGiveInventoryCategory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dGiveInventoryCategory()" << LL_ENDL;
	if(drop && obj)
	{
		LLViewerInventoryItem* item;
		LLViewerInventoryCategory* cat;
		locateInventory(item, cat);
		if(!cat) return ACCEPT_NO;
		LLGiveInventory::doGiveInventoryCategory(obj->getID(), cat);
	}
	return ACCEPT_YES_SINGLE;
}
EAcceptance LLToolDragAndDrop::dad3dRezFromObjectOnLand(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dRezFromObjectOnLand()" << LL_ENDL;
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	if(!gAgent.allowOperation(PERM_COPY, item->getPermissions())
		|| !item->getPermissions().allowTransferTo(LLUUID::null))
	{
		return ACCEPT_NO_LOCKED;
	}
	if(drop)
	{
		dropObject(obj, TRUE, TRUE, FALSE);
	}
	return ACCEPT_YES_SINGLE;
}
EAcceptance LLToolDragAndDrop::dad3dRezFromObjectOnObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	LL_DEBUGS() << "LLToolDragAndDrop::dad3dRezFromObjectOnObject()" << LL_ENDL;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	if((mask & MASK_CONTROL))
	{
		return ACCEPT_NO;
	}
	if(!item->getPermissions().allowCopyBy(gAgent.getID(),
										   gAgent.getGroupID())
	   || !item->getPermissions().allowTransferTo(LLUUID::null))
	{
		return ACCEPT_NO_LOCKED;
	}
	if(drop)
	{
		dropObject(obj, FALSE, TRUE, FALSE);
	}
	return ACCEPT_YES_SINGLE;
}
EAcceptance LLToolDragAndDrop::dad3dCategoryOnLand(
	LLViewerObject *obj, S32 face, MASK mask, BOOL drop)
{
	return ACCEPT_NO;
}
EAcceptance LLToolDragAndDrop::dad3dAssetOnLand(
	LLViewerObject *obj, S32 face, MASK mask, BOOL drop)
{
	return ACCEPT_NO;
}
LLInventoryObject* LLToolDragAndDrop::locateInventory(
	LLViewerInventoryItem*& item,
	LLViewerInventoryCategory*& cat)
{
	item = NULL;
	cat = NULL;
	if (mCargoIDs.empty()
		|| (mSource == SOURCE_PEOPLE))
	{
		return NULL;
	}
	if((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY))
	{
		item = (LLViewerInventoryItem*)gInventory.getItem(mCargoIDs[mCurItemIndex]);
		cat = (LLViewerInventoryCategory*)gInventory.getCategory(mCargoIDs[mCurItemIndex]);
	}
	else if(mSource == SOURCE_WORLD)
	{
		LLViewerObject* obj = gObjectList.findObject(mSourceID);
		if(obj)
		{
			if((mCargoTypes[mCurItemIndex] == DAD_CATEGORY)
			   || (mCargoTypes[mCurItemIndex] == DAD_ROOT_CATEGORY))
			{
				cat = (LLViewerInventoryCategory*)obj->getInventoryObject(mCargoIDs[mCurItemIndex]);
			}
			else
			{
			   item = (LLViewerInventoryItem*)obj->getInventoryObject(mCargoIDs[mCurItemIndex]);
			}
		}
	}
	else if(mSource == SOURCE_NOTECARD)
	{
		LLPreviewNotecard* preview = static_cast<LLPreviewNotecard*>(LLPreview::find(mSourceID));
		if(preview)
		{
			item = (LLViewerInventoryItem*)preview->getDragItem();
		}
	}
	if(item) return item;
	if(cat) return cat;
	return NULL;
}
void pack_permissions_slam(LLMessageSystem* msg, U32 flags, const LLPermissions& perms)
{
	U32 group_mask		= perms.getMaskGroup();
	U32 everyone_mask	= perms.getMaskEveryone();
	U32 next_owner_mask	= perms.getMaskNextOwner();
	msg->addU32Fast(_PREHASH_ItemFlags, flags);
	msg->addU32Fast(_PREHASH_GroupMask, group_mask);
	msg->addU32Fast(_PREHASH_EveryoneMask, everyone_mask);
	msg->addU32Fast(_PREHASH_NextOwnerMask, next_owner_mask);
}
