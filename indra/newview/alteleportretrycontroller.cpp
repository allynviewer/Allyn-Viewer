#include "llviewerprecompiledheaders.h"
#include "alteleportretrycontroller.h"
#include "alfloaterteleportretrychoice.h"
#include "alfloaterteleportretrystatus.h"
#include "llagent.h"
#include "llregionhandle.h"
#include "llstring.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llworldmap.h"
#include "llworldmapmessage.h"

static const F32 BASE_INTERVAL_SECONDS = 15.f;
static const S32 ATTEMPT_WATCHDOG_SECONDS = 60;
static const S32 TERMINAL_STATE_SECONDS = 3;

ALTeleportRetryController::ALTeleportRetryController()
:	LLEventTimer(1.f),
	mState(STATE_IDLE)
{
	mEventTimer.stop();
}

ALTeleportRetryController::~ALTeleportRetryController()
{
	mEventTimer.stop();
}

static std::string normalizeFailureText(const std::string& text)
{
	std::string normalized = text;
	LLStringUtil::trim(normalized);
	LLStringUtil::toLower(normalized);
	while (!normalized.empty())
	{
		const char tail = normalized[normalized.length() - 1];
		if (tail != '.' && tail != '!' && tail != '?' && tail != ' '
			&& tail != '\r' && tail != '\n')
		{
			break;
		}
		normalized.resize(normalized.length() - 1);
	}
	return normalized;
}

static bool hasPhrase(const std::string& text, const char* phrase)
{
	return text.find(phrase) != std::string::npos;
}

static std::string failureBlob(const std::string& alert_id, const std::string& reason)
{
	const std::string alert_text = normalizeFailureText(alert_id);
	const std::string reason_text = normalizeFailureText(reason);
	if (reason_text.empty() || reason_text == alert_text)
	{
		return alert_text;
	}
	if (alert_text.empty())
	{
		return reason_text;
	}
	return alert_text + "\n" + reason_text;
}

static bool isBannedFailure(const std::string& alert_id, const std::string& text)
{
	if (alert_id == "URBannedFromRegion"
		|| alert_id == "RegionParcelBan"
		|| alert_id == "Cannot enter parcel: banned")
	{
		return true;
	}
	return hasPhrase(text, "banned")
		|| hasPhrase(text, "banido")
		|| hasPhrase(text, "banida");
}

static bool isMissingDestination(const std::string& alert_id, const std::string& text)
{
	if (alert_id == "no_host"
		|| alert_id == "nolandmark_tport"
		|| alert_id == "NoDestRegion"
		|| alert_id == "CantTeleportCouldNotFindUser")
	{
		return true;
	}
	return hasPhrase(text, "does not exist")
		|| hasPhrase(text, "no longer exist")
		|| hasPhrase(text, "nao existe")
		|| hasPhrase(text, "não existe")
		|| hasPhrase(text, "nao existir")
		|| hasPhrase(text, "não existir")
		|| hasPhrase(text, "unable to find teleport destination")
		|| hasPhrase(text, "unable to locate landmark")
		|| hasPhrase(text, "no destination region")
		|| hasPhrase(text, "nenhuma regiao de destino")
		|| hasPhrase(text, "nenhuma região de destino")
		|| hasPhrase(text, "could not find user")
		|| hasPhrase(text, "nao foi possivel encontrar o destino")
		|| hasPhrase(text, "não foi possível encontrar o destino");
}

static bool isAccessFailure(const std::string& alert_id, const std::string& text)
{
	if (alert_id == "noaccess_tport"
		|| alert_id == "NotAllowedInDest"
		|| alert_id == "RegionTPAccessBlocked"
		|| alert_id == "ImproperPaymentStatus"
		|| alert_id == "MustGetAgeRegion"
		|| alert_id == "MustGetAgeParcel"
		|| alert_id == "NoTeenGridAccess"
		|| alert_id == "NoHelpIslandTP"
		|| alert_id == "Cannot enter parcel: not on access list"
		|| alert_id == "Cannot enter parcel: not a group member"
		|| alert_id.compare(0, 26, "TeleportEntryAccessBlocked") == 0
		|| alert_id.compare(0, 24, "RegionEntryAccessBlocked") == 0)
	{
		return true;
	}
	return hasPhrase(text, "do not have access")
		|| hasPhrase(text, "nao tem acesso")
		|| hasPhrase(text, "não tem acesso")
		|| hasPhrase(text, "not allowed")
		|| hasPhrase(text, "nao e permitido")
		|| hasPhrase(text, "não é permitido")
		|| hasPhrase(text, "not on the access list")
		|| hasPhrase(text, "not on access list");
}

