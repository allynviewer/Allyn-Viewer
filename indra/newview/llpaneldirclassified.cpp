/** 
 * @file llpaneldirclassified.cpp
 * @brief Classified panel in the Find directory.
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
#include "llpaneldirclassified.h"
#include "llclassifiedflags.h"
#include "llfontgl.h"
#include "message.h"
#include "llqueryflags.h"
#include "llagent.h"
#include "llbutton.h"
#include "llcontrol.h"
#include "llcombobox.h"
#include "llclassifiedinfo.h"
#include "lluiconstants.h"
#include "llpaneldirbrowser.h"
#include "lltextbox.h"
#include "llcheckboxctrl.h"
#include "llfloaterdirectory.h"
#include "lllineeditor.h"
#include "llpaneldirbrowser.h"
#include "lltextbox.h"
#include "llviewermessage.h"
#include "lluictrlfactory.h"
#include "llworldmap.h"
#include "llfloateravatarinfo.h"
#include "llviewermenu.h"
#include "llnotificationsutil.h"
LLPanelDirClassified::LLPanelDirClassified(const std::string& name, LLFloaterDirectory* floater)
:	LLPanelDirBrowser(name, floater)
{
}
BOOL LLPanelDirClassified::postBuild()
{
	LLPanelDirBrowser::postBuild();
	if (gAgent.wantsPGOnly())
	{
		childSetValue("incmature", FALSE);
		childSetValue("incadult", FALSE);
		childHide("incmature");
		childHide("incadult");
		childSetValue("incpg", TRUE);
		childDisable("incpg");
	}
	bool mature_enabled = gAgent.canAccessMature();
	if (!mature_enabled)
	{
		childSetValue("incmature", FALSE);
		childDisable("incmature");
	}
	bool adult_enabled = gAgent.canAccessAdult();
	if (!adult_enabled)
	{
		childSetValue("incadult", FALSE);
		childDisable("incadult");
	}
	getChild<LLLineEditor>("name")->setKeystrokeCallback(boost::bind(&LLPanelDirClassified::onKeystrokeNameClassified,this,_1));
	getChild<LLButton>("Search")->setClickedCallback(boost::bind(&LLPanelDirBrowser::onClickSearchCore,this));
	getChild<LLButton>("Browse")->setClickedCallback(boost::bind(&LLPanelDirBrowser::onClickSearchCore,this));
	setDefaultBtn( "Browse" );
	getChild<LLButton>("Delete")->setClickedCallback(boost::bind(&LLPanelDirClassified::onClickDelete,this));
	childDisable("Delete");
	childHide("Delete");
	LLViewerRegion* region(gAgent.getRegion());
	getChildView("filter_gaming")->setVisible(region && (region->getGamingFlags() & REGION_GAMING_PRESENT) && !(region->getGamingFlags() & REGION_GAMING_HIDE_FIND_CLASSIFIEDS));
	return TRUE;
}
LLPanelDirClassified::~LLPanelDirClassified()
{
}
void LLPanelDirClassified::draw()
{
	refresh();
	LLPanelDirBrowser::draw();
}
void LLPanelDirClassified::refresh()
{
	BOOL godlike = gAgent.isGodlike();
	childSetVisible("Delete", godlike);
	childSetEnabled("Delete", godlike);
	updateMaturityCheckbox();
}
void LLPanelDirClassified::onClickDelete()
{
	LLUUID classified_id;
	S32 type;
	getSelectedInfo(&classified_id, &type);
	setupNewSearch();
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ClassifiedGodDelete);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_Data);
	msg->addUUIDFast(_PREHASH_ClassifiedID, classified_id);
	msg->addUUIDFast(_PREHASH_QueryID, mSearchID);
	gAgent.sendReliableMessage();
}
void LLPanelDirClassified::performQuery()
{
	LL_DEBUGS() << "LLPanelDirClassified::performQuery()" << LL_ENDL;
	BOOL inc_pg = childGetValue("incpg").asBoolean();
	BOOL inc_mature = childGetValue("incmature").asBoolean();
	BOOL inc_adult = childGetValue("incadult").asBoolean();
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotificationsUtil::add("NoContentToSearch");
		return;
	}
	setupNewSearch();
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_DirClassifiedQuery);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	BOOL filter_auto_renew = FALSE;
	U32 query_flags = pack_classified_flags_request(filter_auto_renew, inc_pg, inc_mature, inc_adult);
	if (childGetValue("filter_gaming")) query_flags |= DFQ_FILTER_GAMING;
	U32 category = childGetValue("Category").asInteger();
	msg->nextBlockFast(_PREHASH_QueryData);
	msg->addUUIDFast(_PREHASH_QueryID, mSearchID );
	msg->addStringFast(_PREHASH_QueryText, childGetValue("name").asString());
	msg->addU32Fast(_PREHASH_QueryFlags, query_flags);
	msg->addU32Fast(_PREHASH_Category, category);
	msg->addS32Fast(_PREHASH_QueryStart,mSearchStart);
	gAgent.sendReliableMessage();
}
void LLPanelDirClassified::onKeystrokeNameClassified(LLLineEditor* line)
{
	S32 len = line->getLength();
	if (len == 0
		|| len >= 3)
	{
		setDefaultBtn( "Search" );
		childEnable("Search");
	}
	else
	{
		setDefaultBtn();
		childDisable("Search");
	}
	if (len > 0)
	{
		childSetVisible("Search", TRUE);
		childSetVisible("Browse", FALSE);
	}
	else
	{
		setDefaultBtn( "Browse" );
		childSetVisible("Search", FALSE);
		childSetVisible("Browse", TRUE);
	}
}
