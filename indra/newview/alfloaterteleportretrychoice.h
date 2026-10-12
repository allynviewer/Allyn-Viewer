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
