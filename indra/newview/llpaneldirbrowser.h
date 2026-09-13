/** 
 * @file llpaneldirbrowser.h
 * @brief LLPanelDirBrowser class definition
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
#ifndef LL_LLPANELDIRBROWSER_H
#define LL_LLPANELDIRBROWSER_H
#include "llpanel.h"
#include "lluuid.h"
#include "llframetimer.h"
#include "llmap.h"
class LLMessageSystem;
class LLFloaterDirectory;
class LLPanelDirBrowser: public LLPanel
{
public:
	LLPanelDirBrowser(const std::string& name, LLFloaterDirectory* floater);
	virtual ~LLPanelDirBrowser();
	virtual BOOL postBuild();
	virtual void draw();
	virtual void handleVisibilityChange(BOOL curVisibilityIn);
	virtual void prevPage();
	virtual void nextPage();
	void resetSearchStart();
	virtual void performQuery() {};
	const LLUUID& getSearchID() const { return mSearchID; }
	void selectByUUID(const LLUUID& id);
	void selectEventByID(S32 event_id);
	U32 getSelectedEventID() const;
	void getSelectedInfo(LLUUID* id, S32 *type);
	void showDetailPanel(S32 type, LLSD item_id);
	void setupNewSearch();
	void onClickSearchCore();
	void onKeystrokeName(LLLineEditor* line);
	static void sendDirFindQuery(
		LLMessageSystem* msg,
		const LLUUID& query_id,
		const std::string& text,
		U32 flags,
		S32 query_start);
	void newClassified();
	void showEvent(const U32 event_id);
	void onCommitList();
	static void processDirPeopleReply(LLMessageSystem* msg, void**);
	static void processDirPlacesReply(LLMessageSystem* msg, void**);
	static void processDirEventsReply(LLMessageSystem* msg, void**);
	static void processDirGroupsReply(LLMessageSystem* msg, void**);
	static void processDirClassifiedReply(LLMessageSystem* msg, void**);
	static void processDirLandReply(LLMessageSystem *msg, void**);
	std::string filterShortWords( const std::string source_string, int shortest_word_length, bool& was_filtered );
	void updateMaturityCheckbox();
protected:
	void updateResultCount();
	void addClassified(LLCtrlListInterface *list, const LLUUID& classified_id, const std::string& name, const U32 creation_date, const S32 price_for_listing);
	LLSD createLandSale(const LLUUID& parcel_id, BOOL is_auction, BOOL is_for_sale, const std::string& name, S32 *type);
	S32 showNextButton(S32 rows);
protected:
	LLUUID			mSearchID;
	LLUUID			mWantSelectID;
	std::string     mCurrentSortColumn;
	BOOL            mCurrentSortAscending;
	S32				mSearchStart;
	S32				mResultsPerPage;
	S32				mResultsReceived;
	U32				mMinSearchChars;
	LLSD			mResultsContents;
	BOOL			mHaveSearchResults;
	BOOL			mDidAutoSelect;
	LLFrameTimer	mLastResultTimer;
	LLFloaterDirectory* mFloaterDirectory;
};
const S32 INVALID_CODE = -1;
const S32 EVENT_CODE = 0;
const S32 PLACE_CODE = 1;
const S32 AVATAR_CODE = 3;
const S32 GROUP_CODE = 4;
const S32 CLASSIFIED_CODE = 5;
const S32 FOR_SALE_CODE = 6;
const S32 AUCTION_CODE = 7;
const S32 POPULAR_CODE = 8;
const S32 SEARCH_NONE = 0;
const S32 SEARCH_PG = 1;
const S32 SEARCH_MATURE = 2;
const S32 SEARCH_ADULT = 4;
extern LLMap< const LLUUID, LLPanelDirBrowser* > gDirBrowserInstances;
#endif
