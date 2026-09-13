/** 
* @file llfolderviewitem.h
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
#ifndef LLFOLDERVIEWITEM_H
#define LLFOLDERVIEWITEM_H
#include "llview.h"
#include "lluiimage.h"
#include "lluictrl.h"
#include <boost/unordered_set.hpp>
class LLFontGL;
class LLFolderView;
class LLFolderViewEventListener;
class LLFolderViewFolder;
class LLFolderViewFunctor;
class LLFolderViewItem;
class LLFolderViewListenerFunctor;
class LLInventoryFilter;
class LLMenuGL;
class LLUIImage;
class LLViewerInventoryItem;
enum EInventorySortGroup
{
	SG_SYSTEM_FOLDER,
	SG_TRASH_FOLDER,
	SG_NORMAL_FOLDER,
	SG_ITEM
};
class LLInventorySort
{
public:
	LLInventorySort()
		: mSortOrder(0),
		mByDate(false),
		mSystemToTop(false),
		mFoldersByName(false),
		mFoldersByWeight(false) { }
	bool updateSort(U32 order);
	U32 getSort() { return mSortOrder; }
	bool isByDate() { return mByDate; }
	bool operator()(const LLFolderViewItem* const& a, const LLFolderViewItem* const& b);
private:
	U32  mSortOrder;
	bool mByDate;
	bool mSystemToTop;
	bool mFoldersByName;
	bool mFoldersByWeight;
};
class LLFolderViewItem : public LLUICtrl
{
public:
	static void initClass();
	static void cleanupClass();
	friend class LLFolderViewEventListener;
	static const S32 LEFT_PAD = 5;
	static const S32 LEFT_INDENTATION = 6;
	static const S32 ICON_PAD = 2;
	static const S32 ICON_WIDTH = 16;
	static const S32 TEXT_PAD = 1;
	static const S32 ARROW_SIZE = 12;
	static const S32 MAX_FOLDER_ITEM_OVERLAP = 2;
	static const F32 FOLDER_CLOSE_TIME_CONSTANT;
	static const F32 FOLDER_OPEN_TIME_CONSTANT;
	BOOL isLoading() const { return mIsLoading; }
private:
	BOOL					mIsSelected;
protected:
	static LLUIImagePtr			sArrowImage;
	static LLUIImagePtr			sBoxImage;
	std::string					mLabel;
	std::string					mSearchableLabel;
	std::string					mSearchableLabelDesc;
	std::string					mSearchableLabelCreator;
	S32							mLabelWidth;
	bool						mLabelWidthDirty;
	time_t						mCreationDate;
	LLFolderViewFolder*			mParentFolder;
	LLFolderViewEventListener*	mListener;
	BOOL						mIsCurSelection;
	BOOL						mSelectPending;
	LLFontGL::StyleFlags		mLabelStyle;
	std::string					mLabelSuffix;
	LLUIImagePtr				mIcon;
	std::string					mStatusText;
	LLUIImagePtr				mIconOpen;
	LLUIImagePtr				mIconOverlay;
	BOOL						mHasVisibleChildren;
	S32							mIndentation;
	S32							mItemHeight;
	BOOL						mPassedFilter;
	S32							mLastFilterGeneration;
	std::string::size_type		mStringMatchOffset;
	F32							mControlLabelRotation;
	LLFolderView*				mRoot;
	BOOL						mDragAndDropTarget;
	BOOL                            mIsLoading;
	LLTimer                         mTimeSinceRequestStart;
	bool						mShowLoadStatus;
	bool						mAllowDrop;
	bool						mAllowWear;
	std::string					mSearchable;
	U32							mSearchType;
	void updateExtraSearchCriteria();
	void updateSearchLabelType();
	virtual BOOL addItem(LLFolderViewItem*) { return FALSE; }
	virtual BOOL addFolder(LLFolderViewFolder*) { return FALSE; }
	static LLFontGL* getLabelFontForStyle(U8 style);
	virtual void setCreationDate(time_t creation_date_utc)	{ mCreationDate = creation_date_utc; }
public:
	BOOL postBuild();
	void setSelectionFromRoot(LLFolderViewItem* selection, BOOL openitem,
		BOOL take_keyboard_focus = TRUE);
	void arrangeFromRoot();
	void filterFromRoot( void );
	void arrangeAndSet(BOOL set_selection, BOOL take_keyboard_focus);
	LLFolderViewItem( const std::string& name, LLUIImagePtr icon, LLUIImagePtr icon_open, LLUIImagePtr icon_overlay, S32 creation_date, LLFolderView* root, LLFolderViewEventListener* listener );
	virtual ~LLFolderViewItem( void );
	enum { ARRANGE = TRUE, DO_NOT_ARRANGE = FALSE };
	virtual BOOL addToFolder(LLFolderViewFolder* folder, LLFolderView* root);
	virtual EInventorySortGroup getSortGroup() const;
	virtual S32 arrange( S32* width, S32* height, S32 filter_generation );
	virtual S32 getItemHeight();
	virtual void filter( LLInventoryFilter& filter);
	S32		getLastFilterGeneration() { return mLastFilterGeneration; }
	virtual void	dirtyFilter();
	virtual BOOL setSelection(LLFolderViewItem* selection, BOOL openitem, BOOL take_keyboard_focus);
	virtual BOOL changeSelection(LLFolderViewItem* selection, BOOL selected);
	void deselectItem();
	virtual void selectItem();
	virtual uuid_set_t getSelectionList() const;
	virtual BOOL isRemovable();
	virtual BOOL isMovable();
	virtual void destroyView();
	BOOL isSelected() const { return mIsSelected; }
	void setUnselected() { mIsSelected = FALSE; }
	void setIsCurSelection(BOOL select) { mIsCurSelection = select; }
	BOOL getIsCurSelection() { return mIsCurSelection; }
	BOOL hasVisibleChildren() { return mHasVisibleChildren; }
	void setShowLoadStatus(bool status) { mShowLoadStatus = status; }
	void setAllowDrop(bool allow) { mAllowDrop = allow; }
	void setAllowWear(bool allow) { mAllowWear = allow; }
	BOOL remove();
	void buildContextMenu(class LLMenuGL& menu, U32 flags);
	const std::string& getName( void ) const;
	const std::string& getSearchableLabel( void );
	const std::string& getLabel() const { return mLabel; }
	virtual time_t getCreationDate() const { return mCreationDate; }
	LLFolderViewFolder* getParentFolder( void ) { return mParentFolder; }
	const LLFolderViewFolder* getParentFolder( void ) const { return mParentFolder; }
	LLFolderViewItem* getNextOpenNode( BOOL include_children = TRUE );
	LLFolderViewItem* getPreviousOpenNode( BOOL include_children = TRUE );
	const LLFolderViewEventListener* getListener( void ) const { return mListener; }
	LLFolderViewEventListener* getListener( void ) { return mListener; }
	LLViewerInventoryItem * getInventoryItem(void);
	void rename(const std::string& new_name);
	void nameOrDescriptionChanged(void) const;
	virtual void openItem( void );
	virtual void preview(void);
	virtual void setOpen(BOOL open = TRUE) {};
	virtual BOOL isOpen() const { return FALSE; }
	virtual LLFolderView*	getRoot();
	virtual const LLFolderView*	getRoot() const;
	BOOL			isDescendantOf( const LLFolderViewFolder* potential_ancestor );
	S32				getIndentation() { return mIndentation; }
	virtual BOOL	potentiallyVisible();
	virtual BOOL	getFiltered();
	virtual BOOL	getFiltered(S32 filter_generation);
	virtual void	setFiltered(BOOL filtered, S32 filter_generation);
	void setIcon(LLUIImagePtr icon);
	void refreshFromListener();
	virtual void refresh();
	virtual void applyListenerFunctorRecursively(LLFolderViewListenerFunctor& functor);
	virtual BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleHover( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseUp( S32 x, S32 y, MASK mask );
	virtual BOOL handleDoubleClick( S32 x, S32 y, MASK mask );
	virtual BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual LLView* getChildView(const std::string& name, BOOL recurse, BOOL create_if_missing) const
	{
		if(create_if_missing)
			return LLView::getChildView(name, recurse, TRUE);
		else
			return NULL;
	}
	virtual void draw();
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		EAcceptance* accept,
		std::string& tooltip_msg);
private:
	static std::map<U8, LLFontGL*> sFonts;
};
typedef bool (*sort_order_f)(LLFolderViewItem* a, LLFolderViewItem* b);
class LLFolderViewFolder : public LLFolderViewItem
{
protected:
	LLFolderViewFolder( const std::string& name, LLUIImagePtr icon,
						LLUIImagePtr icon_open,
						LLUIImagePtr icon_link,
						LLFolderView* root,
						LLFolderViewEventListener* listener);
	friend class LLBuildNewViewsScheduler;
	friend class LLPanelObjectInventory;
	friend class LLInventoryPanel;
public:
	typedef enum e_trash
	{
		UNKNOWN, TRASH, NOT_TRASH
	} ETrash;
	typedef std::list<LLFolderViewItem*> items_t;
	typedef std::list<LLFolderViewFolder*> folders_t;
protected:
	items_t mItems;
	folders_t mFolders;
	LLInventorySort	mSortFunction;
	BOOL		mIsOpen;
	BOOL		mExpanderHighlighted;
	F32			mCurHeight;
	F32			mTargetHeight;
	F32			mAutoOpenCountdown;
	time_t		mSubtreeCreationDate;
	mutable ETrash mAmTrash;
	S32			mLastArrangeGeneration;
	S32			mLastCalculatedWidth;
	S32			mCompletedFilterGeneration;
	S32			mMostFilteredDescendantGeneration;
	bool		mNeedsSort;
	bool		mPassedFolderFilter;
public:
	typedef enum e_recurse_type
	{
		RECURSE_NO,
		RECURSE_UP,
		RECURSE_DOWN,
		RECURSE_UP_DOWN
	} ERecurseType;
	virtual ~LLFolderViewFolder( void );
	virtual BOOL	potentiallyVisible();
	LLFolderViewItem* getNextFromChild( LLFolderViewItem*, BOOL include_children = TRUE );
	LLFolderViewItem* getPreviousFromChild( LLFolderViewItem*, BOOL include_children = TRUE  );
	virtual BOOL addToFolder(LLFolderViewFolder* folder, LLFolderView* root);
	virtual S32 arrange( S32* width, S32* height, S32 filter_generation );
	BOOL needsArrange();
	void requestSort();
	virtual EInventorySortGroup getSortGroup() const;
	virtual void	setCompletedFilterGeneration(S32 generation, BOOL recurse_up);
	virtual S32		getCompletedFilterGeneration() { return mCompletedFilterGeneration; }
	BOOL hasFilteredDescendants(S32 filter_generation);
	BOOL hasFilteredDescendants();
	virtual void filter( LLInventoryFilter& filter);
	virtual void setFiltered(BOOL filtered, S32 filter_generation);
	virtual BOOL getFiltered();
	virtual BOOL getFiltered(S32 filter_generation);
	virtual void dirtyFilter();
	void filterFolder(LLInventoryFilter& filter);
	void setFilteredFolder(bool filtered, S32 filter_generation);
	bool getFilteredFolder(S32 filter_generation);
	virtual BOOL setSelection(LLFolderViewItem* selection, BOOL openitem, BOOL take_keyboard_focus);
	virtual BOOL changeSelection(LLFolderViewItem* selection, BOOL selected);
	void extendSelectionTo(LLFolderViewItem* selection);
	virtual BOOL isRemovable();
	virtual BOOL isMovable();
	virtual void destroyView();
	BOOL removeItem(LLFolderViewItem* item);
	void removeView(LLFolderViewItem* item);
	void extractItem( LLFolderViewItem* item );
	void resort(LLFolderViewItem* item);
	void setItemSortOrder(U32 ordering);
	void sortBy(U32);
	void setAutoOpenCountdown(F32 countdown) { mAutoOpenCountdown = countdown; }
	virtual void toggleOpen();
	virtual void setOpen(BOOL openitem = TRUE);
	virtual void requestArrange(BOOL include_descendants = FALSE);
	virtual void setOpenArrangeRecursively(BOOL openitem, ERecurseType recurse = RECURSE_NO);
	virtual BOOL isOpen() const { return mIsOpen; }
	BOOL handleDragAndDropFromChild(MASK mask,
		BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		EAcceptance* accept,
		std::string& tooltip_msg);
	void applyFunctorRecursively(LLFolderViewFunctor& functor);
	virtual void applyListenerFunctorRecursively(LLFolderViewListenerFunctor& functor);
	void applyFunctorToChildren(LLFolderViewFunctor& functor);
	virtual void openItem( void );
	virtual BOOL addItem(LLFolderViewItem* item);
	virtual BOOL addFolder( LLFolderViewFolder* folder);
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleDoubleClick( S32 x, S32 y, MASK mask );
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
		BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		EAcceptance* accept,
		std::string& tooltip_msg);
	BOOL handleDragAndDropToThisFolder(MASK mask,
									   BOOL drop,
									   EDragAndDropType cargo_type,
									   void* cargo_data,
									   EAcceptance* accept,
									   std::string& tooltip_msg);
	virtual void draw();
	time_t getCreationDate() const;
	bool isTrash() const;
	folders_t::const_iterator getFoldersBegin() const { return mFolders.begin(); }
	folders_t::const_iterator getFoldersEnd() const { return mFolders.end(); }
	folders_t::size_type getFoldersCount() const { return mFolders.size(); }
	items_t::const_iterator getItemsBegin() const { return mItems.begin(); }
	items_t::const_iterator getItemsEnd() const { return mItems.end(); }
	items_t::size_type getItemsCount() const { return mItems.size(); }
	LLFolderViewFolder* getCommonAncestor(LLFolderViewItem* item_a, LLFolderViewItem* item_b, bool& reverse);
	void gatherChildRangeExclusive(LLFolderViewItem* start, LLFolderViewItem* end, bool reverse,  std::vector<LLFolderViewItem*>& items);
};
class LLFolderViewListenerFunctor
{
public:
	virtual ~LLFolderViewListenerFunctor() {}
	virtual void operator()(LLFolderViewEventListener* listener) = 0;
};
typedef std::deque<LLFolderViewItem*> folder_view_item_deque;
class LLFolderViewGroupedItemModel : public LLRefCount
{
public:
	virtual void groupFilterContextMenu(folder_view_item_deque& selected_items, LLMenuGL& menu) = 0;
};
#endif
