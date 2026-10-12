#include "llviewerprecompiledheaders.h"
#include "alfloaterteleportretrystatus.h"
#include "alteleportretrycontroller.h"
#include "llbutton.h"
#include "llstring.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

static std::string formatElapsedSeconds(S32 seconds)
{
	if (seconds < 0)
	{
		seconds = 0;
	}
	const S32 hours = seconds / 3600;
	const S32 minutes = (seconds % 3600) / 60;
	const S32 secs = seconds % 60;
	if (hours > 0)
	{
		return llformat("%d h %d min %d s", hours, minutes, secs);
	}
	if (minutes > 0)
	{
		return llformat("%d min %d s", minutes, secs);
	}
	return llformat("%d s", secs);
}

ALFloaterTeleportRetryStatus::ALFloaterTeleportRetryStatus(const LLSD&)
	: LLFloater(),
	  LLEventTimer(1.f)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_teleport_retry_status.xml");
	mEventTimer.stop();
}

ALFloaterTeleportRetryStatus::~ALFloaterTeleportRetryStatus()
{
	mEventTimer.stop();
}

BOOL ALFloaterTeleportRetryStatus::postBuild()
{
	mPauseBtn = getChild<LLButton>("pause_btn");
	mRegionText = getChild<LLTextBox>("dest_region");
	mCoordText = getChild<LLTextBox>("coords");
	mAttemptsText = getChild<LLTextBox>("attempts");
	mElapsedText = getChild<LLTextBox>("elapsed");
	mIntervalText = getChild<LLTextBox>("interval");
	mCountdownText = getChild<LLTextBox>("countdown");
	mStateText = getChild<LLTextBox>("state");
	mMessageLabel = getChild<LLTextBox>("message_label");
	mMessageText = getChild<LLTextBox>("server_message");
	mPauseBtn->setClickedCallback(boost::bind(&ALFloaterTeleportRetryStatus::onClickPause, this));
	mMessageLabel->setVisible(FALSE);
	mMessageText->setVisible(FALSE);
	return TRUE;
}

void ALFloaterTeleportRetryStatus::onOpen()
{
	LLFloater::onOpen();
	mEventTimer.start();
	refreshStatus();
}

void ALFloaterTeleportRetryStatus::onClose(bool app_quitting)
{
	mEventTimer.stop();
	if (!app_quitting
		&& ALTeleportRetryController::instanceExists()
		&& ALTeleportRetryController::instance().isOperationActive())
	{
		ALTeleportRetryController::instance().onStatusWindowClosed();
	}
	destroy();
}

BOOL ALFloaterTeleportRetryStatus::tick()
{
	if (!ALTeleportRetryController::instanceExists()
		|| !ALTeleportRetryController::instance().isOperationActive())
	{
		storeRectControl();
		destroy();
		return FALSE;
	}
	refreshStatus();
	return FALSE;
}

void ALFloaterTeleportRetryStatus::onClickPause()
{
	if (!ALTeleportRetryController::instanceExists()
		|| !ALTeleportRetryController::instance().isOperationActive())
	{
		return;
	}
	close(false);
}

void ALFloaterTeleportRetryStatus::refreshStatus()
{
	if (!ALTeleportRetryController::instanceExists())
	{
		return;
	}
	const ALTeleportRetryController& controller = ALTeleportRetryController::instance();

	const BOOL resolving = controller.isResolvingDestination() ? TRUE : FALSE;
	mRegionText->setValue(controller.getRegionName().empty()
		? getString(resolving ? "ResolvingDestination" : "CoordsUnknown")
		: controller.getRegionName());
	if (controller.hasDestinationCoords())
	{
		const LLVector3d& pos = controller.getDestination();
		mCoordText->setValue(llformat("%d, %d, %d",
			(S32)pos.mdV[VX], (S32)pos.mdV[VY], (S32)pos.mdV[VZ]));
	}
	else
	{
		mCoordText->setValue(getString(resolving ? "ResolvingDestination" : "CoordsUnknown"));
	}
	mAttemptsText->setValue(llformat("%d", controller.getAttemptCount()));
	mElapsedText->setValue(formatElapsedSeconds(controller.getElapsedSeconds()));
	std::string interval_text = llformat("%.0f s", controller.getCurrentInterval());
	if (controller.isIntervalRaised())
	{
		interval_text += " ";
		interval_text += getString("IntervalIncreased");
	}
	mIntervalText->setValue(interval_text);
	switch (controller.getState())
	{
		case ALTeleportRetryController::STATE_WAITING:
			mStateText->setValue(getString("StateWaiting"));
			break;
		case ALTeleportRetryController::STATE_ATTEMPTING:
			mStateText->setValue(getString("StateTrying"));
			break;
		case ALTeleportRetryController::STATE_PAUSED:
			mStateText->setValue(getString("StatePaused"));
			break;
		case ALTeleportRetryController::STATE_DONE:
			mStateText->setValue(getString("StateDone"));
			break;
		case ALTeleportRetryController::STATE_STOPPED:
			mStateText->setValue(getString("StateStopped"));
			break;
		case ALTeleportRetryController::STATE_ERROR:
			mStateText->setValue(getString("StateError"));
			break;
		default:
			mStateText->setValue(std::string("-"));
			break;
	}
	if (controller.getState() == ALTeleportRetryController::STATE_WAITING
		&& controller.getSecondsRemaining() > 0)
	{
		mCountdownText->setValue(llformat("%d s", controller.getSecondsRemaining()));
	}
	else
	{
		mCountdownText->setValue(std::string("-"));
	}
	const std::string& server_message = controller.getServerMessage();
	const BOOL has_message = server_message.empty() ? FALSE : TRUE;
	mMessageLabel->setVisible(has_message);
	mMessageText->setVisible(has_message);
	if (has_message)
	{
		mMessageText->setWrappedText(server_message);
	}
	mPauseBtn->setLabel(controller.isPaused() ? getString("Resume") : getString("Pause"));
}
