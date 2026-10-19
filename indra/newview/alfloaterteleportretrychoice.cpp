/**
 * @file alfloaterteleportretrychoice.cpp
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "alfloaterteleportretrychoice.h"
#include "alteleportretrycontroller.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"

ALFloaterTeleportRetryChoice::ALFloaterTeleportRetryChoice(const LLSD&)
	: LLFloater()
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_teleport_retry_choice.xml");
}

ALFloaterTeleportRetryChoice::~ALFloaterTeleportRetryChoice()
{
}

BOOL ALFloaterTeleportRetryChoice::postBuild()
{
	mModeCombo = getChild<LLComboBox>("mode_combo");
	mAskAgainCheck = getChild<LLCheckBoxCtrl>("ask_again");
	mRegionText = getChild<LLTextBox>("dest_region");
	getChild<LLButton>("ok_button")->setClickedCallback(
		boost::bind(&ALFloaterTeleportRetryChoice::onClickConfirm, this));
	getChild<LLButton>("cancel_button")->setClickedCallback(
		boost::bind(&ALFloaterTeleportRetryChoice::onClickCancel, this));
	return TRUE;
}

void ALFloaterTeleportRetryChoice::onOpen()
{
	LLFloater::onOpen();
	const S32 mode = gSavedSettings.getS32("ALLTeleportRetryMode");
	mModeCombo->setCurrentByIndex(
		mode == ALTeleportRetryController::MODE_MULTIPLE ? 1 : 0);
	mAskAgainCheck->setValue(gSavedSettings.getBOOL("ALLTeleportRetryAskAgain"));
	mRegionText->setValue(mRegionName.empty() ? std::string("-") : mRegionName);
}

void ALFloaterTeleportRetryChoice::onClickConfirm()
{
	const S32 mode = (mModeCombo->getCurrentIndex() == 1)
		? ALTeleportRetryController::MODE_MULTIPLE
		: ALTeleportRetryController::MODE_SINGLE;
	const BOOL ask_again = mAskAgainCheck->getValue().asBoolean();
	gSavedSettings.setS32("ALLTeleportRetryMode", mode);
	gSavedSettings.setBOOL("ALLTeleportRetryAskAgain", ask_again);
	if (mode == ALTeleportRetryController::MODE_MULTIPLE)
	{
		ALTeleportRetryController::instance().startFromFailedRequest(mRegionName);
	}
	close(false);
}

void ALFloaterTeleportRetryChoice::onClickCancel()
{
	close(false);
}
