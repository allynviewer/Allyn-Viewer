/**
 * @file alfloaterteleportretrychoice.h
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
#include "llfloater.h"
class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLTextBox;
class ALFloaterTeleportRetryChoice : public LLFloater
, public LLFloaterSingleton<ALFloaterTeleportRetryChoice>
{
	friend class LLUISingleton<ALFloaterTeleportRetryChoice, VisibilityPolicy<LLFloater> >;
private:
	ALFloaterTeleportRetryChoice(const LLSD& key);
	virtual ~ALFloaterTeleportRetryChoice();
public:
	BOOL postBuild() override;
	void onOpen() override;
	void setDestinationRegion(const std::string& region) { mRegionName = region; }
	using LLFloaterSingleton<ALFloaterTeleportRetryChoice>::getInstance;
private:
	void onClickConfirm();
	void onClickCancel();
	LLComboBox*		mModeCombo = nullptr;
	LLCheckBoxCtrl* mAskAgainCheck = nullptr;
	LLTextBox*		mRegionText = nullptr;
	std::string		mRegionName;
};
