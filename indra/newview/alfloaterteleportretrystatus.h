#pragma once
#include "lleventtimer.h"
#include "llfloater.h"
class LLButton;
class LLTextBox;
class ALFloaterTeleportRetryStatus : public LLFloater, public LLEventTimer
, public LLFloaterSingleton<ALFloaterTeleportRetryStatus>
{
	friend class LLUISingleton<ALFloaterTeleportRetryStatus, VisibilityPolicy<LLFloater> >;
private:
	ALFloaterTeleportRetryStatus(const LLSD& key);
	virtual ~ALFloaterTeleportRetryStatus();
public:
	BOOL postBuild() override;
	void onOpen() override;
	void onClose(bool app_quitting) override;
	BOOL tick() override;
	using LLFloaterSingleton<ALFloaterTeleportRetryStatus>::getInstance;
	using LLFloaterSingleton<ALFloaterTeleportRetryStatus>::showInstance;
private:
	void onClickPause();
	void refreshStatus();
	LLButton*	 mPauseBtn = nullptr;
	LLTextBox*	 mRegionText = nullptr;
	LLTextBox*	 mCoordText = nullptr;
	LLTextBox*	 mAttemptsText = nullptr;
	LLTextBox*	 mElapsedText = nullptr;
	LLTextBox*	 mIntervalText = nullptr;
	LLTextBox*	 mCountdownText = nullptr;
	LLTextBox*	 mStateText = nullptr;
	LLTextBox*	 mMessageLabel = nullptr;
	LLTextBox*	 mMessageText = nullptr;
};
