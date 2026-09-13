/** 
 * @file llfolderview.cpp
 * @brief Implementation of the folder view collection of classes.
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
#include "llfolderview.h"
#include "llcallbacklist.h"
#include "llinventorybridge.h"
#include "llinventoryclipboard.h"
#include "llinventoryfilter.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "llfoldertype.h"
#include "llfloaterinventory.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llresmgr.h"
#include "llpreview.h"
#include "llscrollcontainer.h"
#include "lltooldraganddrop.h"
#include "lltrans.h"
#include "llui.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewerjointattachment.h"
#include "llviewermenu.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewerfoldertype.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llfloaterproperties.h"
#include "llnotificationsutil.h"
#include "lldbstrings.h"
#include "llfavoritesbar.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llrender.h"
#include "llinventory.h"
#include <algorithm>
const S32 RENAME_WIDTH_PAD = 4;
const S32 RENAME_HEIGHT_PAD = 1;
const S32 AUTO_OPEN_STACK_DEPTH = 16;
const S32 MIN_ITEM_WIDTH_VISIBLE = LLFolderViewItem::ICON_WIDTH
			+ LLFolderViewItem::ICON_PAD
			+ LLFolderViewItem::ARROW_SIZE
			+ LLFolderViewItem::TEXT_PAD
			+ 40;
const S32 MINIMUM_RENAMER_WIDTH = 80;
const S32 STATUS_TEXT_HPAD = 6;
const S32 STATUS_TEXT_VPAD = 8;
enum {
	SIGNAL_NO_KEYBOARD_FOCUS = 1,
	SIGNAL_KEYBOARD_FOCUS = 2
};
F32 LLFolderView::sAutoOpenTime = 1.f;
void delete_selected_item(void* user_data);
void copy_selected_item(void* user_data);
void open_selected_items(void* user_data);
void properties_selected_items(void* user_data);
void paste_items(void* user_data);
class LLSetItemSortFunction : public LLFolderViewFunctor
{
public:
	LLSetItemSortFunction(U32 ordering)
		: mSortOrder(ordering) {}
	virtual ~LLSetItemSortFunction() {}
	virtual void doFolder(LLFolderViewFolder* folder);
	virtual void doItem(LLFolderViewItem* item);
	U32 mSortOrder;
};
void LLSetItemSortFunction::doFolder(LLFolderViewFolder* folder)
{
	folder->setItemSortOrder(mSortOrder);
}
void LLSetItemSortFunction::doItem(LLFolderViewItem* item)
{
	return;
}
class LLCloseAllFoldersFunctor : public LLFolderViewFunctor
{
public:
	LLCloseAllFoldersFunctor(BOOL close) { mOpen = !close; }
	virtual ~LLCloseAllFoldersFunctor() {}
	virtual void doFolder(LLFolderViewFolder* folder);
	virtual void doItem(LLFolderViewItem* item);
	BOOL mOpen;
};
void LLCloseAllFoldersFunctor::doFolder(LLFolderViewFolder* folder)
{
	folder->setOpenArrangeRecursively(mOpen);
}
void LLCloseAllFoldersFunctor::doItem(LLFolderViewItem* item)
{ }
LLFolderView::LLFolderView( const std::string& name,
						   const LLRect& rect, const LLUUID& source_id, LLPanel* parent_panel, LLFolderViewEventListener* listener, LLFolderViewGroupedItemModel* group_model ) :
#if LL_WINDOWS
#pragma warning( push )
#pragma warning( disable : 4355 )
#endif
	LLFolderViewFolder( name, NULL, NULL, NULL, this, listener ),
#if LL_WINDOWS
#pragma warning( pop )
#endif
	mRunningHeight(0),
	mScrollContainer( NULL ),
	mPopupMenuHandle(),
	mAllowMultiSelect(TRUE),
	mShowEmptyMessage(TRUE),
	mShowFolderHierarchy(FALSE),
	mSourceID(source_id),
	mRenameItem( NULL ),
	mNeedsScroll( FALSE ),
	mUseLabelSuffix( TRUE ),
	mPinningSelectedItem(FALSE),
	mNeedsAutoSelect( FALSE ),
	mAutoSelectOverride(FALSE),
	mNeedsAutoRename(FALSE),
	mDebugFilters(FALSE),
	mSortOrder(LLInventoryFilter::SO_FOLDERS_BY_NAME),
	mFilter(LLInventoryFilter::Params().name(name)),
	mShowSelectionContext(FALSE),
	mShowSingleSelection(FALSE),
	mArrangeGeneration(0),
	mSignalSelectCallback(0),
	mMinWidth(0),
	mDragAndDropThisFrame(FALSE),
	mUseEllipses(FALSE),
	mDraggingOverItem(NULL),
	mStatusTextBox(NULL),
	mSearchType(1),
	mGroupedItemModel(group_model)
{
	LLPanel* panel = parent_panel;
	mParentPanel = panel->getHandle();
	mRoot = this;
	mShowLoadStatus = TRUE;
	LLRect new_rect(rect.mLeft, rect.mBottom + getRect().getHeight(), rect.mLeft + getRect().getWidth(), rect.mBottom);
	setRect( rect );
	reshape(rect.getWidth(), rect.getHeight());
	mIsOpen = TRUE;
	mAutoOpenItems.setDepth(AUTO_OPEN_STACK_DEPTH);
	mAutoOpenCandidate = NULL;
	mAutoOpenTimer.stop();
	mKeyboardSelection = FALSE;
	mIndentation = -LEFT_INDENTATION;
	gIdleCallbacks.addFunction(idle, this);
	mLabel = LLStringUtil::null;
	mRenamer = new LLLineEditor(std::string("ren"), getRect(), LLStringUtil::null, getLabelFontForStyle(LLFontGL::NORMAL),
								DB_INV_ITEM_NAME_STR_LEN,
								boost::bind(&LLFolderView::commitRename,this),
								NULL,
								NULL,
								&LLLineEditor::prevalidatePrintableNotPipe);
	mRenamer->setCommitOnFocusLost(TRUE);
	mRenamer->setVisible(FALSE);
	addChild(mRenamer);
	LLFontGL* font = getLabelFontForStyle(mLabelStyle);
	LLRect new_r = LLRect(rect.mLeft + ICON_PAD,
			      rect.mTop - TEXT_PAD,
			      rect.mRight,
			      rect.mTop - TEXT_PAD - llfloor(font->getLineHeight()));
	mStatusTextBox = new LLTextBox(name, new_r, std::string(), font, false);
	mStatusTextBox->setVisible(true);
	mStatusTextBox->setHPad(STATUS_TEXT_HPAD);
	mStatusTextBox->setVPad(STATUS_TEXT_VPAD);
	mStatusTextBox->setFollows(FOLLOWS_LEFT|FOLLOWS_TOP);
	LLMenuGL* menu = LLUICtrlFactory::getInstance()->buildMenu("menu_inventory.xml", parent_panel);
	if (!menu)
	{
		menu = new LLMenuGL(LLStringUtil::null);
	}
	menu->setBackgroundColor(gColors.getColor("MenuPopupBgColor"));
	menu->setVisible(FALSE);
	mPopupMenuHandle = menu->getHandle();
	setTabStop(TRUE);
	mListener->openItem();
}
LLFolderView::~LLFolderView( void )
{
	closeRenamer();
	mScrollContainer = NULL;
	mRenameItem = NULL;
	mRenamer = NULL;
	mStatusTextBox = NULL;
	mAutoOpenItems.removeAllNodes();
	gIdleCallbacks.deleteFunction(idle, this);
	if (mPopupMenuHandle.get()) mPopupMenuHandle.get()->die();
	mAutoOpenItems.removeAllNodes();
	clearSelection();
	mItems.clear();
	mFolders.clear();
	mItemMap.clear();
}
BOOL LLFolderView::canFocusChildren() const
{
	return FALSE;
}
static LLTrace::BlockTimerStatHandle FTM_SORT("Sort Inventory");
void LLFolderView::setSortOrder(U32 order)
{
	if (order != mSortOrder)
	{
		LL_RECORD_BLOCK_TIME(FTM_SORT);
		mSortOrder = order;
		sortBy(order);
		arrangeAll();
	}
}
U32 LLFolderView::getSortOrder() const
{
	return mSortOrder;
}
U32 LLFolderView::toggleSearchType(std::string toggle)
{
	if (toggle == "name")
	{
		if (mSearchType & 1)
		{
			mSearchType &= 6;
		}
		else
		{
			mSearchType |= 1;
		}
	}
	else if (toggle == "description")
	{
		if (mSearchType & 2)
		{
			mSearchType &= 5;
		}
		else
		{
			mSearchType |= 2;
		}
	}
	else if (toggle == "creator")
	{
		if (mSearchType & 4)
		{
			mSearchType &= 3;
		}
		else
		{
			mSearchType |= 4;
		}
	}
	if (mSearchType == 0)
	{
		mSearchType = 1;
	}
	if (getFilterSubString().length())
	{
		mFilter.setModified(LLInventoryFilter::FILTER_RESTART);
	}
	return mSearchType;
}
U32 LLFolderView::getSearchType() const
{
	return mSearchType;
}
BOOL LLFolderView::addFolder( LLFolderViewFolder* folder)
{
	if (folder->getListener()->getUUID() == gInventory.getLibraryRootFolderID())
	{
		mFolders.push_back(folder);
	}
	else
	{
		mFolders.insert(mFolders.begin(), folder);
	}
	folder->setShowLoadStatus(mShowLoadStatus);
	folder->setOrigin(0, 0);
	folder->reshape(getRect().getWidth(), 0);
	folder->setVisible(FALSE);
	addChild( folder );
	folder->dirtyFilter();
	folder->requestArrange();
	return TRUE;
}
void LLFolderView::closeAllFolders()
{
	setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_DOWN);
	arrangeAll();
}
void LLFolderView::openTopLevelFolders()
{
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		(*fit)->setOpen(TRUE);
	}
}
void LLFolderView::setOpenArrangeRecursively(BOOL openitem, ERecurseType recurse)
{
	LLFolderViewFolder::setOpenArrangeRecursively(openitem, recurse);
	mIsOpen = TRUE;
}
static LLTrace::BlockTimerStatHandle FTM_ARRANGE("Arrange");
S32 LLFolderView::arrange( S32* unused_width, S32* unused_height, S32 filter_generation )
{
	if(!mScrollContainer)
		return 1;
	if (getListener()->getUUID().notNull())
	{
		if (mNeedsSort)
		{
			mFolders.sort(mSortFunction);
			mItems.sort(mSortFunction);
			mNeedsSort = false;
		}
	}
	LL_RECORD_BLOCK_TIME(FTM_ARRANGE);
	filter_generation = mFilter.getFirstSuccessGeneration();
	mMinWidth = 0;
	mHasVisibleChildren = hasFilteredDescendants(filter_generation);
	mLastArrangeGeneration = getRoot()->getArrangeGeneration();
	LLInventoryFilter::EFolderShow show_folder_state =
		getRoot()->getFilter().getShowFolderState();
	S32 total_width = LEFT_PAD;
	S32 running_height = mDebugFilters ? llceil(LLFontGL::getFontMonospace()->getLineHeight()) : 0;
	S32 target_height = running_height;
	S32 parent_item_height = getRect().getHeight();
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		LLFolderViewFolder* folderp = (*fit);
		if (getDebugFilters())
		{
			folderp->setVisible(TRUE);
		}
		else
		{
			folderp->setVisible(show_folder_state == LLInventoryFilter::SHOW_ALL_FOLDERS ||
									(folderp->getFiltered(filter_generation) || folderp->hasFilteredDescendants(filter_generation)));
		}
		if (folderp->getVisible())
		{
			S32 child_height = 0;
			S32 child_width = 0;
			S32 child_top = parent_item_height - running_height;
			target_height += folderp->arrange( &child_width, &child_height, filter_generation );
			mMinWidth = llmax(mMinWidth, child_width);
			total_width = llmax( total_width, child_width );
			running_height += child_height;
			folderp->setOrigin( ICON_PAD, child_top - (*fit)->getRect().getHeight() );
		}
	}
	for (items_t::iterator iter = mItems.begin();
		 iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		LLFolderViewItem* itemp = (*iit);
		itemp->setVisible(itemp->getFiltered(filter_generation));
		if (itemp->getVisible())
		{
			S32 child_width = 0;
			S32 child_height = 0;
			S32 child_top = parent_item_height - running_height;
			target_height += itemp->arrange( &child_width, &child_height, filter_generation );
			itemp->reshape(itemp->getRect().getWidth(), child_height);
			mMinWidth = llmax(mMinWidth, child_width);
			total_width = llmax( total_width, child_width );
			running_height += child_height;
			itemp->setOrigin( ICON_PAD, child_top - itemp->getRect().getHeight() );
		}
	}
	if(!mHasVisibleChildren)
	{
		running_height = mStatusTextBox->getTextPixelHeight();
		target_height = running_height;
	}
	mRunningHeight = running_height;
	LLRect scroll_rect = mScrollContainer->getContentWindowRect();
	reshape( llmax(scroll_rect.getWidth(), total_width), running_height );
	LLRect new_scroll_rect = mScrollContainer->getContentWindowRect();
	if (new_scroll_rect.getWidth() != scroll_rect.getWidth())
	{
		reshape( llmax(scroll_rect.getWidth(), total_width), running_height );
	}
	updateRenamerPosition();
	mTargetHeight = (F32)target_height;
	return ll_round(mTargetHeight);
}
const std::string LLFolderView::getFilterSubString(BOOL trim)
{
	return mFilter.getFilterSubString(trim);
}
static LLTrace::BlockTimerStatHandle FTM_FILTER("Filter Inventory");
void LLFolderView::filter( LLInventoryFilter& filter )
{
	LL_RECORD_BLOCK_TIME(FTM_FILTER);
	filter.setFilterCount(llclamp(gSavedSettings.getS32("FilterItemsPerFrame"), 1, 5000));
	if (getCompletedFilterGeneration() < filter.getCurrentGeneration())
	{
		mPassedFilter = FALSE;
		mMinWidth = 0;
		LLFolderViewFolder::filter(filter);
	}
	else
	{
		mPassedFilter = TRUE;
	}
}
void LLFolderView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLRect scroll_rect;
	if (mScrollContainer)
	{
		LLView::reshape(width, height, called_from_parent);
		scroll_rect = mScrollContainer->getContentWindowRect();
	}
	width = llmax(mMinWidth, scroll_rect.getWidth());
	height = llmax(mRunningHeight, scroll_rect.getHeight());
	if (mUseEllipses)
		width = scroll_rect.getWidth();
	LLView::reshape(width, height, called_from_parent);
	mReshapeSignal(mSelectedItems, FALSE);
}
void LLFolderView::addToSelectionList(LLFolderViewItem* item)
{
	if (item->isSelected())
	{
		removeFromSelectionList(item);
	}
	if (mSelectedItems.size())
	{
		mSelectedItems.back()->setIsCurSelection(FALSE);
	}
	item->setIsCurSelection(TRUE);
	mSelectedItems.push_back(item);
}
void LLFolderView::removeFromSelectionList(LLFolderViewItem* item)
{
	if (mSelectedItems.size())
	{
		mSelectedItems.back()->setIsCurSelection(FALSE);
	}
	selected_items_t::iterator item_iter;
	for (item_iter = mSelectedItems.begin(); item_iter != mSelectedItems.end();)
	{
		if (*item_iter == item)
		{
			item_iter = mSelectedItems.erase(item_iter);
		}
		else
		{
			++item_iter;
		}
	}
	if (mSelectedItems.size())
	{
		mSelectedItems.back()->setIsCurSelection(TRUE);
	}
}
LLFolderViewItem* LLFolderView::getCurSelectedItem( void )
{
	if(mSelectedItems.size())
	{
		LLFolderViewItem* itemp = mSelectedItems.back();
		llassert(itemp->getIsCurSelection());
		return itemp;
	}
	return NULL;
}
LLFolderView::selected_items_t& LLFolderView::getSelectedItems( void )
{
	return mSelectedItems;
}
BOOL LLFolderView::setSelection(LLFolderViewItem* selection, BOOL openitem,
								BOOL take_keyboard_focus)
{
	mSignalSelectCallback = take_keyboard_focus ? SIGNAL_KEYBOARD_FOCUS : SIGNAL_NO_KEYBOARD_FOCUS;
	if( selection == this )
	{
		return FALSE;
	}
	if( selection && take_keyboard_focus)
	{
		mParentPanel.get()->setFocus(TRUE);
	}
	clearSelection();
	if(selection)
	{
		addToSelectionList(selection);
	}
	BOOL rv = LLFolderViewFolder::setSelection(selection, openitem, take_keyboard_focus);
	if(openitem && selection)
	{
		selection->getParentFolder()->requestArrange();
	}
	llassert(mSelectedItems.size() <= 1);
	return rv;
}
void LLFolderView::setSelectionByID(const LLUUID& obj_id, BOOL take_keyboard_focus)
{
	LLFolderViewItem* itemp = getItemByID(obj_id);
	if(itemp && itemp->getListener())
	{
		itemp->arrangeAndSet(TRUE, take_keyboard_focus);
		mSelectThisID.setNull();
		return;
	}
	else
	{
		mSelectThisID = obj_id;
	}
}
void LLFolderView::updateSelection()
{
	if (mSelectThisID.notNull())
	{
		setSelectionByID(mSelectThisID, false);
	}
}
BOOL LLFolderView::changeSelection(LLFolderViewItem* selection, BOOL selected)
{
	BOOL rv = FALSE;
	if(!selection || selection == this)
	{
		return FALSE;
	}
	if (!mAllowMultiSelect)
	{
		clearSelection();
	}
	selected_items_t::iterator item_iter;
	for (item_iter = mSelectedItems.begin(); item_iter != mSelectedItems.end(); ++item_iter)
	{
		if (*item_iter == selection)
		{
			break;
		}
	}
	BOOL on_list = (item_iter != mSelectedItems.end());
	if(selected && !on_list)
	{
		addToSelectionList(selection);
	}
	if(!selected && on_list)
	{
		removeFromSelectionList(selection);
	}
	rv = LLFolderViewFolder::changeSelection(selection, selected);
	mSignalSelectCallback = SIGNAL_KEYBOARD_FOCUS;
	return rv;
}
static LLTrace::BlockTimerStatHandle FTM_SANITIZE_SELECTION("Sanitize Selection");
void LLFolderView::sanitizeSelection()
{
	LL_RECORD_BLOCK_TIME(FTM_SANITIZE_SELECTION);
	LLFolderViewItem* original_selected_item = getCurSelectedItem();
	BOOL show_all_folders = (getRoot()->getFilter().getShowFolderState() == LLInventoryFilter::SHOW_ALL_FOLDERS);
	std::vector<LLFolderViewItem*> items_to_remove;
	selected_items_t::iterator item_iter;
	for (item_iter = mSelectedItems.begin(); item_iter != mSelectedItems.end(); ++item_iter)
	{
		LLFolderViewItem* item = *item_iter;
		BOOL visible = item->potentiallyVisible();
		LLFolderViewFolder* parent_folder = item->getParentFolder();
		if ( parent_folder )
		{
			if ( show_all_folders )
			{
				visible = TRUE;
			}
			else
			{
				while(parent_folder)
				{
					visible = visible && parent_folder->isOpen() && parent_folder->potentiallyVisible();
					parent_folder = parent_folder->getParentFolder();
				}
			}
		}
		if (!visible)
		{
			items_to_remove.push_back(item);
		}
		selected_items_t::iterator other_item_iter;
		for (other_item_iter = mSelectedItems.begin(); other_item_iter != mSelectedItems.end(); ++other_item_iter)
		{
			LLFolderViewItem* other_item = *other_item_iter;
			for( parent_folder = other_item->getParentFolder(); parent_folder; parent_folder = parent_folder->getParentFolder())
			{
				if (parent_folder == item)
				{
					items_to_remove.push_back(other_item);
					break;
				}
			}
		}
		if (item == getRoot())
		{
			items_to_remove.push_back(item);
		}
	}
	std::vector<LLFolderViewItem*>::iterator item_it;
	for (item_it = items_to_remove.begin(); item_it != items_to_remove.end(); ++item_it )
	{
		changeSelection(*item_it, FALSE);
	}
	if (mSelectedItems.empty())
	{
		LLFolderViewItem* new_selection = NULL;
		if (original_selected_item)
		{
			for(LLFolderViewFolder* parent_folder = original_selected_item->getParentFolder();
				parent_folder;
				parent_folder = parent_folder->getParentFolder())
			{
				if (parent_folder->potentiallyVisible())
				{
					if (!new_selection)
					{
						new_selection = parent_folder;
					}
					if (!parent_folder->isOpen())
					{
						new_selection = parent_folder;
					}
				}
			}
		}
		else
		{
			new_selection = NULL;
		}
		if (new_selection)
		{
			setSelection(new_selection, FALSE, FALSE);
		}
	}
}
void LLFolderView::clearSelection()
{
	for (selected_items_t::const_iterator item_it = mSelectedItems.begin();
		 item_it != mSelectedItems.end();
		 ++item_it)
	{
		(*item_it)->setUnselected();
	}
	mSelectedItems.clear();
	mSelectThisID.setNull();
}
uuid_set_t LLFolderView::getSelectionList() const
{
	uuid_set_t selection;
	for (const auto& item : mSelectedItems)
	{
		selection.insert(item->getListener()->getUUID());
	}
	return selection;
}
bool LLFolderView::startDrag(LLToolDragAndDrop::ESource source)
{
	std::vector<EDragAndDropType> types;
	uuid_vec_t cargo_ids;
	selected_items_t::iterator item_it;
	bool can_drag = true;
	if (!mSelectedItems.empty())
	{
		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
		{
			EDragAndDropType type = DAD_NONE;
			LLUUID id = LLUUID::null;
			can_drag = can_drag && (*item_it)->getListener()->startDrag(&type, &id);
			types.push_back(type);
			cargo_ids.push_back(id);
		}
		LLToolDragAndDrop::getInstance()->beginMultiDrag(types, cargo_ids, source, mSourceID);
	}
	return can_drag;
}
void LLFolderView::commitRename( )
{
	finishRenamingItem();
}
void LLFolderView::draw()
{
	if (mDebugFilters)
	{
		std::string current_filter_string = llformat("Current Filter: %d, Least Filter: %d, Auto-accept Filter: %d",
										mFilter.getCurrentGeneration(), mFilter.getFirstSuccessGeneration(), mFilter.getFirstRequiredGeneration());
		LLFontGL::getFontMonospace()->renderUTF8(current_filter_string, 0, 2,
			getRect().getHeight() - LLFontGL::getFontMonospace()->getLineHeight(), LLColor4(0.5f, 0.5f, 0.8f, 1.f),
			LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,  S32_MAX, S32_MAX, NULL, FALSE );
	}
	if (!mDragAndDropThisFrame)
	{
		closeAutoOpenedFolders();
	}
	LLToolDragAndDrop& dad_inst(LLToolDragAndDrop::instance());
	if (dad_inst.hasMouseCapture())
	{
		EAcceptance last_accept = dad_inst.getLastAccept();
		setShowSingleSelection(last_accept == ACCEPT_YES_SINGLE || last_accept == ACCEPT_YES_COPY_SINGLE);
	}
	else
	{
		setShowSingleSelection(FALSE);
	}
	static LLUICachedControl<F32> type_ahead_timeout("TypeAheadTimeout", 0);
	if (mSearchTimer.getElapsedTimeF32() > type_ahead_timeout || !mSearchString.size())
	{
		mSearchString.clear();
	}
	if (hasVisibleChildren()
		|| mFilter.getShowFolderState() == LLInventoryFilter::SHOW_ALL_FOLDERS)
	{
		mStatusText.clear();
		mStatusTextBox->setVisible( FALSE );
	}
	else if (mShowEmptyMessage)
	{
		static LLCachedControl<LLColor4> sSearchStatusColor(gColors, "InventorySearchStatusColor", LLColor4::white );
		if (LLInventoryModelBackgroundFetch::instance().folderFetchActive() || mCompletedFilterGeneration < mFilter.getFirstSuccessGeneration())
		{
			mStatusText = LLTrans::getString("Searching");
		}
		else
		{
			mStatusText = getFilter().getEmptyLookupMessage();
		}
		mStatusTextBox->setWrappedText(mStatusText);
		mStatusTextBox->setVisible( TRUE );
		const LLRect local_rect = getLocalRect();
		mStatusTextBox->setShape(local_rect);
		S32 pixel_height = mStatusTextBox->getTextPixelHeight();
		bool height_changed = (local_rect.getHeight() != pixel_height);
		if (height_changed)
		{
			arrangeFromRoot();
		}
	}
	LLView::draw();
	mDragAndDropThisFrame = FALSE;
}
void LLFolderView::finishRenamingItem( void )
{
	if(!mRenamer)
	{
		return;
	}
	if( mRenameItem )
	{
		mRenameItem->rename( mRenamer->getText() );
	}
	closeRenamer();
	scrollToShowSelection();
}
void LLFolderView::closeRenamer( void )
{
	if (mRenamer && mRenamer->getVisible())
	{
		gFocusMgr.setTopCtrl( NULL );
	}
}
bool isDescendantOfASelectedItem(LLFolderViewItem* item, const std::vector<LLFolderViewItem*>& selectedItems)
{
	LLFolderViewItem* item_parent = dynamic_cast<LLFolderViewItem*>(item->getParent());
	if (item_parent)
	{
		for(std::vector<LLFolderViewItem*>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it)
		{
			const LLFolderViewItem* const selected_item = (*it);
			LLFolderViewItem* parent = item_parent;
			while (parent)
			{
				if (selected_item == parent)
				{
					return true;
				}
				parent = dynamic_cast<LLFolderViewItem*>(parent->getParent());
			}
		}
	}
	return false;
}
void LLFolderView::removeCutItems()
{
	if (!LLInventoryClipboard::instance().isCutMode())
		return;
	uuid_vec_t objects;
	LLInventoryClipboard::instance().retrieve(objects);
	for (auto iter = objects.begin();
		 iter != objects.end();
		 ++iter)
	{
		gInventory.removeObject(*iter);
	}
}
void LLFolderView::removeSelectedItems()
{
	if(getVisible() && getEnabled())
	{
		mRenameItem = NULL;
		std::vector<LLFolderViewItem*> items;
		if(mSelectedItems.empty()) return;
		LLFolderViewItem* item = NULL;
		selected_items_t::iterator item_it;
		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
		{
			item = *item_it;
			if (item && item->isRemovable())
			{
				items.push_back(item);
			}
			else
			{
				LL_INFOS() << "Cannot delete " << item->getName() << LL_ENDL;
				return;
			}
		}
		size_t count = items.size();
		LLUUID new_selection_id;
		if(count == 1)
		{
			LLFolderViewItem* item_to_delete = items[0];
			LLFolderViewFolder* parent = item_to_delete->getParentFolder();
			LLFolderViewItem* new_selection = item_to_delete->getNextOpenNode(FALSE);
			if (!new_selection)
			{
				new_selection = item_to_delete->getPreviousOpenNode(FALSE);
			}
			if(parent)
			{
				if (parent->removeItem(item_to_delete))
				{
					if (new_selection)
					{
						setSelectionFromRoot(new_selection, new_selection->isOpen(), mParentPanel.get()->hasFocus());
					}
					else
					{
						setSelectionFromRoot(NULL, mParentPanel.get()->hasFocus());
					}
				}
			}
			arrangeAll();
		}
		else if (count > 1)
		{
			std::vector<LLFolderViewEventListener*> listeners;
			LLFolderViewEventListener* listener;
			LLFolderViewItem* last_item = items[count - 1];
			LLFolderViewItem* new_selection = last_item->getNextOpenNode(FALSE);
			while(new_selection && new_selection->isSelected())
			{
				new_selection = new_selection->getNextOpenNode(FALSE);
			}
			if (!new_selection)
			{
				new_selection = last_item->getPreviousOpenNode(FALSE);
				while (new_selection && (new_selection->isSelected() || isDescendantOfASelectedItem(new_selection, items)))
				{
					new_selection = new_selection->getPreviousOpenNode(FALSE);
				}
			}
			if (new_selection)
			{
				setSelectionFromRoot(new_selection, new_selection->isOpen(), mParentPanel.get()->hasFocus());
			}
			else
			{
				setSelectionFromRoot(NULL, mParentPanel.get()->hasFocus());
			}
			for(size_t i = 0; i < count; ++i)
			{
				listener = items[i]->getListener();
				if(listener && (std::find(listeners.begin(), listeners.end(), listener) == listeners.end()))
				{
					listeners.push_back(listener);
				}
			}
			listener = listeners.at(0);
			if(listener)
			{
				listener->removeBatch(listeners);
			}
		}
		arrangeAll();
		scrollToShowSelection();
	}
}
void LLFolderView::openSelectedItems( void )
{
	if(getVisible() && getEnabled())
	{
		if (mSelectedItems.size() == 1)
		{
			mSelectedItems.front()->openItem();
		}
		else
		{
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLMultiPreview* multi_previewp = new LLMultiPreview(LLRect(left, top, left + 300, top - 100));
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLMultiProperties* multi_propertiesp = new LLMultiProperties(LLRect(left, top, left + 300, top - 100));
			selected_items_t::iterator item_it;
			for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
			{
				LLFolderViewEventListener* listener = (*item_it)->getListener();
				bool is_prop = listener && (listener->getInventoryType() == LLInventoryType::IT_OBJECT || listener->getInventoryType() == LLInventoryType::IT_ATTACHMENT);
				if (is_prop)
					LLFloater::setFloaterHost(multi_propertiesp);
				else
					LLFloater::setFloaterHost(multi_previewp);
				(*item_it)->openItem();
			}
			LLFloater::setFloaterHost(NULL);
			multi_previewp->open();
			multi_propertiesp->open();
		}
	}
}
void LLFolderView::propertiesSelectedItems( void )
{
	if(getVisible() && getEnabled())
	{
		if (mSelectedItems.size() == 1)
		{
			LLFolderViewItem* folder_item = mSelectedItems.front();
			if(!folder_item) return;
			folder_item->getListener()->showProperties();
		}
		else
		{
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLMultiProperties* multi_propertiesp = new LLMultiProperties(LLRect(left, top, left + 100, top - 100));
			LLFloater::setFloaterHost(multi_propertiesp);
			selected_items_t::iterator item_it;
			for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
			{
				(*item_it)->getListener()->showProperties();
			}
			LLFloater::setFloaterHost(NULL);
			multi_propertiesp->open();
		}
	}
}
void LLFolderView::changeType(LLInventoryModel *model, LLFolderType::EType new_folder_type)
{
	LLFolderBridge *folder_bridge = LLFolderBridge::sSelf.get();
	if (!folder_bridge) return;
	LLViewerInventoryCategory *cat = folder_bridge->getCategory();
	if (!cat) return;
	cat->changeType(new_folder_type);
}
void LLFolderView::autoOpenItem( LLFolderViewFolder* item )
{
	if ((mAutoOpenItems.check() == item) ||
		(mAutoOpenItems.getDepth() >= (U32)AUTO_OPEN_STACK_DEPTH) ||
		item->isOpen())
	{
		return;
	}
	LLFolderViewFolder* close_item = mAutoOpenItems.check();
	while (close_item && close_item != item->getParentFolder())
	{
		mAutoOpenItems.pop();
		close_item->setOpenArrangeRecursively(FALSE);
		close_item = mAutoOpenItems.check();
	}
	item->requestArrange();
	mAutoOpenItems.push(item);
	item->setOpen(TRUE);
	LLRect content_rect = mScrollContainer->getContentWindowRect();
	LLRect constraint_rect(0,content_rect.getHeight(), content_rect.getWidth(), 0);
	scrollToShowItem(item, constraint_rect);
}
void LLFolderView::closeAutoOpenedFolders()
{
	while (mAutoOpenItems.check())
	{
		LLFolderViewFolder* close_item = mAutoOpenItems.pop();
		close_item->setOpen(FALSE);
	}
	if (mAutoOpenCandidate)
	{
		mAutoOpenCandidate->setAutoOpenCountdown(0.f);
	}
	mAutoOpenCandidate = NULL;
	mAutoOpenTimer.stop();
}
BOOL LLFolderView::autoOpenTest(LLFolderViewFolder* folder)
{
	if (folder && mAutoOpenCandidate == folder)
	{
		if (mAutoOpenTimer.getStarted())
		{
			if (!mAutoOpenCandidate->isOpen())
			{
				mAutoOpenCandidate->setAutoOpenCountdown(clamp_rescale(mAutoOpenTimer.getElapsedTimeF32(), 0.f, sAutoOpenTime, 0.f, 1.f));
			}
			if (mAutoOpenTimer.getElapsedTimeF32() > sAutoOpenTime)
			{
				autoOpenItem(folder);
				mAutoOpenTimer.stop();
				return TRUE;
			}
		}
		return FALSE;
	}
	if (mAutoOpenCandidate)
	{
		mAutoOpenCandidate->setAutoOpenCountdown(0.f);
	}
	mAutoOpenCandidate = folder;
	mAutoOpenTimer.start();
	return FALSE;
}
BOOL LLFolderView::canCopy() const
{
	if (!(getVisible() && getEnabled() && (mSelectedItems.size() > 0)))
	{
		return FALSE;
	}
	for (selected_items_t::const_iterator selected_it = mSelectedItems.begin(); selected_it != mSelectedItems.end(); ++selected_it)
	{
		const LLFolderViewItem* item = *selected_it;
		if (!item->getListener()->isItemCopyable())
		{
			return FALSE;
		}
	}
	return TRUE;
}
void LLFolderView::copy() const
{
	LLInventoryClipboard::instance().reset();
	S32 count = mSelectedItems.size();
	if(getVisible() && getEnabled() && (count > 0))
	{
		for (auto item : mSelectedItems)
		{
			if(auto listener = item->getListener())
			{
				listener->copyToClipboard();
			}
		}
	}
}
BOOL LLFolderView::canCut() const
{
	if (!(getVisible() && getEnabled() && (mSelectedItems.size() > 0)))
	{
		return FALSE;
	}
	for (selected_items_t::const_iterator selected_it = mSelectedItems.begin(); selected_it != mSelectedItems.end(); ++selected_it)
	{
		const LLFolderViewItem* item = *selected_it;
		const LLFolderViewEventListener* listener = item->getListener();
		if (!listener || !listener->isItemRemovable())
		{
			return FALSE;
		}
	}
	return TRUE;
}
void LLFolderView::cut()
{
	LLInventoryClipboard::instance().reset();
	S32 count = mSelectedItems.size();
	if(getVisible() && getEnabled() && (count > 0))
	{
		LLFolderViewEventListener* listener = NULL;
		selected_items_t::iterator item_it;
		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
		{
			listener = (*item_it)->getListener();
			if(listener)
			{
				listener->cutToClipboard();
			}
		}
		LLFolderView::removeCutItems();
	}
	mSearchString.clear();
}
BOOL LLFolderView::canPaste() const
{
	if (mSelectedItems.empty())
	{
		return FALSE;
	}
	if(getVisible() && getEnabled())
	{
		for (selected_items_t::const_iterator item_it = mSelectedItems.begin();
			 item_it != mSelectedItems.end(); ++item_it)
		{
			const LLFolderViewItem* item = (*item_it);
			const LLFolderViewEventListener* listener = item->getListener();
			if(!listener || !listener->isClipboardPasteable())
			{
				const LLFolderViewFolder* folderp = item->getParentFolder();
				listener = folderp->getListener();
				if (!listener || !listener->isClipboardPasteable())
				{
					return FALSE;
				}
			}
		}
		return TRUE;
	}
	return FALSE;
}
void LLFolderView::paste()
{
	if(getVisible() && getEnabled())
	{
		std::set<LLFolderViewItem*> folder_set;
		selected_items_t::iterator selected_it;
		for (selected_it = mSelectedItems.begin(); selected_it != mSelectedItems.end(); ++selected_it)
		{
			LLFolderViewItem* item = *selected_it;
			LLFolderViewEventListener* listener = item->getListener();
			if (listener->getInventoryType() != LLInventoryType::IT_CATEGORY)
			{
				item = item->getParentFolder();
			}
			folder_set.insert(item);
		}
		std::set<LLFolderViewItem*>::iterator set_iter;
		for(set_iter = folder_set.begin(); set_iter != folder_set.end(); ++set_iter)
		{
			LLFolderViewEventListener* listener = (*set_iter)->getListener();
			if(listener && listener->isClipboardPasteable())
			{
				listener->pasteFromClipboard();
			}
		}
	}
	mSearchString.clear();
}
void LLFolderView::startRenamingSelectedItem( void )
{
	scrollToShowSelection();
	S32 count = mSelectedItems.size();
	LLFolderViewItem* item = NULL;
	if(count > 0)
	{
		item = mSelectedItems.front();
	}
	if(getVisible() && getEnabled() && (count == 1) && item && item->getListener() &&
	   item->getListener()->isItemRenameable())
	{
		mRenameItem = item;
		updateRenamerPosition();
		mRenamer->setText(item->getName());
		mRenamer->selectAll();
		mRenamer->setVisible( TRUE );
		mRenamer->setFocus( TRUE );
		mRenamer->setTopLostCallback(boost::bind(&LLFolderView::onRenamerLost, this));
		gFocusMgr.setTopCtrl( mRenamer );
	}
}
BOOL LLFolderView::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (menu && menu->isOpen())
	{
		LLMenuGL::sMenuContainer->hideMenus();
	}
	switch( key )
	{
	case KEY_F2:
		mSearchString.clear();
		startRenamingSelectedItem();
		handled = TRUE;
		break;
	case KEY_RETURN:
		if (mask == MASK_NONE)
		{
			if( mRenameItem && mRenamer->getVisible() )
			{
				finishRenamingItem();
				mSearchString.clear();
				handled = TRUE;
			}
			else
			{
				LLFolderView::openSelectedItems();
				handled = TRUE;
			}
		}
		break;
	case KEY_ESCAPE:
		if( mRenameItem && mRenamer->getVisible() )
		{
			closeRenamer();
			handled = TRUE;
		}
		mSearchString.clear();
		break;
	case KEY_PAGE_UP:
		mSearchString.clear();
		if (mScrollContainer)
		{
			mScrollContainer->pageUp(30);
		}
		handled = TRUE;
		break;
	case KEY_PAGE_DOWN:
		mSearchString.clear();
		if (mScrollContainer)
		{
			mScrollContainer->pageDown(30);
		}
		handled = TRUE;
		break;
	case KEY_HOME:
		mSearchString.clear();
		if (mScrollContainer)
		{
			mScrollContainer->goToTop();
		}
		handled = TRUE;
		break;
	case KEY_END:
		mSearchString.clear();
		if (mScrollContainer)
		{
			mScrollContainer->goToBottom();
		}
		break;
	case KEY_DOWN:
		if((mSelectedItems.size() > 0) && mScrollContainer)
		{
			LLFolderViewItem* last_selected = getCurSelectedItem();
			bool shift_select = mask & MASK_SHIFT;
			LLFolderViewItem* next = last_selected->getNextOpenNode(!shift_select);
			if (!mKeyboardSelection || (!shift_select && (!next || next == last_selected)))
			{
				setSelection(last_selected, FALSE, TRUE);
				mKeyboardSelection = TRUE;
			}
			if (shift_select)
			{
				if (next)
				{
					if (next->isSelected())
					{
						changeSelection(last_selected, FALSE);
					}
					else if (last_selected->getParentFolder() == next->getParentFolder())
					{
						changeSelection(next, TRUE);
					}
				}
			}
			else
			{
				if( next )
				{
					if (next == last_selected)
					{
						if(notifyParent(LLSD().with("action","select_next")) > 0 )
						{
							clearSelection();
							return TRUE;
						}
						return FALSE;
					}
					setSelection( next, FALSE, TRUE );
				}
				else
				{
					if(notifyParent(LLSD().with("action","select_next")) > 0 )
					{
						clearSelection();
						return TRUE;
					}
					return FALSE;
				}
			}
			scrollToShowSelection();
			mSearchString.clear();
			handled = TRUE;
		}
		break;
	case KEY_UP:
		if((mSelectedItems.size() > 0) && mScrollContainer)
		{
			LLFolderViewItem* last_selected = mSelectedItems.back();
			bool shift_select = mask & MASK_SHIFT;
			LLFolderViewItem* prev = last_selected->getPreviousOpenNode(!shift_select);
			if (!mKeyboardSelection || (!shift_select && prev == this))
			{
				setSelection(last_selected, FALSE, TRUE);
				mKeyboardSelection = TRUE;
			}
			if (shift_select)
			{
				if (prev)
				{
					if (prev->isSelected())
					{
						changeSelection(last_selected, FALSE);
					}
					else if (last_selected->getParentFolder() == prev->getParentFolder())
					{
						changeSelection(prev, TRUE);
					}
				}
			}
			else
			{
				if( prev )
				{
					if (prev == this)
					{
						if(notifyParent(LLSD().with("action","select_prev")) > 0 )
						{
							clearSelection();
							return TRUE;
						}
						return FALSE;
					}
					setSelection( prev, FALSE, TRUE );
				}
			}
			scrollToShowSelection();
			mSearchString.clear();
			handled = TRUE;
		}
		break;
	case KEY_RIGHT:
		if(mSelectedItems.size())
		{
			LLFolderViewItem* last_selected = getCurSelectedItem();
			last_selected->setOpen( TRUE );
			mSearchString.clear();
			handled = TRUE;
		}
		break;
	case KEY_LEFT:
		if(mSelectedItems.size())
		{
			LLFolderViewItem* last_selected = getCurSelectedItem();
			LLFolderViewItem* parent_folder = last_selected->getParentFolder();
			if (!last_selected->isOpen() && parent_folder && parent_folder->getParentFolder())
			{
				setSelection(parent_folder, FALSE, TRUE);
			}
			else
			{
				last_selected->setOpen( FALSE );
			}
			mSearchString.clear();
			scrollToShowSelection();
			handled = TRUE;
		}
		break;
	}
	if (!handled && mParentPanel.get()->hasFocus())
	{
		if (key == KEY_BACKSPACE)
		{
			mSearchTimer.reset();
			if (mSearchString.size())
			{
				mSearchString.erase(mSearchString.size() - 1, 1);
			}
			search(getCurSelectedItem(), wstring_to_utf8str(mSearchString), FALSE);
			handled = TRUE;
		}
	}
	return handled;
}
BOOL LLFolderView::handleUnicodeCharHere(llwchar uni_char)
{
	if ((uni_char < 0x20) || (uni_char == 0x7F))
	{
		return FALSE;
	}
	BOOL handled = FALSE;
	if (mParentPanel.get()->hasFocus())
	{
		LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
		if (menu && menu->isOpen())
		{
			LLMenuGL::sMenuContainer->hideMenus();
		}
		static LLUICachedControl<F32> type_ahead_timeout("TypeAheadTimeout", 0.f);
		if (mSearchTimer.getElapsedTimeF32() > type_ahead_timeout)
		{
			mSearchString.clear();
		}
		mSearchTimer.reset();
		if (mSearchString.size() < 128)
		{
			mSearchString += uni_char;
		}
		search(getCurSelectedItem(), wstring_to_utf8str(mSearchString), FALSE);
		handled = TRUE;
	}
	return handled;
}
BOOL LLFolderView::canDoDelete() const
{
	if (mSelectedItems.size() == 0) return FALSE;
	for (selected_items_t::const_iterator item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
	{
		if (!(*item_it)->getListener()->isItemRemovable())
		{
			return FALSE;
		}
	}
	return TRUE;
}
void LLFolderView::doDelete()
{
	if(mSelectedItems.size() > 0)
	{
		removeSelectedItems();
	}
}
BOOL LLFolderView::handleMouseDown( S32 x, S32 y, MASK mask )
{
	mKeyboardSelection = FALSE;
	mSearchString.clear();
	mParentPanel.get()->setFocus(TRUE);
	LLEditMenuHandler::gEditMenuHandler = this;
	return LLView::handleMouseDown( x, y, mask );
}
BOOL LLFolderView::search(LLFolderViewItem* first_item, const std::string &search_string, BOOL backward)
{
	LLFolderViewItem* search_item = first_item;
	std::string upper_case_string = search_string;
	LLStringUtil::toUpper(upper_case_string);
	if (!search_item)
	{
		search_item = getNextFromChild(NULL);
	}
	BOOL found = FALSE;
	LLFolderViewItem* original_search_item = search_item;
	do
	{
		if (!search_item)
		{
			if (backward)
			{
				search_item = getPreviousFromChild(NULL);
			}
			else
			{
				search_item = getNextFromChild(NULL);
			}
			if (!search_item || search_item == original_search_item)
			{
				break;
			}
		}
		std::string current_item_label(search_item->getSearchableLabel());
		S32 search_string_length = llmin(upper_case_string.size(), current_item_label.size());
		if (!current_item_label.compare(0, search_string_length, upper_case_string))
		{
			found = TRUE;
			break;
		}
		if (backward)
		{
			search_item = search_item->getPreviousOpenNode();
		}
		else
		{
			search_item = search_item->getNextOpenNode();
		}
	} while(search_item != original_search_item);
	if (found)
	{
		setSelection(search_item, FALSE, TRUE);
		scrollToShowSelection();
	}
	return found;
}
BOOL LLFolderView::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	return LLView::handleDoubleClick( x, y, mask );
}
BOOL LLFolderView::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	mParentPanel.get()->setFocus(TRUE);
	BOOL handled = childrenHandleRightMouseDown(x, y, mask) != NULL;
	S32 count = mSelectedItems.size();
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (   handled
		&& ( count > 0 && (hasVisibleChildren() || mFilter.getShowFolderState() == LLInventoryFilter::SHOW_ALL_FOLDERS) )
		&& menu )
	{
		updateMenuOptions(menu);
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLMenuGL::showPopup(this, menu, x, y);
	}
	else
	{
		if(menu && menu->getVisible())
		{
			menu->setVisible(FALSE);
		}
		setSelection(NULL, FALSE, TRUE);
	}
	return handled;
}
BOOL LLFolderView::addNoOptions(LLMenuGL* menu) const
{
	const std::string nooptions_str = "--no options--";
	LLView *nooptions_item = NULL;
	const LLView::child_list_t *list = menu->getChildList();
	for (LLView::child_list_t::const_iterator itor = list->begin();
		 itor != list->end();
		 ++itor)
	{
		LLView *menu_item = (*itor);
		if (menu_item->getVisible())
		{
			return FALSE;
		}
		std::string name = menu_item->getName();
		if (menu_item->getName() == nooptions_str)
		{
			nooptions_item = menu_item;
		}
	}
	if (nooptions_item)
	{
		nooptions_item->setVisible(TRUE);
		nooptions_item->setEnabled(FALSE);
		return TRUE;
	}
	return FALSE;
}
BOOL LLFolderView::handleHover( S32 x, S32 y, MASK mask )
{
	return LLView::handleHover( x, y, mask );
}
BOOL LLFolderView::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	mDragAndDropThisFrame = TRUE;
	BOOL handled = LLView::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data,
											 accept, tooltip_msg);
	if (!handled)
	{
		if (getListener()->getUUID().notNull())
		{
			handled = LLFolderViewFolder::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
		}
		else
		{
			if (!mFolders.empty())
			{
				handled = mFolders.back()->handleDragAndDropFromChild(mask,drop,cargo_type,cargo_data,accept,tooltip_msg);
			}
		}
	}
	if (handled)
	{
		LL_DEBUGS("UserInput") << "dragAndDrop handled by LLFolderView" << LL_ENDL;
	}
	return handled;
}
BOOL LLFolderView::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (mScrollContainer)
	{
		return mScrollContainer->handleScrollWheel(x, y, clicks);
	}
	return FALSE;
}
void LLFolderView::deleteAllChildren()
{
	closeRenamer();
	if (mPopupMenuHandle.get()) mPopupMenuHandle.get()->die();
	mPopupMenuHandle = LLHandle<LLView>();
	mScrollContainer = NULL;
	mRenameItem = NULL;
	mRenamer = NULL;
	mStatusTextBox = NULL;
	clearSelection();
	LLView::deleteAllChildren();
}
void LLFolderView::scrollToShowSelection()
{
	if ( (LLInventoryModelBackgroundFetch::instance().isEverythingFetched() || mAutoSelectOverride)
			&& mSelectedItems.size() )
	{
		mNeedsScroll = TRUE;
	}
}
void LLFolderView::scrollToShowItem(LLFolderViewItem* item, const LLRect& constraint_rect)
{
	if (!mScrollContainer) return;
	if (gFocusMgr.childHasMouseCapture(mScrollContainer))
	{
		mNeedsScroll = FALSE;
		return;
	}
	if(item)
	{
		LLRect local_rect = item->getLocalRect();
		LLRect item_scrolled_rect;
		S32 icon_height = mIcon.isNull() ? 0 : mIcon->getHeight();
		S32 label_height = ll_round(getLabelFontForStyle(mLabelStyle)->getLineHeight());
		S32 max_height_to_show = item->isOpen() && mScrollContainer->hasFocus() ? (llmax( icon_height, label_height ) + ICON_PAD) : local_rect.getHeight();
		LLRect item_local_rect = LLRect(item->getIndentation(),
										local_rect.getHeight(),
										llmin(MIN_ITEM_WIDTH_VISIBLE, local_rect.getWidth()),
										llmax(0, local_rect.getHeight() - max_height_to_show));
		LLRect item_doc_rect;
		item->localRectToOtherView(item_local_rect, &item_doc_rect, this);
		mScrollContainer->scrollToShowRect( item_doc_rect, constraint_rect );
	}
}
void LLFolderView::setScrollContainer(LLScrollContainer* parent)
{
	mScrollContainer = parent;
	parent->setPassBackToChildren(false);
}
LLRect LLFolderView::getVisibleRect()
{
	S32 visible_height = mScrollContainer->getRect().getHeight();
	S32 visible_width = mScrollContainer->getRect().getWidth();
	LLRect visible_rect;
	visible_rect.setLeftTopAndSize(-getRect().mLeft, visible_height - getRect().mBottom, visible_width, visible_height);
	return visible_rect;
}
BOOL LLFolderView::getShowSelectionContext()
{
	if (mShowSelectionContext)
	{
		return TRUE;
	}
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (menu && menu->getVisible())
	{
		return TRUE;
	}
	return FALSE;
}
void LLFolderView::setShowSingleSelection(bool show)
{
	if (show != mShowSingleSelection)
	{
		mMultiSelectionFadeTimer.reset();
		mShowSingleSelection = show;
	}
}
void LLFolderView::addItemID(const LLUUID& id, LLFolderViewItem* itemp)
{
	mItemMap[id] = itemp;
}
void LLFolderView::removeItemID(const LLUUID& id)
{
	mItemMap.erase(id);
}
LLTrace::BlockTimerStatHandle FTM_GET_ITEM_BY_ID("Get FolderViewItem by ID");
LLFolderViewItem* LLFolderView::getItemByID(const LLUUID& id)
{
	LL_RECORD_BLOCK_TIME(FTM_GET_ITEM_BY_ID);
	if (id == getListener()->getUUID())
	{
		return this;
	}
	auto map_it = mItemMap.find(id);
	if (map_it != mItemMap.end())
	{
		return map_it->second;
	}
	return NULL;
}
LLFolderViewFolder* LLFolderView::getFolderByID(const LLUUID& id)
{
	if (id == getListener()->getUUID())
	{
		return this;
	}
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();
		 ++iter)
	{
		LLFolderViewFolder *folder = (*iter);
		if (folder->getListener()->getUUID() == id)
		{
			return folder;
		}
	}
	return NULL;
}
static LLTrace::BlockTimerStatHandle FTM_AUTO_SELECT("Open and Select");
static LLTrace::BlockTimerStatHandle FTM_INVENTORY("Inventory");
void LLFolderView::doIdle()
{
	const LLInventoryPanel *inventory_panel = dynamic_cast<LLInventoryPanel*>(mParentPanel.get());
	if (inventory_panel && !inventory_panel->getIsViewsInitialized())
	{
		return;
	}
	BOOL collectFavoriteItems(LLInventoryModel::item_array_t&);
	LLInventoryModel::item_array_t items;
	collectFavoriteItems(items);
	LL_RECORD_BLOCK_TIME(FTM_INVENTORY);
	BOOL debug_filters = gSavedSettings.getBOOL("DebugInventoryFilters");
	if (debug_filters != getDebugFilters())
	{
		mDebugFilters = debug_filters;
		arrangeAll();
	}
	mFilter.clearModified();
	BOOL filter_modified_and_active = mCompletedFilterGeneration < mFilter.getCurrentGeneration() &&
										mFilter.isNotDefault();
	mNeedsAutoSelect = filter_modified_and_active &&
							!(gFocusMgr.childHasKeyboardFocus(this) || gFocusMgr.getMouseCapture());
	filterFromRoot();
	if (mNeedsAutoSelect)
	{
		LL_RECORD_BLOCK_TIME(FTM_AUTO_SELECT);
		LLFolderViewItem* selected_itemp = mSelectedItems.empty() ? NULL : mSelectedItems.back();
		if ((selected_itemp && !selected_itemp->getFiltered()) && !mAutoSelectOverride)
		{
			LLSelectFirstFilteredItem filter;
			applyFunctorRecursively(filter);
		}
		if (mAutoSelectOverride && !mFilter.getFilterSubString().empty())
		{
			LLOpenFilteredFolders filter;
			applyFunctorRecursively(filter);
		}
		scrollToShowSelection();
	}
	if (filter_modified_and_active)
	{
		if (!mPinningSelectedItem && !mSelectedItems.empty())
		{
			mPinningSelectedItem = TRUE;
			LLRect visible_content_rect = mScrollContainer->getVisibleContentRect();
			LLFolderViewItem* selected_item = mSelectedItems.back();
			LLRect item_rect;
			selected_item->localRectToOtherView(selected_item->getLocalRect(), &item_rect, this);
			if (visible_content_rect.overlaps(item_rect))
			{
				mScrollConstraintRect = item_rect;
				mScrollConstraintRect.translate(-visible_content_rect.mLeft, -visible_content_rect.mBottom);
			}
			else
			{
				LLRect content_rect = mScrollContainer->getContentWindowRect();
				mScrollConstraintRect.setOriginAndSize(0, 0, content_rect.getWidth(), content_rect.getHeight());
			}
		}
	}
	else
	{
		if (!needsArrange())
		{
			mPinningSelectedItem = FALSE;
		}
	}
	LLRect constraint_rect;
	if (mPinningSelectedItem)
	{
		constraint_rect = mScrollConstraintRect;
	}
	else
	{
		LLRect content_rect = mScrollContainer->getContentWindowRect();
		constraint_rect.setOriginAndSize(0, 0, content_rect.getWidth(), content_rect.getHeight());
	}
	BOOL is_visible = isInVisibleChain();
	if ( is_visible )
	{
		sanitizeSelection();
		if( needsArrange() )
		{
			arrangeFromRoot();
		}
	}
	if (mSelectedItems.size() && mNeedsScroll)
	{
		scrollToShowItem(mSelectedItems.back(), constraint_rect);
		if (!filter_modified_and_active
			&& (!needsArrange() || !is_visible))
		{
			mNeedsScroll = FALSE;
		}
	}
	if (mSignalSelectCallback)
	{
		BOOL take_keyboard_focus = (mSignalSelectCallback == SIGNAL_KEYBOARD_FOCUS);
		mSelectSignal(mSelectedItems, take_keyboard_focus);
	}
	mSignalSelectCallback = FALSE;
}
void LLFolderView::idle(void* user_data)
{
	LLFolderView* self = (LLFolderView*)user_data;
	if ( self )
	{
		self->doIdle();
	}
}
void LLFolderView::dumpSelectionInformation()
{
	LL_INFOS() << "LLFolderView::dumpSelectionInformation()" << LL_NEWLINE
				<< "****************************************" << LL_ENDL;
	selected_items_t::iterator item_it;
	for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
	{
		LL_INFOS() << "  " << (*item_it)->getName() << LL_ENDL;
	}
	LL_INFOS() << "****************************************" << LL_ENDL;
}
void LLFolderView::updateRenamerPosition()
{
	if(mRenameItem)
	{
		S32 x = ARROW_SIZE + TEXT_PAD + ICON_WIDTH + ICON_PAD + mRenameItem->getIndentation();
		S32 y = mRenameItem->getRect().getHeight() - mRenameItem->getItemHeight() - RENAME_HEIGHT_PAD;
		mRenameItem->localPointToScreen( x, y, &x, &y );
		screenPointToLocal( x, y, &x, &y );
		mRenamer->setOrigin( x, y );
		LLRect scroller_rect(0, 0, gViewerWindow->getWindowWidthScaled(), 0);
		if (mScrollContainer)
		{
			scroller_rect = mScrollContainer->getContentWindowRect();
		}
		S32 width = llmax(llmin(mRenameItem->getRect().getWidth() - x, scroller_rect.getWidth() - x - getRect().mLeft), MINIMUM_RENAMER_WIDTH);
		S32 height = mRenameItem->getItemHeight() - RENAME_HEIGHT_PAD;
		mRenamer->reshape( width, height, TRUE );
	}
}
void LLFolderView::updateMenuOptions(LLMenuGL* menu)
{
	const LLView::child_list_t *list = menu->getChildList();
	LLView::child_list_t::const_iterator menu_itor;
	for (menu_itor = list->begin(); menu_itor != list->end(); ++menu_itor)
	{
		(*menu_itor)->setVisible(FALSE);
		(*menu_itor)->pushVisible(TRUE);
		(*menu_itor)->setEnabled(TRUE);
	}
	U32 multi_select_flag = (mSelectedItems.size() > 1 ? ITEM_IN_MULTI_SELECTION : 0x0);
	U32 flags = multi_select_flag | FIRST_SELECTED_ITEM;
	for (selected_items_t::iterator item_itor = mSelectedItems.begin();
			item_itor != mSelectedItems.end();
			++item_itor)
	{
		LLFolderViewItem* selected_item = (*item_itor);
		selected_item->buildContextMenu(*menu, flags);
		flags = multi_select_flag;
	}
	if (getFolderViewGroupedItemModel())
	{
		getFolderViewGroupedItemModel()->groupFilterContextMenu(mSelectedItems, *menu);
	}
	addNoOptions(menu);
}
void LLFolderView::updateMenu()
{
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (menu && menu->getVisible())
	{
		updateMenuOptions(menu);
		menu->needsArrange();
	}
}
void LLFolderView::saveFolderState()
{
	mSavedFolderState = std::unique_ptr<LLSaveFolderState>(new LLSaveFolderState());
	applyFunctorRecursively(*mSavedFolderState);
}
void LLFolderView::restoreFolderState()
{
	if (mSavedFolderState)
	{
		mSavedFolderState->setApply(true);
		applyFunctorRecursively(*mSavedFolderState);
		mSavedFolderState.reset();
	}
}
bool LLFolderView::selectFirstItem()
{
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();++iter)
	{
		LLFolderViewFolder* folder = (*iter );
		if (folder->getVisible())
		{
			LLFolderViewItem* itemp = folder->getNextFromChild(0,true);
			if(itemp)
				setSelection(itemp,FALSE,TRUE);
			return true;
		}
	}
	for(items_t::iterator iit = mItems.begin();
		iit != mItems.end(); ++iit)
	{
		LLFolderViewItem* itemp = (*iit);
		if (itemp->getVisible())
		{
			setSelection(itemp,FALSE,TRUE);
			return true;
		}
	}
	return false;
}
bool LLFolderView::selectLastItem()
{
	for(items_t::reverse_iterator iit = mItems.rbegin();
		iit != mItems.rend(); ++iit)
	{
		LLFolderViewItem* itemp = (*iit);
		if (itemp->getVisible())
		{
			setSelection(itemp,FALSE,TRUE);
			return true;
		}
	}
	for (folders_t::reverse_iterator iter = mFolders.rbegin();
		 iter != mFolders.rend();++iter)
	{
		LLFolderViewFolder* folder = (*iter);
		if (folder->getVisible())
		{
			LLFolderViewItem* itemp = folder->getPreviousFromChild(0,true);
			if(itemp)
				setSelection(itemp,FALSE,TRUE);
			return true;
		}
	}
	return false;
}
S32	LLFolderView::notify(const LLSD& info)
{
	if(info.has("action"))
	{
		std::string str_action = info["action"];
		if(str_action == "select_first")
		{
			setFocus(true);
			selectFirstItem();
			scrollToShowSelection();
			return 1;
		}
		else if(str_action == "select_last")
		{
			setFocus(true);
			selectLastItem();
			scrollToShowSelection();
			return 1;
		}
	}
	return 0;
}
void LLFolderView::onRenamerLost()
{
	if (mRenamer && mRenamer->getVisible())
	{
		mRenamer->setVisible(FALSE);
		mRenamer->setFocus(FALSE);
	}
	if( mRenameItem )
	{
		setSelectionFromRoot( mRenameItem, TRUE );
		mRenameItem = NULL;
	}
}
void LLFolderView::setFilterPermMask( PermissionMask filter_perm_mask )
{
	mFilter.setFilterPermissions(filter_perm_mask);
}
U32 LLFolderView::getFilterObjectTypes() const
{
	return mFilter.getFilterObjectTypes();
}
PermissionMask LLFolderView::getFilterPermissions() const
{
	return mFilter.getFilterPermissions();
}
BOOL LLFolderView::isFilterModified()
{
	return mFilter.isNotDefault();
}
void delete_selected_item(void* user_data)
{
	if(user_data)
	{
		LLFolderView* fv = reinterpret_cast<LLFolderView*>(user_data);
		fv->removeSelectedItems();
	}
}
void copy_selected_item(void* user_data)
{
	if(user_data)
	{
		LLFolderView* fv = reinterpret_cast<LLFolderView*>(user_data);
		fv->copy();
	}
}
void paste_items(void* user_data)
{
	if(user_data)
	{
		LLFolderView* fv = reinterpret_cast<LLFolderView*>(user_data);
		fv->paste();
	}
}
void open_selected_items(void* user_data)
{
	if(user_data)
	{
		LLFolderView* fv = reinterpret_cast<LLFolderView*>(user_data);
		fv->openSelectedItems();
	}
}
void properties_selected_items(void* user_data)
{
	if(user_data)
	{
		LLFolderView* fv = reinterpret_cast<LLFolderView*>(user_data);
		fv->propertiesSelectedItems();
	}
}
