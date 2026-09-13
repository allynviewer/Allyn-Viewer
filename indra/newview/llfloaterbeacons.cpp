/** 
 * @file llfloaterbeacons.cpp
 * @brief Front-end to LLPipeline controls for highlighting various kinds of objects.
 * @author Coco
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
#include "llfloaterbeacons.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llcheckboxctrl.h"
#include "pipeline.h"
#include "rlvhandler.h"
LLFloaterBeacons::LLFloaterBeacons(const LLSD& seed)
{
	LLPipeline::setRenderScriptedTouchBeacons(gSavedSettings.getBOOL("scripttouchbeacon"));
	LLPipeline::setRenderScriptedBeacons(     gSavedSettings.getBOOL("scriptsbeacon"));
	LLPipeline::setRenderPhysicalBeacons(     gSavedSettings.getBOOL("physicalbeacon"));
	LLPipeline::setRenderSoundBeacons(        gSavedSettings.getBOOL("soundsbeacon"));
	LLPipeline::setRenderParticleBeacons(     gSavedSettings.getBOOL("particlesbeacon"));
	LLPipeline::setRenderHighlights(          gSavedSettings.getBOOL("renderhighlights"));
	LLPipeline::setRenderBeacons(             gSavedSettings.getBOOL("renderbeacons"));
	LLPipeline::setRenderMOAPBeacons(		  gSavedSettings.getBOOL("moapbeacon"));
	mCommitCallbackRegistrar.add("Beacons.UICheck",	boost::bind(&LLFloaterBeacons::onClickUICheck, this,_1));
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_beacons.xml");
}
BOOL LLFloaterBeacons::postBuild()
{
	return TRUE;
}
void LLFloaterBeacons::open()
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_EDIT))
	{
		return;
	}
	LLFloater::open();
	gSavedSettings.setBOOL("ShowBeaconsFloater", TRUE);
	gSavedSettings.setBOOL("BeaconsVisible", TRUE);
}
void LLFloaterBeacons::close(bool app_quitting)
{
	LLFloater::close(app_quitting);
	if(!app_quitting)
	{
		gSavedSettings.setBOOL("ShowBeaconsFloater", FALSE);
		if (!gSavedSettings.getBOOL("BeaconsKeepVisible"))
		{
			gSavedSettings.setBOOL("BeaconsVisible", FALSE);
		}
	}
}
void LLFloaterBeacons::onClickUICheck(LLUICtrl *ctrl)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	std::string name = check->getName();
	if (name == "touch_only")
	{
		LLPipeline::toggleRenderScriptedTouchBeacons(NULL);
		if (LLPipeline::getRenderScriptedTouchBeacons(NULL) &&
			LLPipeline::getRenderScriptedBeacons(NULL) )
		{
			LLPipeline::setRenderScriptedBeacons(FALSE);
			getChild<LLCheckBoxCtrl>("scripted")->setControlValue(LLSD(FALSE));
		}
	}
	else if (name == "scripted")
 	{
		LLPipeline::toggleRenderScriptedBeacons(NULL);
		if (LLPipeline::getRenderScriptedTouchBeacons(NULL) &&
			LLPipeline::getRenderScriptedBeacons(NULL) )
		{
			LLPipeline::setRenderScriptedTouchBeacons(FALSE);
			getChild<LLCheckBoxCtrl>("touch_only")->setControlValue(LLSD(FALSE));
		}
	}
	else if (name == "physical")   LLPipeline::setRenderPhysicalBeacons(check->get());
	else if (name == "sounds")     LLPipeline::setRenderSoundBeacons(check->get());
	else if (name == "particles")  LLPipeline::setRenderParticleBeacons(check->get());
	else if (name == "moapbeacon") LLPipeline::setRenderMOAPBeacons(check->get());
	else if (name == "highlights")
	{
		LLPipeline::toggleRenderHighlights(NULL);
		if (!LLPipeline::getRenderBeacons(NULL) &&
			!LLPipeline::getRenderHighlights(NULL))
		{
			LLPipeline::setRenderBeacons(TRUE);
			getChild<LLCheckBoxCtrl>("beacons")->setControlValue(LLSD(TRUE));
		}
	}
	else if (name == "beacons")
	{
		LLPipeline::toggleRenderBeacons(NULL);
		if (!LLPipeline::getRenderBeacons(NULL) &&
			!LLPipeline::getRenderHighlights(NULL))
		{
			LLPipeline::setRenderHighlights(TRUE);
			getChild<LLCheckBoxCtrl>("highlights")->setControlValue(LLSD(TRUE));
		}
	}
}