static bool isFullRegionFailure(const std::string& alert_id, const std::string& text)
{
	if (alert_id == "FullRegionTryAgain"
		|| alert_id == "FullRegionCantEnter"
		|| alert_id == "region_full"
		|| alert_id == "NoEnterRegionMaybeFull")
	{
		return true;
	}
	const bool capacity = hasPhrase(text, "full")
		|| hasPhrase(text, "cheia")
		|| hasPhrase(text, "cheio")
		|| hasPhrase(text, "lotada")
		|| hasPhrase(text, "lotado");
	if (!capacity)
	{
		return false;
	}
	return hasPhrase(text, "region")
		|| hasPhrase(text, "regi")
		|| hasPhrase(text, "simulator")
		|| hasPhrase(text, "currently full");
}

static bool isMaintenanceFailure(const std::string& alert_id, const std::string& text)
{
	if (alert_id == "RegionAboutToShutdown")
	{
		return true;
	}
	return hasPhrase(text, "shut down")
		|| hasPhrase(text, "shutting down")
		|| hasPhrase(text, "prestes a desligar")
		|| hasPhrase(text, "maintenance")
		|| hasPhrase(text, "manuten")
		|| hasPhrase(text, "restarting")
		|| hasPhrase(text, "reinici")
		|| hasPhrase(text, "starting up");
}

static bool isRateLimitFailure(const std::string& alert_id, const std::string& text)
{
	if (alert_id == "blocked_tport")
	{
		return true;
	}
	return hasPhrase(text, "currently blocked")
		|| hasPhrase(text, "teleport throttle")
		|| hasPhrase(text, "teleporting too frequently")
		|| hasPhrase(text, "too many teleport")
		|| hasPhrase(text, "atualmente bloqueados")
		|| hasPhrase(text, "bloqueados no momento")
		|| hasPhrase(text, "estao bloqueados")
		|| hasPhrase(text, "estão bloqueados");
}

ALTeleportRetryController::EFailureKind ALTeleportRetryController::classifyFailure(
	const std::string& alert_id, const std::string& reason)
{
	const std::string text = failureBlob(alert_id, reason);
	if (isBannedFailure(alert_id, text))
	{
		return FAIL_ACCESS;
	}
	if (isMissingDestination(alert_id, text))
	{
		return FAIL_INVALID;
	}
	if (isAccessFailure(alert_id, text))
	{
		return FAIL_ACCESS;
	}
	if (isFullRegionFailure(alert_id, text))
	{
		return FAIL_REGION_FULL;
	}
	if (isMaintenanceFailure(alert_id, text))
	{
		return FAIL_MAINTENANCE;
	}
	if (isRateLimitFailure(alert_id, text))
	{
		return FAIL_RATE_LIMIT;
	}
	if (alert_id == "timeout_tport" || alert_id == "expired_tport"
		|| alert_id == "invalid_region_handoff"
		|| alert_id == "expired_region_handoff"
		|| alert_id == "too_many_uploads_tport"
		|| alert_id == "missing_attach_tport"
		|| alert_id == "no_inventory_host")
	{
		return FAIL_TIMEOUT;
	}
	std::map<std::string, std::string>::const_iterator it =
		LLAgent::sTeleportErrorMessages.find(alert_id);
	if (it != LLAgent::sTeleportErrorMessages.end() && !it->second.empty())
	{
		return FAIL_INVALID;
	}
	return FAIL_GENERIC;
}

