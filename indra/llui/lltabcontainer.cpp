/**
 * @file lltabcontainer.cpp
 * @brief LLTabContainer class
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
#include "linden_common.h"
#include <vector>
#include "lltabcontainer.h"
#include "llfocusmgr.h"
#include "llbutton.h"
#include "lllocalcliprect.h"
#include "llrect.h"
#include "llresmgr.h"
#include "llresizehandle.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llcriticaldamp.h"
#include "lluictrlfactory.h"
#include "llui.h"
#include "llrender.h"
#include "llmultifloater.h"
#include "llfontgl.h"
#include "llwindow.h"
const F32 SCROLL_STEP_TIME = 0.4f;
const F32 SCROLL_DELAY_TIME = 0.5f;
const S32 TAB_PADDING = 15;
const S32 TABCNTR_TAB_MIN_WIDTH = 60;
const S32 TABCNTR_VERT_TAB_MIN_WIDTH = 188;
const S32 TABCNTR_TAB_MAX_WIDTH = 150;
const S32 TABCNTR_TAB_PARTIAL_WIDTH = 12;
const S32 TABCNTR_TAB_HEIGHT = 22;
const S32 TABCNTR_ARROW_BTN_SIZE = 16;
const S32 TABCNTR_BUTTON_PANEL_OVERLAP = 1;
const S32 TABCNTR_TAB_H_PAD = 4;
const S32 TABCNTR_CLOSE_BTN_SIZE = 16;
const S32 TABCNTR_HEADER_HEIGHT = LLPANEL_BORDER_WIDTH + TABCNTR_CLOSE_BTN_SIZE;
const S32 TABCNTRV_CLOSE_BTN_SIZE = 16;
const S32 TABCNTRV_HEADER_HEIGHT = LLPANEL_BORDER_WIDTH + TABCNTRV_CLOSE_BTN_SIZE;
const S32 TABCNTRV_TAB_HEIGHT = 32;
const S32 TABCNTRV_ARROW_BTN_SIZE = 16;
const S32 TABCNTRV_PAD = 3;
const S32 TABCNTRV_SPLITTER_PAD = 6;
const S32 TABCNTRV_USER_MIN_WIDTH = 80;
const S32 TABCNTRV_USER_MAX_WIDTH = 320;
const S32 TABCNTRV_CONTENT_MIN_WIDTH = 180;
static LLRegisterWidget<LLTabContainer> r("tab_container");
class LLTabTuple
{
public:
	LLTabTuple( LLTabContainer* c, LLPanel* p, LLButton* b, LLTextBox* placeholder = NULL)
		:
		mTabContainer(c),
		mTabPanel(p),
		mButton(b),
		mOldState(FALSE),
		mPlaceholderText(placeholder),
		mPadding(0),
		mVisible(TRUE)
		{}
	LLTabContainer*  mTabContainer;
	LLPanel*		 mTabPanel;
	LLButton*		 mButton;
	BOOL			 mOldState;
	LLTextBox*		 mPlaceholderText;
	S32				 mPadding;
	BOOL			 mVisible;
};
LLTabContainer::LLTabContainer(const std::string& name, const LLRect& rect, TabPosition pos,
							   BOOL bordered, BOOL is_vertical )
	:
	LLPanel(name, rect, bordered),
	mCurrentTabIdx(-1),
	mTabsHidden(FALSE),
	mScrolled(FALSE),
	mScrollPos(0),
	mScrollPosPixels(0),
	mMaxScrollPos(0),
	mTitleBox(NULL),
	mTopBorderHeight(LLPANEL_BORDER_WIDTH),
	mTabPosition(pos),
	mLockedTabCount(0),
	mMinTabWidth(TABCNTR_TAB_MIN_WIDTH),
	mMaxTabWidth(TABCNTR_TAB_MAX_WIDTH),
	mPrevArrowBtn(NULL),
	mNextArrowBtn(NULL),
	mIsVertical(is_vertical),
	mJumpPrevArrowBtn(NULL),
	mJumpNextArrowBtn(NULL),
	mRightTabBtnOffset(0),
	mTotalTabWidth(0),
	mUserResize(FALSE),
	mResizingTabs(FALSE),
	mHoverSplitter(FALSE),
	mDragStartX(0),
	mDragStartTabWidth(TABCNTR_VERT_TAB_MIN_WIDTH)
{
	if (mIsVertical)
	{
		mMinTabWidth = TABCNTR_VERT_TAB_MIN_WIDTH;
	}
	setMouseOpaque(FALSE);
	initButtons( );
	mDragAndDropDelayTimer.stop();
}
LLTabContainer::~LLTabContainer()
{
	std::for_each(mTabList.begin(), mTabList.end(), DeletePointer());
}
void LLTabContainer::setValue(const LLSD& value)
{
	selectTab((S32) value.asInteger());
}
void LLTabContainer::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape( width, height, called_from_parent );
	if (mIsVertical)
	{
		if (mUserResize)
		{
			setVerticalTabWidth(mMinTabWidth);
		}
		updateMaxScrollPos();
		return;
	}
	if (!mTabList.empty())
	{
		const LLFontGL* fontp = LLResMgr::getInstance()->getRes(LLFONT_SANSSERIF_SMALL);
		std::vector<S32> natural;
		natural.reserve(mTabList.size());
		S32 total = 0;
		for (tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			S32 w = llclamp(fontp->getWidth(tuple->mButton->getLabelSelected()) + TAB_PADDING + tuple->mPadding,
							mMinTabWidth, mMaxTabWidth);
			natural.push_back(w);
			total += w;
		}
		const S32 available = getRect().getWidth() - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_TAB_H_PAD);
		if (total > 0 && total < available)
		{
			const S32 extra = available - total;
			const S32 n = (S32)natural.size();
			const S32 add = extra / n;
			S32 rem = extra % n;
			total = 0;
			S32 i = 0;
			for (tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter, ++i)
			{
				const S32 w = natural[i] + add + (rem-- > 0 ? 1 : 0);
				(*iter)->mButton->reshape(w, (*iter)->mButton->getRect().getHeight());
				total += w;
			}
		}
		else
		{
			S32 i = 0;
			for (tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter, ++i)
			{
				(*iter)->mButton->reshape(natural[i], (*iter)->mButton->getRect().getHeight());
			}
		}
		mTotalTabWidth = total;
	}
	updateMaxScrollPos();
}
LLView* LLTabContainer::getChildView(const std::string& name, BOOL recurse, BOOL create_if_missing) const
{
	tuple_list_t::const_iterator itor;
	for (itor = mTabList.begin(); itor != mTabList.end(); ++itor)
	{
		LLPanel *panel = (*itor)->mTabPanel;
		if (panel->getName() == name)
		{
			return panel;
		}
	}
	if (recurse)
	{
		for (itor = mTabList.begin(); itor != mTabList.end(); ++itor)
		{
			LLPanel *panel = (*itor)->mTabPanel;
			LLView *child = panel->getChildView(name, recurse, FALSE);
			if (child)
			{
				return child;
			}
		}
	}
	return LLView::getChildView(name, recurse, create_if_missing);
}
void LLTabContainer::draw()
{
	S32 target_pixel_scroll = 0;
	S32 cur_scroll_pos = getScrollPos();
	if (cur_scroll_pos > 0)
	{
		if (!mIsVertical)
		{
			S32 available_width_with_arrows = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE  + TABCNTR_ARROW_BTN_SIZE + 1);
			for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
			{
				if (cur_scroll_pos == 0)
				{
					break;
				}
				target_pixel_scroll += (*iter)->mButton->getRect().getWidth();
				cur_scroll_pos--;
			}
			target_pixel_scroll -= TABCNTR_TAB_PARTIAL_WIDTH;
			target_pixel_scroll = llmin(mTotalTabWidth - available_width_with_arrows, target_pixel_scroll);
		}
		else
		{
			S32 available_height_with_arrows = getRect().getHeight() - getTopBorderHeight() - (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE + TABCNTR_ARROW_BTN_SIZE + 1);
			for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
			{
				if (cur_scroll_pos==0)
				{
					break;
				}
				target_pixel_scroll += (*iter)->mButton->getRect().getHeight();
				cur_scroll_pos--;
			}
			S32 total_tab_height = (TABCNTRV_TAB_HEIGHT + TABCNTRV_PAD) * getTabCount() + TABCNTRV_PAD;
			target_pixel_scroll = llmin(total_tab_height - available_height_with_arrows, target_pixel_scroll);
		}
	}
	setScrollPosPixels(mIsVertical ? target_pixel_scroll : lerp((F32)getScrollPosPixels(), (F32)target_pixel_scroll, LLSmoothInterpolation::getInterpolant(0.08f)));
	BOOL has_scroll_arrows = !getTabsHidden() && ((mMaxScrollPos > 0) || (mScrollPosPixels > 0));
	if (!mIsVertical)
	{
		mJumpPrevArrowBtn->setVisible( has_scroll_arrows );
		mJumpNextArrowBtn->setVisible( has_scroll_arrows );
	}
	mPrevArrowBtn->setVisible( has_scroll_arrows );
	mNextArrowBtn->setVisible( has_scroll_arrows );
	S32 left = 0, top = 0;
	if (mIsVertical)
	{
		top = getRect().getHeight() - getTopBorderHeight() - LLPANEL_BORDER_WIDTH - 1 - (has_scroll_arrows ? TABCNTRV_ARROW_BTN_SIZE : 0);
		top += getScrollPosPixels();
	}
	else
	{
		left = LLPANEL_BORDER_WIDTH + (has_scroll_arrows ? (TABCNTR_ARROW_BTN_SIZE * 2) : TABCNTR_TAB_H_PAD);
		left -= getScrollPosPixels();
	}
	if (getTabsHidden() || mIsVertical)
	{
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			tuple->mButton->setVisible( FALSE );
		}
	}
	{
		LLRect clip_rect = getLocalRect();
		clip_rect.mLeft+=(LLPANEL_BORDER_WIDTH + 2);
		clip_rect.mRight-=(LLPANEL_BORDER_WIDTH + 2);
		LLLocalClipRect clip(clip_rect);
		LLPanel::draw();
	}
	if (!getTabsHidden())
	{
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			tuple->mButton->setVisible( TRUE );
		}
		LLRect clip_rect = getLocalRect();
		if (has_scroll_arrows && mIsVertical)
		{
			clip_rect.mTop = top - getScrollPosPixels();
			clip_rect.mBottom = TABCNTR_ARROW_BTN_SIZE+1;
		}
		LLLocalClipRect clip(clip_rect);
		S32 max_scroll_visible = getVisibleTabCount() - getMaxScrollPos() + getScrollPos();
		S32 idx = 0;
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			if (!tuple->mVisible)
			{
				tuple->mButton->setVisible(FALSE);
				continue;
			}
			tuple->mButton->translate( left ? left - tuple->mButton->getRect().mLeft : 0,
									   top ? top - tuple->mButton->getRect().mTop : 0 );
			if (top) top -= TABCNTRV_TAB_HEIGHT + TABCNTRV_PAD;
			if (left) left += tuple->mButton->getRect().getWidth();
			if (!mIsVertical)
			{
				if( idx < getScrollPos() )
				{
					if( tuple->mButton->getFlashing() )
					{
						mPrevArrowBtn->setFlashing( TRUE );
					}
				}
				else if( max_scroll_visible < idx )
				{
					if( tuple->mButton->getFlashing() )
					{
						mNextArrowBtn->setFlashing( TRUE );
					}
				}
			}
			else
			{
				LLUI::pushMatrix();
				LLUI::translate((F32)tuple->mButton->getRect().mLeft, (F32)tuple->mButton->getRect().mBottom, 0.f);
				tuple->mButton->draw();
				LLUI::popMatrix();
			}
			idx++;
		}
		if( mIsVertical && has_scroll_arrows )
		{
			gGL.pushUIMatrix();
			gGL.translateUI((F32)mPrevArrowBtn->getRect().mLeft, (F32)mPrevArrowBtn->getRect().mBottom, 0.f);
			mPrevArrowBtn->draw();
			gGL.popUIMatrix();
			gGL.pushUIMatrix();
			gGL.translateUI((F32)mNextArrowBtn->getRect().mLeft, (F32)mNextArrowBtn->getRect().mBottom, 0.f);
			mNextArrowBtn->draw();
			gGL.popUIMatrix();
		}
	}
	mPrevArrowBtn->setFlashing(FALSE);
	mNextArrowBtn->setFlashing(FALSE);
	drawVerticalSplitter();
}
BOOL LLTabContainer::handleMouseDown( S32 x, S32 y, MASK mask )
{
	if (isOverVerticalSplitter(x, y))
	{
		gFocusMgr.setMouseCapture(this);
		mResizingTabs = TRUE;
		mDragStartX = x;
		mDragStartTabWidth = mMinTabWidth;
		return TRUE;
	}
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0) && !getTabsHidden();
	if (has_scroll_arrows)
	{
		if (mJumpPrevArrowBtn&& mJumpPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpPrevArrowBtn->getRect().mBottom;
			handled = mJumpPrevArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
		else if (mJumpNextArrowBtn && mJumpNextArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpNextArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpNextArrowBtn->getRect().mBottom;
			handled = mJumpNextArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
		else if (mPrevArrowBtn && mPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mPrevArrowBtn->getRect().mBottom;
			handled = mPrevArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
		else if (mNextArrowBtn && mNextArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mNextArrowBtn->getRect().mLeft;
			S32 local_y = y - mNextArrowBtn->getRect().mBottom;
			handled = mNextArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
	}
	if (!handled)
	{
		handled = LLPanel::handleMouseDown( x, y, mask );
	}
	S32 tab_count = getTabCount();
	if (tab_count > 0 && !getTabsHidden())
	{
		LLTabTuple* firsttuple = getTab(0);
		LLRect tab_rect;
		if (mIsVertical)
		{
			tab_rect = LLRect(firsttuple->mButton->getRect().mLeft,
							  has_scroll_arrows ? mPrevArrowBtn->getRect().mBottom - TABCNTRV_PAD : mPrevArrowBtn->getRect().mTop,
							  firsttuple->mButton->getRect().mRight,
							  has_scroll_arrows ? mNextArrowBtn->getRect().mTop + TABCNTRV_PAD : mNextArrowBtn->getRect().mBottom );
		}
		else
		{
			tab_rect = LLRect(has_scroll_arrows ? mPrevArrowBtn->getRect().mRight : mJumpPrevArrowBtn->getRect().mLeft,
							  firsttuple->mButton->getRect().mTop,
							  has_scroll_arrows ? mNextArrowBtn->getRect().mLeft : mJumpNextArrowBtn->getRect().mRight,
							  firsttuple->mButton->getRect().mBottom );
		}
		if( tab_rect.pointInRect( x, y ) )
		{
			S32 index = getCurrentPanelIndex();
			index = llclamp(index, 0, tab_count-1);
			LLButton* tab_button = getTab(index)->mButton;
			gFocusMgr.setMouseCapture(this);
			tab_button->setFocus(TRUE);
			gFocusMgr.setKeyboardFocus(tab_button);
		}
	}
	return handled;
}
BOOL LLTabContainer::handleHover( S32 x, S32 y, MASK mask )
{
	mHoverSplitter = FALSE;
	if (mResizingTabs && hasMouseCapture())
	{
		setVerticalTabWidth(mDragStartTabWidth + (x - mDragStartX));
		if (LLWindow* window = getWindow())
		{
			window->setCursor(UI_CURSOR_SIZEWE);
		}
		return TRUE;
	}
	if (isOverVerticalSplitter(x, y))
	{
		mHoverSplitter = TRUE;
		if (LLWindow* window = getWindow())
		{
			window->setCursor(UI_CURSOR_SIZEWE);
		}
		return TRUE;
	}
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0) && !getTabsHidden();
	if (has_scroll_arrows)
	{
		if (mJumpPrevArrowBtn && mJumpPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpPrevArrowBtn->getRect().mBottom;
			handled = mJumpPrevArrowBtn->handleHover(local_x, local_y, mask);
		}
		else if (mJumpNextArrowBtn && mJumpNextArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpNextArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpNextArrowBtn->getRect().mBottom;
			handled = mJumpNextArrowBtn->handleHover(local_x, local_y, mask);
		}
		else if (mPrevArrowBtn && mPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mPrevArrowBtn->getRect().mBottom;
			handled = mPrevArrowBtn->handleHover(local_x, local_y, mask);
		}
		else if (mNextArrowBtn && mNextArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mNextArrowBtn->getRect().mLeft;
			S32 local_y = y - mNextArrowBtn->getRect().mBottom;
			handled = mNextArrowBtn->handleHover(local_x, local_y, mask);
		}
	}
	if (!handled)
	{
		handled = LLPanel::handleHover(x, y, mask);
	}
	commitHoveredButton(x, y);
	return handled;
}
BOOL LLTabContainer::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if (mResizingTabs && hasMouseCapture())
	{
		mResizingTabs = FALSE;
		gFocusMgr.setMouseCapture(NULL);
		if (mTabWidthCallback)
		{
			mTabWidthCallback(mMinTabWidth);
		}
		return TRUE;
	}
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0)  && !getTabsHidden();
	if (has_scroll_arrows)
	{
		if (mJumpPrevArrowBtn && mJumpPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpPrevArrowBtn->getRect().mBottom;
			handled = mJumpPrevArrowBtn->handleMouseUp(local_x, local_y, mask);
		}
		else if (mJumpNextArrowBtn && mJumpNextArrowBtn->getRect().pointInRect(x,	y))
		{
			S32	local_x	= x	- mJumpNextArrowBtn->getRect().mLeft;
			S32	local_y	= y	- mJumpNextArrowBtn->getRect().mBottom;
			handled = mJumpNextArrowBtn->handleMouseUp(local_x,	local_y, mask);
		}
		else if (mPrevArrowBtn && mPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mPrevArrowBtn->getRect().mBottom;
			handled = mPrevArrowBtn->handleMouseUp(local_x, local_y, mask);
		}
		else if (mNextArrowBtn && mNextArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mNextArrowBtn->getRect().mLeft;
			S32 local_y = y - mNextArrowBtn->getRect().mBottom;
			handled = mNextArrowBtn->handleMouseUp(local_x, local_y, mask);
		}
	}
	if (!handled)
	{
		handled = LLPanel::handleMouseUp( x, y, mask );
	}
	commitHoveredButton(x, y);
	LLPanel* cur_panel = getCurrentPanel();
	if (hasMouseCapture())
	{
		if (cur_panel)
		{
			if (!cur_panel->focusFirstItem(FALSE))
			{
				getTab(getCurrentPanelIndex())->mButton->setFocus(TRUE);
			}
		}
		gFocusMgr.setMouseCapture(NULL);
	}
	return handled;
}
BOOL LLTabContainer::handleToolTip( S32 x, S32 y, std::string& msg, LLRect* sticky_rect )
{
	BOOL handled = LLPanel::handleToolTip( x, y, msg, sticky_rect );
	if (!handled && getTabCount() > 0 && !getTabsHidden())
	{
		LLTabTuple* firsttuple = getTab(0);
		BOOL has_scroll_arrows = (getMaxScrollPos() > 0);
		LLRect clip;
		if (mIsVertical)
		{
			clip = LLRect(firsttuple->mButton->getRect().mLeft,
						  has_scroll_arrows ? mPrevArrowBtn->getRect().mBottom - TABCNTRV_PAD : mPrevArrowBtn->getRect().mTop,
						  firsttuple->mButton->getRect().mRight,
						  has_scroll_arrows ? mNextArrowBtn->getRect().mTop + TABCNTRV_PAD : mNextArrowBtn->getRect().mBottom );
		}
		else
		{
			clip = LLRect(has_scroll_arrows ? mPrevArrowBtn->getRect().mRight : mJumpPrevArrowBtn->getRect().mLeft,
						  firsttuple->mButton->getRect().mTop,
						  has_scroll_arrows ? mNextArrowBtn->getRect().mLeft : mJumpNextArrowBtn->getRect().mRight,
						  firsttuple->mButton->getRect().mBottom );
		}
		if( clip.pointInRect( x, y ) )
		{
			for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
			{
				LLTabTuple* tuple = *iter;
				tuple->mButton->setVisible( TRUE );
				S32 local_x = x - tuple->mButton->getRect().mLeft;
				S32 local_y = y - tuple->mButton->getRect().mBottom;
				handled = tuple->mButton->handleToolTip( local_x, local_y, msg, sticky_rect );
				if( handled )
				{
					break;
				}
			}
		}
	}
	return handled;
}
BOOL LLTabContainer::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = FALSE;
	if (key == KEY_LEFT && mask == MASK_ALT)
	{
		selectPrevTab();
		handled = TRUE;
	}
	else if (key == KEY_RIGHT && mask == MASK_ALT)
	{
		selectNextTab();
		handled = TRUE;
	}
	if (handled)
	{
		if (getCurrentPanel())
		{
 			getCurrentPanel()->setFocus(TRUE);
		}
	}
	if (!gFocusMgr.childHasKeyboardFocus(getCurrentPanel()))
	{
		if (mIsVertical)
		{
			switch(key)
			{
			  case KEY_UP:
				selectPrevTab();
				handled = TRUE;
				break;
			  case KEY_DOWN:
				selectNextTab();
				handled = TRUE;
				break;
			  case KEY_LEFT:
				handled = TRUE;
				break;
			  case KEY_RIGHT:
				if (getTabPosition() == LEFT && getCurrentPanel())
				{
					getCurrentPanel()->setFocus(TRUE);
				}
				handled = TRUE;
				break;
			  default:
				break;
			}
		}
		else
		{
			switch(key)
			{
			  case KEY_UP:
				if (getTabPosition() == BOTTOM && getCurrentPanel())
				{
					getCurrentPanel()->setFocus(TRUE);
				}
				handled = TRUE;
				break;
			  case KEY_DOWN:
				if (getTabPosition() == TOP && getCurrentPanel())
				{
					getCurrentPanel()->setFocus(TRUE);
				}
				handled = TRUE;
				break;
			  case KEY_LEFT:
				selectPrevTab();
				handled = TRUE;
				break;
			  case KEY_RIGHT:
				selectNextTab();
				handled = TRUE;
				break;
			  default:
				break;
			}
		}
	}
	return handled;
}
LLXMLNodePtr LLTabContainer::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLPanel::getXML();
	node->setName(LL_TAB_CONTAINER_COMMON_TAG);
	node->createChild("tab_position", TRUE)->setStringValue((getTabPosition() == TOP ? "top" : "bottom"));
	return node;
}
BOOL LLTabContainer::handleDragAndDrop(S32 x, S32 y, MASK mask,	BOOL drop,	EDragAndDropType type, void* cargo_data, EAcceptance *accept, std::string	&tooltip)
{
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0);
	if(!getTabsHidden())
	{
		if (mDragAndDropDelayTimer.getStarted())
		{
			if (mDragAndDropDelayTimer.getElapsedTimeF32() > SCROLL_DELAY_TIME)
			{
				if (has_scroll_arrows)
				{
					if (mJumpPrevArrowBtn && mJumpPrevArrowBtn->getRect().pointInRect(x, y))
					{
						S32	local_x	= x	- mJumpPrevArrowBtn->getRect().mLeft;
						S32	local_y	= y	- mJumpPrevArrowBtn->getRect().mBottom;
						mJumpPrevArrowBtn->handleHover(local_x,	local_y, mask);
					}
					if (mJumpNextArrowBtn && mJumpNextArrowBtn->getRect().pointInRect(x, y))
					{
						S32	local_x	= x	- mJumpNextArrowBtn->getRect().mLeft;
						S32	local_y	= y	- mJumpNextArrowBtn->getRect().mBottom;
						mJumpNextArrowBtn->handleHover(local_x,	local_y, mask);
					}
					if (mPrevArrowBtn->getRect().pointInRect(x,	y))
					{
						S32	local_x	= x	- mPrevArrowBtn->getRect().mLeft;
						S32	local_y	= y	- mPrevArrowBtn->getRect().mBottom;
						mPrevArrowBtn->handleHover(local_x,	local_y, mask);
					}
					else if	(mNextArrowBtn->getRect().pointInRect(x, y))
					{
						S32	local_x	= x	- mNextArrowBtn->getRect().mLeft;
						S32	local_y	= y	- mNextArrowBtn->getRect().mBottom;
						mNextArrowBtn->handleHover(local_x, local_y, mask);
					}
				}
				for(tuple_list_t::iterator iter	= mTabList.begin();	iter !=	 mTabList.end(); ++iter)
				{
					LLTabTuple*	tuple =	*iter;
					tuple->mButton->setVisible(	TRUE );
					S32	local_x	= x	- tuple->mButton->getRect().mLeft;
					S32	local_y	= y	- tuple->mButton->getRect().mBottom;
					if (tuple->mButton->pointInView(local_x, local_y) &&  tuple->mButton->getEnabled() && !tuple->mTabPanel->getVisible())
					{
						tuple->mButton->onCommit();
					}
				}
				mDragAndDropDelayTimer.stop();
			}
		}
		else
		{
			mDragAndDropDelayTimer.start();
		}
	}
	return LLView::handleDragAndDrop(x,	y, mask, drop, type, cargo_data,  accept, tooltip);
}
void LLTabContainer::addTabPanel(LLPanel* child,
								 const std::string& label,
								 BOOL select,
								 S32 indent,
								 BOOL placeholder,
								 eInsertionPoint insertion_point)
{
	if (child->getParent() == this)
	{
		return;
	}
	const LLFontGL* font = LLResMgr::getInstance()->getRes( mIsVertical ? LLFONT_SANSSERIF : LLFONT_SANSSERIF_SMALL );
	child->setLabel(label);
	std::string trimmed_label = label;
	LLStringUtil::trim(trimmed_label);
	S32 button_width = mMinTabWidth;
	if (!mIsVertical)
	{
		button_width = llclamp(font->getWidth(trimmed_label) + TAB_PADDING, mMinTabWidth, mMaxTabWidth);
	}
	S32 tab_panel_top;
	S32 tab_panel_bottom;
	if (!getTabsHidden())
	{
		if( getTabPosition() == LLTabContainer::TOP )
		{
			S32 tab_height = mIsVertical ? TABCNTRV_TAB_HEIGHT : TABCNTR_TAB_HEIGHT;
			tab_panel_top = getRect().getHeight() - getTopBorderHeight() - (tab_height - TABCNTR_BUTTON_PANEL_OVERLAP);
			tab_panel_bottom = LLPANEL_BORDER_WIDTH;
		}
		else
		{
			tab_panel_top = getRect().getHeight() - getTopBorderHeight();
			tab_panel_bottom = (TABCNTR_TAB_HEIGHT + LLPANEL_BORDER_WIDTH*2 - TABCNTR_BUTTON_PANEL_OVERLAP);
		}
	}
	else
	{
		tab_panel_top = getRect().getHeight();
		tab_panel_bottom = LLPANEL_BORDER_WIDTH;
	}
	LLRect tab_panel_rect;
	if (!getTabsHidden() && mIsVertical)
	{
		tab_panel_rect = LLRect(mMinTabWidth + mRightTabBtnOffset + (LLPANEL_BORDER_WIDTH * 2) + TABCNTRV_PAD,
								getRect().getHeight() - LLPANEL_BORDER_WIDTH,
								getRect().getWidth() - LLPANEL_BORDER_WIDTH,
								LLPANEL_BORDER_WIDTH);
	}
	else
	{
		tab_panel_rect = LLRect(LLPANEL_BORDER_WIDTH,
								tab_panel_top,
								getRect().getWidth()-LLPANEL_BORDER_WIDTH,
								tab_panel_bottom );
	}
	child->setFollowsAll();
	child->translate( tab_panel_rect.mLeft - child->getRect().mLeft, tab_panel_rect.mBottom - child->getRect().mBottom);
	child->reshape( tab_panel_rect.getWidth(), tab_panel_rect.getHeight(), TRUE );
	child->setVisible( FALSE );
	mTotalTabWidth += button_width;
	LLRect btn_rect;
	std::string tab_img;
	std::string tab_selected_img;
	S32 tab_fudge = 1;
	if (mIsVertical)
	{
		btn_rect.setLeftTopAndSize(TABCNTRV_PAD + LLPANEL_BORDER_WIDTH + 2,
								   (getRect().getHeight() - getTopBorderHeight() - LLPANEL_BORDER_WIDTH - 1) - ((TABCNTRV_TAB_HEIGHT + TABCNTRV_PAD) * getTabCount()),
								   mMinTabWidth,
								   TABCNTRV_TAB_HEIGHT);
	}
	else if( getTabPosition() == LLTabContainer::TOP )
	{
		btn_rect.setLeftTopAndSize( 0, getRect().getHeight() - getTopBorderHeight() + tab_fudge, button_width, TABCNTR_TAB_HEIGHT );
		tab_img = "tab_top_blue.tga";
		tab_selected_img = "tab_top_selected_blue.tga";
	}
	else
	{
		btn_rect.setOriginAndSize( 0, 0 + tab_fudge, button_width, TABCNTR_TAB_HEIGHT );
		tab_img = "tab_bottom_blue.tga";
		tab_selected_img = "tab_bottom_selected_blue.tga";
	}
	LLTextBox* textbox = NULL;
	LLButton* btn = NULL;
	if (placeholder)
	{
		btn_rect.translate(0, -LLBUTTON_V_PAD-2);
		textbox = new LLTextBox(trimmed_label, btn_rect, trimmed_label, font);
		textbox->setHAlign(mIsVertical ? LLFontGL::LEFT : LLFontGL::HCENTER);
		btn = new LLButton(LLStringUtil::null);
	}
	else
	{
		if (mIsVertical)
		{
			btn = new LLButton(std::string("vert tab button"),
							   btn_rect,
							   LLStringUtil::null,
							   LLStringUtil::null,
							   LLStringUtil::null,
							   NULL,
							   font,
							   trimmed_label, trimmed_label);
			btn->setImages(std::string("tab_left.tga"), std::string("tab_left_selected.tga"));
			btn->setImageHoverUnselected(LLUI::getUIImage("tab_left_hover.tga"));
			btn->setImageHoverSelected(LLUI::getUIImage("tab_left_selected_hover.tga"));
			btn->setScaleImage(TRUE);
			btn->setHAlign(LLFontGL::LEFT);
			btn->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
			btn->setTabStop(FALSE);
			btn->setLeftHPad(4);
			btn->setRightHPad(4);
			btn->setHoverGlowStrength(0.16f);
			btn->setUnselectedLabelColor(LLColor4(0.69f, 0.71f, 0.82f, 1.f));
			btn->setSelectedLabelColor(LLColor4(1.f, 1.f, 1.f, 1.f));
			if (indent)
			{
				btn->setLeftHPad(4 + indent);
			}
		}
		else
		{
			btn = new LLButton(std::string(child->getName()) + " tab",
							   btn_rect,
							   LLStringUtil::null, LLStringUtil::null, LLStringUtil::null,
							   NULL,
							   font,
							   trimmed_label, trimmed_label );
			btn->setVisible( FALSE );
			btn->setScaleImage(TRUE);
			btn->setImages(tab_img, tab_selected_img);
			btn->setLeftHPad(TABCNTR_TAB_H_PAD);
			btn->setRightHPad(TABCNTR_TAB_H_PAD);
			btn->setHAlign(LLFontGL::HCENTER);
			btn->setTabStop(FALSE);
			btn->setHoverGlowStrength(0.12f);
			btn->setUnselectedLabelColor(LLColor4(0.69f, 0.71f, 0.82f, 1.f));
			btn->setSelectedLabelColor(LLColor4(1.f, 1.f, 1.f, 1.f));
			if (indent)
			{
				btn->setLeftHPad(TABCNTR_TAB_H_PAD + indent);
				btn->setRightHPad(TABCNTR_TAB_H_PAD + indent);
			}
			if( getTabPosition() == TOP )
			{
				btn->setFollowsTop();
			}
			else
			{
				btn->setFollowsBottom();
			}
		}
		std::string tooltip = trimmed_label;
		LLStringUtil::format_map_t args;
		args["[ALT]"] = LLTrans::getString(
		"accel-win-alt"
		);
		tooltip += '\n' + LLTrans::getString("tab_tooltip_prev", args);
		tooltip += '\n' + LLTrans::getString("tab_tooltip_next", args);
		btn->setToolTip( tooltip );
	}
	LLTabTuple* tuple = new LLTabTuple( this, child, btn, textbox );
	insertTuple( tuple, insertion_point );
	if (!getTabsHidden())
	{
		if (textbox)
		{
			textbox->setSaveToXML(false);
			addChild( textbox, 0 );
		}
	}
	if (btn)
	{
		btn->setSaveToXML(false);
		btn->setClickedCallback(boost::bind(&LLTabContainer::onTabBtn, this, _2, child));
		addChild( btn, 0 );
	}
	if (child)
	{
		addChild(child, 1);
	}
	sendChildToFront(mPrevArrowBtn);
	sendChildToFront(mNextArrowBtn);
	sendChildToFront(mJumpPrevArrowBtn);
	sendChildToFront(mJumpNextArrowBtn);
	if( select )
	{
		selectLastTab();
	}
	updateMaxScrollPos();
}
void LLTabContainer::addPlaceholder(LLPanel* child, const std::string& label)
{
	addTabPanel(child, label, FALSE, 0, TRUE);
}
void LLTabContainer::removeTabPanel(LLPanel* child)
{
	if (mIsVertical)
	{
		S32 tab_count = 0;
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			LLRect rect;
			rect.setLeftTopAndSize(TABCNTRV_PAD + LLPANEL_BORDER_WIDTH + 2,
								   (getRect().getHeight() - LLPANEL_BORDER_WIDTH - 1) - ((TABCNTRV_TAB_HEIGHT + TABCNTRV_PAD) * (tab_count)),
								   mMinTabWidth,
								   TABCNTRV_TAB_HEIGHT);
			if (tuple->mPlaceholderText)
			{
				tuple->mPlaceholderText->setRect(rect);
			}
			else
			{
				tuple->mButton->setRect(rect);
			}
			tab_count++;
		}
	}
	else
	{
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			if( tuple->mTabPanel == child )
			{
				mTotalTabWidth -= tuple->mButton->getRect().getWidth();
				break;
			}
		}
	}
	BOOL has_focus = gFocusMgr.childHasKeyboardFocus(this);
	for(std::vector<LLTabTuple*>::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if( tuple->mTabPanel == child )
		{
 			removeChild( tuple->mButton );
 			delete tuple->mButton;
 			removeChild( tuple->mTabPanel );
			mTabList.erase( iter );
			delete tuple;
			break;
		}
	}
	mLockedTabCount = llmin(getTabCount(), mLockedTabCount);
	if (mCurrentTabIdx >= (S32)mTabList.size())
	{
		mCurrentTabIdx = mTabList.size()-1;
	}
	selectTab(mCurrentTabIdx);
	if (has_focus)
	{
		LLPanel* panelp = getPanelByIndex(mCurrentTabIdx);
		if (panelp)
		{
			panelp->setFocus(TRUE);
		}
	}
	updateMaxScrollPos();
}
void LLTabContainer::lockTabs(S32 num_tabs)
{
	mLockedTabCount = num_tabs > 0 ? llmin(getTabCount(), num_tabs) : getTabCount();
}
void LLTabContainer::unlockTabs()
{
	mLockedTabCount = 0;
}
void LLTabContainer::enableTabButton(S32 which, BOOL enable)
{
	if (which >= 0 && which < (S32)mTabList.size())
	{
		mTabList[which]->mButton->setEnabled(enable);
	}
	mDragAndDropDelayTimer.stop();
}
void LLTabContainer::deleteAllTabs()
{
	for(std::vector<LLTabTuple*>::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		removeChild( tuple->mButton );
		delete tuple->mButton;
 		removeChild( tuple->mTabPanel );
	}
	std::for_each(mTabList.begin(), mTabList.end(), DeletePointer());
	mTabList.clear();
	mCurrentTabIdx = -1;
}
LLPanel* LLTabContainer::getCurrentPanel()
{
	if (mCurrentTabIdx >= 0 && mCurrentTabIdx < (S32) mTabList.size())
	{
		return mTabList[mCurrentTabIdx]->mTabPanel;
	}
	return NULL;
}
S32 LLTabContainer::getCurrentPanelIndex()
{
	return mCurrentTabIdx;
}
S32 LLTabContainer::getTabCount()
{
	return mTabList.size();
}
LLPanel* LLTabContainer::getPanelByIndex(S32 index)
{
	if (index >= 0 && index < (S32)mTabList.size())
	{
		return mTabList[index]->mTabPanel;
	}
	return NULL;
}
S32 LLTabContainer::getIndexForPanel(LLPanel* panel)
{
	for (S32 index = 0; index < (S32)mTabList.size(); index++)
	{
		if (mTabList[index]->mTabPanel == panel)
		{
			return index;
		}
	}
	return -1;
}
S32 LLTabContainer::getPanelIndexByTitle(const std::string& title)
{
	for (S32 index = 0 ; index < (S32)mTabList.size(); index++)
	{
		if (title == mTabList[index]->mButton->getLabelSelected())
		{
			return index;
		}
	}
	return -1;
}
LLPanel *LLTabContainer::getPanelByName(const std::string& name)
{
	for (S32 index = 0 ; index < (S32)mTabList.size(); index++)
	{
		LLPanel *panel = mTabList[index]->mTabPanel;
		if (name == panel->getName())
		{
			return panel;
		}
	}
	return NULL;
}
void LLTabContainer::setCurrentTabName(const std::string& name)
{
	if (mCurrentTabIdx < 0) return;
	mTabList[mCurrentTabIdx]->mButton->setLabelSelected(name);
	mTabList[mCurrentTabIdx]->mButton->setLabelUnselected(name);
}
void LLTabContainer::selectFirstTab()
{
	selectTab( 0 );
}
void LLTabContainer::selectLastTab()
{
	selectTab( mTabList.size()-1 );
}
void LLTabContainer::selectNextTab()
{
	BOOL tab_has_focus = FALSE;
	if (mCurrentTabIdx >= 0 && mTabList[mCurrentTabIdx]->mButton->hasFocus())
	{
		tab_has_focus = TRUE;
	}
	S32 idx = mCurrentTabIdx+1;
	if (idx >= (S32)mTabList.size())
		idx = 0;
	while (!selectTab(idx) && idx != mCurrentTabIdx)
	{
		idx = (idx + 1 ) % (S32)mTabList.size();
	}
	if (tab_has_focus)
	{
		mTabList[idx]->mButton->setFocus(TRUE);
	}
}
void LLTabContainer::selectPrevTab()
{
	BOOL tab_has_focus = FALSE;
	if (mCurrentTabIdx >= 0 && mTabList[mCurrentTabIdx]->mButton->hasFocus())
	{
		tab_has_focus = TRUE;
	}
	S32 idx = mCurrentTabIdx-1;
	if (idx < 0)
		idx = mTabList.size()-1;
	while (!selectTab(idx) && idx != mCurrentTabIdx)
	{
		idx = idx - 1;
		if (idx < 0)
			idx = mTabList.size()-1;
	}
	if (tab_has_focus)
	{
		mTabList[idx]->mButton->setFocus(TRUE);
	}
}
BOOL LLTabContainer::selectTabPanel(LLPanel* child)
{
	S32 idx = 0;
	for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if( tuple->mTabPanel == child )
		{
			return selectTab( idx );
		}
		idx++;
	}
	return FALSE;
}
BOOL LLTabContainer::selectTab(S32 which)
{
	if (which >= getTabCount() || which < 0)
		return FALSE;
	LLTabTuple* selected_tuple = getTab(which);
	if (!selected_tuple)
	{
		return FALSE;
	}
	if (!selected_tuple->mVisible)
	{
		return FALSE;
	}
	LLSD cbdata;
	if (selected_tuple->mTabPanel)
		cbdata = selected_tuple->mTabPanel->getName();
	BOOL res = FALSE;
	if( !mValidateSignal || (*mValidateSignal)( this, cbdata ) )
	{
		res = setTab(which);
		if (res && mCommitSignal)
		{
			(*mCommitSignal)(this, cbdata);
		}
	}
	return res;
}
BOOL LLTabContainer::setTab(S32 which)
{
	LLTabTuple* selected_tuple = getTab(which);
	if (!selected_tuple)
	{
		return FALSE;
	}
	BOOL is_visible = FALSE;
	if (selected_tuple->mButton->getEnabled())
	{
		setCurrentPanelIndex(which);
		S32 i = 0;
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			BOOL is_selected = ( tuple == selected_tuple );
			tuple->mTabPanel->setVisible( is_selected );
			tuple->mButton->setToggleState( is_selected );
			tuple->mButton->setTabStop( is_selected );
			if (is_selected)
			{
				if (mIsVertical)
				{
					if (getMaxScrollPos() > 0)
					{
						if( i < getScrollPos() )
						{
							setScrollPos(i);
						}
						else
						{
							S32 available_height_with_arrows = getRect().getHeight() - getTopBorderHeight() - (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE + TABCNTR_ARROW_BTN_SIZE + 1);
							S32 min_scroll_pos = llmax(i-llfloor(((F32)available_height_with_arrows)/((F32)TABCNTRV_TAB_HEIGHT))+1,0);
							setScrollPos(llclamp(getScrollPos(), min_scroll_pos, i));
							setScrollPos(llmin(getScrollPos(), getMaxScrollPos()));
						}
						is_visible = TRUE;
					}
					else
					{
						is_visible = TRUE;
					}
				}
				else if (getMaxScrollPos() > 0)
				{
					if( i < getScrollPos() )
					{
						setScrollPos(i);
					}
					else
					{
						S32 available_width_with_arrows = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE  + TABCNTR_ARROW_BTN_SIZE + 1);
						S32 running_tab_width = tuple->mButton->getRect().getWidth();
						S32 j = i - 1;
						S32 min_scroll_pos = i;
						if (running_tab_width < available_width_with_arrows)
						{
							while (j >= 0)
							{
								LLTabTuple* other_tuple = getTab(j);
								running_tab_width += other_tuple->mButton->getRect().getWidth();
								if (running_tab_width > available_width_with_arrows)
								{
									break;
								}
								j--;
							}
							min_scroll_pos = j + 1;
						}
						setScrollPos(llclamp(getScrollPos(), min_scroll_pos, i));
						setScrollPos(llmin(getScrollPos(), getMaxScrollPos()));
					}
					is_visible = TRUE;
				}
				else
				{
					is_visible = TRUE;
				}
			}
			i++;
		}
	}
	return is_visible;
}
BOOL LLTabContainer::selectTabByName(const std::string& name)
{
	LLPanel* panel = getPanelByName(name);
	if (!panel)
	{
		LL_WARNS() << "LLTabContainer::selectTabByName("
			<< name << ") failed" << LL_ENDL;
		return FALSE;
	}
	BOOL result = selectTabPanel(panel);
	return result;
}
BOOL LLTabContainer::getTabPanelFlashing(LLPanel *child)
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		return tuple->mButton->getFlashing();
	}
	return FALSE;
}
void LLTabContainer::setTabPanelFlashing(LLPanel* child, BOOL state )
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		tuple->mButton->setFlashing( state, TRUE );
	}
}
void LLTabContainer::setTabUnreadCount(LLPanel* child, S32 count)
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		tuple->mButton->setUnreadCount(count);
		if (count > 0)
		{
			tuple->mButton->setFlashing(TRUE, TRUE);
		}
	}
}
void LLTabContainer::setTabImage(LLPanel* child, std::string image_name, const LLColor4& color)
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		tuple->mButton->setImageOverlay(image_name, mIsVertical ? LLFontGL::LEFT : LLFontGL::RIGHT, color);
		if (mIsVertical)
		{
			tuple->mButton->setHAlign(LLFontGL::LEFT);
			tuple->mButton->setLeftHPad(4);
		}
		reshapeTuple(tuple);
	}
}
void LLTabContainer::setTabImage(LLPanel* child, const LLUUID& image_id, const LLColor4& color)
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		tuple->mButton->setImageOverlay(image_id, LLFontGL::LEFT, color);
		if (mIsVertical)
		{
			tuple->mButton->setHAlign(LLFontGL::LEFT);
			tuple->mButton->setLeftHPad(4);
		}
		reshapeTuple(tuple);
	}
}
void LLTabContainer::reshapeTuple(LLTabTuple* tuple)
{
	if (!mIsVertical)
	{
		const LLFontGL* fontp = LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF_SMALL );
		S32 image_overlay_width = 0;
		image_overlay_width = tuple->mButton->getImageOverlay().notNull() ?
				tuple->mButton->getImageOverlay()->getImage()->getWidth(0) : 0;
		mTotalTabWidth -= tuple->mButton->getRect().getWidth();
		tuple->mPadding = image_overlay_width;
		tuple->mButton->setLeftHPad(TABCNTR_TAB_H_PAD);
		tuple->mButton->setRightHPad(TABCNTR_TAB_H_PAD);
		tuple->mButton->reshape(llclamp(fontp->getWidth(tuple->mButton->getLabelSelected()) + TAB_PADDING + tuple->mPadding, mMinTabWidth, mMaxTabWidth),
									tuple->mButton->getRect().getHeight());
		mTotalTabWidth += tuple->mButton->getRect().getWidth();
		updateMaxScrollPos();
	}
}
void LLTabContainer::setTitle(const std::string& title)
{
	if (mTitleBox)
	{
		mTitleBox->setText( title );
	}
}
const std::string LLTabContainer::getPanelTitle(S32 index)
{
	if (index >= 0 && index < (S32)mTabList.size())
	{
		LLButton* tab_button = mTabList[index]->mButton;
		return tab_button->getLabelSelected();
	}
	return LLStringUtil::null;
}
void LLTabContainer::setTopBorderHeight(S32 height)
{
	mTopBorderHeight = height;
}
S32 LLTabContainer::getTopBorderHeight() const
{
	return mTopBorderHeight;
}
S32 LLTabContainer::getTabStripBottom() const
{
	S32 strip_bottom = getRect().getHeight() - TABCNTR_TAB_HEIGHT;
	bool found = false;
	for (tuple_list_t::const_iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLButton* btn = (*iter)->mButton;
		if (!btn)
		{
			continue;
		}
		if (!found)
		{
			strip_bottom = btn->getRect().mBottom;
			found = true;
		}
		else
		{
			strip_bottom = llmin(strip_bottom, btn->getRect().mBottom);
		}
	}
	return found ? strip_bottom : (getRect().getHeight() - TABCNTR_TAB_HEIGHT);
}
void LLTabContainer::setRightTabBtnOffset(S32 offset)
{
	mNextArrowBtn->translate( -offset - mRightTabBtnOffset, 0 );
	mRightTabBtnOffset = offset;
	updateMaxScrollPos();
}
S32 LLTabContainer::getVerticalSplitterX() const
{
	return mMinTabWidth + mRightTabBtnOffset + (LLPANEL_BORDER_WIDTH * 2) + TABCNTRV_PAD;
}
S32 LLTabContainer::clampVerticalTabWidth(S32 width) const
{
	const S32 max_for_content = llmax(TABCNTRV_USER_MIN_WIDTH,
		getRect().getWidth() - TABCNTRV_CONTENT_MIN_WIDTH);
	const S32 max_w = llmin(TABCNTRV_USER_MAX_WIDTH, max_for_content);
	return llclamp(width, TABCNTRV_USER_MIN_WIDTH, llmax(TABCNTRV_USER_MIN_WIDTH, max_w));
}
void LLTabContainer::setVerticalTabWidth(S32 width)
{
	if (!mIsVertical)
	{
		return;
	}
	width = clampVerticalTabWidth(width);
	mMinTabWidth = width;
	for (tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if (tuple->mButton)
		{
			const LLRect& r = tuple->mButton->getRect();
			tuple->mButton->reshape(mMinTabWidth, r.getHeight());
		}
		if (tuple->mPlaceholderText)
		{
			const LLRect& r = tuple->mPlaceholderText->getRect();
			tuple->mPlaceholderText->reshape(mMinTabWidth, r.getHeight());
		}
	}
	LLRect tab_panel_rect(
		mMinTabWidth + mRightTabBtnOffset + (LLPANEL_BORDER_WIDTH * 2) + TABCNTRV_PAD,
		getRect().getHeight() - LLPANEL_BORDER_WIDTH,
		getRect().getWidth() - LLPANEL_BORDER_WIDTH,
		LLPANEL_BORDER_WIDTH);
	for (tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLPanel* child = (*iter)->mTabPanel;
		if (!child)
		{
			continue;
		}
		child->translate(tab_panel_rect.mLeft - child->getRect().mLeft,
						 tab_panel_rect.mBottom - child->getRect().mBottom);
		child->reshape(tab_panel_rect.getWidth(), tab_panel_rect.getHeight(), TRUE);
	}
	if (mPrevArrowBtn)
	{
		const LLRect& r = mPrevArrowBtn->getRect();
		mPrevArrowBtn->translate((mMinTabWidth / 2) - r.mLeft, 0);
	}
	if (mNextArrowBtn)
	{
		const LLRect& r = mNextArrowBtn->getRect();
		mNextArrowBtn->translate((mMinTabWidth / 2) - r.mLeft, 0);
	}
	updateMaxScrollPos();
}
bool LLTabContainer::isOverVerticalSplitter(S32 x, S32 y) const
{
	if (!mIsVertical || !mUserResize || getTabsHidden())
	{
		return false;
	}
	const S32 split = getVerticalSplitterX();
	if (x < split - TABCNTRV_SPLITTER_PAD || x > split + TABCNTRV_SPLITTER_PAD)
	{
		return false;
	}
	if (y < LLPANEL_BORDER_WIDTH || y > getRect().getHeight() - LLPANEL_BORDER_WIDTH)
	{
		return false;
	}
	return true;
}
void LLTabContainer::drawVerticalSplitter()
{
	if (!mIsVertical || !mUserResize || getTabsHidden())
	{
		return;
	}
	const S32 x = getVerticalSplitterX();
	const S32 top = getRect().getHeight() - LLPANEL_BORDER_WIDTH - 2;
	const S32 bottom = LLPANEL_BORDER_WIDTH + 2;
	const bool hot = mResizingTabs || mHoverSplitter;
	const F32 alpha = hot ? 0.90f : 0.38f;
	const LLColor4 line(0.62f, 0.66f, 0.78f, alpha);
	gl_line_2d(x, bottom, x, top, line);
	const LLFontGL* font = LLFontGL::getFontSansSerifSmall();
	if (!font)
	{
		return;
	}
	static const std::string glyph("<->");
	const S32 gw = font->getWidth(glyph);
	const S32 gh = font->getLineHeight();
	const S32 gx = x - (gw / 2);
	const S32 gy = ((top + bottom) / 2) - (gh / 2);
	const LLColor4 chip(0.14f, 0.16f, 0.22f, hot ? 0.78f : 0.42f);
	gl_rect_2d(gx - 4, gy + gh + 2, gx + gw + 4, gy - 3, chip);
	font->renderUTF8(glyph, 0, gx, gy,
		LLColor4(0.80f, 0.84f, 0.94f, hot ? 1.f : 0.72f),
		LLFontGL::LEFT, LLFontGL::BOTTOM);
}
void LLTabContainer::setPanelTitle(S32 index, const std::string& title)
{
	if (index >= 0 && index < getTabCount())
	{
		LLTabTuple* tuple = getTab(index);
		LLButton* tab_button = tuple->mButton;
		if (!mIsVertical)
		{
			const LLFontGL* fontp = LLFontGL::getFontSansSerifSmall();
			mTotalTabWidth -= tab_button->getRect().getWidth();
			tab_button->reshape(llclamp(fontp->getWidth(title)
			                                + TAB_PADDING
			                                + tuple->mPadding,
			                            mMinTabWidth,
			                            mMaxTabWidth),
			                    tab_button->getRect().getHeight());
			mTotalTabWidth += tab_button->getRect().getWidth();
		}
		tab_button->setLabelSelected(title);
		tab_button->setLabelUnselected(title);
	}
	updateMaxScrollPos();
}
void LLTabContainer::onTabBtn( const LLSD& data, LLPanel* panel )
{
	LLTabTuple* tuple = getTabByPanel(panel);
	selectTabPanel( panel );
	if (tuple)
	{
		tuple->mTabPanel->setFocus(TRUE);
	}
}
void LLTabContainer::onNextBtn( const LLSD& data )
{
	if (!mScrolled)
	{
		scrollNext();
	}
	mScrolled = FALSE;
}
void LLTabContainer::onNextBtnHeld( const LLSD& data )
{
	if (mScrollTimer.getElapsedTimeF32() > SCROLL_STEP_TIME)
	{
		mScrollTimer.reset();
		scrollNext();
		mScrolled = TRUE;
	}
}
void LLTabContainer::onPrevBtn( const LLSD& data )
{
	if (!mScrolled)
	{
		scrollPrev();
	}
	mScrolled = FALSE;
}
void LLTabContainer::onJumpFirstBtn( const LLSD& data )
{
	mScrollPos = 0;
}
void LLTabContainer::onJumpLastBtn( const LLSD& data )
{
	mScrollPos = mMaxScrollPos;
}
void LLTabContainer::onPrevBtnHeld( const LLSD& data )
{
	if (mScrollTimer.getElapsedTimeF32() > SCROLL_STEP_TIME)
	{
		mScrollTimer.reset();
		scrollPrev();
		mScrolled = TRUE;
	}
}
LLView* LLTabContainer::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	std::string name("tab_container");
	node->getAttributeString("name", name);
	bool is_vertical = false;
	LLTabContainer::TabPosition tab_position = LLTabContainer::TOP;
	if (node->hasAttribute("tab_position"))
	{
		std::string tab_position_string;
		node->getAttributeString("tab_position", tab_position_string);
		LLStringUtil::toLower(tab_position_string);
		if ("top" == tab_position_string)
		{
			tab_position = LLTabContainer::TOP;
			is_vertical = false;
		}
		else if ("bottom" == tab_position_string)
		{
			tab_position = LLTabContainer::BOTTOM;
			is_vertical = false;
		}
		else if ("left" == tab_position_string)
		{
			is_vertical = true;
		}
	}
	BOOL border = FALSE;
	node->getAttributeBOOL("border", border);
	LLTabContainer*	tab_container = new LLTabContainer(name, LLRect::null, tab_position, border, is_vertical);
	S32 tab_min_width = tab_container->mMinTabWidth;
	if (node->hasAttribute("tab_width"))
	{
		node->getAttributeS32("tab_width", tab_min_width);
	}
	else if( node->hasAttribute("tab_min_width"))
	{
		node->getAttributeS32("tab_min_width", tab_min_width);
	}
	S32	tab_max_width = tab_container->mMaxTabWidth;
	if (node->hasAttribute("tab_max_width"))
	{
		node->getAttributeS32("tab_max_width", tab_max_width);
	}
	tab_container->setMinTabWidth(tab_min_width);
	tab_container->setMaxTabWidth(tab_max_width);
	BOOL user_resize = FALSE;
	if (node->hasAttribute("user_resize"))
	{
		node->getAttributeBOOL("user_resize", user_resize);
	}
	tab_container->setUserResize(user_resize);
	BOOL hidden(tab_container->getTabsHidden());
	node->getAttributeBOOL("hide_tabs", hidden);
	tab_container->setTabsHidden(hidden);
	tab_container->setPanelParameters(node, parent);
	if (LLFloater::getFloaterHost())
	{
		LLFloater::getFloaterHost()->setTabContainer(tab_container);
	}
	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		LLView *control = factory->createCtrlWidget(tab_container, child);
		if (control && control->isPanel())
		{
			LLPanel* panelp = (LLPanel*)control;
			std::string label;
			child->getAttributeString("label", label);
			if (label.empty())
			{
				label = panelp->getLabel();
			}
			BOOL placeholder = FALSE;
			child->getAttributeBOOL("placeholder", placeholder);
			tab_container->addTabPanel(panelp, label, false, 0, placeholder);
		}
	}
	tab_container->selectFirstTab();
	tab_container->postBuild();
	tab_container->initButtons();
	return tab_container;
}
void LLTabContainer::initButtons()
{
	if (getRect().getHeight() == 0 || mPrevArrowBtn)
	{
		return;
	}
	std::string out_id;
	std::string in_id;
	if (mIsVertical)
	{
		S32 btn_top = getRect().getHeight();
		S32 btn_top_lower = TABCNTRV_ARROW_BTN_SIZE;
		LLRect up_arrow_btn_rect;
		up_arrow_btn_rect.setLeftTopAndSize( mMinTabWidth/2 , btn_top, TABCNTRV_ARROW_BTN_SIZE, TABCNTRV_ARROW_BTN_SIZE );
		LLRect down_arrow_btn_rect;
		down_arrow_btn_rect.setLeftTopAndSize( mMinTabWidth/2 , btn_top_lower, TABCNTRV_ARROW_BTN_SIZE, TABCNTRV_ARROW_BTN_SIZE );
		out_id = "UIImgBtnScrollUpOutUUID";
		in_id = "UIImgBtnScrollUpInUUID";
		mPrevArrowBtn = new LLButton(std::string("Up Arrow"), up_arrow_btn_rect,
									 out_id, in_id, LLStringUtil::null, boost::bind(&LLTabContainer::onPrevBtn,this,_2));
		mPrevArrowBtn->setFollowsTop();
		mPrevArrowBtn->setFollowsLeft();
		mPrevArrowBtn->setHeldDownCallback(boost::bind(&LLTabContainer::onPrevBtnHeld, this, _2));
		out_id = "UIImgBtnScrollDownOutUUID";
		in_id = "UIImgBtnScrollDownInUUID";
		mNextArrowBtn = new LLButton(std::string("Down Arrow"), down_arrow_btn_rect,
									 out_id, in_id, LLStringUtil::null, boost::bind(&LLTabContainer::onNextBtn,this,_2) );
		mNextArrowBtn->setFollowsBottom();
		mNextArrowBtn->setFollowsLeft();
		mNextArrowBtn->setHeldDownCallback(boost::bind(&LLTabContainer::onNextBtnHeld, this, _2));
	}
	else
	{
		S32 arrow_fudge = 1;
		if (getTabPosition() == BOTTOM)
		{
			mRightTabBtnOffset = RESIZE_HANDLE_WIDTH;
		}
		S32 btn_top = (getTabPosition() == TOP ) ? getRect().getHeight() - getTopBorderHeight() : TABCNTR_ARROW_BTN_SIZE + 1;
		LLRect left_arrow_btn_rect;
		left_arrow_btn_rect.setLeftTopAndSize( LLPANEL_BORDER_WIDTH+1+TABCNTR_ARROW_BTN_SIZE, btn_top + arrow_fudge, TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );
		LLRect jump_left_arrow_btn_rect;
		jump_left_arrow_btn_rect.setLeftTopAndSize( LLPANEL_BORDER_WIDTH+1, btn_top + arrow_fudge, TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );
		S32 right_pad = TABCNTR_ARROW_BTN_SIZE + LLPANEL_BORDER_WIDTH + 1;
		LLRect right_arrow_btn_rect;
		right_arrow_btn_rect.setLeftTopAndSize( getRect().getWidth() - mRightTabBtnOffset - right_pad - TABCNTR_ARROW_BTN_SIZE,
												btn_top + arrow_fudge,
												TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );
		LLRect jump_right_arrow_btn_rect;
		jump_right_arrow_btn_rect.setLeftTopAndSize( getRect().getWidth() - mRightTabBtnOffset - right_pad,
													 btn_top + arrow_fudge,
													 TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );
		out_id = "UIImgBtnJumpLeftOutUUID";
		in_id = "UIImgBtnJumpLeftInUUID";
		mJumpPrevArrowBtn = new LLButton(std::string("Jump Left Arrow"), jump_left_arrow_btn_rect,
										 out_id, in_id, LLStringUtil::null,
										boost::bind(&LLTabContainer::onJumpFirstBtn, this, _2), LLFontGL::getFontSansSerif() );
		mJumpPrevArrowBtn->setFollowsLeft();
		out_id = "UIImgBtnScrollLeftOutUUID";
		in_id = "UIImgBtnScrollLeftInUUID";
		mPrevArrowBtn = new LLButton(std::string("Left Arrow"), left_arrow_btn_rect,
									 out_id, in_id, LLStringUtil::null,
									 boost::bind(&LLTabContainer::onPrevBtn, this, _2), LLFontGL::getFontSansSerif() );
		mPrevArrowBtn->setHeldDownCallback(boost::bind(&LLTabContainer::onPrevBtnHeld, this, _2));
		mPrevArrowBtn->setFollowsLeft();
		out_id = "UIImgBtnJumpRightOutUUID";
		in_id = "UIImgBtnJumpRightInUUID";
		mJumpNextArrowBtn = new LLButton(std::string("Jump Right Arrow"), jump_right_arrow_btn_rect,
										 out_id, in_id, LLStringUtil::null,
										 boost::bind(&LLTabContainer::onJumpLastBtn, this, _2), LLFontGL::getFontSansSerif());
		mJumpNextArrowBtn->setFollowsRight();
		out_id = "UIImgBtnScrollRightOutUUID";
		in_id = "UIImgBtnScrollRightInUUID";
		mNextArrowBtn = new LLButton(std::string("Right Arrow"), right_arrow_btn_rect,
									 out_id, in_id, LLStringUtil::null,
									 boost::bind(&LLTabContainer::onNextBtn, this, _2), LLFontGL::getFontSansSerif());
		mNextArrowBtn->setHeldDownCallback(boost::bind(&LLTabContainer::onNextBtnHeld, this, _2));
		mNextArrowBtn->setFollowsRight();
		if( getTabPosition() == TOP )
		{
			mNextArrowBtn->setFollowsTop();
			mPrevArrowBtn->setFollowsTop();
			mJumpPrevArrowBtn->setFollowsTop();
			mJumpNextArrowBtn->setFollowsTop();
		}
		else
		{
			mNextArrowBtn->setFollowsBottom();
			mPrevArrowBtn->setFollowsBottom();
			mJumpPrevArrowBtn->setFollowsBottom();
			mJumpNextArrowBtn->setFollowsBottom();
		}
	}
	mPrevArrowBtn->setSaveToXML(false);
	mPrevArrowBtn->setTabStop(FALSE);
	addChild(mPrevArrowBtn);
	mNextArrowBtn->setSaveToXML(false);
	mNextArrowBtn->setTabStop(FALSE);
	addChild(mNextArrowBtn);
	if (mJumpPrevArrowBtn)
	{
		mJumpPrevArrowBtn->setSaveToXML(false);
		mJumpPrevArrowBtn->setTabStop(FALSE);
		addChild(mJumpPrevArrowBtn);
	}
	if (mJumpNextArrowBtn)
	{
		mJumpNextArrowBtn->setSaveToXML(false);
		mJumpNextArrowBtn->setTabStop(FALSE);
		addChild(mJumpNextArrowBtn);
	}
	setDefaultTabGroup(1);
}
LLTabTuple* LLTabContainer::getTabByPanel(LLPanel* child)
{
	for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if( tuple->mTabPanel == child )
		{
			return tuple;
		}
	}
	return NULL;
}
void LLTabContainer::insertTuple(LLTabTuple * tuple, eInsertionPoint insertion_point)
{
	switch(insertion_point)
	{
	case START:
		mTabList.insert(mTabList.begin() + mLockedTabCount, tuple);
		break;
	case LEFT_OF_CURRENT:
		{
		tuple_list_t::iterator current_iter = mTabList.begin() + llmax(mLockedTabCount, mCurrentTabIdx);
		mTabList.insert(current_iter, tuple);
		}
		break;
	case RIGHT_OF_CURRENT:
		{
		tuple_list_t::iterator current_iter = mTabList.begin() + llmax(mLockedTabCount, mCurrentTabIdx + 1);
		mTabList.insert(current_iter, tuple);
		}
		break;
	case END:
	default:
		mTabList.push_back( tuple );
	}
}
void LLTabContainer::updateMaxScrollPos()
{
	BOOL no_scroll = TRUE;
	if (mIsVertical)
	{
		S32 tab_total_height = (TABCNTRV_TAB_HEIGHT + TABCNTRV_PAD) * getVisibleTabCount() + TABCNTRV_PAD;
		S32 available_height = getRect().getHeight() - getTopBorderHeight() - 2 * LLPANEL_BORDER_WIDTH;
		if( tab_total_height > available_height )
		{
			S32 available_height_with_arrows = getRect().getHeight() - getTopBorderHeight() - (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE + TABCNTR_ARROW_BTN_SIZE + 1);
			S32 additional_needed = tab_total_height - available_height_with_arrows;
			setMaxScrollPos((S32) ceil(additional_needed / float(TABCNTRV_TAB_HEIGHT + TABCNTRV_PAD) ) );
			no_scroll = FALSE;
		}
	}
	else
	{
		S32 tab_space = 0;
		S32 available_space = 0;
		tab_space = mTotalTabWidth;
		available_space = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_TAB_H_PAD);
		if( tab_space > available_space )
		{
			S32 available_width_with_arrows = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE  + TABCNTR_ARROW_BTN_SIZE + 1);
			available_width_with_arrows -= TABCNTR_TAB_PARTIAL_WIDTH;
			S32 running_tab_width = 0;
			setMaxScrollPos(getTabCount());
			for(tuple_list_t::reverse_iterator tab_it = mTabList.rbegin(); tab_it != mTabList.rend(); ++tab_it)
			{
				running_tab_width += (*tab_it)->mButton->getRect().getWidth();
				if (running_tab_width > available_width_with_arrows)
				{
					break;
				}
				setMaxScrollPos(getMaxScrollPos()-1);
			}
			setMaxScrollPos(llmin(getMaxScrollPos(), getTabCount() - 1));
			no_scroll = FALSE;
		}
	}
	if (no_scroll)
	{
		setMaxScrollPos(0);
		setScrollPos(0);
	}
	if (getScrollPos() > getMaxScrollPos())
	{
		setScrollPos(getMaxScrollPos());
	}
}
void LLTabContainer::commitHoveredButton(S32 x, S32 y)
{
	if (!getTabsHidden() && hasMouseCapture())
	{
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			S32 local_x = x - tuple->mButton->getRect().mLeft;
			S32 local_y = y - tuple->mButton->getRect().mBottom;
			if (tuple->mButton->pointInView(local_x, local_y) && tuple->mButton->getEnabled() && !tuple->mTabPanel->getVisible())
			{
				tuple->mButton->onCommit();
			}
		}
	}
}
S32 LLTabContainer::getTotalTabWidth() const
{
	return mTotalTabWidth;
}
S32 LLTabContainer::getVisibleTabCount() const
{
	S32 count = 0;
	for (tuple_list_t::const_iterator itr = mTabList.begin(); itr != mTabList.end(); ++itr)
	{
		if ((*itr)->mVisible)
		{
			++count;
		}
	}
	return count;
}
void LLTabContainer::setTabVisibility(LLPanel const* aPanel, BOOL aVisible)
{
	for (tuple_list_t::iterator itr = mTabList.begin(); itr != mTabList.end(); ++itr)
	{
		LLTabTuple* pTT = *itr;
		if (pTT->mTabPanel == aPanel)
		{
			pTT->mVisible = aVisible;
			break;
		}
	}
	BOOL foundTab = FALSE;
	S32 current = getCurrentPanelIndex();
	if (current >= 0 && current < (S32)mTabList.size() && mTabList[current]->mVisible)
	{
		foundTab = TRUE;
	}
	else
	{
		for (tuple_list_t::iterator itr = mTabList.begin(); itr != mTabList.end(); ++itr)
		{
			LLTabTuple* pTT = *itr;
			if (pTT->mVisible)
			{
				selectTab((S32)(itr - mTabList.begin()));
				foundTab = TRUE;
				break;
			}
		}
	}
	setVisible(foundTab);
	updateMaxScrollPos();
}
BOOL LLTabContainer::getTabVisibility(const LLPanel* panel) const
{
	for (tuple_list_t::const_iterator itr = mTabList.begin(); itr != mTabList.end(); ++itr)
	{
		LLTabTuple const* pTT = *itr;
		if (pTT->mTabPanel == panel)
		{
			return pTT->mVisible;
		}
	}
	return FALSE;
}
