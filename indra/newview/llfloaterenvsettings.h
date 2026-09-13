/** 
 * @file llfloaterskysettings.h
 * @brief LLFloaterEnvSettings class definition
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
#ifndef LL_LLFLOATERENVSETTINGS_H
#define LL_LLFLOATERENVSETTINGS_H
#include "llfloater.h"
class LLFloaterEnvSettings : public LLFloater
{
public:
	LLFloaterEnvSettings();
	virtual ~LLFloaterEnvSettings();
	void initCallbacks(void);
	static LLFloaterEnvSettings* instance();
	static void onClickHelp(void* data);
	static void onChangeDayTime(LLUICtrl* ctrl, void* userData);
	static void onChangeCloudCoverage(LLUICtrl* ctrl, void* userData);
	static void onChangeWaterFogDensity(LLUICtrl* ctrl, void* userData);
	static void onChangeUnderWaterFogMod(LLUICtrl* ctrl, void* userData);
	static void onChangeWaterColor(LLUICtrl* ctrl, void* userData);
	static void onOpenAdvancedSky(void* userData);
	static void onOpenAdvancedWater(void* userData);
	static void onUseEstateTime(void* userData);
	static void show();
	static bool isOpen();
	virtual void onClose(bool app_quitting);
	void syncMenu();
	std::string timeToString(F32 curTime);
private:
	static LLFloaterEnvSettings* sEnvSettings;
};
#endif
