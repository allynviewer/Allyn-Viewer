/** 
 * @file llpanelgroup.cpp
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
#include "llviewerprecompiledheaders.h"
#include "llpanelgroup.h"
#include "lfidbearer.h"
#include "llbutton.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "llnotificationsutil.h"
#include "llagent.h"
#include "llpanelgroupnotices.h"
#include "llpanelgroupgeneral.h"
#include "llpanelgrouproles.h"
#include "llpanelgroupvoting.h"
#include "llpanelgrouplandmoney.h"
#include "llpanelgroupexperiences.h"
#include "hippogridmanager.h"
void* LLPanelGroupTab::createTab(void* data)
{
	LLUUID* group_id = static_cast<LLUUID*>(data);
	return new LLPanelGroupTab("panel group tab", *group_id);
}
LLPanelGroupTab::~LLPanelGroupTab()
{
	mObservers.clear();
}
BOOL LLPanelGroupTab::isVisibleByAgent(LLAgent* agentp)
{
	return TRUE;
}
BOOL LLPanelGroupTab::postBuild()
{
	LLButton* button = findChild<LLButton>("help_button");
	if (button)
	{
		button->setClickedCallback(boost::bind(&LLPanelGroupTab::onClickHelp,this));
	}
	mHelpText = getString("help_text");
	return TRUE;
}
void LLPanelGroupTab::addObserver(LLPanelGroupTabObserver *obs)
{
	mObservers.insert(obs);
}
void LLPanelGroupTab::removeObserver(LLPanelGroupTabObserver *obs)
{
	mObservers.erase(obs);
}
void LLPanelGroupTab::notifyObservers()
{
	for (observer_list_t::iterator iter = mObservers.begin();
		 iter != mObservers.end(); )
	{
		LLPanelGroupTabObserver* observer = *iter;
		observer->tabChanged();
		iter = mObservers.upper_bound(observer);
	}
}
void LLPanelGroupTab::onClickHelp(void* user_data)
{
	LLPanelGroupTab* self = static_cast<LLPanelGroupTab*>(user_data);
	self->handleClickHelp();
}
void LLPanelGroupTab::handleClickHelp()
{
	std::string help_text( getHelpText() );
	if ( !help_text.empty() )
	{
		LLSD args;
		args["MESSAGE"] = help_text;
		LLFloater* parent_floater = gFloaterView->getParentFloater(this);
		parent_floater->addContextualNotification("GenericAlert",args);
	}
}
void copy_profile_uri(const LLUUID& id, const LFIDBearer::Type& type);
LLPanelGroup::LLPanelGroup(const LLUUID& group_id)
:	LLPanel("PanelGroup", LLRect(), FALSE),
	LLGroupMgrObserver( group_id ),
	mCurrentTab( NULL ),
	mRequestedTab( NULL ),
	mTabContainer( NULL ),
	mIgnoreTransition( FALSE ),
	mForceClose( FALSE ),
	mAllowEdit( TRUE ),
	mShowingNotifyDialog( FALSE )
{
	mFactoryMap["general_tab"]	= LLCallbackMap(LLPanelGroupGeneral::createTab,
												&mID);
	mFactoryMap["roles_tab"]	= LLCallbackMap(LLPanelGroupRoles::createTab,
												&mID);
	mFactoryMap["notices_tab"]	= LLCallbackMap(LLPanelGroupNotices::createTab,
												&mID);
	mFactoryMap["voting_tab"]	= LLCallbackMap(LLPanelGroupVoting::createTab,
												&mID);
	mFactoryMap["land_money_tab"]= LLCallbackMap(LLPanelGroupLandMoney::createTab,
												 &mID);
	mFactoryMap["experiences_tab"] = LLCallbackMap(LLPanelGroupExperiences::createTab,
												&mID);
	mFactoryMap["members_sub_tab"] = LLCallbackMap(LLPanelGroupMembersSubTab::createTab, &mID);
	mFactoryMap["roles_sub_tab"] = LLCallbackMap(LLPanelGroupRolesSubTab::createTab, &mID);
	mFactoryMap["actions_sub_tab"] = LLCallbackMap(LLPanelGroupActionsSubTab::createTab, &mID);
	mFactoryMap["banlist_sub_tab"] = LLCallbackMap(LLPanelGroupBanListSubTab::createTab, &mID);
	LLGroupMgr::getInstance()->addObserver(this);
	mCommitCallbackRegistrar.add("Group.CopyURI", boost::bind(copy_profile_uri, boost::ref(mID), LFIDBearer::GROUP));
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_group.xml", &getFactoryMap());
}
LLPanelGroup::~LLPanelGroup()
{
	LLGroupMgr::getInstance()->removeObserver(this);
	for (int i = mTabContainer->getTabCount() - 1; i >=0; --i)
	{
		LLPanelGroupTab* panelp =
			(LLPanelGroupTab*) mTabContainer->getPanelByIndex(i);
		if ( panelp ) panelp->removeObserver(this);
	}
}
void LLPanelGroup::updateTabVisibility()
{
	for (int i = mTabContainer->getTabCount() - 1; i >=0; --i)
	{
		LLPanelGroupTab* panelp =
			(LLPanelGroupTab*) mTabContainer->getPanelByIndex(i);
		BOOL visible = panelp->isVisibleByAgent(&gAgent) || gAgent.isGodlike();
		mTabContainer->enableTabButton(i, visible);
		if ( !visible && mCurrentTab == panelp )
		{
			mTabContainer->selectPrevTab();
			mCurrentTab =
				(LLPanelGroupTab*) mTabContainer->getCurrentPanel();
		}
	}
}
BOOL LLPanelGroup::postBuild()
{
	mTabContainer = getChild<LLTabContainer>("group_tab_container");
	if (mTabContainer)
	{
		if (gHippoGridManager->getConnectedGrid()->isSecondLife())
		{
			auto panel = mTabContainer->getPanelByName("voting_tab");
			mTabContainer->removeTabPanel(panel);
			delete panel;
		}
		LLPanelGroupTab* tabp = (LLPanelGroupTab*) mTabContainer->getCurrentPanel();
		if (!tabp)
		{
			mTabContainer->selectFirstTab();
			tabp = (LLPanelGroupTab*) mTabContainer->getCurrentPanel();
		}
		mCurrentTab = tabp;
		for (int i = mTabContainer->getTabCount() - 1; i >=0; --i)
		{
			LLPanel* tab_panel = mTabContainer->getPanelByIndex(i);
			LLPanelGroupTab* panelp =(LLPanelGroupTab*)tab_panel;
			panelp->setAllowEdit(mAllowEdit);
			panelp->addObserver(this);
		}
		mTabContainer->setCommitCallback(boost::bind(&LLPanelGroup::handleClickTab,this));
		updateTabVisibility();
		mCurrentTab->activate();
	}
	mDefaultNeedsApplyMesg = getString("default_needs_apply_text");
	mWantApplyMesg = getString("want_apply_text");
	LLButton* button = getChild<LLButton>("btn_ok");
	if (button)
	{
		button->setClickedCallback(boost::bind(&LLPanelGroup::onBtnOK,this));
		button->setVisible(mAllowEdit);
	}
	button = getChild<LLButton>("btn_cancel");
	if (button)
	{
		button->setClickedCallback(boost::bind(&LLPanelGroup::onBtnCancel,this));
		button->setEnabled(mAllowEdit);
	}
	button = getChild<LLButton>("btn_apply");
	if (button)
	{
		button->setClickedCallback(boost::bind(&LLPanelGroup::onBtnApply,this));
		button->setEnabled(FALSE);
		mApplyBtn = button;
	}
	button = getChild<LLButton>("btn_refresh");
	if (button)
	{
		button->setClickedCallback(boost::bind(&LLPanelGroup::onBtnRefresh,this));
	}
	return TRUE;
}
void LLPanelGroup::changed(LLGroupChange gc)
{
	updateTabVisibility();
	LLPanelGroupTab* panelp = (LLPanelGroupTab*) mTabContainer->getCurrentPanel();
	if (panelp)
	{
		panelp->update(gc);
	}
}
void LLPanelGroup::tabChanged()
{
	std::string str;
	const bool need = mCurrentTab->needsApply(str);
	if ( mApplyBtn )
	{
		mApplyBtn->setEnabled(need);
	}
	if (mAllowEdit) return;
	if (LLUICtrl* ctrl = getChild<LLUICtrl>("btn_cancel"))
		ctrl->setEnabled(need);
}
void LLPanelGroup::handleClickTab()
{
	if (mIgnoreTransition)
	{
		return;
	}
	mRequestedTab = (LLPanelGroupTab*) mTabContainer->getCurrentPanel();
	if (mRequestedTab == mCurrentTab)
	{
		return;
	}
	attemptTransition();
}
void LLPanelGroup::setGroupID(const LLUUID& group_id)
{
	LLGroupMgr::getInstance()->removeObserver(this);
	mID = group_id;
	LLGroupMgr::getInstance()->addObserver(this);
	deleteAllChildren();
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_group.xml", &getFactoryMap());
}
void LLPanelGroup::selectTab(std::string tab_name)
{
	const BOOL recurse = TRUE;
	LLPanelGroupTab* tabp =
		getChild<LLPanelGroupTab>(tab_name, recurse);
	if ( tabp && mTabContainer )
	{
		mTabContainer->selectTabPanel(tabp);
		handleClickTab();
	}
}
BOOL LLPanelGroup::canClose()
{
	if (mShowingNotifyDialog) return FALSE;
	if (mCurrentTab && mCurrentTab->hasModal()) return FALSE;
	if (mForceClose || !mAllowEdit) return TRUE;
	mRequestedTab = NULL;
	return attemptTransition();
}
BOOL LLPanelGroup::attemptTransition()
{
	std::string mesg;
	if (mCurrentTab && mCurrentTab->needsApply(mesg))
	{
		if (mesg.empty())
		{
			mesg = mDefaultNeedsApplyMesg;
		}
		LLSD args;
		args["NEEDS_APPLY_MESSAGE"] = mesg;
		args["WANT_APPLY_MESSAGE"] = mWantApplyMesg;
		LLNotificationsUtil::add("PanelGroupApply", args, LLSD(),
			boost::bind(&LLPanelGroup::handleNotifyCallback, this, _1, _2));
		mShowingNotifyDialog = TRUE;
		if (mTabContainer)
		{
			mIgnoreTransition = TRUE;
			mTabContainer->selectTabPanel( mCurrentTab );
			mIgnoreTransition = FALSE;
		}
		return FALSE;
	}
	else
	{
		if ( mRequestedTab )
		{
			transitionToTab();
		}
		return TRUE;
	}
}
void LLPanelGroup::transitionToTab()
{
	if (mCurrentTab)
	{
		mCurrentTab->deactivate();
	}
	if (mRequestedTab)
	{
		mCurrentTab = mRequestedTab;
		mCurrentTab->activate();
	}
	else
	{
		close();
	}
}
bool LLPanelGroup::handleNotifyCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	mShowingNotifyDialog = FALSE;
	switch (option)
	{
	case 0:
		if ( !apply() )
		{
			break;
		}
		mIgnoreTransition = TRUE;
		mTabContainer->selectTabPanel( mRequestedTab );
		mIgnoreTransition = FALSE;
		transitionToTab();
		break;
	case 1:
		mCurrentTab->cancel();
		mIgnoreTransition = TRUE;
		mTabContainer->selectTabPanel( mRequestedTab );
		mIgnoreTransition = FALSE;
		transitionToTab();
		break;
	case 2:
	default:
		LLAppViewer::instance()->abortQuit();
		break;
	}
	return false;
}
void LLPanelGroup::onBtnOK(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	if(self->apply())
	{
		self->close();
	}
}
void LLPanelGroup::onBtnCancel(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	if (self->mAllowEdit)
		self->close();
	else
		self->refreshData();
}
void LLPanelGroup::onBtnApply(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	self->apply();
}
bool LLPanelGroup::apply()
{
	if (!mTabContainer) return false;
	LLPanelGroupTab* panelp = (LLPanelGroupTab*) mTabContainer->getCurrentPanel();
	if (!panelp) return false;
	std::string mesg;
	if ( !panelp->needsApply(mesg) )
	{
		return true;
	}
	std::string apply_mesg;
	if ( panelp->apply( apply_mesg ) )
	{
		return true;
	}
	if ( !apply_mesg.empty() )
	{
		LLSD args;
		args["MESSAGE"] = apply_mesg;
		LLNotificationsUtil::add("GenericAlert", args);
	}
	return false;
}
void LLPanelGroup::onBtnRefresh(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	self->refreshData();
}
void LLPanelGroup::draw()
{
	LLPanel::draw();
	if (mRefreshTimer.hasExpired())
	{
		mRefreshTimer.stop();
		childEnable("btn_refresh");
	}
	if (mCurrentTab)
	{
		std::string mesg;
		childSetEnabled("btn_apply", mCurrentTab->needsApply(mesg));
	}
}
void LLPanelGroup::refreshData()
{
	LLGroupMgr::getInstance()->clearGroupData(getID());
	mCurrentTab->activate();
	childDisable("btn_refresh");
	mRefreshTimer.start();
	mRefreshTimer.setTimerExpirySec(5);
}
void LLPanelGroup::close()
{
	LLView* viewp = getParent();
	LLFloater* floaterp = dynamic_cast<LLFloater*>(viewp);
	if (floaterp)
	{
		mForceClose = TRUE;
		floaterp->close();
	}
}
void LLPanelGroup::showNotice(const std::string& subject,
							  const std::string& message,
							  const bool& has_inventory,
							  const std::string& inventory_name,
							  LLOfferInfo* inventory_offer)
{
	if (mCurrentTab->getName() != "notices_tab")
	{
		if (inventory_offer)
		{
			inventory_offer->forceResponse(IOR_DECLINE);
		}
		return;
	}
	LLPanelGroupNotices* notices = static_cast<LLPanelGroupNotices*>(mCurrentTab);
	notices->showNotice(subject,message,has_inventory,inventory_name,inventory_offer);
}
