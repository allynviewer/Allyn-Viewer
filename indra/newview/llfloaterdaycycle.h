/** 
 * @file llfloaterdaycycle.h
 * @brief LLFloaterDayCycle class definition
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
#ifndef LL_LLFLOATERDAYCYCLE_H
#define LL_LLFLOATERDAYCYCLE_H
#include "llfloater.h"
#include <vector>
#include "llwlparamset.h"
#include "llwlanimator.h"
struct WLColorControl;
struct WLFloatControl;
struct LLWLSkyKey
{
public:
	std::string presetName;
	F32 time;
};
class LLFloaterDayCycle : public LLFloater
{
public:
	LLFloaterDayCycle();
	virtual ~LLFloaterDayCycle();
	static void onClickHelp(void* data);
	void initHelpBtn(const std::string& name, const std::string& xml_alert);
	void initCallbacks(void);
	static LLFloaterDayCycle* instance();
	static void onTimeSliderMoved(LLUICtrl* ctrl, void* userData);
	static void onKeyTimeMoved(LLUICtrl* ctrl, void* userData);
	static void onKeyTimeChanged(LLUICtrl* ctrl, void* userData);
	static void onKeyPresetChanged(LLUICtrl* ctrl, void* userData);
	static void onRunAnimSky(void* userData);
	static void onStopAnimSky(void* userData);
	static void onTimeRateChanged(LLUICtrl* ctrl, void* userData);
	static void onAddKey(void* userData);
	void deletePreset(std::string& presetName);
	static void onDeleteKey(void* userData);
	static void onNewPreset(void* userData);
	static void onSavePreset(void* userData);
	static bool saveAlertCallback(const LLSD& notification, const LLSD& response);
	static void onDeletePreset(void* userData);
	bool deleteAlertCallback(const LLSD& notification, const LLSD& response);
	static bool newPromptCallback(const LLSD& notification, const LLSD& response);
	static void onChangePresetName(LLUICtrl* ctrl);
	static void onUseLindenTime(void* userData);
	static void show();
	static bool isOpen();
	virtual void onClose(bool app_quitting);
	static void syncMenu();
	static void syncSliderTrack();
	static void syncTrack();
	static void addSliderKey(F32 time, const std::string& presetName);
private:
	static LLFloaterDayCycle* sDayCycle;
	static std::map<std::string, LLWLSkyKey> sSliderToKey;
	static const F32 sHoursPerDay;
};
#endif
