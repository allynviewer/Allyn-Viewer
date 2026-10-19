/**
 * @file llprefsmacro.cpp
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
#include "llprefsmacro.h"

#include "lluictrlfactory.h"
#include "llviewercontrol.h"

LLPrefsMacro::LLPrefsMacro()
:	mAssistEnabled(false),
	mAcquisition(30.f),
	mStrength(50.f)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_macro.xml");
	refreshValues();
}

LLPrefsMacro::~LLPrefsMacro()
{
}

void LLPrefsMacro::refreshValues()
{
	mAssistEnabled = gSavedSettings.getBOOL("AllynMacroAimAssistEnabled");
	mAcquisition = gSavedSettings.getF32("AllynMacroAimAcquisition");
	mStrength = gSavedSettings.getF32("AllynMacroAimStrength");
}

void LLPrefsMacro::apply()
{
	refreshValues();
}

void LLPrefsMacro::cancel()
{
	gSavedSettings.setBOOL("AllynMacroAimAssistEnabled", mAssistEnabled);
	gSavedSettings.setF32("AllynMacroAimAcquisition", mAcquisition);
	gSavedSettings.setF32("AllynMacroAimStrength", mStrength);
}
