/** 
 * @file llfolderview.h
 * @brief Definition of the folder view collection of classes.
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
#ifndef LL_LLFOLDERVIEW_H
#define LL_LLFOLDERVIEW_H
#include "llfolderviewitem.h"
#include "lluictrl.h"
#include "v4color.h"
#include "stdenums.h"
#include "lldepthstack.h"
#include "lleditmenuhandler.h"
#include "llfontgl.h"
#include "llinventoryfilter.h"
#include "lltooldraganddrop.h"
#include "llviewertexture.h"
#include <boost/unordered_map.hpp>
class LLFolderViewEventListener;
class LLFolderViewGroupedItemModel;
class LLFolderViewFolder;
class LLFolderViewItem;
class LLInventoryModel;
class LLPanel;
class LLLineEditor;
class LLMenuGL;
class LLScrollContainer;
class LLUICtrl;
class LLTextBox;
class LLSaveFolderState;
class LLFolderView : public LLFolderViewFolder, public LLEditMenuHandler
{
public:
	typedef void (*SelectCallback)(const std::deque<LLFolderViewItem*> &items, BOOL user_action, void* data);
	LLFolderView( const std::string& name, const LLRect& rect,
					const LLUUID& source_id, LLPanel *parent_view, LLFolderViewEventListener* listener, LLFolderViewGroupedItemModel* group_model = NULL );
	typedef folder_view_item_deque selected_items_t;
	virtual ~LLFolderView( void );
	virtual BOOL canFocusChildren() const;
	virtual const LLFolderView*	getRoot() const { return this; }
	virtual LLFolderView*	getRoot() { return this; }
	LLFolderViewGroupedItemModel* getFolderViewGroupedItemModel() { return mGroupedItemModel; }
	const LLFolderViewGroupedItemModel* getFolderViewGroupedItemModel() const { return mGroupedItemModel; }
	void setSortOrder(U32 order);
	void setFilterPermMask(PermissionMask filter_perm_mask);
	typedef boost::signals2::signal<void (const std::deque<LLFolderViewItem*>& items, BOOL user_action)> signal_t;
	void setSelectCallback(const signal_t::slot_type& cb) { mSelectSignal.connect(cb); }
	void setReshapeCallback(const signal_t::slot_type& cb) { mReshapeSignal.connect(cb); }
	void setAllowMultiSelect(BOOL allow) { mAllowMultiSelect = allow; }
	void setShowEmptyMessage(bool show) { mShowEmptyMessage = show; }
	LLInventoryFilter& getFilter() { return mFilter; }
	const std::string getFilterSubString(BOOL trim = FALSE);
	U32 getFilterObjectTypes() const;
	PermissionMask getFilterPermissions() const;
	U32 getSortOrder() const;
	BOOL isFilterModified();
	bool getAllowMultiSelect() { return mAllowMultiSelect; }
	U32 toggleSearchType(std::string toggle);
	U32 getSearchType() const;
	void closeAllFolders();
	void openTopLevelFolders();
	virtual void toggleOpen() {};
	virtual void setOpenArrangeRecursively(BOOL openitem, ERecurseType recurse);
	virtual BOOL addFolder( LLFolderViewFolder* folder);
	virtual S32 arrange( S32* width, S32* height, S32 filter_generation );
	void arrangeAll() { mArrangeGeneration++; }
	S32 getArrangeGeneration() { return mArrangeGeneration; }
	virtual void filter( LLInventoryFilter& filter);
	virtual LLFolderViewItem* getCurSelectedItem( void );
	selected_items_t& getSelectedItems( void );
	virtual BOOL setSelection(LLFolderViewItem* selection, BOOL openitem,
		BOOL take_keyboard_focus = TRUE);
	void setSelectionByID(const LLUUID& obj_id, BOOL take_keyboard_focus);
	void updateSelection();
	virtual BOOL changeSelection(LLFolderViewItem* selection, BOOL selected);
	virtual uuid_set_t getSelectionList() const;
	void sanitizeSelection();
	virtual void clearSelection();
	void addToSelectionList(LLFolderViewItem* item);
	void removeFromSelectionList(LLFolderViewItem* item);
	bool startDrag(LLToolDragAndDrop::ESource source);
	void setDragAndDropThisFrame() { mDragAndDropThisFrame = TRUE; }
	void setDraggingOverItem(LLFolderViewItem* item) { mDraggingOverItem = item; }
	LLFolderViewItem* getDraggingOverItem() { return mDraggingOverItem; }
 	void removeSelectedItems();
	static void removeCutItems();
	void openSelectedItems( void );
	void propertiesSelectedItems( void );
	void changeType(LLInventoryModel *model, LLFolderType::EType new_folder_type);
	void autoOpenItem(LLFolderViewFolder* item);
	void closeAutoOpenedFolders();
	BOOL autoOpenTest(LLFolderViewFolder* item);
	BOOL isOpen() const { return TRUE; }
	virtual BOOL	canCopy() const;
	virtual void	copy() const override final;
	virtual BOOL	canCut() const;
	virtual void	cut();
	virtual BOOL	canPaste() const;
	virtual void	paste();
	virtual BOOL	canDoDelete() const;
	virtual void	doDelete();
	void startRenamingSelectedItem( void );
	BOOL handleKeyHere( KEY key, MASK mask );
	BOOL handleUnicodeCharHere(llwchar uni_char);
	BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	BOOL handleDoubleClick( S32 x, S32 y, MASK mask );
	BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );
	BOOL handleHover( S32 x, S32 y, MASK mask );
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	void onMouseLeave(S32 x, S32 y, MASK mask) { setShowSelectionContext(FALSE); }
	virtual BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void draw();
	virtual void deleteAllChildren();
	void scrollToShowSelection();
	void scrollToShowItem(LLFolderViewItem* item, const LLRect& constraint_rect);
	void setScrollContainer(LLScrollContainer* parent);
	LLRect getVisibleRect();
	BOOL search(LLFolderViewItem* first_item, const std::string &search_string, BOOL backward);
	void setShowSelectionContext(bool show) { mShowSelectionContext = show; }
	BOOL getShowSelectionContext();
	void setShowSingleSelection(bool show);
	BOOL getShowSingleSelection() { return mShowSingleSelection; }
	F32  getSelectionFadeElapsedTime() { return mMultiSelectionFadeTimer.getElapsedTimeF32(); }
	bool getUseEllipses() { return mUseEllipses; }
	S32 getSelectedCount() { return (S32)mSelectedItems.size(); }
	void addItemID(const LLUUID& id, LLFolderViewItem* itemp);
	void removeItemID(const LLUUID& id);
	LLFolderViewItem* getItemByID(const LLUUID& id);
	LLFolderViewFolder* getFolderByID(const LLUUID& id);
	void	doIdle();
	static void idle(void* user_data);
	BOOL needsAutoSelect() { return mNeedsAutoSelect && !mAutoSelectOverride; }
	BOOL needsAutoRename() { return mNeedsAutoRename; }
	void setNeedsAutoRename(BOOL val) { mNeedsAutoRename = val; }
	void setPinningSelectedItem(BOOL val) { mPinningSelectedItem = val; }
	void setAutoSelectOverride(BOOL val) { mAutoSelectOverride = val; }
	bool getAutoSelectOverride() const { return mAutoSelectOverride; }
	BOOL getDebugFilters() { return mDebugFilters; }
	LLPanel* getParentPanel() { return mParentPanel.get(); }
	void dumpSelectionInformation();
	virtual S32	notify(const LLSD& info) ;
	bool useLabelSuffix() { return mUseLabelSuffix; }
	virtual void updateMenu();
	void saveFolderState();
	void restoreFolderState();
	LLHandle<LLFolderView>	getHandle() const { return getDerivedHandle<LLFolderView>(); }
private:
	void updateMenuOptions(LLMenuGL* menu);
	void updateRenamerPosition();
protected:
	LLScrollContainer* mScrollContainer;
	void commitRename( );
	void onRenamerLost();
	void finishRenamingItem( void );
	void closeRenamer( void );
	bool selectFirstItem();
	bool selectLastItem();
	BOOL addNoOptions(LLMenuGL* menu) const;
private:
	std::unique_ptr<LLSaveFolderState> mSavedFolderState;
protected:
	LLHandle<LLView>					mPopupMenuHandle;
	selected_items_t				mSelectedItems;
	bool							mKeyboardSelection,
									mAllowMultiSelect,
									mShowEmptyMessage,
									mShowFolderHierarchy,
									mNeedsScroll,
									mPinningSelectedItem,
									mNeedsAutoSelect,
									mAutoSelectOverride,
									mNeedsAutoRename,
									mUseLabelSuffix,
									mDragAndDropThisFrame,
									mShowSelectionContext,
									mShowSingleSelection;
	LLUUID							mSourceID;
	LLFolderViewItem*				mRenameItem;
	LLLineEditor*					mRenamer;
	LLRect							mScrollConstraintRect;
	bool							mDebugFilters;
	U32								mSortOrder;
	U32								mSearchType;
	LLDepthStack<LLFolderViewFolder>	mAutoOpenItems;
	LLFolderViewFolder*				mAutoOpenCandidate;
	LLFrameTimer					mAutoOpenTimer;
	LLFrameTimer					mSearchTimer;
	LLWString						mSearchString;
	LLInventoryFilter				mFilter;
	LLFrameTimer					mMultiSelectionFadeTimer;
	S32								mArrangeGeneration;
	signal_t						mSelectSignal;
	signal_t						mReshapeSignal;
	S32								mSignalSelectCallback;
	S32								mMinWidth;
	S32								mRunningHeight;
	boost::unordered_map<LLUUID, LLFolderViewItem*> mItemMap;
	LLUUID							mSelectThisID;
	LLHandle<LLPanel>				mParentPanel;
	LLFolderViewGroupedItemModel*	mGroupedItemModel;
	bool							mUseEllipses;
	LLFolderViewItem*				mDraggingOverItem;
public:
	static F32 sAutoOpenTime;
	LLTextBox*						mStatusTextBox;
};
const U32 SUPPRESS_OPEN_ITEM = 0x1;
const U32 FIRST_SELECTED_ITEM = 0x2;
const U32 ITEM_IN_MULTI_SELECTION = 0x4;
#endif
