/** 
 * @file llpaneldirplaces.cpp
 * @brief "Places" panel in the Find directory (not popular places, just places)
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
#include "llpaneldirplaces.h"
#include "llfontgl.h"
#include "message.h"
#include "lldir.h"
#include "llparcel.h"
#include "llregionflags.h"
#include "llqueryflags.h"
#include "llagent.h"
#include "llagentaccess.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloaterdirectory.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llviewerwindow.h"
#include "llpaneldirbrowser.h"
#include "lltextbox.h"
#include "lluiconstants.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"
#include "llviewerregion.h"
#include "llworldmap.h"
LLPanelDirPlaces::LLPanelDirPlaces(const std::string& name, LLFloaterDirectory* floater)
	:	LLPanelDirBrowser(name, floater)
{
	LL_INFOS() << "called" << LL_ENDL;
	mMinSearchChars = 3;
}
BOOL LLPanelDirPlaces::postBuild()
{
	LLPanelDirBrowser::postBuild();
	getChild<LLLineEditor>("name")->setKeystrokeCallback(boost::bind(&LLPanelDirBrowser::onKeystrokeName,this,_1));
	getChild<LLButton>("Search")->setClickedCallback(boost::bind(&LLPanelDirBrowser::onClickSearchCore,this));
	childDisable("Search");
	mCurrentSortColumn = "dwell";
	mCurrentSortAscending = FALSE;
	if (gAgent.getAgentAccess().isInTransition())
	{
		childSetVisible("Category_Adult", true);
		childSetEnabled("Category_Adult", true);
	}
	else
	{
		childSetVisible("Category", true);
		childSetEnabled("Category", true);
	}
	LLViewerRegion* region(gAgent.getRegion());
	getChildView("filter_gaming")->setVisible(region && (region->getGamingFlags() & REGION_GAMING_PRESENT) && !(region->getGamingFlags() & REGION_GAMING_HIDE_FIND_SIMS));
	return TRUE;
}
LLPanelDirPlaces::~LLPanelDirPlaces()
{
	LL_INFOS() << "called" << LL_ENDL;
}
void LLPanelDirPlaces::draw()
{
	updateMaturityCheckbox();
	LLPanelDirBrowser::draw();
}
void LLPanelDirPlaces::performQuery()
{
	std::string place_name = childGetValue("name").asString();
	if (place_name.length() < mMinSearchChars)
	{
		return;
	}
	std::string query_string = place_name;
	LLStringUtil::trim( query_string );
	bool query_was_filtered = (query_string != place_name);
	if ( query_string.length() < mMinSearchChars )
	{
		LLNotificationsUtil::add("SeachFilteredOnShortWordsEmpty");
		return;
	};
	if ( query_was_filtered )
	{
		LLSD args;
		args["FINALQUERY"] = query_string;
		LLNotificationsUtil::add("SeachFilteredOnShortWords", args);
	};
	std::string catstring = childGetValue("Category").asString();
	if (gAgent.getAgentAccess().isInTransition())
	{
		catstring = childGetValue("Category_Adult").asString();
	}
	else
	{
		catstring = childGetValue("Category").asString();
	}
	S32 category = 0;
	if (catstring == "any")
	{
		category = LLParcel::C_ANY;
	}
	else
	{
		category = LLParcel::getCategoryFromString(catstring);
	}
	U32 flags = 0x0;
	bool adult_enabled = gAgent.canAccessAdult();
	bool mature_enabled = gAgent.canAccessMature();
	if (gSavedSettings.getBOOL("ShowPGSims") ||
	    (!adult_enabled && !mature_enabled))
	{
		flags |= DFQ_INC_PG;
	}
	if( gSavedSettings.getBOOL("ShowMatureSims") && mature_enabled)
	{
		flags |= DFQ_INC_MATURE;
	}
	if( gSavedSettings.getBOOL("ShowAdultSims") && adult_enabled)
	{
		flags |= DFQ_INC_ADULT;
	}
	if (childGetValue("filter_gaming").asBoolean())
	{
		flags |= DFQ_FILTER_GAMING;
	}
	if ( ((flags & DFQ_INC_PG) == DFQ_INC_PG) && !((flags & DFQ_INC_MATURE) == DFQ_INC_MATURE) )
	{
		flags |= DFQ_PG_PARCELS_ONLY;
	}
	if (0x0 == flags)
	{
		LLNotificationsUtil::add("NoContentToSearch");
		return;
	}
	queryCore(query_string, category, flags);
}
void LLPanelDirPlaces::initialQuery()
{
	U32 flags = DFQ_INC_PG | DFQ_INC_MATURE;
	queryCore(LLStringUtil::null, LLParcel::C_LINDEN, flags);
}
void LLPanelDirPlaces::queryCore(const std::string& name,
								 S32 category,
								 U32 flags)
{
	setupNewSearch();
	flags |= DFQ_DWELL_SORT;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("DirPlacesQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", getSearchID());
	msg->addString("QueryText", name);
	msg->addU32("QueryFlags", flags);
	msg->addS8("Category", (S8)category);
	msg->addString("SimName", "");
	msg->addS32Fast(_PREHASH_QueryStart,mSearchStart);
	gAgent.sendReliableMessage();
}
