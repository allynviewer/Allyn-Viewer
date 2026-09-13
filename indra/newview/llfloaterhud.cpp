/** 
 * @file llfloaterhud.cpp
 * @brief Implementation of HUD floater
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
#include "llfloaterhud.h"
#include "llviewercontrol.h"
#include "llmediactrl.h"
#include "llnotificationsutil.h"
#include "lluictrlfactory.h"
LLFloaterHUD* LLFloaterHUD::sInstance = 0;
#define super LLFloater
LLFloaterHUD::LLFloaterHUD()
:	LLFloater(std::string("floater_hud")),
	mWebBrowser(0)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_hud.xml");
	setIsChrome(TRUE);
	setTitleVisible(true);
	setBackgroundOpaque(TRUE);
	LLRect saved_position_rect = gSavedSettings.getRect("FloaterHUDRect2");
	reshape(saved_position_rect.getWidth(), saved_position_rect.getHeight(), FALSE);
	setRect(saved_position_rect);
	mWebBrowser = getChild<LLMediaCtrl>("floater_hud_browser" );
	if (mWebBrowser)
	{
		mWebBrowser->setTakeFocusOnClick(false);
		std::string language = LLUI::getLanguage();
		std::string base_url = gSavedSettings.getString("TutorialURL");
		std::string url = base_url + language + "/";
		mWebBrowser->navigateTo(url);
	}
	sInstance = this;
}
LLFloaterHUD* LLFloaterHUD::getInstance()
{
	if (!sInstance)
	{
		new LLFloaterHUD();
	}
	return sInstance;
}
LLFloaterHUD::~LLFloaterHUD()
{
	gSavedSettings.setRect("FloaterHUDRect2", getRect() );
	if (sInstance == this)
	{
		sInstance = NULL;
	}
}
void LLFloaterHUD::showHUD()
{
	if (gSavedSettings.getString("TutorialURL") == "")
	{
		LLNotificationsUtil::add("TutorialNotFound");
		return;
	}
	LLFloaterHUD* hud = getInstance();
	hud->open();
	hud->setFrontmost(FALSE);
}
void LLFloaterHUD::onClose(bool app_quitting)
{
	bool stay_visible = app_quitting;
	gSavedSettings.setBOOL("ShowTutorial", stay_visible);
	destroy();
}
