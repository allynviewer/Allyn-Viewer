/** 
 * @file llfloatergroupinfo.cpp
 * @brief LLFloaterGroupInfo class implementation
 * Floater used both for display of group information and for
 * creating new groups.
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
#include "llviewerprecompiledheaders.h"
#include "llfloatergroupinfo.h"
#include "llcachename.h"
#include "llpanelgroup.h"
#include <sstream>
namespace
{
	const char FLOATER_TITLE[] = "Group Information";
	const S32 GROUP_INFO_WIDTH = 492;
	const S32 GROUP_INFO_HEIGHT = 560;
}
LLFloaterGroupInfo::LLFloaterGroupInfo(const LLUUID& group_id)
:	LLFloater(FLOATER_TITLE, LLRect(0, GROUP_INFO_HEIGHT, GROUP_INFO_WIDTH, 0), FLOATER_TITLE),
	LLInstanceTracker<LLFloaterGroupInfo, LLUUID>(group_id)
{
	mPanelGroupp = new LLPanelGroup(group_id);
	LLRect panel_rect(0, GROUP_INFO_HEIGHT - LLFLOATER_HEADER_SIZE, GROUP_INFO_WIDTH, 0);
	mPanelGroupp->setRect(panel_rect);
	mPanelGroupp->reshape(panel_rect.getWidth(), panel_rect.getHeight());
	mPanelGroupp->setFollowsAll();
	addChild(mPanelGroupp);
	setTitle(getGroupInfoTitle());
	setResizeLimits(GROUP_INFO_WIDTH, GROUP_INFO_HEIGHT);
	if (group_id.notNull())
	{
		gCacheName->get(group_id, true, boost::bind(&LLFloaterGroupInfo::callbackLoadGroupName, this, _2));
	}
}
LLFloaterGroupInfo::~LLFloaterGroupInfo()
{
}
BOOL LLFloaterGroupInfo::canClose()
{
	if ( mPanelGroupp )
	{
		return mPanelGroupp->canClose();
	}
	return TRUE;
}
void LLFloaterGroupInfo::selectTabByName(std::string tab_name)
{
	mPanelGroupp->selectTab(tab_name);
}
std::string LLFloaterGroupInfo::getGroupInfoTitle()
{
	if (mPanelGroupp && mPanelGroupp->hasString("floater_title"))
	{
		return mPanelGroupp->getString("floater_title");
	}
	return std::string(FLOATER_TITLE);
}
void LLFloaterGroupInfo::callbackLoadGroupName(const std::string& full_name)
{
	std::ostringstream title;
	title << full_name << " - " << getGroupInfoTitle();
	setTitle(title.str());
}
