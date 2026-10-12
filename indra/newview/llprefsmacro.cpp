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
