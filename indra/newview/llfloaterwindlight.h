/** 
 * @file llfloaterwindlight.h
 * @brief LLFloaterWindLight class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
#ifndef LL_LLFLOATERWINDLIGHT_H
#define LL_LLFLOATERWINDLIGHT_H
#include "llfloater.h"
struct WLColorControl;
struct WLFloatControl;
class LLComboBox;
class LLFloaterWindLight : public LLFloater
{
public:
	LLFloaterWindLight();
	virtual ~LLFloaterWindLight();
	void initCallbacks(void);
	static LLFloaterWindLight* instance();
	static void onClickHelp(void* data);
	void initHelpBtn(const std::string& name, const std::string& xml_alert);
	bool newPromptCallback(const LLSD& notification, const LLSD& response);
	void setColorSwatch(const std::string& name, const WLColorControl& from_ctrl, F32 k);
	void onColorControlRMoved(LLUICtrl* ctrl, void* userdata);
	void onColorControlGMoved(LLUICtrl* ctrl, void* userdata);
	void onColorControlBMoved(LLUICtrl* ctrl, void* userdata);
	void onColorControlIMoved(LLUICtrl* ctrl, void* userdata);
	void onFloatControlMoved(LLUICtrl* ctrl, void* userdata);
	void onGlowRMoved(LLUICtrl* ctrl, void* userdata);
	void onGlowBMoved(LLUICtrl* ctrl, void* userdata);
	void onSunMoved(LLUICtrl* ctrl, void* userdata);
	void onStarAlphaMoved(LLUICtrl* ctrl);
	void onNewPreset();
	void onSavePreset(LLUICtrl* ctrl);
	bool saveNotecardCallback(const LLSD& notification, const LLSD& response);
	bool saveAlertCallback(const LLSD& notification, const LLSD& response);
	void onDeletePreset();
	bool deleteAlertCallback(const LLSD& notification, const LLSD& response);
	void onChangePresetName(LLUICtrl* ctrl);
	void onCloudScrollXMoved(const LLSD& value);
	void onCloudScrollYMoved(const LLSD& value);
	void onCloudScrollXToggled(const LLSD& value);
	void onCloudScrollYToggled(const LLSD& value);
	static void show();
	static bool isOpen();
	virtual void onClose(bool app_quitting);
	void syncMenu();
	static void selectTab(std::string tab_name);
private:
	static LLFloaterWindLight* sWindLight;
	static std::set<std::string> sDefaultPresets;
	void onClickNext();
	void onClickPrev();
	void populateSkyPresetsList();
	LLComboBox*		mSkyPresetCombo;
};
#endif
