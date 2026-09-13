/** 
 * @file llfloaterregioninfo.h
 * @author Aaron Brashears
 * @brief Declaration of the region info and controls floater and panels.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#ifndef LL_LLFLOATERREGIONINFO_H
#define LL_LLFLOATERREGIONINFO_H
#include <vector>
#include "llagent.h"
#include "llfloater.h"
#include "llpanel.h"
#include "llenvmanager.h"
class LLAvatarName;
class LLDispatcher;
struct LLEstateAccessChangeInfo;
class LLLineEditor;
class LLMessageSystem;
class LLPanelRegionInfo;
class LLTabContainer;
class LLViewerRegion;
class LLViewerTextEditor;
class LLInventoryItem;
class LLCheckBoxCtrl;
class LLComboBox;
class LLNameListCtrl;
class LLRadioGroup;
class LLSliderCtrl;
class LLSpinCtrl;
class LLTextBox;
class LLVFS;
class AIFilePicker;
class LLPanelRegionGeneralInfo;
class LLPanelRegionDebugInfo;
class LLPanelRegionTerrainInfo;
class LLPanelEstateInfo;
class LLPanelEstateCovenant;
class LLPanelExperienceListEditor;
class LLPanelExperiences;
class LLPanelRegionExperiences;
class LLPanelEstateAccess;
class LLEventTimer;
class LLEnvironmentSettings;
class LLWLParamManager;
class LLWaterParamManager;
class LLWLParamSet;
class LLWaterParamSet;
class LLFloaterRegionInfo : public LLFloater, public LLFloaterSingleton<LLFloaterRegionInfo>
{
	friend class LLUISingleton<LLFloaterRegionInfo, VisibilityPolicy<LLFloater>>;
public:
	void onOpen() override;
	void onClose(bool app_quitting) override;
	BOOL postBuild() override;
	void open() override;
	static void processEstateOwnerRequest(LLMessageSystem* msg, void**);
	static void processRegionInfo(LLMessageSystem* msg);
	static const LLUUID& getLastInvoice() { return sRequestInvoice; }
	static void nextInvoice() { sRequestInvoice.generate(); }
	static LLPanelEstateInfo* getPanelEstate();
	static LLPanelEstateAccess* getPanelAccess();
	static LLPanelEstateCovenant* getPanelCovenant();
	static LLPanelRegionTerrainInfo* getPanelRegionTerrain();
	static LLPanelRegionExperiences* getPanelExperiences();
	void refresh() override;
	void requestRegionInfo();
	void requestMeshRezInfo();
private:
protected:
	LLFloaterRegionInfo(const LLSD& seed);
	~LLFloaterRegionInfo();
protected:
	void onTabSelected(const LLSD& param);
	void refreshFromRegion(LLViewerRegion* region);
	void onGodLevelChange(U8 god_level);
	LLTabContainer* mTab;
	typedef std::vector<LLPanelRegionInfo*> info_panels_t;
	info_panels_t mInfoPanels;
	static LLUUID sRequestInvoice;
private:
	LLAgent::god_level_change_slot_t   mGodLevelChangeSlot;
};
class LLPanelRegionInfo : public LLPanel
{
public:
	LLPanelRegionInfo();
	void onBtnSet();
	void onChangeChildCtrl(LLUICtrl* ctrl);
	void onChangeAnything();
	static void onChangeText(LLLineEditor* caller, void* user_data);
	virtual bool refreshFromRegion(LLViewerRegion* region);
	virtual bool estateUpdate(LLMessageSystem* msg) { return true; }
	BOOL postBuild() override;
	virtual void updateChild(LLUICtrl* child_ctrl);
	virtual void onOpen(const LLSD& key) {}
	void enableButton(const std::string& btn_name, BOOL enable = TRUE);
	void disableButton(const std::string& btn_name);
	void onClickManageTelehub();
protected:
	void initCtrl(const std::string& name);
	void initHelpBtn(const std::string& name, const std::string& xml_alert);
	void onClickHelp(const std::string& xml_alert);
	virtual BOOL sendUpdate() { return TRUE; }
	typedef std::vector<std::string> strings_t;
	void sendEstateOwnerMessage(
					 LLMessageSystem* msg,
					 const std::string& request,
					 const LLUUID& invoice,
					 const strings_t& strings);
	LLHost mHost;
};
class LLPanelRegionGeneralInfo : public LLPanelRegionInfo
{
public:
	LLPanelRegionGeneralInfo()
		:	LLPanelRegionInfo()	{}
	~LLPanelRegionGeneralInfo() {}
	bool refreshFromRegion(LLViewerRegion* region) override;
	BOOL postBuild() override;
protected:
	BOOL sendUpdate() override;
	void onClickKick();
	void onKickCommit(const uuid_vec_t& ids);
	static void onClickKickAll(void* userdata);
	bool onKickAllCommit(const LLSD& notification, const LLSD& response);
	static void onClickMessage(void* userdata);
	bool onMessageCommit(const LLSD& notification, const LLSD& response);
};
class LLPanelRegionDebugInfo : public LLPanelRegionInfo
{
public:
	LLPanelRegionDebugInfo()
		:	LLPanelRegionInfo(), mTargetAvatar() {}
	~LLPanelRegionDebugInfo() {}
	BOOL postBuild() override;
	bool refreshFromRegion(LLViewerRegion* region) override;
protected:
	BOOL sendUpdate() override;
	void onClickChooseAvatar();
	void callbackAvatarID(const uuid_vec_t& ids, const std::vector<LLAvatarName>& names);
	static void onClickReturn(void *);
	bool callbackReturn(const LLSD& notification, const LLSD& response);
	static void onClickTopColliders(void*);
	static void onClickTopScripts(void*);
	static void onClickRestart(void* data);
	bool callbackRestart(const LLSD& notification, const LLSD& response);
	static void onClickCancelRestart(void* data);
	static void onClickDebugConsole(void* data);
private:
	LLUUID mTargetAvatar;
};
class LLPanelRegionTerrainInfo : public LLPanelRegionInfo
{
	LOG_CLASS(LLPanelRegionTerrainInfo);
public:
	LLPanelRegionTerrainInfo() : LLPanelRegionInfo() {}
	~LLPanelRegionTerrainInfo() {}
	BOOL postBuild() override;
	bool refreshFromRegion(LLViewerRegion* region) override;
	void setEnvControls(bool available);
	BOOL validateTextureSizes();
protected:
	BOOL sendUpdate() override;
	static void onClickDownloadRaw(void*);
	void onClickDownloadRaw_continued(AIFilePicker* filepicker);
	static void onClickUploadRaw(void*);
	void onClickUploadRaw_continued(AIFilePicker* filepicker);
	static void onClickBakeTerrain(void*);
	bool callbackBakeTerrain(const LLSD& notification, const LLSD& response);
};
class LLPanelEstateInfo : public LLPanelRegionInfo
{
public:
	static void initDispatch(LLDispatcher& dispatch);
	void onChangeFixedSun();
	void onChangeUseGlobalTime();
	void onChangeAccessOverride();
	void onClickEditSky();
	void onClickEditSkyHelp();
	void onClickEditDayCycle();
	void onClickEditDayCycleHelp();
	void onClickKickUser();
	bool kickUserConfirm(const LLSD& notification, const LLSD& response);
	void onKickUserCommit(const uuid_vec_t& ids, const std::vector<LLAvatarName>& names);
	static void onClickMessageEstate(void* data);
	bool onMessageCommit(const LLSD& notification, const LLSD& response);
	LLPanelEstateInfo();
	~LLPanelEstateInfo() {}
	void updateControls(LLViewerRegion* region);
	static void updateEstateName(const std::string& name);
	static void updateEstateOwnerID(const LLUUID& id);
	bool refreshFromRegion(LLViewerRegion* region) override;
	bool estateUpdate(LLMessageSystem* msg) override;
	BOOL postBuild() override;
	void updateChild(LLUICtrl* child_ctrl) override;
	void refresh() override;
	void refreshFromEstate();
	static bool isLindenEstate();
	const std::string getOwnerName() const;
protected:
	BOOL sendUpdate() override;
	bool callbackChangeLindenEstate(const LLSD& notification, const LLSD& response);
	void commitEstateAccess();
	void commitEstateManagers();
	BOOL checkSunHourSlider(LLUICtrl* child_ctrl);
	U32 mEstateID;
};
class LLPanelEstateCovenant : public LLPanelRegionInfo
{
public:
	LLPanelEstateCovenant();
	~LLPanelEstateCovenant() {}
	BOOL postBuild() override;
	void updateChild(LLUICtrl* child_ctrl) override;
	bool refreshFromRegion(LLViewerRegion* region) override;
	bool estateUpdate(LLMessageSystem* msg) override;
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
						   BOOL drop, EDragAndDropType cargo_type,
						   void *cargo_data, EAcceptance *accept,
						   std::string& tooltip_msg) override;
	static bool confirmChangeCovenantCallback(const LLSD& notification, const LLSD& response);
	static void resetCovenantID(void* userdata);
	static bool confirmResetCovenantCallback(const LLSD& notification, const LLSD& response);
	void sendChangeCovenantID(const LLUUID &asset_id);
	void loadInvItem(LLInventoryItem *itemp);
	static void onLoadComplete(LLVFS *vfs,
							   const LLUUID& asset_uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);
	static void updateCovenantText(const std::string& string, const LLUUID& asset_id);
	static void updateEstateName(const std::string& name);
	static void updateLastModified(const std::string& text);
	static void updateEstateOwnerID(const LLUUID& id);
	const LLUUID& getCovenantID() const { return mCovenantID; }
	void setCovenantID(const LLUUID& id) { mCovenantID = id; }
	std::string getEstateName() const;
	void setEstateName(const std::string& name);
	std::string getOwnerName() const;
	void setCovenantTextEditor(const std::string& text);
	typedef enum e_asset_status
	{
		ASSET_ERROR,
		ASSET_UNLOADED,
		ASSET_LOADING,
		ASSET_LOADED
	} EAssetStatus;
protected:
	BOOL sendUpdate() override;
	LLTextBox*				mEstateNameText;
	LLTextBox*				mEstateOwnerText;
	LLTextBox*				mLastModifiedText;
	LLUUID					mCovenantID;
	LLViewerTextEditor*		mEditor;
	EAssetStatus			mAssetStatus;
};
class LLPanelEnvironmentInfo : public LLPanelRegionInfo
{
	LOG_CLASS(LLPanelEnvironmentInfo);
public:
	LLPanelEnvironmentInfo();
	BOOL postBuild() override;
	void onOpen(const LLSD& key) override;
	void handleVisibilityChange(BOOL new_visibility) override;
	bool refreshFromRegion(LLViewerRegion* region) override;
private:
	void refresh() override;
	void setControlsEnabled(bool enabled);
	void setApplyProgress(bool started);
	void setDirty(bool dirty);
	void sendRegionSunUpdate();
	void fixEstateSun();
	void populateWaterPresetsList();
	void populateSkyPresetsList();
	void populateDayCyclesList();
	bool getSelectedWaterParams(LLSD& water_params);
	bool getSelectedSkyParams(LLSD& sky_params, std::string& preset_name);
	bool getSelectedDayCycleParams(LLSD& day_cycle, LLSD& sky_map, short& scope);
	void onSwitchRegionSettings();
	void onSwitchDayCycle();
	void onSelectWaterPreset();
	void onSelectSkyPreset();
	void onSelectDayCycle();
	void onBtnApply();
	void onBtnCancel();
	void onRegionSettingschange();
	void onRegionSettingsApplied(bool ok);
	LLEnvironmentSettings	mNewRegionSettings;
	bool			mEnableEditing;
	LLRadioGroup*	mRegionSettingsRadioGroup;
	LLRadioGroup*	mDayCycleSettingsRadioGroup;
	LLComboBox*		mWaterPresetCombo;
	LLComboBox*		mSkyPresetCombo;
	LLComboBox*		mDayCyclePresetCombo;
};
class LLPanelRegionExperiences : public LLPanelRegionInfo
{
	LOG_CLASS(LLPanelEnvironmentInfo);
public:
	LLPanelRegionExperiences();
	BOOL postBuild() override;
	BOOL sendUpdate() override;
	static bool experienceCoreConfirm(const LLSD& notification, const LLSD& response);
	static void sendEstateExperienceDelta(U32 flags, const LLUUID& agent_id);
	static void infoCallback(LLHandle<LLPanelRegionExperiences> handle, const LLSD& content);
	bool refreshFromRegion(LLViewerRegion* region) override;
	void sendPurchaseRequest()const;
	void processResponse( const LLSD& content );
private:
	void refreshRegionExperiences();
    static std::string regionCapabilityQuery(LLViewerRegion* region, const std::string &cap);
	void setupList(LLPanelExperienceListEditor* child, const char* control_name, U32 add_id, U32 remove_id);
	static LLSD addIds( LLPanelExperienceListEditor* panel );
	void itemChanged(U32 event_type, const LLUUID& id);
	LLPanelExperienceListEditor* mTrusted;
	LLPanelExperienceListEditor* mAllowed;
	LLPanelExperienceListEditor* mBlocked;
	LLUUID mDefaultExperience;
};
class LLPanelEstateAccess : public LLPanelRegionInfo
{
	LOG_CLASS(LLPanelEnvironmentInfo);
public:
	LLPanelEstateAccess();
	virtual BOOL postBuild();
	virtual void updateChild(LLUICtrl* child_ctrl);
	void updateControls(LLViewerRegion* region);
	void updateLists();
	void setPendingUpdate(bool pending) { mPendingUpdate = pending; }
	bool getPendingUpdate() { return mPendingUpdate; }
	virtual bool refreshFromRegion(LLViewerRegion* region);
	static void onEstateAccessReceived(const LLSD& result);
private:
	void onClickAddAllowedAgent();
	void onClickRemoveAllowedAgent();
	void onClickCopyAllowedList();
	void onClickAddAllowedGroup();
	void onClickRemoveAllowedGroup();
	void onClickCopyAllowedGroupList();
	void onClickAddBannedAgent();
	void onClickRemoveBannedAgent();
    void onClickCopyBannedList();
	void onClickAddEstateManager();
	void onClickRemoveEstateManager();
	void onAllowedSearchEdit(const std::string& search_string);
	void onAllowedGroupsSearchEdit(const std::string& search_string);
	void onBannedSearchEdit(const std::string& search_string);
	bool addAllowedGroup(const LLSD& notification, const LLSD& response);
	void addAllowedGroup2(LLUUID id);
	static void accessAddCore(U32 operation_flag, const std::string& dialog_name);
	static bool accessAddCore2(const LLSD& notification, const LLSD& response);
	static void accessAddCore3(const uuid_vec_t& ids, const std::vector<LLAvatarName>& names, LLEstateAccessChangeInfo* change_info);
	static void accessRemoveCore(U32 operation_flag, const std::string& dialog_name, const std::string& list_ctrl_name);
	static bool accessRemoveCore2(const LLSD& notification, const LLSD& response);
	static bool accessCoreConfirm(const LLSD& notification, const LLSD& response);
public:
	static void sendEstateAccessDelta(U32 flags, const LLUUID& agent_id);
private:
	void searchAgent(LLNameListCtrl* listCtrl, const std::string& search_string);
	void copyListToClipboard(std::string list_name);
	bool mPendingUpdate;
	bool mCtrlsEnabled;
};
#endif
