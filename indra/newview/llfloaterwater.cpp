/** 
 * @file llfloaterwater.cpp
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
#include "llviewerprecompiledheaders.h"
#include "llfloaterwater.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llnotificationsutil.h"
#include "llsliderctrl.h"
#include "lltexturectrl.h"
#include "lluictrlfactory.h"
#include "llfloaterdaycycle.h"
#include "llagent.h"
#include "llwaterparammanager.h"
#include "llwaterparamset.h"
#include "rlvactions.h"
#undef max
LLFloaterWater* LLFloaterWater::sWaterMenu = NULL;
std::set<std::string> LLFloaterWater::sDefaultPresets;
LLFloaterWater::LLFloaterWater() : LLFloater(std::string("water floater"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_water.xml");
	reshape(700, 300);
	mWaterPresetCombo = getChild<LLComboBox>("WaterPresetsCombo");
	if (mWaterPresetCombo != NULL)
	{
		populateWaterPresetsList();
		mWaterPresetCombo->setCommitCallback(boost::bind(&LLFloaterWater::onChangePresetName, this, _1));
	}
	sDefaultPresets.insert("");
	boost_tokenizer tokens(getString("WLDefaultWaterNames"), boost::char_separator<char>(":"));
	for (boost_tokenizer::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		std::string tok(*token_iter);
		sDefaultPresets.insert(tok);
	}
	initCallbacks();
}
LLFloaterWater::~LLFloaterWater()
{
}
void LLFloaterWater::initCallbacks(void)
{
	initHelpBtn("WaterFogColorHelp", "HelpWaterFogColor");
	initHelpBtn("WaterFogDensityHelp", "HelpWaterFogDensity");
	initHelpBtn("WaterUnderWaterFogModHelp", "HelpUnderWaterFogMod");
	initHelpBtn("WaterNormalScaleHelp", "HelpWaterNormalScale");
	initHelpBtn("WaterFresnelScaleHelp", "HelpWaterFresnelScale");
	initHelpBtn("WaterFresnelOffsetHelp", "HelpWaterFresnelOffset");
	initHelpBtn("WaterBlurMultiplierHelp", "HelpWaterBlurMultiplier");
	initHelpBtn("WaterScaleBelowHelp", "HelpWaterScaleBelow");
	initHelpBtn("WaterScaleAboveHelp", "HelpWaterScaleAbove");
	initHelpBtn("WaterNormalMapHelp", "HelpWaterNormalMap");
	initHelpBtn("WaterWave1Help", "HelpWaterWave1");
	initHelpBtn("WaterWave2Help", "HelpWaterWave2");
	LLWaterParamManager& water_mgr = LLWaterParamManager::instance();
	getChild<LLUICtrl>("WaterFogColor")->setCommitCallback(boost::bind(&LLFloaterWater::onWaterFogColorMoved, this, _1, &water_mgr.mFogColor));
	getChild<LLUICtrl>("WaterFogDensity")->setCommitCallback(boost::bind(&LLFloaterWater::onExpFloatControlMoved, this, _1, &water_mgr.mFogDensity));
	getChild<LLUICtrl>("WaterUnderWaterFogMod")->setCommitCallback(boost::bind(&LLFloaterWater::onFloatControlMoved, this, _1, &water_mgr.mUnderWaterFogMod));
	getChild<LLUICtrl>("WaterNormalScaleX")->setCommitCallback(boost::bind(&LLFloaterWater::onVector3ControlXMoved, this, _1, &water_mgr.mNormalScale));
	getChild<LLUICtrl>("WaterNormalScaleY")->setCommitCallback(boost::bind(&LLFloaterWater::onVector3ControlYMoved, this, _1, &water_mgr.mNormalScale));
	getChild<LLUICtrl>("WaterNormalScaleZ")->setCommitCallback(boost::bind(&LLFloaterWater::onVector3ControlZMoved, this, _1, &water_mgr.mNormalScale));
	getChild<LLUICtrl>("WaterFresnelScale")->setCommitCallback(boost::bind(&LLFloaterWater::onFloatControlMoved, this, _1, &water_mgr.mFresnelScale));
	getChild<LLUICtrl>("WaterFresnelOffset")->setCommitCallback(boost::bind(&LLFloaterWater::onFloatControlMoved, this, _1, &water_mgr.mFresnelOffset));
	getChild<LLUICtrl>("WaterScaleAbove")->setCommitCallback(boost::bind(&LLFloaterWater::onFloatControlMoved, this, _1, &water_mgr.mScaleAbove));
	getChild<LLUICtrl>("WaterScaleBelow")->setCommitCallback(boost::bind(&LLFloaterWater::onFloatControlMoved, this, _1, &water_mgr.mScaleBelow));
	getChild<LLUICtrl>("WaterBlurMult")->setCommitCallback(boost::bind(&LLFloaterWater::onFloatControlMoved, this, _1, &water_mgr.mBlurMultiplier));
	getChild<LLUICtrl>("WaterNewPreset")->setCommitCallback(boost::bind(&LLFloaterWater::onNewPreset, this));
	getChild<LLUICtrl>("WaterDeletePreset")->setCommitCallback(boost::bind(&LLFloaterWater::onDeletePreset, this));
	getChild<LLUICtrl>("WaterSavePreset")->setCommitCallback(boost::bind(&LLFloaterWater::onSavePreset, this, _1));
	getChild<LLUICtrl>("WaterWave1DirX")->setCommitCallback(boost::bind(&LLFloaterWater::onVector2ControlXMoved, this, _1, &water_mgr.mWave1Dir));
	getChild<LLUICtrl>("WaterWave1DirY")->setCommitCallback(boost::bind(&LLFloaterWater::onVector2ControlYMoved, this, _1, &water_mgr.mWave1Dir));
	getChild<LLUICtrl>("WaterWave2DirX")->setCommitCallback(boost::bind(&LLFloaterWater::onVector2ControlXMoved, this, _1, &water_mgr.mWave2Dir));
	getChild<LLUICtrl>("WaterWave2DirY")->setCommitCallback(boost::bind(&LLFloaterWater::onVector2ControlYMoved, this, _1, &water_mgr.mWave2Dir));
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("WaterNormalMap");
	texture_ctrl->setDefaultImageAssetID(DEFAULT_WATER_NORMAL);
	texture_ctrl->setCommitCallback(boost::bind(&LLFloaterWater::onNormalMapPicked, this, _1));
}
void LLFloaterWater::onClickHelp(void* data)
{
	LLFloaterWater* self = LLFloaterWater::instance();
	const std::string* xml_alert = (std::string*)data;
	self->addContextualNotification(*xml_alert);
}
void LLFloaterWater::initHelpBtn(const std::string& name, const std::string& xml_alert)
{
	childSetAction(name, onClickHelp, new std::string(xml_alert));
}
bool LLFloaterWater::newPromptCallback(const LLSD& notification, const LLSD& response)
{
	std::string text = response["message"].asString();
	if(text.empty())
	{
		return false;
	}
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(option == 0)
	{
		LLWaterParamManager * param_mgr = LLWaterParamManager::getInstance();
		if(!LLWaterParamManager::getInstance()->hasParamSet(text))
		{
			param_mgr->addParamSet(text, param_mgr->mCurParams);
			mWaterPresetCombo->add(text);
			mWaterPresetCombo->sortByName();
			mWaterPresetCombo->selectByValue(text);
			param_mgr->savePreset(text);
			LLEnvManagerNew::instance().setUseWaterPreset(text);
		}
		else
		{
			LLNotificationsUtil::add("ExistsWaterPresetAlert");
		}
	}
	return false;
}
void LLFloaterWater::syncMenu()
{
	bool err;
	LLWaterParamManager& water_mgr = LLWaterParamManager::instance();
	LLWaterParamSet& current_params = water_mgr.mCurParams;
	if (mWaterPresetCombo->getSelectedItemLabel() != LLEnvManagerNew::instance().getWaterPresetName())
	{
		mWaterPresetCombo->selectByValue(LLEnvManagerNew::instance().getWaterPresetName());
	}
	water_mgr.mFogColor = current_params.getVector4(water_mgr.mFogColor.mName, err);
	LLColor4 col = water_mgr.getFogColor();
	col.mV[3] = 1.0f;
	getChild<LLColorSwatchCtrl>("WaterFogColor")->set(col);
	water_mgr.mFogDensity.mExp =
		log(current_params.getFloat(water_mgr.mFogDensity.mName, err)) /
		log(water_mgr.mFogDensity.mBase);
	water_mgr.setDensitySliderValue(water_mgr.mFogDensity.mExp);
	getChild<LLUICtrl>("WaterFogDensity")->setValue(water_mgr.mFogDensity.mExp);
	water_mgr.mUnderWaterFogMod.mX =
		current_params.getFloat(water_mgr.mUnderWaterFogMod.mName, err);
	getChild<LLUICtrl>("WaterUnderWaterFogMod")->setValue(water_mgr.mUnderWaterFogMod.mX);
	water_mgr.mNormalScale = current_params.getVector3(water_mgr.mNormalScale.mName, err);
	getChild<LLUICtrl>("WaterNormalScaleX")->setValue(water_mgr.mNormalScale.mX);
	getChild<LLUICtrl>("WaterNormalScaleY")->setValue(water_mgr.mNormalScale.mY);
	getChild<LLUICtrl>("WaterNormalScaleZ")->setValue(water_mgr.mNormalScale.mZ);
	water_mgr.mFresnelScale.mX = current_params.getFloat(water_mgr.mFresnelScale.mName, err);
	getChild<LLUICtrl>("WaterFresnelScale")->setValue(water_mgr.mFresnelScale.mX);
	water_mgr.mFresnelOffset.mX = current_params.getFloat(water_mgr.mFresnelOffset.mName, err);
	getChild<LLUICtrl>("WaterFresnelOffset")->setValue(water_mgr.mFresnelOffset.mX);
	water_mgr.mScaleAbove.mX = current_params.getFloat(water_mgr.mScaleAbove.mName, err);
	getChild<LLUICtrl>("WaterScaleAbove")->setValue(water_mgr.mScaleAbove.mX);
	water_mgr.mScaleBelow.mX = current_params.getFloat(water_mgr.mScaleBelow.mName, err);
	getChild<LLUICtrl>("WaterScaleBelow")->setValue(water_mgr.mScaleBelow.mX);
	water_mgr.mBlurMultiplier.mX = current_params.getFloat(water_mgr.mBlurMultiplier.mName, err);
	getChild<LLUICtrl>("WaterBlurMult")->setValue(water_mgr.mBlurMultiplier.mX);
	water_mgr.mWave1Dir = current_params.getVector2(water_mgr.mWave1Dir.mName, err);
	getChild<LLUICtrl>("WaterWave1DirX")->setValue(water_mgr.mWave1Dir.mX);
	getChild<LLUICtrl>("WaterWave1DirY")->setValue(water_mgr.mWave1Dir.mY);
	water_mgr.mWave2Dir = current_params.getVector2(water_mgr.mWave2Dir.mName, err);
	getChild<LLUICtrl>("WaterWave2DirX")->setValue(water_mgr.mWave2Dir.mX);
	getChild<LLUICtrl>("WaterWave2DirY")->setValue(water_mgr.mWave2Dir.mY);
	LLTextureCtrl* textCtrl = getChild<LLTextureCtrl>("WaterNormalMap");
	textCtrl->setImageAssetID(water_mgr.getNormalMapID());
}
LLFloaterWater* LLFloaterWater::instance()
{
	if (!sWaterMenu)
	{
		sWaterMenu = new LLFloaterWater();
		sWaterMenu->open();
		sWaterMenu->setFocus(TRUE);
	}
	return sWaterMenu;
}
void LLFloaterWater::show()
{
	if (RlvActions::hasBehaviour(RLV_BHVR_SETENV)) return;
	if (!sWaterMenu)
	{
		LLFloaterWater* water = instance();
		water->syncMenu();
	}
	else
	{
		if (sWaterMenu->getVisible())
		{
			sWaterMenu->close();
		}
		else
		{
			sWaterMenu->open();
		}
	}
}
bool LLFloaterWater::isOpen()
{
	if (sWaterMenu != NULL)
	{
		return sWaterMenu->getVisible();
	}
	return false;
}
void LLFloaterWater::onClose(bool app_quitting)
{
	if (sWaterMenu)
	{
		sWaterMenu->setVisible(FALSE);
	}
}
void LLFloaterWater::onColorControlRMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	color_ctrl->mR = sldr_ctrl->getValueF32();
	if (color_ctrl->mR >= color_ctrl->mG
		&& color_ctrl->mR >= color_ctrl->mB
		&& color_ctrl->mHasSliderName)
	{
		color_ctrl->mI = color_ctrl->mR;
		std::string name = color_ctrl->mSliderName;
		name.append("I");
		getChild<LLUICtrl>(name)->setValue(color_ctrl->mR);
	}
	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onColorControlGMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	color_ctrl->mG = sldr_ctrl->getValueF32();
	if (color_ctrl->mG >= color_ctrl->mR
		&& color_ctrl->mG >= color_ctrl->mB
		&& color_ctrl->mHasSliderName)
	{
		color_ctrl->mI = color_ctrl->mG;
		std::string name = color_ctrl->mSliderName;
		name.append("I");
		getChild<LLUICtrl>(name)->setValue(color_ctrl->mG);
	}
	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onColorControlBMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	color_ctrl->mB = sldr_ctrl->getValueF32();
	if (color_ctrl->mB >= color_ctrl->mR
		&& color_ctrl->mB >= color_ctrl->mG
		&& color_ctrl->mHasSliderName)
	{
		color_ctrl->mI = color_ctrl->mB;
		std::string name = color_ctrl->mSliderName;
		name.append("I");
		getChild<LLUICtrl>(name)->setValue(color_ctrl->mB);
	}
	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onColorControlAMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	color_ctrl->mA = sldr_ctrl->getValueF32();
	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onColorControlIMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	color_ctrl->mI = sldr_ctrl->getValueF32();
	if (color_ctrl->mHasSliderName)
	{
		F32 maxVal = std::max(std::max(color_ctrl->mR, color_ctrl->mG), color_ctrl->mB);
		F32 iVal;
		iVal = color_ctrl->mI;
		std::string rName = color_ctrl->mSliderName;
		rName.append("R");
		std::string gName = color_ctrl->mSliderName;
		gName.append("G");
		std::string bName = color_ctrl->mSliderName;
		bName.append("B");
		if(iVal == 0)
		{
			color_ctrl->mR = 0;
			color_ctrl->mG = 0;
			color_ctrl->mB = 0;
		}
		else if (maxVal == 0)
		{
			color_ctrl->mR = iVal;
			color_ctrl->mG = iVal;
			color_ctrl->mB = iVal;
		}
		else
		{
			F32 delta = (iVal - maxVal) / maxVal;
			color_ctrl->mR *= (1.0f + delta);
			color_ctrl->mG *= (1.0f + delta);
			color_ctrl->mB *= (1.0f + delta);
		}
		getChild<LLUICtrl>(rName)->setValue(color_ctrl->mR);
		getChild<LLUICtrl>(gName)->setValue(color_ctrl->mG);
		getChild<LLUICtrl>(bName)->setValue(color_ctrl->mB);
	}
	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onVector3ControlXMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	vector_ctrl->mX = sldr_ctrl->getValueF32();
	vector_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onVector3ControlYMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	vector_ctrl->mY = sldr_ctrl->getValueF32();
	vector_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onVector3ControlZMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	vector_ctrl->mZ = sldr_ctrl->getValueF32();
	vector_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onVector2ControlXMoved(LLUICtrl* ctrl, WaterVector2Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	vector_ctrl->mX = sldr_ctrl->getValueF32();
	vector_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onVector2ControlYMoved(LLUICtrl* ctrl, WaterVector2Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	vector_ctrl->mY = sldr_ctrl->getValueF32();
	vector_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onFloatControlMoved(LLUICtrl* ctrl, WaterFloatControl* floatControl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	floatControl->mX = sldr_ctrl->getValueF32() / floatControl->mMult;
	floatControl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onExpFloatControlMoved(LLUICtrl* ctrl, WaterExpFloatControl* expFloatControl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	F32 val = sldr_ctrl->getValueF32();
	expFloatControl->mExp = val;
	LLWaterParamManager::getInstance()->setDensitySliderValue(val);
	expFloatControl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onWaterFogColorMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLColorSwatchCtrl* swatch = static_cast<LLColorSwatchCtrl*>(ctrl);
	*color_ctrl = swatch->get();
	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}
void LLFloaterWater::onNormalMapPicked(LLUICtrl* ctrl)
{
	LLTextureCtrl* textCtrl = static_cast<LLTextureCtrl*>(ctrl);
	LLUUID textID = textCtrl->getImageAssetID();
	LLWaterParamManager::getInstance()->setNormalMapID(textID);
}
void LLFloaterWater::onNewPreset()
{
	LLNotificationsUtil::add("NewWaterPreset", LLSD(),  LLSD(), boost::bind(&LLFloaterWater::newPromptCallback, this, _1, _2));
}
void LLFloaterWater::onSavePreset(LLUICtrl* ctrl)
{
	if(mWaterPresetCombo->getSelectedItemLabel().empty())
	{
		return;
	}
	if (ctrl->getValue().asString() == "save_inventory_item")
	{
	}
	else
	{
		LLWaterParamManager::getInstance()->mCurParams.mName = mWaterPresetCombo->getSelectedItemLabel();
		std::set<std::string>::iterator sIt = sDefaultPresets.find(mWaterPresetCombo->getSelectedItemLabel());
		if(sIt != sDefaultPresets.end() && !gSavedSettings.getBOOL("WaterEditPresets"))
		{
			LLNotificationsUtil::add("WLNoEditDefault");
			return;
		}
		LLNotificationsUtil::add("WLSavePresetAlert", LLSD(), LLSD(), boost::bind(&LLFloaterWater::saveAlertCallback, this, _1, _2));
	}
}
bool LLFloaterWater::saveNotecardCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(option == 0)
	{
		LLWaterParamManager * param_mgr = LLWaterParamManager::getInstance();
		param_mgr->setParamSet(param_mgr->mCurParams.mName, param_mgr->mCurParams);
		param_mgr->savePresetToNotecard(param_mgr->mCurParams.mName);
	}
	return false;
}
bool LLFloaterWater::saveAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(option == 0)
	{
		LLWaterParamManager* param_mgr = LLWaterParamManager::getInstance();
		param_mgr->setParamSet(param_mgr->mCurParams.mName, param_mgr->mCurParams);
		param_mgr->savePreset(param_mgr->mCurParams.mName);
	}
	return false;
}
void LLFloaterWater::onDeletePreset()
{
	if(mWaterPresetCombo->getSelectedValue().asString().empty())
	{
		return;
	}
	LLSD args;
	args["SKY"] = mWaterPresetCombo->getSelectedValue().asString();
	LLNotificationsUtil::add("WLDeletePresetAlert", args, LLSD(), boost::bind(&LLFloaterWater::deleteAlertCallback, this, _1, _2));
}
bool LLFloaterWater::deleteAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(option == 0)
	{
		LLFloaterDayCycle* day_cycle = NULL;
		LLComboBox* key_combo = NULL;
		if(LLFloaterDayCycle::isOpen())
		{
			day_cycle = LLFloaterDayCycle::instance();
			key_combo = day_cycle->getChild<LLComboBox>("WaterKeyPresets");
		}
		std::string name = mWaterPresetCombo->getSelectedValue().asString();
		std::set<std::string>::iterator sIt = sDefaultPresets.find(name);
		if(sIt != sDefaultPresets.end())
		{
			LLNotificationsUtil::add("WaterNoEditDefault");
			return false;
		}
		LLWaterParamManager::getInstance()->removeParamSet(name, true);
		S32 new_index = mWaterPresetCombo->getCurrentIndex();
		mWaterPresetCombo->remove(name);
		if(key_combo != NULL)
		{
			key_combo->remove(name);
			day_cycle->deletePreset(name);
		}
		if(new_index > 0)
		{
			--new_index;
		}
		if(mWaterPresetCombo->getItemCount() > 0)
		{
			mWaterPresetCombo->setCurrentByIndex(new_index);
		}
	}
	return false;
}
void LLFloaterWater::onChangePresetName(LLUICtrl* ctrl)
{
	LLComboBox* combo_box = static_cast<LLComboBox*>(ctrl);
	if(combo_box->getSimple().empty())
	{
		return;
	}
	const std::string& wwset = combo_box->getSelectedValue().asString();
	if (LLWaterParamManager::getInstance()->hasParamSet(wwset))
	{
		LLEnvManagerNew::instance().setUseWaterPreset(wwset);
	}
	else
	{
		LLEnvManagerNew::instance().setUseWaterPreset("Default");
	}
	syncMenu();
}
void LLFloaterWater::onClickNext()
{
	S32 index = mWaterPresetCombo->getCurrentIndex();
	++index;
	if (index == mWaterPresetCombo->getItemCount())
		index = 0;
	mWaterPresetCombo->setCurrentByIndex(index);
	LLFloaterWater::onChangePresetName(mWaterPresetCombo);
}
void LLFloaterWater::onClickPrev()
{
	S32 index = mWaterPresetCombo->getCurrentIndex();
	if (index == 0)
		index = mWaterPresetCombo->getItemCount();
	--index;
	mWaterPresetCombo->setCurrentByIndex(index);
	LLFloaterWater::onChangePresetName(mWaterPresetCombo);
}
void LLFloaterWater::populateWaterPresetsList()
{
	mWaterPresetCombo->removeall();
	std::list<std::string> presets;
	LLWaterParamManager::getInstance()->getPresetNames(presets);
	for (std::list<std::string>::const_iterator it = presets.begin(); it != presets.end(); ++it)
	{
		mWaterPresetCombo->add(*it);
	}
	mWaterPresetCombo->selectByValue(LLEnvManagerNew::instance().getWaterPresetName());
}