void ALTeleportRetryController::onTeleportFailureInfo(const std::string& alert_id,
													  const std::string& reason,
													  const std::string& region_name)
{
	const EFailureKind kind = classifyFailure(alert_id, reason);
	if (mState == STATE_IDLE)
	{
		if (kind != FAIL_REGION_FULL && kind != FAIL_MAINTENANCE)
		{
			return;
		}
		if (!gAgent.hasRestartableFailedTeleportRequest() && region_name.empty())
		{
			return;
		}
		setServerMessage(reason);
		if (gSavedSettings.getBOOL("ALLTeleportRetryAskAgain"))
		{
			ALFloaterTeleportRetryChoice* choicep = ALFloaterTeleportRetryChoice::getInstance();
			choicep->setDestinationRegion(region_name);
			choicep->open();
		}
		else if (gSavedSettings.getS32("ALLTeleportRetryMode") == MODE_MULTIPLE)
		{
			startFromFailedRequest(region_name);
		}
		return;
	}
	if (mState != STATE_ATTEMPTING)
	{
		return;
	}
	setServerMessage(reason);
	switch (kind)
	{
		case FAIL_RATE_LIMIT:
			applyRetryInterval(true);
			break;
		case FAIL_REGION_FULL:
		case FAIL_MAINTENANCE:
		case FAIL_TIMEOUT:
			applyRetryInterval(false);
			break;
		case FAIL_ACCESS:
		case FAIL_INVALID:
		case FAIL_GENERIC:
		default:
			stopOperation(STATE_ERROR);
			break;
	}
}

void ALTeleportRetryController::startFromFailedRequest(const std::string& region_name)
{
	if (mState != STATE_IDLE)
	{
		return;
	}
	mRegionName = region_name;
	mPosGlobal.clearVec();
	mHasDestination = false;
	mAttemptCount = 0;
	mElapsedSeconds = 0;
	mInterval = BASE_INTERVAL_SECONDS;
	mResolvingRegion = false;
	mResolveToken.clear();

	LLVector3d request_pos;
	if (gAgent.getTeleportRequestDestination(request_pos))
	{
		mPosGlobal = request_pos;
		mHasDestination = true;
	}
	if (mRegionName.empty() && mHasDestination)
	{
		LLSimInfo* infop = LLWorldMap::getInstance()->simInfoFromPosGlobal(mPosGlobal);
		if (infop)
		{
			mRegionName = infop->getName();
		}
	}

	const bool restartable = gAgent.hasRestartableFailedTeleportRequest();
	if (restartable)
	{
		mAttemptCount = 1;
	}
	if (!restartable && !mHasDestination && mRegionName.empty())
	{
		return;
	}
	if (restartable || mHasDestination)
	{
		beginCycle();
		if (mRegionName.empty() && mHasDestination)
		{
			requestDestinationName();
		}
		return;
	}
	mResolvingRegion = true;
	mResolveToken = region_name;
	LLWorldMapMessage::getInstance()->sendNamedRegionRequest(region_name,
		resolvedRegionCallback, mResolveToken, false);
}

void ALTeleportRetryController::requestDestinationName()
{
	mResolvingRegion = true;
	const U64 handle = to_region_handle(mPosGlobal);
	mResolveToken = std::to_string((unsigned long long)handle);
	LLWorldMapMessage::getInstance()->sendHandleRegionRequest(handle,
		resolvedRegionCallback, mResolveToken, false);
}

void ALTeleportRetryController::resolvedRegionCallback(U64 region_handle,
													   const std::string& url,
													   const LLUUID& snapshot_id,
													   bool teleport)
{
	if (!instanceExists())
	{
		return;
	}
	ALTeleportRetryController& self = instance();
	if (!self.mResolvingRegion || url != self.mResolveToken)
	{
		return;
	}
	self.mResolvingRegion = false;
	self.mResolveToken.clear();
	if (!self.mHasDestination)
	{
		self.mPosGlobal = from_region_handle(region_handle);
		self.mHasDestination = true;
	}
	LLSimInfo* infop = LLWorldMap::getInstance()->simInfoFromHandle(region_handle);
	if (infop && !infop->getName().empty())
	{
		self.mRegionName = infop->getName();
	}
	if (self.mState == STATE_IDLE)
	{
		self.beginCycle();
	}
}

void ALTeleportRetryController::onTeleportSucceeded()
{
	if (mState != STATE_ATTEMPTING)
	{
		return;
	}
	stopOperation(STATE_DONE);
}

void ALTeleportRetryController::beginCycle()
{
	mPaused = false;
	mAttemptSeconds = 0;
	mElapsedSeconds = 0;
	mFinalizeSeconds = 0;
	mState = STATE_WAITING;
	mSecondsRemaining = (S32)llceil(mInterval);
	mEventTimer.start();
	ALFloaterTeleportRetryStatus::showInstance();
}

void ALTeleportRetryController::scheduleNextAttempt()
{
	mState = STATE_WAITING;
	mSecondsRemaining = (S32)llceil(mInterval);
	mAttemptSeconds = 0;
}

