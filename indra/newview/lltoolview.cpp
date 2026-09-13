/** 
 * @file lltoolview.cpp
 * @brief A UI contains for tool palette tools
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
#include "lltoolview.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llagent.h"
#include "llbutton.h"
#include "llpanel.h"
#include "lltool.h"
#include "lltoolmgr.h"
#include "lltextbox.h"
#include "llresmgr.h"
LLToolContainer::LLToolContainer(LLToolView* parent)
:	mParent(parent),
	mButton(NULL),
	mPanel(NULL),
	mTool(NULL)
{ }
LLToolContainer::~LLToolContainer()
{
	delete mTool;
	mTool = NULL;
}
LLToolView::LLToolView(const std::string& name, const LLRect& rect)
:	mButtonCount(0), LLView(name, rect, true)
{
}
LLToolView::~LLToolView()
{
	for_each(mContainList.begin(), mContainList.end(), DeletePointer());
	mContainList.clear();
}
LLRect LLToolView::getButtonRect(S32 button_index)
{
	const S32 HPAD = 7;
	const S32 VPAD = 7;
	const S32 TOOL_SIZE = 32;
	const S32 HORIZ_SPACING = TOOL_SIZE + 5;
	const S32 VERT_SPACING = TOOL_SIZE + 14;
	S32 tools_per_row = getRect().getWidth() / HORIZ_SPACING;
	S32 row = button_index / tools_per_row;
	S32 column = button_index % tools_per_row;
	LLRect rect;
	rect.setLeftTopAndSize( HPAD + (column * HORIZ_SPACING),
							-VPAD + getRect().getHeight() - (row * VERT_SPACING),
							TOOL_SIZE,
							TOOL_SIZE
							);
	return rect;
}
void LLToolView::draw()
{
	LLTool* selected = LLToolMgr::getInstance()->getCurrentToolset()->getSelectedTool();
	for (contain_list_t::iterator iter = mContainList.begin();
		 iter != mContainList.end(); ++iter)
	{
		LLToolContainer* contain = *iter;
		BOOL state = (contain->mTool == selected);
		contain->mButton->setToggleState( state );
		if (contain->mPanel)
		{
			contain->mPanel->setVisible( state );
		}
	}
	LLView::draw();
}
LLToolContainer* LLToolView::findToolContainer( LLTool *tool )
{
	llassert( tool );
	for (contain_list_t::iterator iter = mContainList.begin();
		 iter != mContainList.end(); ++iter)
	{
		LLToolContainer* contain = *iter;
		if( contain->mTool == tool )
		{
			return contain;
		}
	}
	LL_ERRS() << "LLToolView::findToolContainer - tool not found" << LL_ENDL;
	return NULL;
}
void LLToolView::onClickToolButton(void* userdata)
{
	LLToolContainer* clicked = (LLToolContainer*) userdata;
	LLToolMgr::getInstance()->getCurrentToolset()->selectTool( clicked->mTool );
}
