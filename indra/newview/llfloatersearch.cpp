/**
 * @file llfloatersearch.cpp
 * @author Martin Reddy
 * @brief Search floater - uses an embedded web browser control
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "llviewerprecompiledheaders.h"
#include "llcommandhandler.h"
#include "llfloatersearch.h"
#include "llfloaterdirectory.h"
#include "llmediactrl.h"
#include "llnotificationsutil.h"
#include "lluserauth.h"
#include "lluri.h"
#include "llagent.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llweb.h"
void toggle_search_floater()
{
	if (!gSavedSettings.getString("SearchURL").empty() && gSavedSettings.getBOOL("UseWebSearch"))
	{
		if (LLFloaterSearch::instanceExists() && LLFloaterSearch::instance().getVisible())
			LLFloaterSearch::instance().close();
		else
			LLFloaterSearch::getInstance()->open();
	}
	else
	{
		LLFloaterDirectory::toggleFind(0);
	}
}
class LLSearchHandler : public LLCommandHandler
{
public:
	LLSearchHandler() : LLCommandHandler("search", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		const size_t parts = tokens.size();
		std::string category;
		if (parts > 0)
		{
			category = tokens[0].asString();
		}
		std::string search_text;
		if (parts > 1)
		{
			search_text = tokens[1].asString();
		}
		LLFloaterSearch::Params p;
		p.search.category = category;
		p.search.query = LLURI::unescape(search_text);
		LLFloaterSearch::showInstance(p.search, gSavedSettings.getBOOL("UseWebSearchSLURL"));
		return true;
	}
};
LLSearchHandler gSearchHandler;
LLFloaterSearch::SearchQuery::SearchQuery()
:	category("category", ""),
	query("query")
{}
LLFloaterSearch::_Params::_Params()
{
	changeDefault(trusted_content, true);
	changeDefault(allow_address_entry, false);
	changeDefault(window_class, "search");
	changeDefault(id, "search");
}
LLFloaterSearch::LLFloaterSearch(const Params& key) :
	LLFloaterWebContent(key)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_web_content.xml");
}
BOOL LLFloaterSearch::postBuild()
{
	LLFloaterWebContent::postBuild();
	mWebBrowser->addObserver(this);
	mWebBrowser->setTrustedContent(true);
	mWebBrowser->setFocus(true);
	getChild<LLPanel>("status_bar")->setVisible(true);
	getChild<LLPanel>("nav_controls")->setVisible(true);
	getChildView("address")->setEnabled(false);
	getChildView("popexternal")->setEnabled(false);
	setRectControl("FloaterSearchRect");
	applyRectControl();
	search(SearchQuery(), mWebBrowser);
	gSavedSettings.getControl("SearchURL")->getSignal()->connect(boost::bind(LLFloaterSearch::search, SearchQuery(), mWebBrowser));
	return TRUE;
}
void LLFloaterSearch::showInstance(const SearchQuery& search, bool web)
{
	if (!gSavedSettings.getString("SearchURL").empty() && (web || gSavedSettings.getBOOL("UseWebSearch")))
	{
		LLFloaterSearch* floater = getInstance();
		floater->open();
		floater->search(search, floater->mWebBrowser);
	}
	else
	{
		LLFloaterDirectory::search(search);
	}
}
void LLFloaterSearch::onClose(bool app_quitting)
{
	LLFloaterWebContent::onClose(app_quitting);
	destroy();
}
void LLFloaterSearch::search(const SearchQuery &p, LLMediaCtrl* mWebBrowser)
{
	if (! mWebBrowser || !p.validateBlock())
	{
		return;
	}
	LLSD subs;
	static LLSD mCategoryPaths = LLSD::emptyMap();
	if (mCategoryPaths.size() == 0)
	{
		mCategoryPaths["all"]          = "search";
		mCategoryPaths["people"]       = "search/people";
		mCategoryPaths["places"]       = "search/places";
		mCategoryPaths["events"]       = "search/events";
		mCategoryPaths["groups"]       = "search/groups";
		mCategoryPaths["wiki"]         = "search/wiki";
		mCategoryPaths["destinations"] = "destinations";
		mCategoryPaths["classifieds"]  = "classifieds";
	}
	if (mCategoryPaths.has(p.category))
	{
		subs["CATEGORY"] = mCategoryPaths[p.category].asString();
	}
	else
	{
		subs["CATEGORY"] = mCategoryPaths["all"].asString();
	}
	subs["QUERY"] = LLURI::escape(p.query);
	LLSD search_token = LLUserAuth::getInstance()->getResponse("search_token");
	if (search_token.asString().empty())
	{
		search_token = LLUserAuth::getInstance()->getResponse("auth_token");
	}
	subs["AUTH_TOKEN"] = search_token.asString();
	std::string maturity;
	if (gAgent.prefersAdult())
	{
		maturity = "42";
	}
	else if (gAgent.prefersMature())
	{
		maturity = "21";
	}
	else
	{
		maturity = "13";
	}
	subs["MATURITY"] = maturity;
	subs["GODLIKE"] = gAgent.isGodlike() ? "1" : "0";
	std::string url = gSavedSettings.getString("SearchURL");
	url = LLWeb::expandURLSubstitutions(url, subs);
	mWebBrowser->navigateTo(url, "text/html");
}
