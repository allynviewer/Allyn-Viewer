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