void ALTeleportRetryController::attempt()
{
	mState = STATE_ATTEMPTING;
	mAttemptSeconds = 0;
	mSecondsRemaining = 0;
	if (gAgent.hasRestartableFailedTeleportRequest())
	{
		gAgent.restartFailedTeleportRequest();
	}
	else if (mHasDestination)
	{
		gAgent.teleportViaLocation(mPosGlobal);
	}
	else
	{
		stopOperation(STATE_ERROR);
		return;
	}
	if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
	{
		++mAttemptCount;
	}
	else
	{
		stopOperation(STATE_ERROR);
	}
}

void ALTeleportRetryController::stopOperation(EOperationState final_state)
{
	mState = final_state;
	mPaused = false;
	mResolvingRegion = false;
	mResolveToken.clear();
	mSecondsRemaining = 0;
	mFinalizeSeconds = TERMINAL_STATE_SECONDS;
}

void ALTeleportRetryController::resetOperation()
{
	mState = STATE_IDLE;
	mPaused = false;
	mResolvingRegion = false;
	mResolveToken.clear();
	mRegionName.clear();
	mServerMessage.clear();
	mHasDestination = false;
	mPosGlobal.clearVec();
	mAttemptCount = 0;
	mElapsedSeconds = 0;
	mInterval = BASE_INTERVAL_SECONDS;
	mSecondsRemaining = 0;
	mEventTimer.stop();
}

void ALTeleportRetryController::onStatusWindowClosed()
{
	if (mState == STATE_IDLE)
	{
		return;
	}
	if (mState == STATE_DONE || mState == STATE_STOPPED || mState == STATE_ERROR)
	{
		resetOperation();
		return;
	}
	stopOperation(STATE_STOPPED);
}

void ALTeleportRetryController::setPaused(bool paused)
{
	if (mState == STATE_IDLE)
	{
		return;
	}
	if (paused)
	{
		if (mState == STATE_ATTEMPTING)
		{
			mPaused = true;
		}
		else if (mState == STATE_WAITING)
		{
			mState = STATE_PAUSED;
		}
	}
	else
	{
		mPaused = false;
		if (mState == STATE_PAUSED)
		{
			mState = STATE_WAITING;
			if (mSecondsRemaining <= 0)
			{
				mSecondsRemaining = (S32)llceil(mInterval);
			}
		}
	}
}

void ALTeleportRetryController::applyRetryInterval(bool rate_limited)
{
	if (rate_limited)
	{
		registerAlert();
	}
	else
	{
		mInterval = BASE_INTERVAL_SECONDS;
	}
	if (mPaused)
	{
		mState = STATE_PAUSED;
	}
	else
	{
		scheduleNextAttempt();
	}
}

void ALTeleportRetryController::registerAlert()
{
	const F32 next = mInterval * 2.f;
	if (next > mInterval && next < 1.0e9f)
	{
		mInterval = next;
	}
}

void ALTeleportRetryController::setServerMessage(const std::string& reason)
{
	mServerMessage = reason;
	LLStringUtil::trim(mServerMessage);
}

BOOL ALTeleportRetryController::tick()
{
	if (mState == STATE_IDLE)
	{
		return FALSE;
	}
	if (!gAgent.getRegion())
	{
		stopOperation(STATE_STOPPED);
		return FALSE;
	}
	if (mState == STATE_WAITING || mState == STATE_PAUSED)
	{
		if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
		{
			stopOperation(STATE_STOPPED);
			return FALSE;
		}
	}
	switch (mState)
	{
		case STATE_WAITING:
			++mElapsedSeconds;
			if (--mSecondsRemaining <= 0)
			{
				attempt();
			}
			break;
		case STATE_PAUSED:
			++mElapsedSeconds;
			break;
		case STATE_ATTEMPTING:
		{
			++mElapsedSeconds;
			if (gAgent.getTeleportState() == LLAgent::TELEPORT_NONE)
			{
				if (++mAttemptSeconds >= ATTEMPT_WATCHDOG_SECONDS)
				{
					stopOperation(STATE_ERROR);
					return FALSE;
				}
			}
			else
			{
				mAttemptSeconds = 0;
			}
			break;
		}
		case STATE_DONE:
		case STATE_STOPPED:
		case STATE_ERROR:
			if (--mFinalizeSeconds <= 0)
			{
				resetOperation();
			}
			break;
		default:
			break;
	}
	return FALSE;
}
