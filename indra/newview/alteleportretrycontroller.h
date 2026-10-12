#pragma once
#include "lleventtimer.h"
#include "llsingleton.h"
#include "lluuid.h"
#include "v3dmath.h"
#include <string>

class ALTeleportRetryController : public LLEventTimer
, public LLSingleton<ALTeleportRetryController>
{
	friend class LLSingleton<ALTeleportRetryController>;
public:
	enum ERetryMode
	{
		MODE_SINGLE   = 0,
		MODE_MULTIPLE = 1
	};
	enum EOperationState
	{
		STATE_IDLE = 0,
		STATE_WAITING,
		STATE_ATTEMPTING,
		STATE_PAUSED,
		STATE_DONE,
		STATE_STOPPED,
		STATE_ERROR
	};
	enum EFailureKind
	{
		FAIL_GENERIC = 0,
		FAIL_REGION_FULL,
		FAIL_TIMEOUT,
		FAIL_ACCESS,
		FAIL_INVALID,
		FAIL_RATE_LIMIT,
		FAIL_MAINTENANCE
	};

	ALTeleportRetryController();
	virtual ~ALTeleportRetryController();

	void onTeleportFailureInfo(const std::string& alert_id,
							   const std::string& reason,
							   const std::string& region_name);
	void onTeleportSucceeded();

	void startFromFailedRequest(const std::string& region_name);
	void onStatusWindowClosed();
	void setPaused(bool paused);
	bool isPaused() const { return mPaused; }
	bool isOperationActive() const { return mState != STATE_IDLE; }

	EOperationState getState() const { return mState; }
	const std::string& getRegionName() const { return mRegionName; }
	const LLVector3d& getDestination() const { return mPosGlobal; }
	bool hasDestinationCoords() const { return mHasDestination; }
	bool isResolvingDestination() const { return mResolvingRegion; }
	S32 getAttemptCount() const { return mAttemptCount; }
	S32 getElapsedSeconds() const { return mElapsedSeconds; }
	F32 getCurrentInterval() const { return mInterval; }
	bool isIntervalRaised() const { return mInterval > 15.f; }
	S32 getSecondsRemaining() const { return mSecondsRemaining; }
	const std::string& getServerMessage() const { return mServerMessage; }

	static EFailureKind classifyFailure(const std::string& alert_id,
										const std::string& reason);

private:
	static void resolvedRegionCallback(U64 region_handle, const std::string& url,
									   const LLUUID& snapshot_id, bool teleport);
	void beginCycle();
	void scheduleNextAttempt();
	void attempt();
	void stopOperation(EOperationState final_state);
	void resetOperation();
	void applyRetryInterval(bool rate_limited);
	void registerAlert();
	void setServerMessage(const std::string& reason);
	void requestDestinationName();
	BOOL tick() override;

	EOperationState	mState = STATE_IDLE;
	bool			mPaused = false;
	std::string		mRegionName;
	LLVector3d		mPosGlobal;
	bool			mHasDestination = false;
	S32				mAttemptCount = 0;
	S32				mElapsedSeconds = 0;
	F32				mInterval = 15.f;
	S32				mSecondsRemaining = 0;
	S32				mAttemptSeconds = 0;
	S32				mFinalizeSeconds = 0;
	bool			mResolvingRegion = false;
	std::string		mResolveToken;
	std::string		mServerMessage;
};
