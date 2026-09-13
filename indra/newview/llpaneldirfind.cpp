/**
 * @file llpaneldirfind.cpp
 * @brief The "All" panel in the Search directory.
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
#include "llpaneldirfind.h"
#include "llclassifiedflags.h"
#include "llfontgl.h"
#include "llparcel.h"
#include "llqueryflags.h"
#include "message.h"
#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lllineeditor.h"
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "llmenucommands.h"
#include "llmenugl.h"
#include "llnotificationsutil.h"
#include "llpluginclassmedia.h"
#include "lltextbox.h"
#include "lluiconstants.h"
#include "llviewertexturelist.h"
#include "llviewermessage.h"
#include "llfloateravatarinfo.h"
#include "lldir.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "lluictrlfactory.h"
#include "llfloaterdirectory.h"
#include "llpaneldirbrowser.h"
#include "llmediactrl.h"
#include "llui.h"
#include "hippogridmanager.h"
#include "lfsimfeaturehandler.h"
#include <boost/tokenizer.hpp>
namespace
{
	std::string getSearchUrl()
	{
		return LFSimFeatureHandler::instance().searchURL();
	}
	enum SearchType
	{
		SEARCH_ALL_EMPTY,
		SEARCH_ALL_QUERY,
		SEARCH_ALL_TEMPLATE
	};
	std::string slWebSearchUrl(SearchType ty)
	{
		std::string url;
		if (ty == SEARCH_ALL_EMPTY)
		{
			url = gSavedSettings.getString("SearchURLDefault");
		}
		else if (ty == SEARCH_ALL_QUERY)
		{
			url = gSavedSettings.getString("SearchURLQuery");
		}
		else
		{
			return gSavedSettings.getString("SearchURLSuffix2");
		}
		const bool dead_gsa = url.find("client_search.php") != std::string::npos
			|| url.find("secondlife.com/app/search") != std::string::npos;
		if (dead_gsa)
		{
			if (ty == SEARCH_ALL_QUERY)
			{
				return "https://search.secondlife.com/?query_term=[QUERY]&search_type=standard&collection_chosen=[COLLECTION]&";
			}
			return "https://search.secondlife.com/?";
		}
		if (url.compare(0, 5, "http:") == 0 && url.find("search.secondlife.com") != std::string::npos)
		{
			url.replace(0, 4, "https");
		}
		return url;
	}
	std::string getSearchUrl(SearchType ty, bool is_web)
	{
		const std::string mSearchUrl(getSearchUrl());
		if (is_web)
		{
			if (gHippoGridManager->getConnectedGrid()->isSecondLife())
			{
				return slWebSearchUrl(ty);
			}
			else if (!mSearchUrl.empty())
			{
				if (ty == SEARCH_ALL_EMPTY)
				{
					return mSearchUrl;
				}
				else if (ty == SEARCH_ALL_QUERY)
				{
					return mSearchUrl + "q=[QUERY]&s=[COLLECTION]&";
				}
				else if (ty == SEARCH_ALL_TEMPLATE)
				{
					return "lang=[LANG]&mat=[MATURITY]&t=[TEEN]&region=[REGION]&x=[X]&y=[Y]&z=[Z]&session=[SESSION]&dice=[DICE]";
				}
			}
			else
			{
				if (ty == SEARCH_ALL_EMPTY)
				{
					return gSavedSettings.getString("SearchURLDefaultOpenSim");
				}
				else if (ty == SEARCH_ALL_QUERY)
				{
					return gSavedSettings.getString("SearchURLQueryOpenSim");
				}
				else if (ty == SEARCH_ALL_TEMPLATE)
				{
					return gSavedSettings.getString("SearchURLSuffixOpenSim");
				}
			}
		}
		else
		{
			if (ty == SEARCH_ALL_EMPTY)
			{
				return mSearchUrl + "panel=All&";
			}
			else if (ty == SEARCH_ALL_QUERY)
			{
				return mSearchUrl + "q=[QUERY]&s=[COLLECTION]&";
			}
			else if (ty == SEARCH_ALL_TEMPLATE)
			{
				return "lang=[LANG]&m=[MATURITY]&t=[TEEN]&region=[REGION]&x=[X]&y=[Y]&z=[Z]&session=[SESSION]&dice=[DICE]";
			}
		}
		LL_INFOS() << "Illegal search URL type " << ty << LL_ENDL;
		return "";
	}
}
class LLPanelDirFindAll
:	public LLPanelDirFind
{
public:
	LLPanelDirFindAll(const std::string& name, LLFloaterDirectory* floater);
	void search(const std::string& search_text);
};
LLPanelDirFindAll::LLPanelDirFindAll(const std::string& name, LLFloaterDirectory* floater)
:	LLPanelDirFind(name, floater, "find_browser")
{
	LFSimFeatureHandler::getInstance()->setSearchURLCallback(boost::bind(&LLPanelDirFindAll::navigateToDefaultPage, this));
}
LLPanelDirFind::LLPanelDirFind(const std::string& name, LLFloaterDirectory* floater, const std::string& browser_name)
:	LLPanelDirBrowser(name, floater),
	mWebBrowser(NULL),
	mBrowserName(browser_name)
{
}
BOOL LLPanelDirFind::postBuild()
{
	LLPanelDirBrowser::postBuild();
	getChild<LLButton>("back_btn")->setCommitCallback(boost::bind(&LLPanelDirFind::onClickBack,this));
	if (hasChild("home_btn"))
		getChild<LLButton>("home_btn")->setCommitCallback(boost::bind(&LLPanelDirFind::onClickHome,this));
	getChild<LLButton>("forward_btn")->setCommitCallback(boost::bind(&LLPanelDirFind::onClickForward,this));
	getChild<LLButton>("reload_btn")->setCommitCallback(boost::bind(&LLPanelDirFind::onClickRefresh,this));
	if (hasChild("search_editor"))
		getChild<LLButton>("search_editor")->setCommitCallback(boost::bind(&LLPanelDirFind::onClickSearch,this));
	if (hasChild("search_btn"))
		getChild<LLButton>("search_btn")->setCommitCallback(boost::bind(&LLPanelDirFind::onClickSearch,this));
	if (hasChild("?"))
		getChild<LLButton>("?")->setCommitCallback(boost::bind(&LLPanelDirFind::onClickHelp,this));
	if (hasChild("incmature"))
	{
		if (gAgent.wantsPGOnly())
		{
			childSetValue("incmature", FALSE);
			childSetValue("incadult", FALSE);
			childHide("incmature");
			childHide("incadult");
			childSetValue("incpg", TRUE);
			childDisable("incpg");
		}
		if (!gAgent.canAccessMature())
		{
			childSetValue("incmature", FALSE);
			childDisable("incmature");
		}
		if (!gAgent.canAccessAdult())
		{
			childSetValue("incadult", FALSE);
			childDisable("incadult");
		}
	}
	mWebBrowser = getChild<LLMediaCtrl>(mBrowserName);
	if (mWebBrowser)
	{
		mWebBrowser->addObserver(this);
		mWebBrowser->setTrustedContent( true );
		applyBrowserZoom();
		if (loadOnBuild())
		{
			navigateToDefaultPage();
		}
	}
	if (LLUICtrl* ctrl = findChild<LLUICtrl>("filter_gaming"))
	{
		const LLViewerRegion* region(gAgent.getRegion());
		ctrl->setVisible(region && (region->getGamingFlags() & REGION_GAMING_PRESENT) && !(region->getGamingFlags() & REGION_GAMING_HIDE_FIND_ALL));
	}
	return TRUE;
}
LLPanelDirFind::~LLPanelDirFind()
{
}
void LLPanelDirFind::draw()
{
	if ( mWebBrowser )
	{
		bool enable_back = mWebBrowser->canNavigateBack();
		childSetEnabled( "back_btn", enable_back );
		bool enable_forward = mWebBrowser->canNavigateForward();
		childSetEnabled( "forward_btn", enable_forward );
		childSetEnabled( "reload_btn", TRUE );
	}
	if (hasChild("incmature"))
	{
		updateMaturityCheckbox();
	}
	LLPanelDirBrowser::draw();
}
void LLPanelDirFind::handleVisibilityChange(BOOL new_visibility)
{
	if (new_visibility)
	{
		mFloaterDirectory->hideAllDetailPanels();
		if (mWebBrowser)
		{
			applyBrowserZoom();
			const std::string cur = mWebBrowser->getCurrentNavUrl();
			if (cur.empty() || cur == "about:blank")
			{
				navigateToDefaultPage();
			}
		}
	}
	LLPanel::handleVisibilityChange(new_visibility);
}
void LLPanelDirFind::applyBrowserZoom()
{
	if (!mWebBrowser)
	{
		return;
	}
	mWebBrowser->setStretchToFill(true);
	mWebBrowser->setMaintainAspectRatio(false);
	mWebBrowser->setIgnoreUIScale(true);
	mWebBrowser->setContentScale(1.5f);
	const LLRect& r = mWebBrowser->getRect();
	if (r.getWidth() > 0 && r.getHeight() > 0)
	{
		mWebBrowser->reshape(r.getWidth(), r.getHeight(), FALSE);
	}
}
void LLPanelDirFindAll::search(const std::string& search_text)
{
	BOOL inc_pg = childGetValue("incpg").asBoolean();
	BOOL inc_mature = childGetValue("incmature").asBoolean();
	BOOL inc_adult = childGetValue("incadult").asBoolean();
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotificationsUtil::add("NoContentToSearch");
		return;
	}
	if (!search_text.empty())
	{
		bool is_web = false;
		LLPanel* tabs_panel = mFloaterDirectory->getChild<LLTabContainer>("Directory Tabs")->getCurrentPanel();
		if (tabs_panel)
		{
			is_web = tabs_panel->getName() == "find_all_panel";
		}
		else
		{
			LL_WARNS() << "search panel not found! How can this be?!" << LL_ENDL;
		}
		std::string selected_collection = childGetValue( "Category" ).asString();
		std::string url = buildSearchURL(search_text, selected_collection, inc_pg, inc_mature, inc_adult, is_web);
		if (mWebBrowser)
		{
			mWebBrowser->navigateTo(url);
		}
	}
	else
	{
		navigateToDefaultPage();
	}
	childSetText("search_editor", search_text);
}
void LLPanelDirFind::focus()
{
	if (hasChild("search_editor"))
		childSetFocus("search_editor");
}
void LLPanelDirFind::navigateToDefaultPage()
{
	bool showcase(mBrowserName == "showcase_browser");
	std::string start_url = showcase ? LLWeb::expandURLSubstitutions(LFSimFeatureHandler::instance().destinationGuideURL(), LLSD()) : getSearchUrl();
	bool secondlife(gHippoGridManager->getConnectedGrid()->isSecondLife());
	if (start_url.empty() && !secondlife)
	{
		start_url = gSavedSettings.getString("SearchURLDefaultOpenSim");
	}
	else
	{
		if (!showcase)
		{
			if (secondlife)
				start_url = getSearchUrl(SEARCH_ALL_EMPTY, true);
			else
				start_url += "panel=" + getName() + "&";
			if (hasChild("incmature"))
			{
				bool inc_pg = getChildView("incpg")->getValue().asBoolean();
				bool inc_mature = getChildView("incmature")->getValue().asBoolean();
				bool inc_adult = getChildView("incadult")->getValue().asBoolean();
				if (!(inc_pg || inc_mature || inc_adult))
				{
					inc_pg = true;
				}
				start_url += getSearchURLSuffix(inc_pg, inc_mature, inc_adult, true);
			}
		}
	}
	LL_INFOS() << "default web search url: "  << start_url << LL_ENDL;
	if (mWebBrowser)
	{
		mWebBrowser->navigateTo( start_url );
	}
}
const std::string LLPanelDirFind::buildSearchURL(const std::string& search_text, const std::string& collection,
										   bool inc_pg, bool inc_mature, bool inc_adult, bool is_web) const
{
	std::string url;
	if (search_text.empty())
	{
		url = getSearchUrl(SEARCH_ALL_EMPTY, is_web);
	}
	else
	{
		std::string search_text_with_plus = search_text;
		std::string::iterator it = search_text_with_plus.begin();
		for ( ; it != search_text_with_plus.end(); ++it )
		{
			if ( std::isspace( *it ) )
			{
				*it = '+';
			}
		}
		const char* allowed =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
			"0123456789"
			"-._~$+!*'()";
		std::string query = LLURI::escape(search_text_with_plus, allowed);
		url = getSearchUrl(SEARCH_ALL_QUERY, is_web);
		std::string substring = "[QUERY]";
		std::string::size_type where = url.find(substring);
		if (where != std::string::npos)
		{
			url.replace(where, substring.length(), query);
		}
		substring = "[COLLECTION]";
		where = url.find(substring);
		if (where != std::string::npos)
		{
			url.replace(where, substring.length(), collection);
		}
	}
	url += getSearchURLSuffix(inc_pg, inc_mature, inc_adult, is_web);
	LL_INFOS() << "web search url " << url << LL_ENDL;
	return url;
}
const std::string LLPanelDirFind::getSearchURLSuffix(bool inc_pg, bool inc_mature, bool inc_adult, bool is_web) const
{
	std::string url = getSearchUrl(SEARCH_ALL_TEMPLATE, is_web);
	LL_INFOS() << "Suffix template " << url << LL_ENDL;
	if (!url.empty())
	{
		if (gHippoGridManager->getConnectedGrid()->isSecondLife() || !getSearchUrl().empty())
		{
			std::string substring="[MATURITY]";
			S32 maturityFlag =
				(inc_pg ? SEARCH_PG : SEARCH_NONE) |
				(inc_mature ? SEARCH_MATURE : SEARCH_NONE) |
				(inc_adult ? SEARCH_ADULT : SEARCH_NONE);
			url.replace(url.find(substring), substring.length(), fmt::to_string(maturityFlag));
			std::string region_name;
			LLViewerRegion* region = gAgent.getRegion();
			if (region)
			{
				region_name = region->getName();
			}
			region_name = LLURI::escape(region_name);
			substring = "[REGION]";
			url.replace(url.find(substring), substring.length(), region_name);
			LLVector3 pos_region = gAgent.getPositionAgent();
			std::string x = llformat("%.0f", pos_region.mV[VX]);
			substring = "[X]";
			url.replace(url.find(substring), substring.length(), x);
			std::string y = llformat("%.0f", pos_region.mV[VY]);
			substring = "[Y]";
			url.replace(url.find(substring), substring.length(), y);
			std::string z = llformat("%.0f", pos_region.mV[VZ]);
			substring = "[Z]";
			url.replace(url.find(substring), substring.length(), z);
			LLUUID session_id = gAgent.getSessionID();
			std::string session_string = session_id.getString();
			substring = "[SESSION]";
			url.replace(url.find(substring), substring.length(), session_string);
			std::string language_string = LLUI::getLanguage();
			std::string language_tag = "[LANG]";
			url.replace( url.find( language_tag ), language_tag.length(), language_string );
			std::string teen_string = gAgent.isTeen() ? "y" : "n";
			std::string teen_tag = "[TEEN]";
			url.replace( url.find( teen_tag ), teen_tag.length(), teen_string );
			if (!gHippoGridManager->getConnectedGrid()->isSecondLife())
			{
				substring = "[DICE]";
				url.replace(url.find(substring), substring.length(), (hasChild("filter_gaming") && childGetValue("filter_gaming").asBoolean()) ? "y" : "n");
			}
		}
	}
	return url;
}
void LLPanelDirFind::onClickBack()
{
	if ( mWebBrowser )
	{
		mWebBrowser->navigateBack();
	}
}
void LLPanelDirFind::onClickHelp()
{
	LLNotificationsUtil::add("ClickSearchHelpAll");
}
void LLPanelDirFind::onClickForward()
{
	if ( mWebBrowser )
	{
		mWebBrowser->navigateForward();
	}
}
void LLPanelDirFind::onClickHome()
{
	if ( mWebBrowser )
	{
		mWebBrowser->navigateHome();
	}
}
void LLPanelDirFind::onClickRefresh()
{
	if ( mWebBrowser )
	{
		mWebBrowser->navigateTo(mWebBrowser->getCurrentNavUrl());
	}
}
void LLPanelDirFind::onClickSearch()
{
	std::string search_text = childGetText("search_editor");
	search(search_text);
	LLFloaterDirectory::sNewSearchCount++;
}
void LLPanelDirFind::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	switch(event)
	{
		case MEDIA_EVENT_NAVIGATE_BEGIN:
			childSetText("status_text", getString("loading_text"));
		break;
		case MEDIA_EVENT_NAVIGATE_COMPLETE:
			childSetText("status_text", getString("done_text"));
		break;
		case MEDIA_EVENT_LOCATION_CHANGED:
			LL_INFOS() << self->getLocation() << LL_ENDL;
		break;
		default:
		break;
	}
}
static LLPanelDirFindAll* sFindAll = NULL;
LLPanelDirFindAll* LLPanelDirFindAllInterface::create(LLFloaterDirectory* floater)
{
	return new LLPanelDirFindAll("find_all_panel", floater);
}
static LLPanelDirFindAllOld* sFindAllOld = NULL;
void LLPanelDirFindAllInterface::search(LLFloaterDirectory* inst,
										const LLFloaterSearch::SearchQuery& search, bool show)
{
	if (!inst)
	{
		return;
	}
	LLPanel* panel = inst->findChild<LLPanel>("people_panel");
	if (sFindAll)
	{
		sFindAll->search(search.query);
		panel = sFindAll;
	}
	if (sFindAllOld)
	{
		sFindAllOld->search(search.query);
		if (!sFindAll)
		{
			panel = sFindAllOld;
		}
	}
	if (show && panel)
	{
		if (LLTabContainer* tabs = inst->findChild<LLTabContainer>("Directory Tabs"))
		{
			tabs->selectTabPanel(panel);
		}
		panel->setFocus(true);
	}
}
void LLPanelDirFindAllInterface::focus(LLPanelDirFindAll* panel)
{
	panel->focus();
}
LLPanelDirFindAllOld::LLPanelDirFindAllOld(const std::string& name, LLFloaterDirectory* floater)
	:	LLPanelDirBrowser(name, floater)
{
	sFindAllOld = this;
	mMinSearchChars = 3;
}
BOOL LLPanelDirFindAllOld::postBuild()
{
	LLPanelDirBrowser::postBuild();
	getChild<LLLineEditor>("name")->setKeystrokeCallback(boost::bind(&LLPanelDirBrowser::onKeystrokeName,this,_1));
	getChild<LLButton>("Search")->setCommitCallback(boost::bind(&LLPanelDirFindAllOld::onClickSearch,this));
	childDisable("Search");
	setDefaultBtn( "Search" );
	if (LLUICtrl* ctrl = findChild<LLUICtrl>("filter_gaming"))
		ctrl->setVisible(gAgent.getRegion() && (gAgent.getRegion()->getGamingFlags() & REGION_GAMING_PRESENT) && !(gAgent.getRegion()->getGamingFlags() & REGION_GAMING_HIDE_FIND_ALL_CLASSIC));
	return TRUE;
}
LLPanelDirFindAllOld::~LLPanelDirFindAllOld()
{
	sFindAllOld = NULL;
}
void LLPanelDirFindAllOld::draw()
{
	updateMaturityCheckbox();
	LLPanelDirBrowser::draw();
}
void LLPanelDirFindAllOld::search(const std::string& query)
{
	getChildView("name")->setValue(query);
	onClickSearch();
}
void LLPanelDirFindAllOld::onClickSearch()
{
	if (childGetValue("name").asString().length() < mMinSearchChars)
	{
		return;
	}
	BOOL inc_pg = childGetValue("incpg").asBoolean();
	BOOL inc_mature = childGetValue("incmature").asBoolean();
	BOOL inc_adult = childGetValue("incadult").asBoolean();
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotificationsUtil::add("NoContentToSearch");
		return;
	}
	setupNewSearch();
	U32 scope = 0x0;
	scope |= DFQ_PEOPLE;
	scope |= DFQ_EVENTS;
	scope |= DFQ_GROUPS;
	if (inc_pg)
	{
		scope |= DFQ_INC_PG;
	}
	if (inc_mature)
	{
		scope |= DFQ_INC_MATURE;
	}
	if (inc_adult)
	{
		scope |= DFQ_INC_ADULT;
	}
	if (hasChild("filter_gaming") && childGetValue("filter_gaming").asBoolean())
	{
		scope |= DFQ_FILTER_GAMING;
	}
	LLMessageSystem *msg = gMessageSystem;
	S32 start_row = 0;
	sendDirFindQuery(msg, mSearchID, childGetValue("name").asString(), scope, start_row);
	BOOL filter_auto_renew = FALSE;
	U32 classified_flags = pack_classified_flags_request(filter_auto_renew, inc_pg, inc_mature, inc_adult);
	msg->newMessage("DirClassifiedQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", mSearchID);
	msg->addString("QueryText", childGetValue("name").asString());
	msg->addU32("QueryFlags", classified_flags);
	msg->addU32("Category", 0);
	msg->addS32("QueryStart", 0);
	gAgent.sendReliableMessage();
	U32 query_flags = DFQ_DWELL_SORT;
	if (inc_pg)
	{
		query_flags |= DFQ_INC_PG;
	}
	if (inc_mature)
	{
		query_flags |= DFQ_INC_MATURE;
	}
	if (inc_adult)
	{
		query_flags |= DFQ_INC_ADULT;
	}
	msg->newMessage("DirPlacesQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", mSearchID );
	msg->addString("QueryText", childGetValue("name").asString());
	msg->addU32("QueryFlags", query_flags );
	msg->addS32("QueryStart", 0 );
	msg->addS8("Category", LLParcel::C_ANY);
	msg->addString("SimName", NULL);
	gAgent.sendReliableMessage();
}
