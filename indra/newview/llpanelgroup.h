/** 
 * @file llpanelgroup.h
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#ifndef LL_LLPANELGROUP_H
#define LL_LLPANELGROUP_H
#include "llgroupmgr.h"
#include "llpanel.h"
#include "lltimer.h"
class LLOfferInfo;
const F32 UPDATE_MEMBERS_SECONDS_PER_FRAME = 0.005f;
class LLPanelGroupTab;
class LLTabContainer;
class LLAgent;
class LLPanelGroupTabObserver
{
public:
	LLPanelGroupTabObserver() {};
	virtual ~LLPanelGroupTabObserver(){};
	virtual void tabChanged() = 0;
};
class LLPanelGroup : public LLPanel,
					 public LLGroupMgrObserver,
					 public LLPanelGroupTabObserver
{
public:
	LLPanelGroup(const LLUUID& group_id);
	virtual ~LLPanelGroup();
	virtual BOOL postBuild();
	static void onBtnOK(void*);
	static void onBtnCancel(void*);
	static void onBtnApply(void*);
	static void onBtnRefresh(void*);
	void handleClickTab();
	void setGroupID(const LLUUID& group_id);
	void selectTab(std::string tab_name);
	BOOL canClose();
	BOOL attemptTransition();
	void transitionToTab();
	void updateTabVisibility();
	bool handleNotifyCallback(const LLSD& notification, const LLSD& response);
	bool apply();
	void refreshData();
	void close();
	void draw();
	virtual void changed(LLGroupChange gc);
	virtual void tabChanged();
	void setAllowEdit(BOOL v) { mAllowEdit = v; }
	void showNotice(const std::string& subject,
					const std::string& message,
					const bool& has_inventory,
					const std::string& inventory_name,
					LLOfferInfo* inventory_offer);
protected:
	LLPanelGroupTab*		mCurrentTab;
	LLPanelGroupTab*		mRequestedTab;
	LLTabContainer*	mTabContainer;
	BOOL mIgnoreTransition;
	LLButton* mApplyBtn;
	LLTimer mRefreshTimer;
	BOOL mForceClose;
	std::string mDefaultNeedsApplyMesg;
	std::string mWantApplyMesg;
	BOOL mAllowEdit;
	BOOL mShowingNotifyDialog;
};
class LLPanelGroupTab : public LLPanel
{
public:
	LLPanelGroupTab(const std::string& name, const LLUUID& group_id)
	: LLPanel(name), mGroupID(group_id), mAllowEdit(TRUE), mHasModal(FALSE) { }
	virtual ~LLPanelGroupTab();
	static void* createTab(void* data);
	virtual void activate() { }
	virtual void deactivate() { }
	virtual bool needsApply(std::string& mesg) { return false; }
	virtual BOOL hasModal() { return mHasModal; }
	virtual bool apply(std::string& mesg) { return true; }
	virtual void cancel() { }
	virtual void update(LLGroupChange gc) { }
	virtual std::string getHelpText() const { return mHelpText; }
	static void onClickHelp(void* data);
	void handleClickHelp();
	virtual BOOL postBuild();
	virtual BOOL isVisibleByAgent(LLAgent* agentp);
	virtual void setGroupID(const LLUUID& id) { mGroupID = id; }
	void setAllowEdit(BOOL v) { mAllowEdit = v; }
	void addObserver(LLPanelGroupTabObserver *obs);
	void removeObserver(LLPanelGroupTabObserver *obs);
	void notifyObservers();
	const LLUUID& getGroupID() const { return mGroupID; }
protected:
	LLUUID	mGroupID;
	LLTabContainer*	mTabContainer;
	std::string	mHelpText;
	BOOL mAllowEdit;
	BOOL mHasModal;
	typedef std::set<LLPanelGroupTabObserver*> observer_list_t;
	observer_list_t mObservers;
};
#endif
