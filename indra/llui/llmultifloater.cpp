/** 
 * @file llmultifloater.cpp
 * @brief LLFloater that hosts other floaters
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "linden_common.h"
#include "llmultifloater.h"
#include "llresizehandle.h"
LLMultiFloater::LLMultiFloater() :
	mTabContainer(NULL),
	mTabPos(LLTabContainer::TOP),
	mAutoResize(TRUE),
	mOrigMinWidth(0),
	mOrigMinHeight(0)
{
}
LLMultiFloater::LLMultiFloater(LLTabContainer::TabPosition tab_pos) :
	mTabContainer(NULL),
	mTabPos(tab_pos),
	mAutoResize(TRUE),
	mOrigMinWidth(0),
	mOrigMinHeight(0)
{
}
LLMultiFloater::LLMultiFloater(const std::string &name) :
	LLFloater(name),
	mTabContainer(NULL),
	mTabPos(LLTabContainer::TOP),
	mAutoResize(FALSE),
	mOrigMinWidth(0),
	mOrigMinHeight(0)
{
}
LLMultiFloater::LLMultiFloater(
	const std::string& name,
	const LLRect& rect,
	LLTabContainer::TabPosition tab_pos,
	BOOL auto_resize) :
	LLFloater(name, rect, name),
	mTabContainer(NULL),
	mTabPos(LLTabContainer::TOP),
	mAutoResize(auto_resize),
	mOrigMinWidth(0),
	mOrigMinHeight(0)
{
	mTabContainer = new LLTabContainer(std::string("Preview Tabs"),
		LLRect(LLPANEL_BORDER_WIDTH, getRect().getHeight() - LLFLOATER_HEADER_SIZE, getRect().getWidth() - LLPANEL_BORDER_WIDTH, 0),
		mTabPos,
		FALSE,
		FALSE);
	mTabContainer->setFollowsAll();
	if (isResizable())
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}
	mTabContainer->setCommitCallback(boost::bind(&LLMultiFloater::onTabSelected, this));
	addChild(mTabContainer);
}
LLMultiFloater::LLMultiFloater(
	const std::string& name,
	const std::string& rect_control,
	LLTabContainer::TabPosition tab_pos,
	BOOL auto_resize) :
	LLFloater(name, rect_control, name),
	mTabContainer(NULL),
	mTabPos(tab_pos),
	mAutoResize(auto_resize),
	mOrigMinWidth(0),
	mOrigMinHeight(0)
{
	mTabContainer = new LLTabContainer(std::string("Preview Tabs"),
		LLRect(LLPANEL_BORDER_WIDTH, getRect().getHeight() - LLFLOATER_HEADER_SIZE, getRect().getWidth() - LLPANEL_BORDER_WIDTH, 0),
		mTabPos,
		FALSE,
		FALSE);
	mTabContainer->setFollowsAll();
	if (isResizable() && mTabPos == LLTabContainer::BOTTOM)
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}
	mTabContainer->setCommitCallback(boost::bind(&LLMultiFloater::onTabSelected, this));
	addChild(mTabContainer);
}
LLXMLNodePtr LLMultiFloater::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLFloater::getXML();
	node->setName(LL_MULTI_FLOATER_TAG);
	return node;
}
void LLMultiFloater::open()
{
	if (mTabContainer->getTabCount() > 0)
	{
		LLFloater::open();
	}
	else
	{
		close();
	}
}
void LLMultiFloater::onClose(bool app_quitting)
{
	if(closeAllFloaters() == TRUE)
	{
		LLFloater::onClose(app_quitting);
	}
}
void LLMultiFloater::draw()
{
	if (mTabContainer->getTabCount() == 0)
	{
		close();
	}
	else
	{
		for (S32 i = 0; i < mTabContainer->getTabCount(); i++)
		{
			LLFloater* floaterp = (LLFloater*)mTabContainer->getPanelByIndex(i);
			if (floaterp->getShortTitle() != mTabContainer->getPanelTitle(i))
			{
				mTabContainer->setPanelTitle(i, floaterp->getShortTitle());
			}
		}
		LLFloater::draw();
	}
}
BOOL LLMultiFloater::closeAllFloaters()
{
	S32	tabToClose = 0;
	S32	lastTabCount = mTabContainer->getTabCount();
	while (tabToClose < mTabContainer->getTabCount())
	{
		LLFloater* first_floater = (LLFloater*)mTabContainer->getPanelByIndex(tabToClose);
		first_floater->close();
		if(lastTabCount == mTabContainer->getTabCount())
		{
			tabToClose++;
		}
		else
		{
			lastTabCount = mTabContainer->getTabCount();
		}
	}
	if( mTabContainer->getTabCount() != 0 )
		return FALSE;
	return TRUE;
}
void LLMultiFloater::growToFit(S32 content_width, S32 content_height)
{
	S32 new_width = llmax(getRect().getWidth(), content_width + LLPANEL_BORDER_WIDTH * 2);
	S32 new_height = llmax(getRect().getHeight(), content_height + LLFLOATER_HEADER_SIZE + TABCNTR_HEADER_HEIGHT);
    if (isMinimized())
    {
        LLRect newrect;
        newrect.setLeftTopAndSize(getExpandedRect().mLeft, getExpandedRect().mTop, new_width, new_height);
        setExpandedRect(newrect);
    }
	else
	{
		S32 old_height = getRect().getHeight();
		reshape(new_width, new_height);
		translate(0, old_height - new_height);
	}
}
void LLMultiFloater::addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point)
{
	if (!floaterp)
	{
		return;
	}
	if (!mTabContainer)
	{
		LL_ERRS() << "Tab Container used without having been initialized." << LL_ENDL;
		return;
	}
	if (floaterp->getHost() == this)
	{
		mFloaterDataMap.erase(floaterp->getHandle());
		mTabContainer->removeTabPanel(floaterp);
	}
	else if (floaterp->getHost())
	{
		floaterp->getHost()->removeFloater(floaterp);
	}
	else if (floaterp->getParent() == gFloaterView)
	{
		gFloaterView->removeChild(floaterp);
	}
	LLFloaterData floater_data;
	floater_data.mWidth = floaterp->getRect().getWidth();
	floater_data.mHeight = floaterp->getRect().getHeight();
	floater_data.mCanMinimize = floaterp->isMinimizeable();
	floater_data.mCanResize = floaterp->isResizable();
	floaterp->setCanMinimize(FALSE);
	floaterp->setCanResize(FALSE);
	floaterp->setCanDrag(FALSE);
	floaterp->storeRectControl();
	floaterp->setBackgroundVisible(FALSE);
	if (mAutoResize)
	{
		growToFit(floater_data.mWidth, floater_data.mHeight);
	}
	mTabContainer->addTabPanel(floaterp, floaterp->getShortTitle(), FALSE, 0, FALSE, insertion_point);
	mFloaterDataMap[floaterp->getHandle()] = floater_data;
	updateResizeLimits();
	if ( select_added_floater )
	{
		mTabContainer->selectTabPanel(floaterp);
	}
	else
	{
		mTabContainer->selectTab(mTabContainer->getCurrentPanelIndex());
	}
	floaterp->setHost(this);
	if (isMinimized())
	{
		floaterp->setVisible(FALSE);
	}
	moveResizeHandlesToFront();
}
void LLMultiFloater::updateFloaterTitle(LLFloater* floaterp)
{
	S32 index = mTabContainer->getIndexForPanel(floaterp);
	if (index != -1)
	{
		mTabContainer->setPanelTitle(index, floaterp->getShortTitle());
	}
}
BOOL LLMultiFloater::selectFloater(LLFloater* floaterp)
{
	return mTabContainer->selectTabPanel(floaterp);
}
void LLMultiFloater::selectNextFloater()
{
	mTabContainer->selectNextTab();
}
void LLMultiFloater::selectPrevFloater()
{
	mTabContainer->selectPrevTab();
}
void LLMultiFloater::showFloater(LLFloater* floaterp, LLTabContainer::eInsertionPoint insertion_point)
{
	if(!floaterp) return;
	if (floaterp != mTabContainer->getCurrentPanel() &&
		!mTabContainer->selectTabPanel(floaterp))
	{
		addFloater(floaterp, TRUE, insertion_point);
	}
}
void LLMultiFloater::removeFloater(LLFloater* floaterp)
{
	if (!floaterp || floaterp->getHost() != this )
		return;
	floater_data_map_t::iterator found_data_it = mFloaterDataMap.find(floaterp->getHandle());
	if (found_data_it != mFloaterDataMap.end())
	{
		LLFloaterData& floater_data = found_data_it->second;
		floaterp->setCanMinimize(floater_data.mCanMinimize);
		if (!floater_data.mCanResize)
		{
			floaterp->reshape(floater_data.mWidth, floater_data.mHeight);
		}
		floaterp->setCanResize(floater_data.mCanResize);
		mFloaterDataMap.erase(found_data_it);
	}
	mTabContainer->removeTabPanel(floaterp);
	floaterp->setBackgroundVisible(TRUE);
	floaterp->setCanDrag(TRUE);
	floaterp->setHost(NULL);
	floaterp->applyRectControl();
	updateResizeLimits();
	tabOpen((LLFloater*)mTabContainer->getCurrentPanel());
}
void LLMultiFloater::tabOpen(LLFloater* opened_floater)
{
}
void LLMultiFloater::tabClose()
{
	if (mTabContainer->getTabCount() == 0)
	{
		close();
	}
}
void LLMultiFloater::setVisible(BOOL visible)
{
	LLFloater::setVisible(visible);
	if (mTabContainer)
	{
		LLPanel* cur_floaterp = mTabContainer->getCurrentPanel();
		if (cur_floaterp)
		{
			cur_floaterp->setVisible(visible);
		}
		if (visible && !cur_floaterp)
		{
			mTabContainer->selectLastTab();
		}
	}
}
BOOL LLMultiFloater::handleKeyHere(KEY key, MASK mask)
{
	if (key == 'W' && mask == MASK_CONTROL)
	{
		LLFloater* floater = getActiveFloater();
		if (floater && floater->canClose() && floater->isCloseable())
		{
			floater->close();
			if(mTabContainer->getTabCount() > 0)
			{
				mTabContainer->setFocus(TRUE);
			}
		}
		return TRUE;
	}
	return LLFloater::handleKeyHere(key, mask);
}
LLFloater* LLMultiFloater::getActiveFloater()
{
	return (LLFloater*)mTabContainer->getCurrentPanel();
}
S32	LLMultiFloater::getFloaterCount()
{
	return mTabContainer->getTabCount();
}
BOOL LLMultiFloater::isFloaterFlashing(LLFloater* floaterp)
{
	if ( floaterp && floaterp->getHost() == this )
		return mTabContainer->getTabPanelFlashing(floaterp);
	return FALSE;
}
void LLMultiFloater::setFloaterFlashing(LLFloater* floaterp, BOOL flashing)
{
	if ( floaterp && floaterp->getHost() == this )
		mTabContainer->setTabPanelFlashing(floaterp, flashing);
}
void LLMultiFloater::setFloaterUnreadCount(LLFloater* floaterp, S32 count)
{
	if ( floaterp && floaterp->getHost() == this )
		mTabContainer->setTabUnreadCount(floaterp, count);
}
void LLMultiFloater::onTabSelected()
{
	LLFloater* floaterp = dynamic_cast<LLFloater*>(mTabContainer->getCurrentPanel());
	if (floaterp)
	{
		tabOpen(floaterp);
	}
}
void LLMultiFloater::setCanResize(BOOL can_resize)
{
	LLFloater::setCanResize(can_resize);
	if (!mTabContainer) return;
	if (isResizable() && mTabContainer->getTabPosition() == LLTabContainer::BOTTOM)
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}
	else
	{
		mTabContainer->setRightTabBtnOffset(0);
	}
}
void LLMultiFloater::updateResizeLimits()
{
	S32 new_min_width = mOrigMinWidth;
	S32 new_min_height = mOrigMinHeight;
	computeResizeLimits(new_min_width, new_min_height);
	setResizeLimits(new_min_width, new_min_height);
	S32 cur_height = getRect().getHeight();
	S32 new_width = llmax(getRect().getWidth(), new_min_width);
	S32 new_height = llmax(getRect().getHeight(), new_min_height);
	if (isMinimized())
	{
		const LLRect& expanded = getExpandedRect();
		LLRect newrect;
		newrect.setLeftTopAndSize(expanded.mLeft, expanded.mTop, llmax(expanded.getWidth(), new_width), llmax(expanded.getHeight(), new_height));
		setExpandedRect(newrect);
	}
	else
	{
		reshape(new_width, new_height);
		translate(0, cur_height - getRect().getHeight());
		gFloaterView->adjustToFitScreen(this, TRUE);
	}
}
void LLMultiFloater::computeResizeLimits(S32& new_min_width, S32& new_min_height)
{
	for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
	{
		LLFloater* floaterp = (LLFloater*)mTabContainer->getPanelByIndex(tab_idx);
		if (floaterp)
		{
			new_min_width = llmax(new_min_width, floaterp->getMinWidth() + LLPANEL_BORDER_WIDTH * 2);
			new_min_height = llmax(new_min_height, floaterp->getMinHeight() + LLFLOATER_HEADER_SIZE + TABCNTR_HEADER_HEIGHT);
		}
	}
}
