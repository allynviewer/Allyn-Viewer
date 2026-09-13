/** 
 * @file lltoolbar.h
 * @brief Large friendly buttons at bottom of screen.
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
#ifndef LL_LLTOOLBAR_H
#define LL_LLTOOLBAR_H
#include "llpanel.h"
#include "lllayoutstack.h"
#include "llframetimer.h"
extern S32 TOOL_BAR_HEIGHT;
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
	void refresh();
	void onClickCommunicate(const LLSD& selected);
	static F32 sInventoryAutoOpenTime;
private:
	void updateCommunicateList();
	void hookToolbarButtons(LLView* view);
	void onToolbarButtonMouseDown(LLUICtrl* ctrl, S32 x, S32 y, MASK mask);
	void onToolbarButtonRightClick(LLUICtrl* ctrl, S32 x, S32 y, MASK mask);
	void showToolbarContextMenu(S32 x, S32 y);
	void pollToolbarDrag();
	void updateButtonDrag(S32 x, S32 y);
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
	LLLayoutPanel* mDragStartPanel;
	S32 mDragStartX;
	S32 mDragStartY;
	bool mDragging;
	bool mOrderDirty;
};
extern LLToolBar *gToolBar;
#endif
