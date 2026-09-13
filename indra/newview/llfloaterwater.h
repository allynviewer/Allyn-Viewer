/** 
 * @file llfloaterwindlight.h
 * @brief LLFloaterWater class definition
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
#ifndef LL_LLFLOATER_WATER_H
#define LL_LLFLOATER_WATER_H
#include "llfloater.h"
#include "llwlparamset.h"
#include "llwlparammanager.h"
class LLComboBox;
struct WaterVector2Control;
struct WaterVector3Control;
struct WaterColorControl;
struct WaterFloatControl;
struct WaterExpFloatControl;
class LLFloaterWater : public LLFloater
{
public:
	LLFloaterWater();
	virtual ~LLFloaterWater();
	void initCallbacks(void);
	static LLFloaterWater* instance();
	static void onClickHelp(void* data);
	void initHelpBtn(const std::string& name, const std::string& xml_alert);
	bool newPromptCallback(const LLSD& notification, const LLSD& response);
	void onColorControlRMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);
	void onColorControlGMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);
	void onColorControlBMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);
	void onColorControlAMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);
	void onColorControlIMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);
	void onVector3ControlXMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl);
	void onVector3ControlYMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl);
	void onVector3ControlZMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl);
	void onVector2ControlXMoved(LLUICtrl* ctrl, WaterVector2Control* vector_ctrl);
	void onVector2ControlYMoved(LLUICtrl* ctrl, WaterVector2Control* vector_ctrl);
	void onFloatControlMoved(LLUICtrl* ctrl, WaterFloatControl* floatControl);
	void onExpFloatControlMoved(LLUICtrl* ctrl, WaterExpFloatControl* expFloatControl);
	void onWaterFogColorMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);
	void onNormalMapPicked(LLUICtrl* ctrl);
	void onNewPreset();
	void onSavePreset(LLUICtrl* ctrl);
	bool saveNotecardCallback(const LLSD& notification, const LLSD& response);
	bool saveAlertCallback(const LLSD& notification, const LLSD& response);
	void onDeletePreset();
	bool deleteAlertCallback(const LLSD& notification, const LLSD& response);
	void onChangePresetName(LLUICtrl* ctrl);
	static void show();
	static bool isOpen();
	virtual void onClose(bool app_quitting);
	void syncMenu();
private:
	static LLFloaterWater* sWaterMenu;
	static std::set<std::string> sDefaultPresets;
	void onClickNext();
	void onClickPrev();
	void populateWaterPresetsList();
	LLComboBox*		mWaterPresetCombo;
};
#endif
