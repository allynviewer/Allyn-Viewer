/** 
 * @file llfloaterpreference.cpp
 * @brief Global preferences with and without persistence.
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
#include "llfloaterpreference.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lldir.h"
#include "llfocusmgr.h"
#include "llscrollbar.h"
#include "llspinctrl.h"
#include "message.h"
#include "llcommandhandler.h"
#include "llfloaterabout.h"
#include "llfloaterpreference.h"
#include "llpanelnetwork.h"
#include "llpanelaudioprefs.h"
#include "llpaneldisplay.h"
#include "llpanelgeneral.h"
#include "llpanelinput.h"
#include "llpanellogin.h"
#include "llpanelmsgs.h"
#include "llpanelweb.h"
#include "llpanelskins.h"
#include "hippopanelgrids.h"
#include "llprefschat.h"
#include "llprefsvoice.h"
#include "llprefsim.h"
#include "ascentprefschat.h"
#include "ascentprefssys.h"
#include "ascentprefsvan.h"
#include "llprefstranslation.h"
#include "llfiltereditor.h"
#include "llsearchableui.h"
#include "llresizehandle.h"
#include "llresmgr.h"
#include "llassetstorage.h"
#include "llagent.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "lluictrlfactory.h"
#include "lltabcontainer.h"
#include "llview.h"
#include "llpanel.h"
#include "llstring.h"
#include "llviewerwindow.h"
#include "llkeyboard.h"
#include "llscrollcontainer.h"
#include "hippopanelgrids.h"
const S32 PREF_BORDER = 4;
const S32 PREF_PAD = 5;
const S32 PREF_BUTTON_WIDTH = 70;
const S32 PREF_CATEGORY_WIDTH = 188;
const S32 PREF_FLOATER_MIN_HEIGHT = 2 * SCROLLBAR_SIZE + 2 * LLPANEL_BORDER_WIDTH + 96;
LLFloaterPreference* LLFloaterPreference::sInstance = NULL;
class LLPreferencesHandler : public LLCommandHandler
{
public:
	LLPreferencesHandler() : LLCommandHandler("preferences", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		LLFloaterPreference::show(NULL);
		return true;
	}
};
LLPreferencesHandler gPreferencesHandler;
S32 pref_min_width()
{
	return
	2 * PREF_BORDER +
	2 * PREF_BUTTON_WIDTH +
	PREF_PAD + RESIZE_HANDLE_WIDTH +
	PREF_CATEGORY_WIDTH +
	PREF_PAD;
}
S32 pref_min_height()
{
	return
	2 * PREF_BORDER +
	3*(BTN_HEIGHT + PREF_PAD) +
	PREF_FLOATER_MIN_HEIGHT;
}
LLPreferenceCore::LLPreferenceCore(LLTabContainer* tab_container, LLButton * default_btn) :
	mTabContainer(tab_container),
	mGeneralPanel(NULL),
	mInputPanel(NULL),
	mNetworkPanel(NULL),
	mDisplayPanel(NULL),
	mAudioPanel(NULL),
	mMsgPanel(NULL),
	mSkinsPanel(NULL),
	mGridsPanel(NULL),
	mPrefsAscentChat(NULL),
	mPrefsAscentSys(NULL),
	mPrefsAscentVan(NULL),
	mPrefsTranslation(NULL)
{
	mGeneralPanel = new LLPanelGeneral();
	mTabContainer->addTabPanel(mGeneralPanel, mGeneralPanel->getLabel());
	mGeneralPanel->setDefaultBtn(default_btn);
	mInputPanel = new LLPanelInput();
	mTabContainer->addTabPanel(mInputPanel, mInputPanel->getLabel());
	mInputPanel->setDefaultBtn(default_btn);
	mNetworkPanel = new LLPanelNetwork();
	mTabContainer->addTabPanel(mNetworkPanel, mNetworkPanel->getLabel());
	mNetworkPanel->setDefaultBtn(default_btn);
	mWebPanel = new LLPanelWeb();
	mTabContainer->addTabPanel(mWebPanel, mWebPanel->getLabel());
	mWebPanel->setDefaultBtn(default_btn);
	mDisplayPanel = new LLPanelDisplay();
	mTabContainer->addTabPanel(mDisplayPanel, mDisplayPanel->getLabel());
	mDisplayPanel->setDefaultBtn(default_btn);
	mAudioPanel = new LLPanelAudioPrefs();
	mTabContainer->addTabPanel(mAudioPanel, mAudioPanel->getLabel());
	mAudioPanel->setDefaultBtn(default_btn);
	mPrefsChat = new LLPrefsChat();
	mTabContainer->addTabPanel(mPrefsChat->getPanel(), mPrefsChat->getPanel()->getLabel());
	mPrefsChat->getPanel()->setDefaultBtn(default_btn);
	mPrefsVoice = new LLPrefsVoice();
	mTabContainer->addTabPanel(mPrefsVoice, mPrefsVoice->getLabel());
	mPrefsVoice->setDefaultBtn(default_btn);
	mPrefsIM = new LLPrefsIM();
	mTabContainer->addTabPanel(mPrefsIM->getPanel(), mPrefsIM->getPanel()->getLabel());
	mPrefsIM->getPanel()->setDefaultBtn(default_btn);
	mMsgPanel = new LLPanelMsgs();
	mTabContainer->addTabPanel(mMsgPanel, mMsgPanel->getLabel());
	mMsgPanel->setDefaultBtn(default_btn);
	mSkinsPanel = new LLPanelSkins();
	mTabContainer->addTabPanel(mSkinsPanel, mSkinsPanel->getLabel());
	mSkinsPanel->setDefaultBtn(default_btn);
	mGridsPanel = HippoPanelGrids::create();
	mTabContainer->addTabPanel(mGridsPanel, mGridsPanel->getLabel());
	mGridsPanel->setDefaultBtn(default_btn);
	mPrefsAscentChat = new LLPrefsAscentChat();
	mTabContainer->addTabPanel(mPrefsAscentChat, mPrefsAscentChat->getLabel());
	mPrefsAscentChat->setDefaultBtn(default_btn);
	mPrefsAscentSys = new LLPrefsAscentSys();
	mTabContainer->addTabPanel(mPrefsAscentSys, mPrefsAscentSys->getLabel());
	mPrefsAscentSys->setDefaultBtn(default_btn);
	mPrefsAscentVan = new LLPrefsAscentVan();
	mTabContainer->addTabPanel(mPrefsAscentVan, mPrefsAscentVan->getLabel());
	mPrefsAscentVan->setDefaultBtn(default_btn);
	mPrefsTranslation = new LLPrefsTranslation();
	mTabContainer->addTabPanel(mPrefsTranslation, mPrefsTranslation->getLabel());
	mPrefsTranslation->setDefaultBtn(default_btn);
	mTabContainer->setTabImage(mGeneralPanel, "pref_nav_general.tga");
	mTabContainer->setTabImage(mInputPanel, "pref_nav_input.tga");
	mTabContainer->setTabImage(mNetworkPanel, "pref_nav_network.tga");
	mTabContainer->setTabImage(mWebPanel, "pref_nav_web.tga");
	mTabContainer->setTabImage(mDisplayPanel, "pref_nav_graphics.tga");
	mTabContainer->setTabImage(mAudioPanel, "pref_nav_audio.tga");
	mTabContainer->setTabImage(mPrefsChat->getPanel(), "pref_nav_chat.tga");
	mTabContainer->setTabImage(mPrefsVoice, "pref_nav_voice.tga");
	mTabContainer->setTabImage(mPrefsIM->getPanel(), "pref_nav_im.tga");
	mTabContainer->setTabImage(mMsgPanel, "pref_nav_notify.tga");
	mTabContainer->setTabImage(mSkinsPanel, "pref_nav_skins.tga");
	mTabContainer->setTabImage(mGridsPanel, "pref_nav_grids.tga");
	mTabContainer->setTabImage(mPrefsAscentChat, "pref_nav_advchat.tga");
	mTabContainer->setTabImage(mPrefsAscentSys, "pref_nav_system.tga");
	mTabContainer->setTabImage(mPrefsAscentVan, "pref_nav_vanity.tga");
	mTabContainer->setTabImage(mPrefsTranslation, "pref_nav_translate.tga");
	mTabContainer->setCommitCallback(boost::bind(LLPreferenceCore::onTabChanged,_1));
	if (!mTabContainer->selectTab(gSavedSettings.getS32("LastPrefTab")))
	{
		mTabContainer->selectFirstTab();
	}
}
LLPreferenceCore::~LLPreferenceCore()
{
	if (mGeneralPanel)
	{
		delete mGeneralPanel;
		mGeneralPanel = NULL;
	}
	if (mInputPanel)
	{
		delete mInputPanel;
		mInputPanel = NULL;
	}
	if (mNetworkPanel)
	{
		delete mNetworkPanel;
		mNetworkPanel = NULL;
	}
	if (mDisplayPanel)
	{
		delete mDisplayPanel;
		mDisplayPanel = NULL;
	}
	if (mAudioPanel)
	{
		delete mAudioPanel;
		mAudioPanel = NULL;
	}
	if (mPrefsChat)
	{
		delete mPrefsChat;
		mPrefsChat = NULL;
	}
	if (mPrefsIM)
	{
		delete mPrefsIM;
		mPrefsIM = NULL;
	}
	if (mMsgPanel)
	{
		delete mMsgPanel;
		mMsgPanel = NULL;
	}
	if (mWebPanel)
	{
		delete mWebPanel;
		mWebPanel = NULL;
	}
	if (mSkinsPanel)
	{
		delete mSkinsPanel;
		mSkinsPanel = NULL;
	}
	if (mGridsPanel)
	{
		delete mGridsPanel;
		mGridsPanel = NULL;
	}
	if (mPrefsAscentChat)
	{
		delete mPrefsAscentChat;
		mPrefsAscentChat = NULL;
	}
	if (mPrefsAscentSys)
	{
		delete mPrefsAscentSys;
		mPrefsAscentSys = NULL;
	}
	if (mPrefsAscentVan)
	{
		delete mPrefsAscentVan;
		mPrefsAscentVan = NULL;
	}
	if (mPrefsTranslation)
	{
		delete mPrefsTranslation;
		mPrefsTranslation = NULL;
	}
}
void LLPreferenceCore::apply()
{
	mGeneralPanel->apply();
	mInputPanel->apply();
	mNetworkPanel->apply();
	mDisplayPanel->apply();
	mAudioPanel->apply();
	mPrefsChat->apply();
	mPrefsVoice->apply();
	mPrefsIM->apply();
	mMsgPanel->apply();
	mSkinsPanel->apply();
	mGridsPanel->apply();
	mPrefsAscentChat->apply();
	mPrefsAscentSys->apply();
	mPrefsAscentVan->apply();
	mPrefsTranslation->apply();
	mWebPanel->apply();
}
void LLPreferenceCore::cancel()
{
	mGeneralPanel->cancel();
	mInputPanel->cancel();
	mNetworkPanel->cancel();
	mDisplayPanel->cancel();
	mAudioPanel->cancel();
	mPrefsChat->cancel();
	mPrefsVoice->cancel();
	mPrefsIM->cancel();
	mMsgPanel->cancel();
	mSkinsPanel->cancel();
	mGridsPanel->cancel();
	mPrefsAscentChat->cancel();
	mPrefsAscentSys->cancel();
	mPrefsAscentVan->cancel();
	mPrefsTranslation->cancel();
	mWebPanel->cancel();
}
void LLPreferenceCore::onTabChanged(LLUICtrl* ctrl)
{
	LLTabContainer* self = (LLTabContainer*)ctrl;
	gSavedSettings.setS32("LastPrefTab", self->getCurrentPanelIndex());
}
void LLPreferenceCore::setPersonalInfo(const std::string& visibility, bool im_via_email, const std::string& email, bool is_verified)
{
	mPrefsIM->setPersonalInfo(visibility, im_via_email, email, is_verified);
}
void reset_to_default(const std::string& control)
{
	LLUI::getControlControlGroup(control).getControl(control)->resetToDefault(true);
}
LLFloaterPreference::LLFloaterPreference()
:	mFilterEdit(NULL)
,	mSearchDataDirty(true)
{
	mExitWithoutSaving = false;
	mCommitCallbackRegistrar.add("Prefs.Reset", boost::bind(reset_to_default, _2));
	mCommitCallbackRegistrar.add("UpdateFilter", boost::bind(&LLFloaterPreference::onUpdateFilterTerm, this, false));
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_preferences.xml");
}
BOOL LLFloaterPreference::postBuild()
{
	requires<LLButton>("About...");
	requires<LLButton>("OK");
	requires<LLButton>("Cancel");
	requires<LLButton>("Apply");
	requires<LLTabContainer>("pref core");
	requires<LLFilterEditor>("search_prefs_edit");
	if (!checkRequirements())
	{
		return FALSE;
	}
	getChild<LLUICtrl>("About...")->setCommitCallback(boost::bind(LLFloaterAbout::show, (void*)0));
	getChild<LLUICtrl>("Apply")->setCommitCallback(boost::bind(&LLFloaterPreference::onBtnApply, this));
	getChild<LLUICtrl>("Cancel")->setCommitCallback(boost::bind(&LLFloaterPreference::onBtnCancel, this));
	getChild<LLUICtrl>("OK")->setCommitCallback(boost::bind(&LLFloaterPreference::onBtnOK, this));
	mPreferenceCore = new LLPreferenceCore(getChild<LLTabContainer>("pref core"), getChild<LLButton>("OK"));
	mFilterEdit = getChild<LLFilterEditor>("search_prefs_edit");
	mFilterEdit->setKeystrokeCallback(boost::bind(&LLFloaterPreference::onUpdateFilterTerm, this, false));
	collectSearchableItems();
	sInstance = this;
	return TRUE;
}
LLFloaterPreference::~LLFloaterPreference()
{
	sInstance = NULL;
	delete mPreferenceCore;
}
void LLFloaterPreference::apply()
{
	mPreferenceCore->apply();
}
void LLFloaterPreference::cancel()
{
	mPreferenceCore->cancel();
}
void LLFloaterPreference::show(void*)
{
	if (!sInstance)
	{
		new LLFloaterPreference();
		sInstance->center();
	}
	sInstance->open();
	gAgent.sendAgentUserInfoRequest();
	LLPanelLogin::setAlwaysRefresh(true);
}
void LLFloaterPreference::onBtnOK()
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	if (canClose())
	{
		apply();
		gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);
		if (gAgentID.notNull())
			gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), TRUE);
		mExitWithoutSaving = true;
		close(false);
	}
	else
	{
		LL_INFOS() << "Can't close preferences!" << LL_ENDL;
	}
	LLPanelLogin::updateLocationSelectorsVisibility();
}
void LLFloaterPreference::onBtnApply()
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	apply();
	gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);
	if (gAgentID.notNull())
		gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), TRUE);
	LLPanelLogin::updateLocationSelectorsVisibility();
}
void LLFloaterPreference::onClose(bool app_quitting)
{
	LLPanelLogin::setAlwaysRefresh(false);
	if (!mExitWithoutSaving)
	{
		cancel();
	}
	LLFloater::onClose(app_quitting);
}
void LLFloaterPreference::onBtnCancel()
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	close();
}
void LLFloaterPreference::updateUserInfo(const std::string& visibility, bool im_via_email, const std::string& email, bool is_verified)
{
	if (sInstance && sInstance->mPreferenceCore)
	{
		sInstance->mPreferenceCore->setPersonalInfo(visibility, im_via_email, email, is_verified);
	}
}
void LLFloaterPreference::switchTab(S32 i)
{
	sInstance->mPreferenceCore->getTabContainer()->selectTab(i);
}
void LLFloaterPreference::closeWithoutSaving()
{
	sInstance->mExitWithoutSaving = true;
	sInstance->close();
}
void LLFloaterPreference::onUpdateFilterTerm(bool force)
{
	if (!mFilterEdit)
		return;

	LLWString searchValue = utf8str_to_wstring(mFilterEdit->getValue().asString());
	LLWStringUtil::toLower(searchValue);

	if (!mSearchData || (mSearchData->mLastFilter == searchValue && !force))
		return;

	if (mSearchDataDirty)
	{
		collectSearchableItems();
	}

	mSearchData->mLastFilter = searchValue;

	if (!mSearchData->mRootTab)
		return;

	mSearchData->mRootTab->hightlightAndHide(searchValue);
	if (LLTabContainer* pRoot = getChild<LLTabContainer>("pref core"))
	{
		if (!searchValue.empty())
		{
			pRoot->selectFirstTab();
		}
	}
}
static void collectChildren(LLView const* aView, ll::prefs::PanelDataPtr aParentPanel, ll::prefs::TabContainerDataPtr aParentTabContainer)
{
	if (!aView)
		return;

	llassert_always(aParentPanel || aParentTabContainer);

	const LLView::child_list_t* child_list = aView->getChildList();
	for (LLView::child_list_const_iter_t it = child_list->begin(); it != child_list->end(); ++it)
	{
		LLView* pView = *it;
		if (!pView)
			continue;

		ll::prefs::PanelDataPtr pCurPanelData = aParentPanel;
		ll::prefs::TabContainerDataPtr pCurTabContainer = aParentTabContainer;

		LLPanel const* pPanel = dynamic_cast<LLPanel const*>(pView);
		LLTabContainer const* pTabContainer = dynamic_cast<LLTabContainer const*>(pView);
		ll::ui::SearchableControl const* pSCtrl = dynamic_cast<ll::ui::SearchableControl const*>(pView);

		if (pTabContainer)
		{
			pCurPanelData.reset();

			pCurTabContainer = ll::prefs::TabContainerDataPtr(new ll::prefs::TabContainerData);
			pCurTabContainer->mTabContainer = const_cast<LLTabContainer*>(pTabContainer);
			pCurTabContainer->mLabel = pTabContainer->getLabel();
			pCurTabContainer->mPanel = 0;

			if (aParentPanel)
				aParentPanel->mChildPanel.push_back(pCurTabContainer);
			if (aParentTabContainer)
				aParentTabContainer->mChildPanel.push_back(pCurTabContainer);
		}
		else if (pPanel)
		{
			pCurTabContainer.reset();

			pCurPanelData = ll::prefs::PanelDataPtr(new ll::prefs::PanelData);
			pCurPanelData->mPanel = pPanel;
			pCurPanelData->mLabel = pPanel->getLabel();

			llassert_always(aParentPanel || aParentTabContainer);

			if (aParentTabContainer)
				aParentTabContainer->mChildPanel.push_back(pCurPanelData);
			else if (aParentPanel)
				aParentPanel->mChildPanel.push_back(pCurPanelData);
		}
		else if (pSCtrl && pSCtrl->getSearchText().size())
		{
			ll::prefs::SearchableItemPtr item = ll::prefs::SearchableItemPtr(new ll::prefs::SearchableItem());
			item->mView = pView;
			item->mCtrl = pSCtrl;

			item->mLabel = utf8str_to_wstring(pSCtrl->getSearchText());
			LLWStringUtil::toLower(item->mLabel);

			llassert_always(aParentPanel || aParentTabContainer);

			if (aParentPanel)
				aParentPanel->mChildren.push_back(item);
			if (aParentTabContainer)
				aParentTabContainer->mChildren.push_back(item);
		}
		collectChildren(pView, pCurPanelData, pCurTabContainer);
	}
}
void LLFloaterPreference::collectSearchableItems()
{
	mSearchData.reset();
	LLTabContainer* pRoot = getChild<LLTabContainer>("pref core");
	if (mFilterEdit && pRoot)
	{
		mSearchData.reset(new ll::prefs::SearchData());

		ll::prefs::TabContainerDataPtr pRootTabcontainer = ll::prefs::TabContainerDataPtr(new ll::prefs::TabContainerData);
		pRootTabcontainer->mTabContainer = pRoot;
		pRootTabcontainer->mLabel = pRoot->getLabel();
		mSearchData->mRootTab = pRootTabcontainer;

		collectChildren(this, ll::prefs::PanelDataPtr(), pRootTabcontainer);
	}
	mSearchDataDirty = false;
}
