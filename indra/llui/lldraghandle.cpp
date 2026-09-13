/** 
 * @file lldraghandle.cpp
 * @brief LLDragHandle base class
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
#include "lldraghandle.h"
#include "llmath.h"
#include "llui.h"
#include "llmenugl.h"
#include "lltextbox.h"
#include "llcontrol.h"
#include "llresmgr.h"
#include "llfontgl.h"
#include "llwindow.h"
#include "llfocusmgr.h"
const S32 LEADING_PAD = 6;
const S32 TITLE_PAD = 10;
const S32 CORNER_CLEAR = 16;
const S32 LEFT_PAD = CORNER_CLEAR + TITLE_PAD;
const S32 TITLE_ICON_SIZE = 16;
const S32 TITLE_ICON_GAP = 8;
const S32 RIGHT_PAD = CORNER_CLEAR + 52;
const S32 TITLE_BAR_HEIGHT = 26;
S32 LLDragHandle::sSnapMargin = 5;
LLDragHandle::LLDragHandle( const std::string& name, const LLRect& rect, const std::string& title )
:	LLView( name, rect, TRUE ),
	mDragLastScreenX( 0 ),
	mDragLastScreenY( 0 ),
	mLastMouseScreenX( 0 ),
	mLastMouseScreenY( 0 ),
	mDragHighlightColor(	LLUI::sColorsGroup->getColor( "DefaultHighlightLight" ) ),
	mDragShadowColor(		LLUI::sColorsGroup->getColor( "DefaultShadowDark" ) ),
	mTitleBox( NULL ),
	mMaxTitleWidth( 0 ),
	mForeground( TRUE )
{
	sSnapMargin = LLUI::sConfigGroup->getS32("SnapMargin");
	setSaveToXML(false);
}
void LLDragHandle::setTitleVisible(BOOL visible)
{
	if(mTitleBox)
	{
		mTitleBox->setVisible(visible);
	}
}
void LLDragHandle::setTitleBox(LLTextBox* titlebox)
{
	if( mTitleBox )
	{
		removeChild(mTitleBox);
		delete mTitleBox;
	}
	mTitleBox = titlebox;
	if(mTitleBox)
	{
		addChild( mTitleBox );
	}
}
void LLDragHandle::setTitleIcon(const std::string& image_name)
{
	mTitleIcon = image_name;
}
void LLDragHandleTop::setTitleIcon(const std::string& image_name)
{
	LLDragHandle::setTitleIcon(image_name);
	reshapeTitleBox();
}
LLDragHandleTop::LLDragHandleTop(const std::string& name, const LLRect &rect, const std::string& title)
:	LLDragHandle(name, rect, title)
{
	setFollowsAll();
	setTitle( title );
}
LLDragHandleLeft::LLDragHandleLeft(const std::string& name, const LLRect &rect, const std::string& title)
:	LLDragHandle(name, rect, title)
{
	setFollowsAll();
	setTitle( title );
}
void LLDragHandleTop::setTitle(const std::string& title)
{
	std::string trimmed_title = title;
	LLStringUtil::trim(trimmed_title);
	const LLFontGL* font = LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF );
	LLTextBox* titlebox = new LLTextBox( std::string("Drag Handle Title"), getRect(), trimmed_title, font );
	titlebox->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT | FOLLOWS_RIGHT);
	titlebox->setFontShadow(LLFontGL::NO_SHADOW);
	titlebox->setUseEllipses(TRUE);
	titlebox->setToolTip(trimmed_title);
	titlebox->setColor(LLUI::sColorsGroup->getColor("LabelTextColor"));
	setTitleBox(titlebox);
	reshapeTitleBox();
}
const std::string& LLDragHandleTop::getTitle() const
{
	return getTitleBox() == NULL ? LLStringUtil::null : getTitleBox()->getText();
}
void LLDragHandleLeft::setTitle(const std::string& )
{
	setTitleBox(NULL);
}
const std::string& LLDragHandleLeft::getTitle() const
{
	return LLStringUtil::null;
}
void LLDragHandleTop::draw()
{
	if (getTitleBox())
	{
		getTitleBox()->setEnabled(getForeground());
	}
	if (!getTitleIcon().empty())
	{
		LLUIImagePtr icon = LLUI::getUIImage(getTitleIcon());
		if (icon.notNull())
		{
			const S32 icon_bottom = getRect().getHeight()
				- ((TITLE_BAR_HEIGHT - TITLE_ICON_SIZE) / 2)
				- TITLE_ICON_SIZE;
			icon->draw(LEFT_PAD, icon_bottom, TITLE_ICON_SIZE, TITLE_ICON_SIZE);
		}
	}
	LLView::draw();
}
void LLDragHandleLeft::draw()
{
	if (getTitleBox())
	{
		getTitleBox()->setEnabled(getForeground());
	}
	LLView::draw();
}
void LLDragHandleTop::reshapeTitleBox()
{
	if( ! getTitleBox())
	{
		return;
	}
	const LLFontGL* font = LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF );
	S32 title_left = LEFT_PAD;
	if (!getTitleIcon().empty())
	{
		title_left += TITLE_ICON_SIZE + TITLE_ICON_GAP;
	}
	S32 right_limit = getRect().getWidth() - RIGHT_PAD;
	if (getMaxTitleWidth() > 0)
	{
		right_limit = llmin(right_limit, getMaxTitleWidth());
	}
	S32 title_width = llmax(16, right_limit - title_left);
	S32 title_height = ll_round(font->getLineHeight());
	const S32 title_top = getRect().getHeight() - ((TITLE_BAR_HEIGHT - title_height) / 2);
	LLRect title_rect;
	title_rect.setLeftTopAndSize(title_left, title_top, title_width, title_height);
	getTitleBox()->setShape(title_rect);
	getTitleBox()->setUseEllipses(TRUE);
}
void LLDragHandleTop::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape(width, height, called_from_parent);
	reshapeTitleBox();
}
void LLDragHandleLeft::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape(width, height, called_from_parent);
}
BOOL LLDragHandle::handleMouseDown(S32 x, S32 y, MASK mask)
{
	gFocusMgr.setMouseCapture(this);
	localPointToScreen(x, y, &mDragLastScreenX, &mDragLastScreenY);
	mLastMouseScreenX = mDragLastScreenX;
	mLastMouseScreenY = mDragLastScreenY;
	return TRUE;
}
BOOL LLDragHandle::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		gFocusMgr.setMouseCapture( NULL );
	}
	return TRUE;
}
void LLDragHandle::setTextColor(const LLColor4& color)
{
	if (mTitleBox)
	{
		mTitleBox->setColor(color);
	}
}
BOOL LLDragHandle::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;
	if( hasMouseCapture() )
	{
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y);
		S32 delta_x = screen_x - mDragLastScreenX;
		S32 delta_y = screen_y - mDragLastScreenY;
		LLRect original_rect = getParent()->getRect();
		LLRect translated_rect = getParent()->getRect();
		translated_rect.translate(delta_x, delta_y);
		getParent()->setRect(translated_rect);
		S32 pre_snap_x = getParent()->getRect().mLeft;
		S32 pre_snap_y = getParent()->getRect().mBottom;
		mDragLastScreenX = screen_x;
		mDragLastScreenY = screen_y;
		LLRect new_rect;
		LLCoordGL mouse_dir;
		mouse_dir.mX = (screen_x == mLastMouseScreenX) ? mLastMouseDir.mX : screen_x - mLastMouseScreenX;
		mouse_dir.mY = (screen_y == mLastMouseScreenY) ? mLastMouseDir.mY : screen_y - mLastMouseScreenY;
		mLastMouseDir = mouse_dir;
		mLastMouseScreenX = screen_x;
		mLastMouseScreenY = screen_y;
		LLView* snap_view = getParent()->findSnapRect(new_rect, mouse_dir, SNAP_PARENT_AND_SIBLINGS, sSnapMargin);
		getParent()->setSnappedTo(snap_view);
		delta_x = new_rect.mLeft - pre_snap_x;
		delta_y = new_rect.mBottom - pre_snap_y;
		translated_rect.translate(delta_x, delta_y);
		getParent()->setRect(original_rect);
		getParent()->setShape(translated_rect,true);
		mDragLastScreenX += delta_x;
		mDragLastScreenY += delta_y;
		getWindow()->setCursor(UI_CURSOR_ARROW);
		LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (active)" <<LL_ENDL;
		handled = TRUE;
	}
	else
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
		LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (inactive)" << LL_ENDL;
		handled = TRUE;
	}
	return handled;
}
void LLDragHandle::setValue(const LLSD& value)
{
	setTitle(value.asString());
}
