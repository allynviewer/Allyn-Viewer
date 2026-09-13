/** 
 * @file llfloaterenvsettings.cpp
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
#include "llviewerprecompiledheaders.h"
#include "llfloaterenvsettings.h"
#include "llfloaterwindlight.h"
#include "llfloaterwater.h"
#include "lluictrlfactory.h"
#include "llsliderctrl.h"
#include "llcombobox.h"
#include "llcolorswatch.h"
#include "llwlanimator.h"
#include "llviewergenericmessage.h"
#include "meta7windlight.h"
#include "llwlparamset.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
#include "llmath.h"
#include "llviewerwindow.h"
#include "pipeline.h"
#include "rlvactions.h"
#include <sstream>
LLFloaterEnvSettings* LLFloaterEnvSettings::sEnvSettings = NULL;
LLFloaterEnvSettings::LLFloaterEnvSettings() : LLFloater(std::string("Environment Settings Floater"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_env_settings.xml");
	initCallbacks();
	syncMenu();
}
LLFloaterEnvSettings::~LLFloaterEnvSettings()
{
}
void LLFloaterEnvSettings::onClickHelp(void* data)
{
	LLFloaterEnvSettings* self = (LLFloaterEnvSettings*)data;
	self->addContextualNotification("EnvSettingsHelpButton");
}
void LLFloaterEnvSettings::initCallbacks(void)
{
	childSetCommitCallback("EnvTimeSlider", onChangeDayTime, NULL);
	childSetCommitCallback("EnvCloudSlider", onChangeCloudCoverage, NULL);
	childSetCommitCallback("EnvWaterFogSlider", onChangeWaterFogDensity,
		&LLWaterParamManager::getInstance()->mFogDensity);
	childSetCommitCallback("EnvWaterColor", onChangeWaterColor,
		&LLWaterParamManager::getInstance()->mFogColor);
	childSetAction("EnvAdvancedSkyButton", onOpenAdvancedSky, NULL);
	childSetAction("EnvAdvancedWaterButton", onOpenAdvancedWater, NULL);
	childSetAction("EnvUseEstateTimeButton", onUseEstateTime, NULL);
	childSetAction("EnvSettingsHelpButton", onClickHelp, this);
}
void LLFloaterEnvSettings::syncMenu()
{
	LLSliderCtrl* sldr;
	sldr = getChild<LLSliderCtrl>("EnvTimeSlider");
	F32 val = (F32)LLWLParamManager::getInstance()->mAnimator.getDayTime();
	std::string timeStr = timeToString(val);
	LLTextBox* textBox;
	textBox = getChild<LLTextBox>("EnvTimeText");
	textBox->setValue(timeStr);
	val -= 0.25;
	if(val < 0)
	{
		val++;
	}
	sldr->setValue(val);
	bool err;
	childSetValue("EnvCloudSlider", LLWLParamManager::getInstance()->mCurParams.getFloat("cloud_shadow", err));
	LLWaterParamManager * param_mgr = LLWaterParamManager::getInstance();
	LLColor4 col = param_mgr->getFogColor();
	LLColorSwatchCtrl* colCtrl = getChild<LLColorSwatchCtrl>("EnvWaterColor");
	col.mV[3] = 1.0f;
	colCtrl->set(col);
	childSetValue("EnvWaterFogSlider", param_mgr->mFogDensity.mExp);
	param_mgr->setDensitySliderValue(param_mgr->mFogDensity.mExp);
	if(LLWLParamManager::getInstance()->mAnimator.getUseLindenTime())
	{
		childDisable("EnvUseEstateTimeButton");
	} else {
		childEnable("EnvUseEstateTimeButton");
	}
	if (!LLGLSLShader::sNoFixedFunction)
	{
		childDisable("EnvWaterColor");
		childDisable("EnvWaterColorText");
	}
	else
	{
		childEnable("EnvWaterColor");
		childEnable("EnvWaterColorText");
	}
	if(!gPipeline.canUseWindLightShaders())
	{
		childDisable("EnvCloudSlider");
		childDisable("EnvCloudText");
	}
	else
	{
		childEnable("EnvCloudSlider");
		childEnable("EnvCloudText");
	}
}
LLFloaterEnvSettings* LLFloaterEnvSettings::instance()
{
	if (!sEnvSettings)
	{
		sEnvSettings = new LLFloaterEnvSettings();
	}
	return sEnvSettings;
}
void LLFloaterEnvSettings::show()
{
	if (RlvActions::hasBehaviour(RLV_BHVR_SETENV)) return;
	if (!sEnvSettings)
	{
		sEnvSettings = new LLFloaterEnvSettings();
		return;
	}
	LLFloaterEnvSettings* envSettings = instance();
	envSettings->syncMenu();
	if (envSettings->getVisible())
		envSettings->close();
	else
		envSettings->open();
}
bool LLFloaterEnvSettings::isOpen()
{
	if (sEnvSettings != NULL)
	{
		return sEnvSettings->getVisible();
	}
	return false;
}
void LLFloaterEnvSettings::onClose(bool app_quitting)
{
	if (sEnvSettings)
	{
		sEnvSettings->setVisible(FALSE);
	}
}
void LLFloaterEnvSettings::onChangeDayTime(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldr;
	sldr = sEnvSettings->getChild<LLSliderCtrl>("EnvTimeSlider");
	LLWLParamManager::getInstance()->mAnimator.deactivate();
	F32 val = sldr->getValueF32() + 0.25f;
	if(val > 1.0)
	{
		val--;
	}
	LLWLParamManager::getInstance()->mAnimator.setDayTime((F64)val);
	LLWLParamManager::getInstance()->mAnimator.update(
		LLWLParamManager::getInstance()->mCurParams);
}
void LLFloaterEnvSettings::onChangeCloudCoverage(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldr;
	sldr = sEnvSettings->getChild<LLSliderCtrl>("EnvCloudSlider");
	F32 val = sldr->getValueF32();
	LLWLParamManager::getInstance()->mCurParams.set("cloud_shadow", val);
}
void LLFloaterEnvSettings::onChangeWaterFogDensity(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldr;
	sldr = sEnvSettings->getChild<LLSliderCtrl>("EnvWaterFogSlider");
	if(NULL == userData)
	{
		return;
	}
	WaterExpFloatControl * expFloatControl = static_cast<WaterExpFloatControl *>(userData);
	F32 val = sldr->getValueF32();
	expFloatControl->mExp = val;
	LLWaterParamManager::getInstance()->setDensitySliderValue(val);
	expFloatControl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterEnvSettings::onChangeWaterColor(LLUICtrl* ctrl, void* userData)
{
	LLColorSwatchCtrl* swatch = static_cast<LLColorSwatchCtrl*>(ctrl);
	WaterColorControl * colorControl = static_cast<WaterColorControl *>(userData);
	*colorControl = swatch->get();
	colorControl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterEnvSettings::onOpenAdvancedSky(void* userData)
{
	LLFloaterWindLight::show();
}
void LLFloaterEnvSettings::onOpenAdvancedWater(void* userData)
{
	LLFloaterWater::show();
}
void LLFloaterEnvSettings::onUseEstateTime(void* userData)
{
	if(LLFloaterWindLight::isOpen())
	{
		LLFloaterWindLight* wl = LLFloaterWindLight::instance();
		LLComboBox* box = wl->getChild<LLComboBox>("WLPresetsCombo");
		box->selectByValue("");
	}
	LLWLParamManager::getInstance()->mAnimator.activate(LLWLAnimator::TIME_LINDEN);
	LLEnvManagerNew::instance().setUseDayCycle(LLEnvManagerNew::instance().getDayCycleName());
}
std::string LLFloaterEnvSettings::timeToString(F32 curTime)
{
	S32 hours;
	S32 min;
	bool isPM = false;
	hours = (S32) (24.0 * curTime);
	curTime -= ((F32) hours / 24.0f);
	min = ll_round(24.0f * 60.0f * curTime);
	if(min == 60)
	{
		hours++;
		min = 0;
	}
	if(hours >= 12 && hours < 24)
	{
		isPM = true;
	}
	if(hours >= 24)
	{
		hours = 12;
	}
	else if(hours > 12)
	{
		hours -= 12;
	}
	else if(hours == 0)
	{
		hours = 12;
	}
	std::stringstream newTime;
	newTime << hours << ":";
	if(min < 10)
	{
		newTime << 0;
	}
	newTime << min << " ";
	if(isPM)
	{
		newTime << "PM";
	}
	else
	{
		newTime << "AM";
	}
	return newTime.str();
}
