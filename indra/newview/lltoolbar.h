/** 
 * @file lltoolbar.h
 * @brief Large friendly buttons at bottom of screen.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * Modifications Copyright (C) 2026 - Allyn Viewer
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
#ifndef LL_LLTOOLBAR_H
#define LL_LLTOOLBAR_H
#include "llpanel.h"
#include "lllayoutstack.h"
#include "llframetimer.h"
#include "llfloatingbarstyle.h"
extern S32 TOOL_BAR_HEIGHT;
class LLButton;
class LLFlyoutButton;
class LLLayoutStack;
class LLLayoutPanel;
class LLMenuGL;
class LLUICtrl;
class LLToolBar
:	public LLLayoutPanel
{
public:
	LLToolBar();
	~LLToolBar();
	BOOL postBuild();
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	void onMouseCaptureLost();
	void draw();
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	void refresh();
	void onClickCommunicate(const LLSD& selected);
	S32 getBarHeight() const;
	S32 getPreferredWidth() const;
	S32 contentWidthForButtonSize(S32 size) const;
	S32 chromeWidth() const;
	S32 buttonSizeForWidth(S32 available) const;
	S32 getButtonSize() const { return mButtonSize; }
	void setMaxButtonSize(S32 size);
	S32 getRowCount() const { return mRowCount; }
	S32 getCompanionReserve() const { return mCompanionReserve; }
	bool getCenterOverride() const { return mCenterOverride; }
	bool getDockLeft() const { return mDockLeft; }
	bool isLayoutDirty() const { return mLayoutDirty; }
	void setCompanionReserve(S32 width);
	void setCenterOverride(bool center);
	void setDockLeft(bool dock);
	void layoutNow();
	S32 getLayoutPillRight() const { return mLayoutPillRight; }
	const LLRect& getPillRect() const { return mPillRect; }
	const LLRect& getAddRect() const { return mAddRect; }
	static F32 sInventoryAutoOpenTime;
private:
	struct Slot
	{
		LLLayoutPanel* panel;
		LLView* main;
		LLButton* action;
		LLButton* arrow;
		LLView* badge;
		S32 natural;
	};
	void buildSlots();
	void markLayoutDirty();
	void layoutToolbar();
	void retargetLeft(S32 left, bool animate);
	S32 sampleLeft();
	void tickSlide();
	S32 getContentWidth(S32 size) const;
	void onClickAddButton();
	void updateCommunicateList();
	void hookToolbarButtons(LLView* view);
	void onToolbarButtonMouseDown(LLUICtrl* ctrl, S32 x, S32 y, MASK mask);
	void onToolbarButtonRightClick(LLUICtrl* ctrl, S32 x, S32 y, MASK mask);
	void showToolbarContextMenu(S32 x, S32 y);
	void removeContextButton();
	static void onRemoveContextButton(void*);
	void pollToolbarDrag();
	void updateButtonDrag(S32 x, S32 y);
	void commitButtonDrag();
	void syncSlotsToStack();
	void applySavedButtonOrder();
	void saveButtonOrder();
	std::string getPanelGroupKey(LLLayoutPanel* panel) const;
	bool getGroupRange(LLLayoutPanel* panel, S32& start, S32& count) const;
	LLLayoutPanel* findLayoutPanelFromChild(LLView* view) const;
	LLLayoutPanel* findPanelAt(S32 x, S32 y) const;
private:
	LLFrameTimer mInventoryAutoOpenTimer;
	S32			mNumUnreadIMs;
	CachedUICtrl<LLFlyoutButton> mCommunicateBtn;
	CachedUICtrl<LLButton> mFlyBtn;
	CachedUICtrl<LLButton> mBuildBtn;
	CachedUICtrl<LLButton> mMapBtn;
	CachedUICtrl<LLButton> mRadarBtn;
	CachedUICtrl<LLButton> mInventoryBtn;
	LLLayoutStack* mToolbarStack;
	LLMenuGL* mContextMenu;
	LLLayoutPanel* mContextPanel;
	LLLayoutPanel* mDragStartPanel;
	S32 mDragStartX;
	S32 mDragStartY;
	bool mDragging;
	S32 mDragInsertIndex;
	bool mDragCaretVisible;
	LLRect mDragCaretRect;
	bool mOrderDirty;
	bool mLayoutDirty;
	bool mMenuRequested;
	LLButton* mAddBtn;
	LLView* mPillBlocker;
	LLRect mPillRect;
	LLRect mAddRect;
	S32 mBarHeight;
	S32 mRowCount;
	S32 mButtonSize;
	S32 mMaxButtonSize;
	S32 mCompanionReserve;
	bool mCenterOverride;
	bool mDockLeft;
	S32 mVisualLeft;
	S32 mFromLeft;
	S32 mTargetLeft;
	S32 mAppliedLeft;
	F64 mSlideStart;
	bool mSliding;
	bool mPlacementReady;
	bool mWasDockLeft;
	bool mWasCenter;
	S32 mLayoutPillRight;
	std::vector<Slot> mSlots;
	std::vector<boost::signals2::connection> mLayoutSlots;
	LLFloatingBarStyle mStyle;
};
extern LLToolBar *gToolBar;
#endif
