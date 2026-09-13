/**
 * @file llfloatersearch.h
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
#ifndef LL_LLFLOATERSEARCH_H
#define LL_LLFLOATERSEARCH_H
#include "llfloaterwebcontent.h"
#include "llviewermediaobserver.h"
#include <string>
class LLMediaCtrl;
class LLFloaterSearch :
	public LLFloaterWebContent, public LLSingleton<LLFloaterSearch>
{
public:
	struct SearchQuery : public LLInitParam::Block<SearchQuery>
	{
		Optional<std::string> category;
		Optional<std::string> query;
		SearchQuery();
	};
	struct _Params : public LLInitParam::Block<_Params, LLFloaterWebContent::Params>
	{
		Optional<SearchQuery> search;
		_Params();
	};
	typedef LLSDParamAdapter<_Params> Params;
	LLFloaterSearch(const Params& key = Params());
	using LLSingleton<LLFloaterSearch>::getInstance;
	static void showInstance(const SearchQuery& p, bool web = false);
	void onOpen() {}
	void onClose(bool app_quitting);
	static void search(const SearchQuery& query, LLMediaCtrl* mWebBrowser);
private:
	BOOL postBuild();
};
#endif
