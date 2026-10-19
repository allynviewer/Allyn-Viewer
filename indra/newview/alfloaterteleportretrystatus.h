/**
 * @file alfloaterteleportretrystatus.h
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
