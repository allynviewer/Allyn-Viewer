/** 
 * @file llfloaterpostprocess.cpp
 * @brief LLFloaterPostProcess class definition
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
#include "llfloaterpostprocess.h"
#include "llsliderctrl.h"
#include "llcheckboxctrl.h"
#include "llnotificationsutil.h"
#include "lluictrlfactory.h"
#include "llviewerdisplay.h"
#include "llpostprocess.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llviewerwindow.h"
LLFloaterPostProcess* LLFloaterPostProcess::sPostProcess = NULL;
LLFloaterPostProcess::LLFloaterPostProcess(const LLSD&) : LLFloater("Post-Process Floater")
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_post_process.xml");
	LLTabContainer *panel_list = getChild<LLTabContainer>("Post-Process Tabs",false,false);
	if(panel_list)
	{
		for(S32 i = 0; i<panel_list->getTabCount(); ++i)
		{
			LLPanel *shader_panel = panel_list->getPanelByIndex(i);
			if(shader_panel->getName() == "Extras")
				continue;
			for ( child_list_const_iter_t child_it = shader_panel->getChildList()->begin(); child_it != shader_panel->getChildList()->end(); ++child_it)
			{
				if(dynamic_cast<LLSliderCtrl*>(*child_it) || dynamic_cast<LLCheckBoxCtrl*>(*child_it))
				{
					LLUICtrl* ctrl = static_cast<LLUICtrl*>(*child_it);
					ctrl->setCommitCallback(boost::bind(&LLFloaterPostProcess::onControlChanged, _1, _2));
				}
			}
		}
	}
	LLComboBox* comboBox = getChild<LLComboBox>("PPEffectsCombo");
	getChild<LLUICtrl>("PPLoadEffect")->setCommitCallback(boost::bind(&LLFloaterPostProcess::onLoadEffect, this, comboBox));
	comboBox->setCommitCallback(boost::bind(&LLFloaterPostProcess::onChangeEffectName, this, _1));
	LLLineEditor* editBox = getChild<LLLineEditor>("PPEffectNameEditor");
	getChild<LLUICtrl>("PPSaveEffect")->setCommitCallback(boost::bind(&LLFloaterPostProcess::onSaveEffect, this, editBox));
	syncMenu();
	LLPostProcess::instance().setSelectedEffectChangeCallback(boost::bind(&LLFloaterPostProcess::syncMenu, this));
}
LLFloaterPostProcess::~LLFloaterPostProcess()
{
}
void LLFloaterPostProcess::onControlChanged(LLUICtrl* ctrl, const LLSD& v)
{
	LLPostProcess::getInstance()->setSelectedEffectValue(ctrl->getName(), v);
}
void LLFloaterPostProcess::onLoadEffect(LLComboBox* comboBox)
{
	LLSD::String effectName(comboBox->getSelectedValue().asString());
	LLPostProcess::getInstance()->setSelectedEffect(effectName);
}
void LLFloaterPostProcess::onSaveEffect(LLLineEditor* editBox)
{
	std::string effectName(editBox->getValue().asString());
	if (LLPostProcess::getInstance()->getAllEffectInfo().has(effectName))
	{
		LLSD payload;
		payload["effect_name"] = effectName;
		LLNotificationsUtil::add("PPSaveEffectAlert", LLSD(), payload, boost::bind(&LLFloaterPostProcess::saveAlertCallback, this, _1, _2));
	}
	else
	{
		LLPostProcess::getInstance()->saveEffectAs(effectName);
	}
}
void LLFloaterPostProcess::onChangeEffectName(LLUICtrl* ctrl)
{
	LLComboBox * comboBox = static_cast<LLComboBox*>(ctrl);
	LLLineEditor* editBox = getChild<LLLineEditor>("PPEffectNameEditor");
	editBox->setValue(comboBox->getSelectedValue());
}
bool LLFloaterPostProcess::saveAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLPostProcess::getInstance()->saveEffectAs(notification["payload"]["effect_name"].asString());
	}
	return false;
}
void LLFloaterPostProcess::onClose(bool app_quitting)
{
	if (app_quitting)
		die();
	else
		setVisible(FALSE);
}
void populatePostProcessList(LLComboBox* comboBox)
{
	comboBox->removeall();
	const auto& inst(LLPostProcess::instance());
	for(auto currEffect = inst.getAllEffectInfo().beginMap(), end = inst.getAllEffectInfo().endMap();
		currEffect != end;
		++currEffect)
	{
		comboBox->add(currEffect->first);
	}
	comboBox->selectByValue(inst.getSelectedEffectName());
}
void LLFloaterPostProcess::syncMenu()
{
	populatePostProcessList(getChild<LLComboBox>("PPEffectsCombo"));
	const LLSD &tweaks = LLPostProcess::getInstance()->getSelectedEffectInfo();
	for (LLSD::map_const_iterator it = tweaks.beginMap(); it != tweaks.endMap(); ++it)
	{
		if(it->second.isArray())
		{
			for(S32 i=0;i<it->second.size();++i)
			{
				childSetValue(it->first+'['+fmt::to_string(i)+']',it->second[i]);
			}
		}
		else
		{
			childSetValue(it->first,it->second);
		}
	}
}
