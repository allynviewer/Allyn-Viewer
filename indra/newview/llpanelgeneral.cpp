/** 
 * @file llpanelgeneral.cpp
 * @brief General preferences panel in preferences floater
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
#include "llpanelgeneral.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llagent.h"
#include "llfloaterpreference.h"
#include "llviewerregion.h"
#include "llavatarnamecache.h"
#include "llvoavatar.h"
#include "llcallingcard.h"
#include "llnotifications.h"
#include "llviewerwindow.h"
LLPanelGeneral::LLPanelGeneral()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_general.xml");
}
BOOL LLPanelGeneral::postBuild()
{
	LLComboBox* fade_out_combobox = getChild<LLComboBox>("fade_out_combobox");
	fade_out_combobox->setCurrentByIndex(gSavedSettings.getS32("RenderName"));
	LLComboBox* namesystem_combobox = getChild<LLComboBox>("namesystem_combobox");
	namesystem_combobox->setCurrentByIndex(gSavedSettings.getS32("PhoenixNameSystem"));
	getChild<LLUICtrl>("show_resident_checkbox")->setValue(gSavedSettings.getBOOL("LiruShowLastNameResident"));
	std::string login_location = gSavedSettings.getString("LoginLocation");
	if(login_location != "last" && login_location != "home")
		login_location = "last";
	childSetValue("default_start_location", login_location);
	childSetValue("show_location_checkbox", gSavedSettings.getBOOL("ShowStartLocation"));
	childSetValue("show_all_title_checkbox", gSavedSettings.getBOOL("RenderHideGroupTitleAll"));
	childSetValue("language_is_public", gSavedSettings.getBOOL("LanguageIsPublic"));
	childSetValue("show_my_name_checkbox", gSavedSettings.getBOOL("RenderNameHideSelf"));
	childSetValue("small_avatar_names_checkbox", gSavedSettings.getBOOL("SmallAvatarNames"));
	childSetValue("show_my_title_checkbox", gSavedSettings.getBOOL("RenderHideGroupTitle"));
	childSetValue("away_when_idle_checkbox", gSavedSettings.getBOOL("AllowIdleAFK"));
	childSetValue("afk_timeout_spinner", gSavedSettings.getF32("AFKTimeout"));
	childSetValue("notify_money_change_checkbox", gSavedSettings.getBOOL("NotifyMoneyChange"));
	childSetValue("no_transaction_clutter_checkbox", gSavedSettings.getBOOL("LiruNoTransactionClutter"));
	childSetValue("ui_scale_slider", gSavedSettings.getF32("UIScaleFactor"));
	childSetValue("ui_auto_scale", gSavedSettings.getBOOL("UIAutoScale"));
	LLComboBox* crash_behavior_combobox = getChild<LLComboBox>("crash_behavior_combobox");
	crash_behavior_combobox->setValue(gSavedSettings.getS32("CrashSubmitBehavior"));
	childSetValue("language_combobox", 	gSavedSettings.getString("Language"));
	bool can_choose = gAgent.getID().notNull() &&
					 (gAgent.isMature() || gAgent.isGodlike());
	if (can_choose)
	{
		if (!gAgent.isAdult() && !gAgent.isGodlike())
		{
			LLComboBox* maturity_combo = getChild<LLComboBox>("maturity_desired_combobox");
			maturity_combo->remove(0);
		}
	}
	U32 preferred_maturity = gSavedSettings.getU32("PreferredMaturity");
	childSetValue("maturity_desired_combobox", int(preferred_maturity));
	std::string selected_item_label = getChild<LLComboBox>("maturity_desired_combobox")->getSelectedItemLabel();
	childSetValue("maturity_desired_textbox", selected_item_label);
	childSetVisible("maturity_desired_combobox", can_choose);
	childSetVisible("maturity_desired_textbox",	!can_choose);
	bool allow_idle = gSavedSettings.getBOOL("AllowIdleAFK");
	childSetEnabled("afk_timeout_spinner", allow_idle);
	childSetEnabled("seconds_textbox", allow_idle);
	childSetCommitCallback("away_when_idle_checkbox", &onClickCheckbox, this);
	childSetEnabled("no_transaction_clutter_checkbox", gSavedSettings.getBOOL("NotifyMoneyChange"));
	childSetCommitCallback("notify_money_change_checkbox", &onClickCheckbox, this);
	childSetAction("clear_settings", &onClickClearSettings, this);
	return TRUE;
}
LLPanelGeneral::~LLPanelGeneral()
{
}
void LLPanelGeneral::apply()
{
	LLComboBox* fade_out_combobox = getChild<LLComboBox>("fade_out_combobox");
	gSavedSettings.setS32("RenderName", fade_out_combobox->getCurrentIndex());
	gSavedSettings.setS32("PhoenixNameSystem", getChild<LLComboBox>("namesystem_combobox")->getCurrentIndex());
	gSavedSettings.setBOOL("LiruShowLastNameResident", getChild<LLUICtrl>("show_resident_checkbox")->getValue());
	gSavedSettings.setString("LoginLocation", childGetValue("default_start_location").asString());
	gSavedSettings.setBOOL("ShowStartLocation", childGetValue("show_location_checkbox"));
	gSavedSettings.setBOOL("RenderHideGroupTitleAll", childGetValue("show_all_title_checkbox"));
	gSavedSettings.setBOOL("LanguageIsPublic", childGetValue("language_is_public"));
	gSavedSettings.setBOOL("RenderNameHideSelf", childGetValue("show_my_name_checkbox"));
	gSavedSettings.setBOOL("SmallAvatarNames", childGetValue("small_avatar_names_checkbox"));
	gSavedSettings.setBOOL("RenderHideGroupTitle", childGetValue("show_my_title_checkbox"));
	gSavedSettings.setBOOL("AllowIdleAFK", childGetValue("away_when_idle_checkbox"));
	gSavedSettings.setF32("AFKTimeout", childGetValue("afk_timeout_spinner").asReal());
	gSavedSettings.setBOOL("NotifyMoneyChange", childGetValue("notify_money_change_checkbox"));
	gSavedSettings.setBOOL("LiruNoTransactionClutter", childGetValue("no_transaction_clutter_checkbox"));
	F32 ui_scale = (F32)childGetValue("ui_scale_slider").asReal();
	gSavedSettings.setF32("UIScaleFactor", ui_scale);
	gSavedSettings.setBOOL("UIAutoScale", childGetValue("ui_auto_scale"));
	gSavedSettings.setString("Language", childGetValue("language_combobox"));
	LLComboBox* crash_behavior_combobox = getChild<LLComboBox>("crash_behavior_combobox");
	gSavedSettings.setS32("CrashSubmitBehavior", crash_behavior_combobox->getValue());
	if (getChild<LLComboBox>("maturity_desired_combobox")->getVisible())
	{
		gSavedSettings.setU32("PreferredMaturity",
			(U32)childGetValue("maturity_desired_combobox").asInteger());
	}
	if (gViewerWindow)
	{
		gViewerWindow->reshape(gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw());
	}
}
void LLPanelGeneral::cancel()
{
}
void LLPanelGeneral::onClickCheckbox(LLUICtrl* ctrl, void* data)
{
	LLPanelGeneral* self = (LLPanelGeneral*)data;
	bool enabled = ctrl->getValue().asBoolean();
	if(ctrl->getName() == "away_when_idle_checkbox")
	{
		self->childSetEnabled("afk_timeout_spinner", enabled);
		self->childSetEnabled("seconds_textbox", enabled);
	}
	else if(ctrl->getName() == "notify_money_change_checkbox")
		self->childSetEnabled("no_transaction_clutter_checkbox", enabled);
}
void LLPanelGeneral::onClickClearSettings(void*)
{
	if(gAgent.getID().notNull()) {
		LLNotifications::instance().add("ResetAllSettingsPrompt", LLSD(), LLSD(), &callbackResetAllSettings);
	}
	else
	{
		LLNotifications::instance().add("ResetSystemSettingsPrompt", LLSD(), LLSD(), &callbackResetAllSettings);
	}
}
void LLPanelGeneral::callbackResetAllSettings(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if(option != 3)
	{
		std::string client_settings_file = gSavedSettings.getString("ClientSettingsFile");
		if(option != 2)
		{
			gSavedSettings.resetToDefaults();
			gSavedSettings.setString("ClientSettingsFile", client_settings_file);
			gSavedSettings.saveToFile(client_settings_file, TRUE);
		}
		if(gAgent.getID().notNull() && (option != 1))
		{
			gSavedPerAccountSettings.resetToDefaults();
			gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), TRUE);
		}
		LLNotifications::instance().add("ResetSettingsComplete");
		LLFloaterPreference::closeWithoutSaving();
	}
}
