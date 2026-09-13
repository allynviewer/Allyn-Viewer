/** 
* @file llfolderviewitem.cpp
* @brief Items and folders that can appear in a hierarchical folder view
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
#include "llfolderviewitem.h"
#include "llfolderview.h"
#include "llfoldervieweventlistener.h"
#include "llviewerfoldertype.h"
#include "llinventorybridge.h"
#include "llinventoryfilter.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llresmgr.h"
#include "llpanel.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llfocusmgr.h"
#include "lltrans.h"
std::map<U8, LLFontGL*> LLFolderViewItem::sFonts;
const F32 LLFolderViewItem::FOLDER_CLOSE_TIME_CONSTANT = 0.02f;
const F32 LLFolderViewItem::FOLDER_OPEN_TIME_CONSTANT = 0.03f;
LLUIImagePtr LLFolderViewItem::sArrowImage;
LLUIImagePtr LLFolderViewItem::sBoxImage;
LLFontGL* LLFolderViewItem::getLabelFontForStyle(U8 style)
{
	LLFontGL* rtn = sFonts[style];
	if (!rtn)
	{
		LLFontDescriptor labelfontdesc("SansSerif", "Small", style);
		rtn = LLFontGL::getFont(labelfontdesc);
		if (!rtn)
		{
			rtn = LLFontGL::getFontMonospace();
		}
		sFonts[style] = rtn;
	}
	return rtn;
}
void LLFolderViewItem::initClass()
{
	sArrowImage = LLUI::getUIImage("folder_arrow.tga");
	sBoxImage = LLUI::getUIImage("Rounded_Square");
}
void LLFolderViewItem::cleanupClass()
{
	sFonts.clear();
	sArrowImage = NULL;
	sBoxImage = NULL;
}
LLFolderViewItem::LLFolderViewItem( const std::string& name, LLUIImagePtr icon,
									LLUIImagePtr icon_open,
									LLUIImagePtr icon_overlay,
								   S32 creation_date,
								   LLFolderView* root,
									LLFolderViewEventListener* listener ) :
	LLUICtrl( name, LLRect(0, 0, 0, 0), TRUE, NULL, FOLLOWS_LEFT|FOLLOWS_TOP|FOLLOWS_RIGHT),
	mLabelWidth(0),
	mLabelWidthDirty(false),
	mParentFolder( NULL ),
	mIsSelected( FALSE ),
	mIsCurSelection( FALSE ),
	mSelectPending(FALSE),
	mLabelStyle( LLFontGL::NORMAL ),
	mHasVisibleChildren(FALSE),
	mIndentation(0),
	mItemHeight(16 + ICON_PAD),
	mPassedFilter(FALSE),
	mLastFilterGeneration(-1),
	mStringMatchOffset(std::string::npos),
	mControlLabelRotation(0.f),
	mDragAndDropTarget(FALSE),
	mIsLoading(FALSE),
	mLabel( name ),
	mRoot( root ),
	mCreationDate(creation_date),
	mIcon(icon),
	mIconOpen(icon_open),
	mIconOverlay(icon_overlay),
	mListener(listener),
	mShowLoadStatus(true),
	mSearchType(0)
{
	postBuild();
}
BOOL LLFolderViewItem::postBuild()
{
	refresh();
	setTabStop(FALSE);
	return TRUE;
}
LLFolderViewItem::~LLFolderViewItem( void )
{
	delete mListener;
	mListener = NULL;
}
LLFolderView* LLFolderViewItem::getRoot()
{
	return mRoot;
}
const LLFolderView* LLFolderViewItem::getRoot() const
{
	return mRoot;
}
BOOL LLFolderViewItem::isDescendantOf( const LLFolderViewFolder* potential_ancestor )
{
	LLFolderViewItem* root = this;
	while( root->mParentFolder )
	{
		if( root->mParentFolder == potential_ancestor )
		{
			return TRUE;
		}
		root = root->mParentFolder;
	}
	return FALSE;
}
LLFolderViewItem* LLFolderViewItem::getNextOpenNode(BOOL include_children)
{
	if (!mParentFolder)
	{
		return NULL;
	}
	LLFolderViewItem* itemp = mParentFolder->getNextFromChild( this, include_children );
	while(itemp && !itemp->getVisible())
	{
		LLFolderViewItem* next_itemp = itemp->mParentFolder->getNextFromChild( itemp, include_children );
		if (itemp == next_itemp)
		{
			return itemp->getVisible() ? itemp : this;
		}
		itemp = next_itemp;
	}
	return itemp;
}
LLFolderViewItem* LLFolderViewItem::getPreviousOpenNode(BOOL include_children)
{
	if (!mParentFolder)
	{
		return NULL;
	}
	LLFolderViewItem* itemp = mParentFolder->getPreviousFromChild( this, include_children );
	while(itemp && !itemp->getVisible())
	{
		LLFolderViewItem* next_itemp = itemp->mParentFolder->getPreviousFromChild( itemp, include_children );
		if (itemp == next_itemp)
		{
			return itemp->getVisible() ? itemp : this;
		}
		itemp = next_itemp;
	}
	return itemp;
}
BOOL LLFolderViewItem::potentiallyVisible()
{
	return getLastFilterGeneration() < getRoot()->getFilter().getFirstSuccessGeneration() || getFiltered();
}
BOOL LLFolderViewItem::getFiltered()
{
	return mPassedFilter && mLastFilterGeneration >= getRoot()->getFilter().getFirstSuccessGeneration();
}
BOOL LLFolderViewItem::getFiltered(S32 filter_generation)
{
	return mPassedFilter && mLastFilterGeneration >= filter_generation;
}
void LLFolderViewItem::setFiltered(BOOL filtered, S32 filter_generation)
{
	mPassedFilter = filtered;
	mLastFilterGeneration = filter_generation;
}
void LLFolderViewItem::setIcon(LLUIImagePtr icon)
{
	mIcon = icon;
}
void LLFolderViewItem::refreshFromListener()
{
	if(mListener)
	{
		mLabel = mListener->getDisplayName();
		setIcon(mListener->getIcon());
		time_t creation_date = mListener->getCreationDate();
		if ((creation_date > 0) && (mCreationDate != creation_date))
		{
			setCreationDate(creation_date);
			dirtyFilter();
		}
		if (mRoot->useLabelSuffix())
		{
			mLabelStyle = mListener->getLabelStyle();
			mLabelSuffix = mListener->getLabelSuffix();
		}
		updateExtraSearchCriteria();
	}
}
void LLFolderViewItem::updateExtraSearchCriteria()
{
	if(!mListener)
		return;
	LLInventoryItem* item = gInventory.getItem(mListener->getUUID());
	std::string desc;
	if (item)
	{
		if (!item->getDescription().empty())
		{
			desc = item->getDescription();
			LLStringUtil::toUpper(desc);
		}
	}
	mSearchableLabelDesc = desc;
	std::string creator_name;
	if (item)
	{
		if (item->getCreatorUUID().notNull())
		{
			gCacheName->getFullName(item->getCreatorUUID(), creator_name);
			LLStringUtil::toUpper(creator_name);
		}
	}
	mSearchableLabelCreator = creator_name;
	updateSearchLabelType();
}
void LLFolderViewItem::refresh()
{
	refreshFromListener();
	std::string searchable_label(mLabel);
	searchable_label.append(mLabelSuffix);
	LLStringUtil::toUpper(searchable_label);
	if (mSearchableLabel.compare(searchable_label))
	{
		mSearchableLabel.assign(searchable_label);
		updateSearchLabelType();
		dirtyFilter();
		if (mParentFolder)
		{
			mParentFolder->requestSort();
			mParentFolder->requestArrange();
		}
	}
	mLabelWidthDirty = true;
}
void LLFolderViewItem::applyListenerFunctorRecursively(LLFolderViewListenerFunctor& functor)
{
	functor(mListener);
}
void LLFolderViewItem::filterFromRoot( void )
{
	LLFolderViewItem* root = getRoot();
	root->filter(((LLFolderView*)root)->getFilter());
}
void LLFolderViewItem::arrangeFromRoot()
{
	LLFolderViewItem* root = getRoot();
	S32 height = 0;
	S32 width = 0;
	S32 total_height = root->arrange( &width, &height, 0 );
	LLSD params;
	params["action"] = "size_changes";
	params["height"] = total_height;
	getParent()->notifyParent(params);
}
void LLFolderViewItem::arrangeAndSet(BOOL set_selection,
									 BOOL take_keyboard_focus)
{
	LLFolderView* root = getRoot();
	if (getParentFolder())
	{
	getParentFolder()->requestArrange();
	}
	if(set_selection)
	{
		setSelectionFromRoot(this, TRUE, take_keyboard_focus);
		if(root)
		{
			root->scrollToShowSelection();
		}
	}
}
void LLFolderViewItem::setSelectionFromRoot(LLFolderViewItem* selection,
											BOOL openitem,
											BOOL take_keyboard_focus)
{
	getRoot()->setSelection(selection, openitem, take_keyboard_focus);
}
uuid_set_t LLFolderViewItem::getSelectionList() const
{
	return uuid_set_t();
}
EInventorySortGroup LLFolderViewItem::getSortGroup()  const
{
	return SG_ITEM;
}
BOOL LLFolderViewItem::addToFolder(LLFolderViewFolder* folder, LLFolderView* root)
{
	if (!folder)
	{
		return FALSE;
	}
	mParentFolder = folder;
	root->addItemID(getListener()->getUUID(), this);
	return folder->addItem(this);
}
S32 LLFolderViewItem::arrange( S32* width, S32* height, S32 filter_generation)
{
	S32 indentation = LEFT_INDENTATION;
	mIndentation = (getParentFolder()
					&& getParentFolder()->getParentFolder() )
		? mParentFolder->getIndentation() + indentation
		: 0;
	if (mLabelWidthDirty)
	{
		mLabelWidth = ARROW_SIZE + TEXT_PAD + ICON_WIDTH + ICON_PAD + getLabelFontForStyle(mLabelStyle)->getWidth(mSearchableLabel);
		mLabelWidthDirty = false;
	}
	*width = llmax(*width, mLabelWidth + mIndentation);
	bool use_ellipses = getRoot()->getUseEllipses();
	if (use_ellipses)
	{
		*width = llmin(*width, getRoot()->getRect().getWidth());
	}
	*height = getItemHeight();
	return *height;
}
S32 LLFolderViewItem::getItemHeight()
{
	return mItemHeight;
}
void LLFolderViewItem::filter( LLInventoryFilter& filter)
{
	const BOOL previous_passed_filter = mPassedFilter;
	const BOOL passed_filter = filter.check(this);
	if (mParentFolder)
	{
		if (getVisible() != passed_filter
			||	previous_passed_filter != passed_filter )
			mParentFolder->requestArrange();
	}
	setFiltered(passed_filter, filter.getCurrentGeneration());
	mStringMatchOffset = filter.getStringMatchOffset();
	if (mSearchType & 4 && mStringMatchOffset >= mSearchable.length()-mSearchableLabelCreator.length())
		mStringMatchOffset = std::string::npos;
	filter.decrementFilterCount();
	if (getRoot()->getDebugFilters())
	{
		mStatusText = llformat("%d", mLastFilterGeneration);
	}
}
void LLFolderViewItem::dirtyFilter()
{
	mLastFilterGeneration = -1;
	if (getParentFolder())
	{
		getParentFolder()->setCompletedFilterGeneration(-1, TRUE);
	}
}
BOOL LLFolderViewItem::setSelection(LLFolderViewItem* selection, BOOL openitem, BOOL take_keyboard_focus)
{
	if (selection == this && !mIsSelected)
	{
		selectItem();
	}
	else if (mIsSelected)
	{
		deselectItem();
	}
	return mIsSelected;
}
BOOL LLFolderViewItem::changeSelection(LLFolderViewItem* selection, BOOL selected)
{
	if (selection == this)
	{
		if (mIsSelected)
		{
			deselectItem();
		}
		else
		{
			selectItem();
		}
		return TRUE;
	}
	return FALSE;
}
void LLFolderViewItem::deselectItem(void)
{
	mIsSelected = FALSE;
}
void LLFolderViewItem::selectItem(void)
{
	if (mIsSelected == FALSE)
	{
		if (mListener)
		{
			mListener->selectItem();
		}
		mIsSelected = TRUE;
	}
}
BOOL LLFolderViewItem::isMovable()
{
	if( mListener )
	{
		return mListener->isItemMovable();
	}
	else
	{
		return TRUE;
	}
}
BOOL LLFolderViewItem::isRemovable()
{
	if( mListener )
	{
		return mListener->isItemRemovable();
	}
	else
	{
		return TRUE;
	}
}
void LLFolderViewItem::destroyView()
{
	if (mParentFolder)
	{
		mParentFolder->removeView(this);
	}
}
BOOL LLFolderViewItem::remove()
{
	if(!isRemovable())
	{
		return FALSE;
	}
	if(mListener)
	{
		return mListener->removeItem();
	}
	return TRUE;
}
void LLFolderViewItem::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	if(mListener)
	{
		mListener->buildContextMenu(menu, flags);
	}
}
void LLFolderViewItem::openItem( void )
{
	if (!mListener) return;
	{
		mListener->openItem();
	}
}
void LLFolderViewItem::preview( void )
{
	if (mListener)
	{
		mListener->previewItem();
	}
}
void LLFolderViewItem::rename(const std::string& new_name)
{
	if( !new_name.empty() )
	{
		mLabel = new_name;
		if( mListener )
		{
			mListener->renameItem(new_name);
			if(mParentFolder)
			{
				mParentFolder->requestSort();
			}
		}
	}
}
void LLFolderViewItem::nameOrDescriptionChanged(void) const
{
	if( mListener )
	{
		mListener->nameOrDescriptionChanged();
	}
}
void LLFolderViewItem::updateSearchLabelType()
{
	mSearchType = mRoot->getSearchType();
	mSearchable.erase();
	if (!mSearchType || mSearchType & 1)
		mSearchable = mSearchableLabel;
	if (mSearchType & 2)
	{
		if (mSearchable.length())
		{
			mSearchable += " ";
		}
		mSearchable += mSearchableLabelDesc;
	}
	if (mSearchType & 4)
	{
		if (mSearchable.length())
		{
			mSearchable += " ";
		}
		mSearchable += mSearchableLabelCreator;
	}
}
const std::string& LLFolderViewItem::getSearchableLabel()
{
	if(mSearchType != mRoot->getSearchType())
		updateSearchLabelType();
	return mSearchable;
}
LLViewerInventoryItem * LLFolderViewItem::getInventoryItem(void)
{
	if (!getListener()) return NULL;
	return gInventory.getItem(getListener()->getUUID());
}
const std::string& LLFolderViewItem::getName( void ) const
{
	if(mListener)
	{
		return mListener->getName();
	}
	return mLabel;
}
BOOL LLFolderViewItem::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	if(!mIsSelected)
	{
		getRoot()->setSelection(this, FALSE);
	}
	make_ui_sound("UISndClick");
	return TRUE;
}
BOOL LLFolderViewItem::handleMouseDown( S32 x, S32 y, MASK mask )
{
	if (LLView::childrenHandleMouseDown(x, y, mask))
	{
		return TRUE;
	}
	gFocusMgr.setMouseCapture( this );
	if (!mIsSelected)
	{
		if(mask & MASK_CONTROL)
		{
			getRoot()->changeSelection(this, !mIsSelected);
		}
		else if (mask & MASK_SHIFT)
		{
			getParentFolder()->extendSelectionTo(this);
		}
		else
		{
			getRoot()->setSelection(this, FALSE);
		}
		make_ui_sound("UISndClick");
	}
	else
	{
		mSelectPending = TRUE;
	}
	if( isMovable() )
	{
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y );
		LLToolDragAndDrop::getInstance()->setDragStart( screen_x, screen_y );
	}
	return TRUE;
}
BOOL LLFolderViewItem::handleHover( S32 x, S32 y, MASK mask )
{
	if( hasMouseCapture() && isMovable() )
	{
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y );
		BOOL can_drag = TRUE;
		if( LLToolDragAndDrop::getInstance()->isOverThreshold( screen_x, screen_y ) )
		{
			LLFolderView* root = getRoot();
			if(root->getCurSelectedItem())
			{
				LLToolDragAndDrop::ESource src = LLToolDragAndDrop::SOURCE_WORLD;
				if (mListener
					&& gInventory.isObjectDescendentOf(mListener->getUUID(), gInventory.getRootFolderID()))
				{
					src = LLToolDragAndDrop::SOURCE_AGENT;
				}
				else if (mListener
					&& gInventory.isObjectDescendentOf(mListener->getUUID(), gInventory.getLibraryRootFolderID()))
				{
					src = LLToolDragAndDrop::SOURCE_LIBRARY;
				}
				can_drag = root->startDrag(src);
				if (can_drag)
				{
					root->autoOpenTest(NULL);
					root->setShowSelectionContext(TRUE);
					gFocusMgr.setKeyboardFocus(NULL);
					return LLToolDragAndDrop::getInstance()->handleHover( screen_x, screen_y, mask );
				}
			}
		}
		if (can_drag)
		{
			gViewerWindow->setCursor(UI_CURSOR_ARROW);
		}
		else
		{
			gViewerWindow->setCursor(UI_CURSOR_NOLOCKED);
		}
		return TRUE;
	}
	else
	{
		getRoot()->setShowSelectionContext(FALSE);
		gViewerWindow->setCursor(UI_CURSOR_ARROW);
		return FALSE;
	}
}
BOOL LLFolderViewItem::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	preview();
	return TRUE;
}
BOOL LLFolderViewItem::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (getParent())
	{
		return getParent()->handleScrollWheel(x, y, clicks);
	}
	return FALSE;
}
BOOL LLFolderViewItem::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if (LLView::childrenHandleMouseUp(x, y, mask))
	{
		return TRUE;
	}
	if ( pointInView(x, y) && mSelectPending )
	{
		if(mask & MASK_CONTROL)
		{
			getRoot()->changeSelection(this, !mIsSelected);
		}
		else if (mask & MASK_SHIFT)
		{
			getParentFolder()->extendSelectionTo(this);
		}
		else
		{
			getRoot()->setSelection(this, FALSE);
		}
	}
	mSelectPending = FALSE;
	if( hasMouseCapture() )
	{
		if (getRoot())
		{
			getRoot()->setShowSelectionContext(FALSE);
		}
		gFocusMgr.setMouseCapture( NULL );
	}
	return TRUE;
}
BOOL LLFolderViewItem::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data,
										 EAcceptance* accept,
										 std::string& tooltip_msg)
{
	BOOL accepted = FALSE;
	BOOL handled = FALSE;
	if(mListener)
	{
		accepted = mListener->dragOrDrop(mask,drop,cargo_type,cargo_data);
		handled = accepted;
		if (accepted)
		{
			mDragAndDropTarget = TRUE;
			*accept = ACCEPT_YES_MULTI;
		}
		else
		{
			*accept = ACCEPT_NO;
		}
	}
	if(mParentFolder && !handled)
	{
		mRoot->setDraggingOverItem(this);
		handled = mParentFolder->handleDragAndDropFromChild(mask,drop,cargo_type,cargo_data,accept,tooltip_msg);
		mRoot->setDraggingOverItem(NULL);
	}
	if (handled)
	{
		LL_DEBUGS("UserInput") << "dragAndDrop handled by LLFolderViewItem" << LL_ENDL;
	}
	return handled;
}
void LLFolderViewItem::draw()
{
	static LLCachedControl<LLColor4> sFgColor(gColors, "MenuItemEnabledColor", LLColor4::white );
	static LLCachedControl<LLColor4> sHighlightBgColor(gColors, "MenuItemHighlightBgColor", LLColor4::white );
	static LLCachedControl<LLColor4> sHighlightFgColor(gColors, "MenuItemHighlightFgColor", LLColor4::white );
	static LLCachedControl<LLColor4> sFilterBGColor(gColors, "FilterBackgroundColor", LLColor4::white );
	static LLCachedControl<LLColor4> sFilterTextColor(gColors, "FilterTextColor", LLColor4::white );
	static LLCachedControl<LLColor4> sSuffixColor(gColors, "InventoryItemSuffixColor", LLColor4::white );
	static LLCachedControl<LLColor4> sSearchStatusColor(gColors, "InventorySearchStatusColor", LLColor4::white );
	const S32 TOP_PAD = 4;
	const S32 FOCUS_LEFT = 1;
	const LLFontGL* font = getLabelFontForStyle(mLabelStyle);
	const LLUUID* id = getListener() ? &getListener()->getUUID() : nullptr;
	const BOOL in_inventory = id && gInventory.isObjectDescendentOf(*id, gInventory.getRootFolderID());
	const BOOL in_library = id && !in_inventory && gInventory.isObjectDescendentOf(*id, gInventory.getLibraryRootFolderID());
	if (in_inventory && !getFiltered() && depth_nesting_in_marketplace(*id) == 1) return;
	const bool up_to_date = mListener && mListener->isUpToDate();
	const bool possibly_has_children = ((up_to_date && hasVisibleChildren())
										|| (!up_to_date && mListener && mListener->hasChildren()));
	if (possibly_has_children && sArrowImage)
	{
		gl_draw_scaled_rotated_image(
			mIndentation, getRect().getHeight() - ARROW_SIZE - TEXT_PAD - TOP_PAD,
			ARROW_SIZE, ARROW_SIZE, mControlLabelRotation, sArrowImage->getImage(), sFgColor);
	}
	const BOOL show_context = getRoot()->getShowSelectionContext();
	const BOOL filled = show_context || (getRoot()->getParentPanel()->hasFocus());
	const S32 focus_top = getRect().getHeight();
	const S32 focus_bottom = getRect().getHeight() - mItemHeight;
	const bool folder_open = (getRect().getHeight() > mItemHeight + 4);
	if (mIsSelected)
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		LLColor4 bg_color = sHighlightBgColor;
		if (!mIsCurSelection)
		{
			F32 fade_time = getRoot()->getSelectionFadeElapsedTime();
			if (getRoot()->getShowSingleSelection())
			{
				bg_color.mV[VALPHA] = clamp_rescale(fade_time, 0.f, 0.4f, bg_color.mV[VALPHA], 0.f);
			}
			else
			{
				bg_color.mV[VALPHA] = clamp_rescale(fade_time, 0.f, 0.4f, 0.f, bg_color.mV[VALPHA]);
			}
		}
		gl_rect_2d(FOCUS_LEFT,
				   focus_top,
				   getRect().getWidth() - 2,
				   focus_bottom,
				   bg_color, filled);
		if (mIsCurSelection)
		{
			gl_rect_2d(FOCUS_LEFT,
					   focus_top,
					   getRect().getWidth() - 2,
					   focus_bottom,
				sHighlightFgColor, FALSE);
		}
		if (folder_open)
		{
			gl_rect_2d(FOCUS_LEFT,
					   focus_bottom + 1,
					   getRect().getWidth() - 2,
					   0,
				sHighlightFgColor, FALSE);
			if (show_context)
			{
				gl_rect_2d(FOCUS_LEFT,
						   focus_bottom + 1,
						   getRect().getWidth() - 2,
						   0,
						   sHighlightBgColor, TRUE);
			}
		}
	}
	if (mDragAndDropTarget)
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gl_rect_2d(FOCUS_LEFT,
				   focus_top,
				   getRect().getWidth() - 2,
				   focus_bottom,
				   sHighlightBgColor, FALSE);
		if (folder_open)
		{
			gl_rect_2d(FOCUS_LEFT,
					   focus_bottom + 1,
					   getRect().getWidth() - 2,
					   0,
					   sHighlightBgColor, FALSE);
		}
		mDragAndDropTarget = FALSE;
	}
	const LLViewerInventoryItem *item = getInventoryItem();
	const BOOL highlight_link = mIconOverlay && item && item->getIsLinkType();
	const S32 icon_x = mIndentation + ARROW_SIZE + TEXT_PAD;
	if (!mIconOpen.isNull() && (llabs(mControlLabelRotation) > 80))
 	{
		mIconOpen->draw(icon_x, getRect().getHeight() - mIconOpen->getHeight() - TOP_PAD + 1);
	}
	else if (mIcon)
	{
 		mIcon->draw(icon_x, getRect().getHeight() - mIcon->getHeight() - TOP_PAD + 1);
 	}
	if (highlight_link)
	{
		mIconOverlay->draw(icon_x, getRect().getHeight() - mIcon->getHeight() - TOP_PAD + 1);
	}
	if (mLabel.empty())
	{
		return;
	}
	LLColor4 color = (mIsSelected && filled) ? sHighlightFgColor : sFgColor;
	F32 right_x  = 0;
	F32 y = (F32)getRect().getHeight() - font->getLineHeight() - (F32)TEXT_PAD - (F32)TOP_PAD;
	F32 text_left = (F32)(ARROW_SIZE + TEXT_PAD + ICON_WIDTH + ICON_PAD + mIndentation);
	if (getRoot()->getDebugFilters())
	{
		if (!getFiltered() && !possibly_has_children)
		{
			color.mV[VALPHA] *= 0.5f;
		}
		LLColor4 filter_color = mLastFilterGeneration >= getRoot()->getFilter().getCurrentGeneration() ?
			LLColor4(0.5f, 0.8f, 0.5f, 1.f) :
			LLColor4(0.8f, 0.5f, 0.5f, 1.f);
		LLFontGL::getFontMonospace()->renderUTF8(mStatusText, 0, text_left, y, filter_color,
												 LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
												 S32_MAX, S32_MAX, &right_x, FALSE );
		text_left = right_x;
	}
	font->renderUTF8(mLabel, 0, text_left, y, color,
					 LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
					 S32_MAX, getRect().getWidth() - (S32) text_left, &right_x, TRUE);
	bool root_is_loading = false;
	if (in_inventory)
	{
		root_is_loading = LLInventoryModelBackgroundFetch::instance().inventoryFetchInProgress();
	}
	if (in_library)
	{
		root_is_loading = LLInventoryModelBackgroundFetch::instance().libraryFetchInProgress();
	}
	if ((mIsLoading
		&&	mTimeSinceRequestStart.getElapsedTimeF32() >= gSavedSettings.getF32("FolderLoadingMessageWaitTime"))
			||	(LLInventoryModelBackgroundFetch::instance().folderFetchActive()
				&&	root_is_loading
				&&	mShowLoadStatus))
	{
		static std::string const load_string = " ( " + LLTrans::getString("LoadingData") + " ) ";
		font->renderUTF8(load_string, 0, right_x, y, sSearchStatusColor,
						 LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
						 S32_MAX, S32_MAX, &right_x, FALSE);
	}
	if (!mLabelSuffix.empty())
	{
		font->renderUTF8( mLabelSuffix, 0, right_x, y, sSuffixColor,
						  LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
						  S32_MAX, S32_MAX, &right_x, FALSE );
	}
	if (sBoxImage.notNull() && mStringMatchOffset != std::string::npos)
	{
		S32 filter_string_length = getRoot()->getFilterSubString().size();
		if (filter_string_length > 0 && (mRoot->getSearchType() & 1))
		{
			std::string combined_string = mLabel + mLabelSuffix;
			S32 left = ll_round(text_left) + font->getWidth(combined_string, 0, mStringMatchOffset) - 1;
			S32 right = left + font->getWidth(combined_string, mStringMatchOffset, filter_string_length) + 2;
			S32 bottom = llfloor(getRect().getHeight() - font->getLineHeight() - 3 - TOP_PAD);
			S32 top = getRect().getHeight() - TOP_PAD;
			LLUIImage* box_image = sBoxImage;
			LLRect box_rect(left, top, right, bottom);
			box_image->draw(box_rect, sFilterBGColor);
			F32 match_string_left = text_left + font->getWidthF32(combined_string, 0, mStringMatchOffset);
			F32 yy = (F32)getRect().getHeight() - font->getLineHeight() - (F32)TEXT_PAD - (F32)TOP_PAD;
			font->renderUTF8( combined_string, mStringMatchOffset, match_string_left, yy,
							sFilterTextColor, LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
							filter_string_length, S32_MAX, &right_x, FALSE );
		}
	}
	if( sDebugRects )
	{
		drawDebugRect();
	}
}
LLFolderViewFolder::LLFolderViewFolder( const std::string& name, LLUIImagePtr icon,
										LLUIImagePtr icon_open,
										LLUIImagePtr icon_link,
										LLFolderView* root,
										LLFolderViewEventListener* listener):
	LLFolderViewItem( name, icon, icon_open, icon_link, 0, root, listener ),
	mIsOpen(FALSE),
	mExpanderHighlighted(FALSE),
	mCurHeight(0.f),
	mTargetHeight(0.f),
	mAutoOpenCountdown(0.f),
	mSubtreeCreationDate(0),
	mAmTrash(LLFolderViewFolder::UNKNOWN),
	mLastArrangeGeneration( -1 ),
	mLastCalculatedWidth(0),
	mCompletedFilterGeneration(-1),
	mMostFilteredDescendantGeneration(-1),
	mNeedsSort(false),
	mPassedFolderFilter(FALSE)
{
}
LLFolderViewFolder::~LLFolderViewFolder( void )
{
	gFocusMgr.releaseFocusIfNeeded( this );
}
void LLFolderViewFolder::setFilteredFolder(bool filtered, S32 filter_generation)
{
	mPassedFolderFilter = filtered;
	mLastFilterGeneration = filter_generation;
}
bool LLFolderViewFolder::getFilteredFolder(S32 filter_generation)
{
	return mPassedFolderFilter && mLastFilterGeneration >= getRoot()->getFilter().getFirstSuccessGeneration();
}
BOOL LLFolderViewFolder::addToFolder(LLFolderViewFolder* folder, LLFolderView* root)
{
	if (!folder)
	{
		return FALSE;
	}
	mParentFolder = folder;
	root->addItemID(getListener()->getUUID(), this);
	return folder->addFolder(this);
}
S32 LLFolderViewFolder::arrange( S32* width, S32* height, S32 filter_generation)
{
	if (mNeedsSort)
	{
		mFolders.sort(mSortFunction);
		mItems.sort(mSortFunction);
		mNeedsSort = false;
	}
	bool filtered = !getFilteredFolder(filter_generation);
	LLFolderViewItem::arrange( width, height, filter_generation );
	mCurHeight = llmax((F32)*height, mCurHeight);
	*height = getItemHeight();
	F32 running_height = (F32)*height;
	F32 target_height = (F32)*height;
	bool marketplace_top = mListener && depth_nesting_in_marketplace(mListener->getUUID()) == 1;
	if (needsArrange())
	{
		mLastArrangeGeneration = getRoot()->getArrangeGeneration();
		mHasVisibleChildren = !filtered && hasFilteredDescendants(filter_generation);
		if (mHasVisibleChildren)
		{
			bool found = false;
			for (items_t::iterator iit = mItems.begin(); iit != mItems.end(); ++iit)
			{
				LLFolderViewItem* itemp = (*iit);
				found = itemp->getFiltered(filter_generation);
				if (found)
					break;
			}
			if (!found)
			{
				for (folders_t::iterator fit = mFolders.begin(); fit != mFolders.end(); ++fit)
				{
					LLFolderViewFolder* folderp = (*fit);
					found = folderp->getListener()
						&& (folderp->getFiltered(filter_generation)
							|| (folderp->getFilteredFolder(filter_generation)
								&& folderp->hasFilteredDescendants(filter_generation)));
					if (found)
						break;
				}
			}
			mHasVisibleChildren = found;
		}
		if (marketplace_top)
		{
			setVisible(!filtered);
			if (filtered) mCurHeight = target_height = 0;
		}
		if (mIsOpen)
		{
			S32 parent_item_height = getRect().getHeight();
			for(folders_t::iterator fit = mFolders.begin(); fit != mFolders.end(); ++fit)
			{
				LLFolderViewFolder* folderp = (*fit);
				if (getRoot()->getDebugFilters())
				{
					folderp->setVisible(TRUE);
				}
				else
				{
					folderp->setVisible( folderp->getListener()
										&&	(folderp->getFiltered(filter_generation)
											||	(folderp->getFilteredFolder(filter_generation)
												&& folderp->hasFilteredDescendants(filter_generation))));
				}
				if (folderp->getVisible())
				{
					S32 child_width = *width;
					S32 child_height = 0;
					S32 child_top = parent_item_height - ll_round(running_height);
					target_height += folderp->arrange( &child_width, &child_height, filter_generation );
					running_height += (F32)child_height;
					*width = llmax(*width, child_width);
					folderp->setOrigin( 0, child_top - folderp->getRect().getHeight() );
				}
			}
			for(items_t::iterator iit = mItems.begin();
				iit != mItems.end(); ++iit)
			{
				LLFolderViewItem* itemp = (*iit);
				if (getRoot()->getDebugFilters())
				{
					itemp->setVisible(TRUE);
				}
				else
				{
					itemp->setVisible(itemp->getFiltered(filter_generation));
				}
				if (itemp->getVisible())
				{
					S32 child_width = *width;
					S32 child_height = 0;
					S32 child_top = parent_item_height - ll_round(running_height);
					target_height += itemp->arrange( &child_width, &child_height, filter_generation );
					itemp->reshape( itemp->getRect().getWidth(), child_height);
					running_height += (F32)child_height;
					*width = llmax(*width, child_width);
					itemp->setOrigin( 0, child_top - itemp->getRect().getHeight() );
				}
			}
		}
		mTargetHeight = target_height;
		mLastCalculatedWidth = *width;
	}
	else
	{
		*width = mLastCalculatedWidth;
	}
	if (!(marketplace_top && filtered) && llabs(mCurHeight - mTargetHeight) > 1.f)
	{
		mCurHeight = lerp(mCurHeight, mTargetHeight, LLSmoothInterpolation::getInterpolant(mIsOpen ? FOLDER_OPEN_TIME_CONSTANT : FOLDER_CLOSE_TIME_CONSTANT));
		requestArrange();
		for (folders_t::iterator iter = mFolders.begin();
			iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			if (getRect().getHeight() - (*fit)->getRect().mTop + (*fit)->getItemHeight()
				> ll_round(mCurHeight) + MAX_FOLDER_ITEM_OVERLAP)
			{
				(*fit)->setVisible(FALSE);
			}
		}
		for (items_t::iterator iter = mItems.begin();
			iter != mItems.end();)
		{
			items_t::iterator iit = iter++;
			if (getRect().getHeight() - (*iit)->getRect().mBottom
				> ll_round(mCurHeight) + MAX_FOLDER_ITEM_OVERLAP)
			{
				(*iit)->setVisible(FALSE);
			}
		}
	}
	else
	{
		mCurHeight = mTargetHeight;
	}
	reshape(getRect().getWidth(),ll_round(mCurHeight));
	*height = ll_round(mCurHeight);
	return ll_round(mTargetHeight);
}
BOOL LLFolderViewFolder::needsArrange()
{
	return mLastArrangeGeneration < getRoot()->getArrangeGeneration();
}
void LLFolderViewFolder::requestSort()
{
	mNeedsSort = true;
	requestArrange();
}
void LLFolderViewFolder::setCompletedFilterGeneration(S32 generation, BOOL recurse_up)
{
	mMostFilteredDescendantGeneration = llmin(mMostFilteredDescendantGeneration, generation);
	mCompletedFilterGeneration = generation;
	if (recurse_up
		&& mParentFolder
		&& generation < mParentFolder->getCompletedFilterGeneration())
	{
		mParentFolder->setCompletedFilterGeneration(generation, TRUE);
	}
}
void LLFolderViewFolder::filter( LLInventoryFilter& filter)
{
	S32 filter_generation = filter.getCurrentGeneration();
	S32 must_pass_generation = filter.getFirstRequiredGeneration();
	bool autoopen_folders = (filter.hasFilterString());
	if (getCompletedFilterGeneration() >= filter_generation)
	{
		return;
	}
	if (getLastFilterGeneration() < filter_generation)
	{
		if (getLastFilterGeneration() >= must_pass_generation
			&& !mPassedFilter)
		{
			mLastFilterGeneration = filter_generation;
		}
		else
		{
			filterFolder(filter);
			LLFolderViewItem::filter( filter );
		}
	}
	if (getRoot()->getDebugFilters())
	{
		mStatusText = llformat("%d", mLastFilterGeneration);
		mStatusText += llformat("(%d)", mCompletedFilterGeneration);
		mStatusText += llformat("+%d", mMostFilteredDescendantGeneration);
	}
	if(getCompletedFilterGeneration() >= must_pass_generation && !hasFilteredDescendants(must_pass_generation))
	{
		return;
	}
	if (filter.getFilterCount() < 0)
	{
		return;
	}
	if (filter.isNotDefault()
		&& getFiltered(filter.getFirstSuccessGeneration())
		&&	(mListener
			&& !gInventory.isCategoryComplete(mListener->getUUID())))
	{
		LLInventoryModelBackgroundFetch::instance().start(mListener->getUUID());
	}
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();
		 ++iter)
	{
		LLFolderViewFolder* folder = (*iter);
		if (filter.getFilterCount() < 0)
		{
			break;
		}
		if (folder->getCompletedFilterGeneration() >= filter_generation)
		{
			if (folder->getFiltered() || folder->hasFilteredDescendants(filter.getFirstSuccessGeneration()))
			{
				mMostFilteredDescendantGeneration = filter_generation;
				requestArrange();
			}
			continue;
		}
		folder->filter( filter );
		if (folder->getFiltered() || folder->hasFilteredDescendants(filter_generation))
		{
			mMostFilteredDescendantGeneration = filter_generation;
			requestArrange();
			if (getRoot()->needsAutoSelect() && autoopen_folders)
			{
				folder->setOpenArrangeRecursively(TRUE);
			}
		}
	}
	for (items_t::iterator iter = mItems.begin();
		 iter != mItems.end();
		 ++iter)
	{
		LLFolderViewItem* item = (*iter);
		if (filter.getFilterCount() < 0)
		{
			break;
		}
		if (item->getLastFilterGeneration() >= filter_generation)
		{
			if (item->getFiltered())
			{
				mMostFilteredDescendantGeneration = filter_generation;
				requestArrange();
			}
			continue;
		}
		if (item->getLastFilterGeneration() >= must_pass_generation &&
			!item->getFiltered(must_pass_generation))
		{
			item->setFiltered(FALSE, filter_generation);
			continue;
		}
		item->filter( filter );
		if (item->getFiltered(filter.getFirstSuccessGeneration()))
		{
			mMostFilteredDescendantGeneration = filter_generation;
			requestArrange();
		}
	}
	if (filter.getFilterCount() > 0)
	{
		setCompletedFilterGeneration(filter_generation, FALSE);
	}
}
void LLFolderViewFolder::filterFolder(LLInventoryFilter& filter)
{
	const BOOL previous_passed_filter = mPassedFolderFilter;
	const BOOL passed_filter = filter.checkFolder(this);
	if (mParentFolder)
	{
		if (getVisible() != passed_filter
			|| previous_passed_filter != passed_filter )
		{
			mParentFolder->requestArrange();
		}
	}
	setFilteredFolder(passed_filter, filter.getCurrentGeneration());
	filter.decrementFilterCount();
	if (getRoot()->getDebugFilters())
	{
		mStatusText = llformat("%d", mLastFilterGeneration);
	}
}
void LLFolderViewFolder::setFiltered(BOOL filtered, S32 filter_generation)
{
	if (filtered && !mPassedFilter)
	{
		mCurHeight = 0.f;
	}
	LLFolderViewItem::setFiltered(filtered, filter_generation);
}
void LLFolderViewFolder::dirtyFilter()
{
	setCompletedFilterGeneration(-1, FALSE);
	for (auto folder : mFolders)
		folder->dirtyFilter();
	LLFolderViewItem::dirtyFilter();
}
BOOL LLFolderViewFolder::getFiltered()
{
	return getFilteredFolder(getRoot()->getFilter().getFirstSuccessGeneration())
		&& LLFolderViewItem::getFiltered();
}
BOOL LLFolderViewFolder::getFiltered(S32 filter_generation)
{
	return getFilteredFolder(filter_generation) && LLFolderViewItem::getFiltered(filter_generation);
}
BOOL LLFolderViewFolder::hasFilteredDescendants(S32 filter_generation)
{
	return mMostFilteredDescendantGeneration >= filter_generation;
}
BOOL LLFolderViewFolder::hasFilteredDescendants()
{
	return mMostFilteredDescendantGeneration >= getRoot()->getFilter().getCurrentGeneration();
}
BOOL LLFolderViewFolder::setSelection(LLFolderViewItem* selection, BOOL openitem,
                                      BOOL take_keyboard_focus)
{
	BOOL rv = FALSE;
	if (selection == this)
	{
		if (!isSelected())
		{
			selectItem();
		}
		rv = TRUE;
	}
	else
	{
		if (isSelected())
		{
			deselectItem();
		}
		rv = FALSE;
	}
	BOOL child_selected = FALSE;
	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		if((*fit)->setSelection(selection, openitem, take_keyboard_focus))
		{
			rv = TRUE;
			child_selected = TRUE;
		}
	}
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		if((*iit)->setSelection(selection, openitem, take_keyboard_focus))
		{
			rv = TRUE;
			child_selected = TRUE;
		}
	}
	if(openitem && child_selected)
	{
		setOpenArrangeRecursively(TRUE);
	}
	return rv;
}
BOOL LLFolderViewFolder::changeSelection(LLFolderViewItem* selection, BOOL selected)
{
	BOOL rv = FALSE;
	if(selection == this)
	{
		if (isSelected() != selected)
		{
			rv = TRUE;
			if (selected)
			{
				selectItem();
			}
			else
			{
				deselectItem();
			}
		}
	}
	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		if((*fit)->changeSelection(selection, selected))
		{
			rv = TRUE;
		}
	}
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		if((*iit)->changeSelection(selection, selected))
		{
			rv = TRUE;
		}
	}
	return rv;
}
LLFolderViewFolder* LLFolderViewFolder::getCommonAncestor(LLFolderViewItem* item_a, LLFolderViewItem* item_b, bool& reverse)
{
	if (!item_a->getParentFolder() || !item_b->getParentFolder()) return NULL;
	std::deque<LLFolderViewFolder*> item_a_ancestors;
	LLFolderViewFolder* parent = item_a->getParentFolder();
	while(parent)
	{
		item_a_ancestors.push_back(parent);
		parent = parent->getParentFolder();
	}
	std::deque<LLFolderViewFolder*> item_b_ancestors;
	parent = item_b->getParentFolder();
	while(parent)
	{
		item_b_ancestors.push_back(parent);
		parent = parent->getParentFolder();
	}
	LLFolderViewFolder* common_ancestor = item_a->getRoot();
	while(item_a_ancestors.size() > item_b_ancestors.size())
	{
		item_a = item_a_ancestors.front();
		item_a_ancestors.pop_front();
	}
	while(item_b_ancestors.size() > item_a_ancestors.size())
	{
		item_b = item_b_ancestors.front();
		item_b_ancestors.pop_front();
	}
	while(item_a_ancestors.size())
	{
		common_ancestor = item_a_ancestors.front();
		if (item_a_ancestors.front() == item_b_ancestors.front())
		{
			for (folders_t::iterator it = common_ancestor->mFolders.begin(), end_it = common_ancestor->mFolders.end();
				it != end_it;
				++it)
			{
				LLFolderViewItem* item = *it;
				if (item == item_a)
				{
					reverse = false;
					return common_ancestor;
				}
				if (item == item_b)
				{
					reverse = true;
					return common_ancestor;
				}
			}
			for (items_t::iterator it = common_ancestor->mItems.begin(), end_it = common_ancestor->mItems.end();
				it != end_it;
				++it)
			{
				LLFolderViewItem* item = *it;
				if (item == item_a)
				{
					reverse = false;
					return common_ancestor;
				}
				if (item == item_b)
				{
					reverse = true;
					return common_ancestor;
				}
			}
			break;
		}
		item_a = item_a_ancestors.front();
		item_a_ancestors.pop_front();
		item_b = item_b_ancestors.front();
		item_b_ancestors.pop_front();
	}
	return NULL;
}
void LLFolderViewFolder::gatherChildRangeExclusive(LLFolderViewItem* start, LLFolderViewItem* end, bool reverse, std::vector<LLFolderViewItem*>& items)
{
	bool selecting = start == NULL;
	if (reverse)
	{
		for (items_t::reverse_iterator it = mItems.rbegin(), end_it = mItems.rend();
			it != end_it;
			++it)
		{
			if (*it == end)
			{
				return;
			}
			if (selecting && (*it)->getVisible())
			{
				items.push_back(*it);
			}
			if (*it == start)
			{
				selecting = true;
			}
		}
		for (folders_t::reverse_iterator it = mFolders.rbegin(), end_it = mFolders.rend();
			it != end_it;
			++it)
		{
			if (*it == end)
			{
				return;
			}
			if (selecting && (*it)->getVisible())
			{
				items.push_back(*it);
			}
			if (*it == start)
			{
				selecting = true;
			}
		}
	}
	else
	{
		for (folders_t::iterator it = mFolders.begin(), end_it = mFolders.end();
			it != end_it;
			++it)
		{
			if (*it == end)
			{
				return;
			}
			if (selecting && (*it)->getVisible())
			{
				items.push_back(*it);
			}
			if (*it == start)
			{
				selecting = true;
			}
		}
		for (items_t::iterator it = mItems.begin(), end_it = mItems.end();
			it != end_it;
			++it)
		{
			if (*it == end)
			{
				return;
			}
			if (selecting && (*it)->getVisible())
			{
				items.push_back(*it);
			}
			if (*it == start)
			{
				selecting = true;
			}
		}
	}
}
void LLFolderViewFolder::extendSelectionTo(LLFolderViewItem* new_selection)
{
	if (getRoot()->getAllowMultiSelect() == FALSE) return;
	LLFolderViewItem* cur_selected_item = getRoot()->getCurSelectedItem();
	if (cur_selected_item == NULL)
	{
		cur_selected_item = new_selection;
	}
	bool reverse = false;
	LLFolderViewFolder* common_ancestor = getCommonAncestor(cur_selected_item, new_selection, reverse);
	if (!common_ancestor) return;
	LLFolderViewItem* last_selected_item_from_cur = cur_selected_item;
	LLFolderViewFolder* cur_folder = cur_selected_item->getParentFolder();
	std::vector<LLFolderViewItem*> items_to_select_forward;
	while(cur_folder != common_ancestor)
	{
		cur_folder->gatherChildRangeExclusive(last_selected_item_from_cur, NULL, reverse, items_to_select_forward);
		last_selected_item_from_cur = cur_folder;
		cur_folder = cur_folder->getParentFolder();
	}
	std::vector<LLFolderViewItem*> items_to_select_reverse;
	LLFolderViewItem* last_selected_item_from_new = new_selection;
	cur_folder = new_selection->getParentFolder();
	while(cur_folder != common_ancestor)
	{
		cur_folder->gatherChildRangeExclusive(last_selected_item_from_new, NULL, !reverse, items_to_select_reverse);
		last_selected_item_from_new = cur_folder;
		cur_folder = cur_folder->getParentFolder();
	}
	common_ancestor->gatherChildRangeExclusive(last_selected_item_from_cur, last_selected_item_from_new, reverse, items_to_select_forward);
	for (std::vector<LLFolderViewItem*>::reverse_iterator it = items_to_select_reverse.rbegin(), end_it = items_to_select_reverse.rend();
		it != end_it;
		++it)
	{
		items_to_select_forward.push_back(*it);
	}
	LLFolderView* root = getRoot();
	for (std::vector<LLFolderViewItem*>::iterator it = items_to_select_forward.begin(), end_it = items_to_select_forward.end();
		it != end_it;
		++it)
	{
		LLFolderViewItem* item = *it;
		if (item->isSelected())
		{
			root->removeFromSelectionList(item);
		}
		else
		{
			item->selectItem();
		}
		root->addToSelectionList(item);
	}
	if (new_selection->isSelected())
	{
		root->removeFromSelectionList(new_selection);
	}
	else
	{
		new_selection->selectItem();
	}
	root->addToSelectionList(new_selection);
}
void LLFolderViewFolder::destroyView()
{
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		LLFolderViewItem* item = (*iit);
		getRoot()->removeItemID(item->getListener()->getUUID());
	}
	std::for_each(mItems.begin(), mItems.end(), DeletePointer());
	mItems.clear();
	while (!mFolders.empty())
	{
		LLFolderViewFolder *folderp = mFolders.back();
		mFolders.pop_back();
		folderp->destroyView();
	}
	if (mParentFolder)
	{
		mParentFolder->removeView(this);
	}
}
BOOL LLFolderViewFolder::removeItem(LLFolderViewItem* item)
{
	if(item->remove())
	{
		return TRUE;
	}
	return FALSE;
}
void LLFolderViewFolder::removeView(LLFolderViewItem* item)
{
	if (!item || item->getParentFolder() != this)
	{
		return;
	}
	if (item->isSelected())
	{
		item->deselectItem();
	}
	getRoot()->removeFromSelectionList(item);
	extractItem(item);
	delete item;
}
void LLFolderViewFolder::extractItem( LLFolderViewItem* item )
{
	items_t::iterator it = std::find(mItems.begin(), mItems.end(), item);
	if(it == mItems.end())
	{
		LLFolderViewFolder* f = static_cast<LLFolderViewFolder*>(item);
		folders_t::iterator ft;
		ft = std::find(mFolders.begin(), mFolders.end(), f);
		if (ft != mFolders.end())
		{
			mFolders.erase(ft);
		}
	}
	else
	{
		mItems.erase(it);
	}
	dirtyFilter();
	requestArrange();
	getRoot()->removeItemID(item->getListener()->getUUID());
	removeChild(item);
}
bool LLFolderViewFolder::isTrash() const
{
	if (mAmTrash == LLFolderViewFolder::UNKNOWN)
	{
		mAmTrash = mListener->getUUID() == gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH, false) ? LLFolderViewFolder::TRASH : LLFolderViewFolder::NOT_TRASH;
	}
	return mAmTrash == LLFolderViewFolder::TRASH;
}
void LLFolderViewFolder::sortBy(U32 order)
{
	if (!mSortFunction.updateSort(order))
	{
		return;
	}
	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		(*fit)->sortBy(order);
	}
	if (mListener->getUUID().notNull())
	{
		mFolders.sort(mSortFunction);
		mItems.sort(mSortFunction);
	}
	if (order & LLInventoryFilter::SO_DATE)
	{
		time_t latest = 0;
		if (!mItems.empty())
		{
			LLFolderViewItem* item = *(mItems.begin());
			latest = item->getCreationDate();
		}
		if (!mFolders.empty())
		{
			LLFolderViewFolder* folder = *(mFolders.begin());
			if (folder->getCreationDate() > latest)
			{
				latest = folder->getCreationDate();
			}
		}
		mSubtreeCreationDate = latest;
	}
}
void LLFolderViewFolder::setItemSortOrder(U32 ordering)
{
	if (mSortFunction.updateSort(ordering))
	{
		for (folders_t::iterator iter = mFolders.begin();
			iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			(*fit)->setItemSortOrder(ordering);
		}
		mFolders.sort(mSortFunction);
		mItems.sort(mSortFunction);
	}
}
EInventorySortGroup LLFolderViewFolder::getSortGroup() const
{
	if (isTrash())
	{
		return SG_TRASH_FOLDER;
	}
	if( mListener )
	{
		if(LLFolderType::lookupIsProtectedType(mListener->getPreferredType()))
		{
			return SG_SYSTEM_FOLDER;
		}
	}
	return SG_NORMAL_FOLDER;
}
BOOL LLFolderViewFolder::isMovable()
{
	if( mListener )
	{
		if( !(mListener->isItemMovable()) )
		{
			return FALSE;
		}
		for (items_t::iterator iter = mItems.begin();
			iter != mItems.end();)
		{
			items_t::iterator iit = iter++;
			if(!(*iit)->isMovable())
			{
				return FALSE;
			}
		}
		for (folders_t::iterator iter = mFolders.begin();
			iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			if(!(*fit)->isMovable())
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}
BOOL LLFolderViewFolder::isRemovable()
{
	if( mListener )
	{
		if( !(mListener->isItemRemovable()) )
		{
			return FALSE;
		}
		for (items_t::iterator iter = mItems.begin();
			iter != mItems.end();)
		{
			items_t::iterator iit = iter++;
			if(!(*iit)->isRemovable())
			{
				return FALSE;
			}
		}
		for (folders_t::iterator iter = mFolders.begin();
			iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			if(!(*fit)->isRemovable())
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}
BOOL LLFolderViewFolder::addItem(LLFolderViewItem* item)
{
	mItems.push_back(item);
	item->setRect(LLRect(0, 0, getRect().getWidth(), 0));
	item->setVisible(FALSE);
	addChild(item);
	item->dirtyFilter();
	bool setting_date = false;
	const time_t item_creation_date = item->getCreationDate();
	if ((item_creation_date > 0) && (mCreationDate == 0))
	{
		setCreationDate(item_creation_date);
		setting_date = true;
	}
	requestArrange();
	requestSort();
	LLFolderViewFolder* parentp = getParentFolder();
	while (parentp)
	{
		if (setting_date && (parentp->mCreationDate == 0))
		{
			parentp->setCreationDate(item_creation_date);
		}
		if (parentp->mSortFunction.isByDate())
		{
			parentp->requestSort();
		}
		parentp = parentp->getParentFolder();
	}
	return TRUE;
}
BOOL LLFolderViewFolder::addFolder(LLFolderViewFolder* folder)
{
	mFolders.push_back(folder);
	folder->setOrigin(0, 0);
	folder->reshape(getRect().getWidth(), 0);
	folder->setVisible(FALSE);
	addChild( folder );
	folder->dirtyFilter();
	folder->requestArrange(TRUE);
	requestSort();
	LLFolderViewFolder* parentp = getParentFolder();
	while (parentp && !parentp->mSortFunction.isByDate())
	{
		parentp->requestSort();
		parentp = parentp->getParentFolder();
	}
	return TRUE;
}
void LLFolderViewFolder::requestArrange(BOOL include_descendants)
{
	mLastArrangeGeneration = -1;
	if (mParentFolder)
	{
		mParentFolder->requestArrange();
	}
	if (include_descendants)
	{
		for (folders_t::iterator iter = mFolders.begin();
			iter != mFolders.end();
			++iter)
		{
			(*iter)->requestArrange(TRUE);
		}
	}
}
void LLFolderViewFolder::toggleOpen()
{
	setOpen(!mIsOpen);
}
void LLFolderViewFolder::setOpen(BOOL openitem)
{
	setOpenArrangeRecursively(openitem);
}
void LLFolderViewFolder::setOpenArrangeRecursively(BOOL openitem, ERecurseType recurse)
{
	BOOL was_open = mIsOpen;
	mIsOpen = openitem;
	if (mListener)
	{
		if(!was_open && openitem)
		{
			mListener->openItem();
		}
		else if(was_open && !openitem)
		{
			mListener->closeItem();
		}
	}
	if (recurse == RECURSE_DOWN || recurse == RECURSE_UP_DOWN)
	{
		for (folders_t::iterator iter = mFolders.begin();
			iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			(*fit)->setOpenArrangeRecursively(openitem, RECURSE_DOWN);
		}
	}
	if (mParentFolder
		&&	(recurse == RECURSE_UP
			|| recurse == RECURSE_UP_DOWN))
	{
		mParentFolder->setOpenArrangeRecursively(openitem, RECURSE_UP);
	}
	if (was_open != mIsOpen)
	{
		requestArrange();
	}
}
BOOL LLFolderViewFolder::handleDragAndDropFromChild(MASK mask,
													BOOL drop,
													EDragAndDropType c_type,
													void* cargo_data,
													EAcceptance* accept,
													std::string& tooltip_msg)
{
	BOOL accepted = mListener && mListener->dragOrDrop(mask,drop,c_type,cargo_data);
	if (accepted)
	{
		mDragAndDropTarget = TRUE;
		*accept = ACCEPT_YES_MULTI;
	}
	else
	{
		*accept = ACCEPT_NO;
	}
	getRoot()->autoOpenTest(NULL);
	return TRUE;
}
void LLFolderViewFolder::openItem( void )
{
	toggleOpen();
}
void LLFolderViewFolder::applyFunctorToChildren(LLFolderViewFunctor& functor)
{
	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		functor.doItem((*fit));
	}
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		functor.doItem((*iit));
	}
}
void LLFolderViewFolder::applyFunctorRecursively(LLFolderViewFunctor& functor)
{
	functor.doFolder(this);
	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		(*fit)->applyFunctorRecursively(functor);
	}
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		functor.doItem((*iit));
	}
}
void LLFolderViewFolder::applyListenerFunctorRecursively(LLFolderViewListenerFunctor& functor)
{
	functor(mListener);
	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		(*fit)->applyListenerFunctorRecursively(functor);
	}
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		(*iit)->applyListenerFunctorRecursively(functor);
	}
}
BOOL LLFolderViewFolder::handleDragAndDrop(S32 x, S32 y, MASK mask,
										   BOOL drop,
										   EDragAndDropType cargo_type,
										   void* cargo_data,
										   EAcceptance* accept,
										   std::string& tooltip_msg)
{
	BOOL handled = FALSE;
	if (mIsOpen)
	{
		handled = (childrenHandleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg) != NULL);
	}
	if (!handled)
	{
		handleDragAndDropToThisFolder(mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
		LL_DEBUGS("UserInput") << "dragAndDrop handled by LLFolderViewFolder" << LL_ENDL;
	}
	return TRUE;
}
BOOL LLFolderViewFolder::handleDragAndDropToThisFolder(MASK mask,
													   BOOL drop,
													   EDragAndDropType cargo_type,
													   void* cargo_data,
													   EAcceptance* accept,
													   std::string& tooltip_msg)
{
	if (!mAllowDrop)
	{
		*accept = ACCEPT_NO;
		tooltip_msg = LLTrans::getString("TooltipOutboxCannotDropOnRoot");
		return TRUE;
	}
	BOOL accepted = mListener && mListener->dragOrDrop(mask, drop, cargo_type, cargo_data);
	if (accepted)
	{
		mDragAndDropTarget = TRUE;
		*accept = ACCEPT_YES_MULTI;
	}
	else
	{
		*accept = ACCEPT_NO;
	}
	if (!drop && accepted)
	{
		getRoot()->autoOpenTest(this);
	}
	return TRUE;
}
BOOL LLFolderViewFolder::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	gInventory.fetchDescendentsOf(mListener->getUUID());
	if( mIsOpen )
	{
		handled = childrenHandleRightMouseDown( x, y, mask ) != NULL;
	}
	if (!handled)
	{
		handled = LLFolderViewItem::handleRightMouseDown( x, y, mask );
	}
	return handled;
}
BOOL LLFolderViewFolder::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLView::handleHover(x, y, mask);
	if (!handled)
	{
		handled = LLFolderViewItem::handleHover(x, y, mask);
	}
	return handled;
}
BOOL LLFolderViewFolder::handleMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	if( mIsOpen )
	{
		handled = childrenHandleMouseDown(x,y,mask) != NULL;
	}
	if( !handled )
	{
		if(mIndentation < x && x < mIndentation + ARROW_SIZE + TEXT_PAD)
		{
			toggleOpen();
			handled = TRUE;
		}
		else
		{
			handled = LLFolderViewItem::handleMouseDown(x, y, mask);
		}
	}
	return handled;
}
BOOL LLFolderViewFolder::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	if( mIsOpen )
	{
		handled = childrenHandleDoubleClick( x, y, mask ) != NULL;
	}
	if( !handled )
	{
		if(mIndentation < x && x < mIndentation + ARROW_SIZE + TEXT_PAD)
		{
			toggleOpen();
		}
		else
		{
			setSelectionFromRoot(this, FALSE);
			toggleOpen();
		}
		handled = TRUE;
	}
	return handled;
}
void LLFolderViewFolder::draw()
{
	if (mAutoOpenCountdown != 0.f)
	{
		mControlLabelRotation = mAutoOpenCountdown * -90.f;
	}
	else if (mIsOpen)
	{
		mControlLabelRotation = lerp(mControlLabelRotation, -90.f, LLSmoothInterpolation::getInterpolant(0.04f));
	}
	else
	{
		mControlLabelRotation = lerp(mControlLabelRotation, 0.f, LLSmoothInterpolation::getInterpolant(0.025f));
	}
	bool possibly_has_children = false;
	bool up_to_date = mListener && mListener->isUpToDate();
	if(!up_to_date
		&& mListener
		&& mListener->hasChildren())
	{
		possibly_has_children = true;
	}
	BOOL loading = (mIsOpen
					&& possibly_has_children
					&& !up_to_date );
	if ( loading && !mIsLoading )
	{
		mTimeSinceRequestStart.reset();
	}
	mIsLoading = loading;
	LLFolderViewItem::draw();
	if( getRoot() == this || (mIsOpen || mCurHeight != mTargetHeight ))
	{
		LLView::draw();
	}
	mExpanderHighlighted = FALSE;
}
time_t LLFolderViewFolder::getCreationDate() const
{
	return llmax<time_t>(mCreationDate, mSubtreeCreationDate);
}
BOOL	LLFolderViewFolder::potentiallyVisible()
{
	return LLFolderViewItem::potentiallyVisible()
		|| hasFilteredDescendants(getRoot()->getFilter().getFirstSuccessGeneration())
		|| getCompletedFilterGeneration() < getRoot()->getFilter().getFirstSuccessGeneration();
}
LLFolderViewItem* LLFolderViewFolder::getNextFromChild( LLFolderViewItem* item, BOOL include_children )
{
	BOOL found_item = FALSE;
	LLFolderViewItem* result = NULL;
	if(item == NULL)
	{
		found_item = TRUE;
	}
	folders_t::iterator fit = mFolders.begin();
	folders_t::iterator fend = mFolders.end();
	items_t::iterator iit = mItems.begin();
	items_t::iterator iend = mItems.end();
	if (!found_item)
	{
		for(; fit != fend; ++fit)
		{
			if(item == (*fit))
			{
				found_item = TRUE;
				if (include_children && (*fit)->isOpen())
				{
					return (*fit)->getNextFromChild(NULL, TRUE);
				}
				++fit;
				include_children = TRUE;
				break;
			}
		}
		if (!found_item)
		{
			for(; iit != iend; ++iit)
			{
				if(item == (*iit))
				{
					found_item = TRUE;
					++iit;
					break;
				}
			}
		}
	}
	if (!found_item)
	{
		llassert(FALSE);
		return NULL;
	}
	while(fit != fend && !(*fit)->getVisible())
	{
		++fit;
	}
	if (fit != fend)
	{
		result = (*fit);
	}
	else
	{
		while(iit != iend && !(*iit)->getVisible())
		{
			++iit;
		}
		if (iit != iend)
		{
			result = (*iit);
		}
	}
	if( !result && mParentFolder )
	{
		result = mParentFolder->getNextFromChild(this, FALSE);
	}
	return result;
}
LLFolderViewItem* LLFolderViewFolder::getPreviousFromChild( LLFolderViewItem* item, BOOL include_children )
{
	BOOL found_item = FALSE;
	LLFolderViewItem* result = NULL;
	if(item == NULL)
	{
		found_item = TRUE;
	}
	folders_t::reverse_iterator fit = mFolders.rbegin();
	folders_t::reverse_iterator fend = mFolders.rend();
	items_t::reverse_iterator iit = mItems.rbegin();
	items_t::reverse_iterator iend = mItems.rend();
	if (!found_item)
	{
		for(; iit != iend; ++iit)
		{
			if(item == (*iit))
			{
				found_item = TRUE;
				++iit;
				break;
			}
		}
		if (!found_item)
		{
			for(; fit != fend; ++fit)
			{
				if(item == (*fit))
				{
					found_item = TRUE;
					++fit;
					break;
				}
			}
		}
	}
	if (!found_item)
	{
		llassert(FALSE);
		return NULL;
	}
	while(iit != iend && !(*iit)->getVisible())
	{
		++iit;
	}
	if (iit != iend)
	{
		result = (*iit);
	}
	else
	{
		while(fit != fend && !(*fit)->getVisible())
		{
			++fit;
		}
		if (fit != fend)
		{
			if ((*fit)->isOpen())
			{
				result = (*fit)->getPreviousFromChild(NULL);
			}
			else
			{
				result = (*fit);
			}
		}
	}
	if( !result )
	{
		result = this;
	}
	return result;
}
bool LLInventorySort::updateSort(U32 order)
{
	if (order != mSortOrder)
	{
		mSortOrder = order;
		mByDate = (order & LLInventoryFilter::SO_DATE);
		mSystemToTop = (order & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP);
		mFoldersByName = (order & LLInventoryFilter::SO_FOLDERS_BY_NAME);
		mFoldersByWeight = (mSortOrder & LLInventoryFilter::SO_FOLDERS_BY_WEIGHT);
		return true;
	}
	return false;
}
bool LLInventorySort::operator()(const LLFolderViewItem* const& a, const LLFolderViewItem* const& b)
{
	bool by_name = ((!mByDate || (mFoldersByName && (a->getSortGroup() != SG_ITEM))) && !mFoldersByWeight);
	if (a->getSortGroup() != b->getSortGroup())
	{
		if (mSystemToTop)
		{
			return (a->getSortGroup() < b->getSortGroup());
		}
		else if (mByDate)
		{
			if ( (a->getSortGroup() == SG_TRASH_FOLDER)
				|| (b->getSortGroup() == SG_TRASH_FOLDER))
			{
				return (b->getSortGroup() == SG_TRASH_FOLDER);
			}
		}
	}
	if (by_name)
	{
		S32 compare = LLStringUtil::compareDict(a->getLabel(), b->getLabel());
		if (0 == compare)
		{
			return (a->getCreationDate() > b->getCreationDate());
		}
		else
		{
			return (compare < 0);
		}
	}
	else if (mFoldersByWeight)
	{
		S32 weight_a = compute_stock_count(a->getListener()->getUUID());
		S32 weight_b = compute_stock_count(b->getListener()->getUUID());
		if (weight_a == weight_b)
		{
			return (LLStringUtil::compareDict(a->getListener()->getDisplayName(), b->getListener()->getDisplayName()) < 0);
		}
		else if (weight_a == COMPUTE_STOCK_INFINITE)
		{
			return false;
		}
		else if (weight_b == COMPUTE_STOCK_INFINITE)
		{
			return true;
		}
		else
		{
			return (weight_a < weight_b);
		}
	}
	else
	{
		time_t first_create = a->getCreationDate();
		time_t second_create = b->getCreationDate();
		if (first_create == second_create)
		{
			return (LLStringUtil::compareDict(a->getLabel(), b->getLabel()) < 0);
		}
		else
		{
			return (first_create > second_create);
		}
	}
}
