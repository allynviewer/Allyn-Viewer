/** 
 * @file llfloaterland.h
 * @author James Cook
 * @brief "About Land" floater, allowing display and editing of land parcel properties.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#ifndef LL_LLFLOATERLAND_H
#define LL_LLFLOATERLAND_H
#include "llfloater.h"
#include "llpointer.h"
#include "llsafehandle.h"
const F32 CACHE_REFRESH_TIME	= 2.5f;
class LLButton;
class LLCheckBoxCtrl;
class LLRadioGroup;
class LLComboBox;
class LLLineEditor;
class LLMessageSystem;
class LLNameListCtrl;
class LLRadioGroup;
class LLParcelSelectionObserver;
class LLSpinCtrl;
class LLTabContainer;
class LLTextBox;
class LLTextEditor;
class LLTextureCtrl;
class LLUIImage;
class LLParcelSelection;
class LLPanelLandGeneral;
class LLPanelLandObjects;
class LLPanelLandOptions;
class LLPanelLandAudio;
class LLPanelLandMedia;
class LLPanelLandAccess;
class LLPanelLandBan;
class LLPanelLandRenters;
class LLPanelLandCovenant;
class LLParcel;
class LLPanelLandExperiences;
class LLFloaterLand final
:	public LLFloater
, public LLFloaterSingleton<LLFloaterLand>
{
	friend class LLUISingleton<LLFloaterLand, VisibilityPolicy<LLFloater> >;
public:
	static void refreshAll();
	static LLPanelLandObjects* getCurrentPanelLandObjects();
	static LLPanelLandCovenant* getCurrentPanelLandCovenant();
	LLParcel* getCurrentSelectedParcel();
	void onClose(bool app_quitting) override;
	void onOpen() override;
	BOOL postBuild() override;
	void open() override;
private:
	LLFloaterLand(const LLSD& seed);
	virtual ~LLFloaterLand();
protected:
	void refresh() override;
	static void* createPanelLandGeneral(void* data);
	static void* createPanelLandCovenant(void* data);
	static void* createPanelLandObjects(void* data);
	static void* createPanelLandOptions(void* data);
	static void* createPanelLandAudio(void* data);
	static void* createPanelLandMedia(void* data);
	static void* createPanelLandAccess(void* data);
	static void* createPanelLandExperiences(void* data);
	static void* createPanelLandBan(void* data);
protected:
	static LLParcelSelectionObserver* sObserver;
	static S32 sLastTab;
	friend class LLPanelLandObjects;
	LLTabContainer*			mTabLand;
	LLPanelLandGeneral*		mPanelGeneral;
	LLPanelLandObjects*		mPanelObjects;
	LLPanelLandOptions*		mPanelOptions;
	LLPanelLandAudio*		mPanelAudio;
	LLPanelLandMedia*		mPanelMedia;
	LLPanelLandAccess*		mPanelAccess;
	LLPanelLandCovenant*	mPanelCovenant;
	LLPanelLandExperiences*	mPanelExperiences;
	LLSafeHandle<LLParcelSelection>	mParcel;
public:
	static BOOL sRequestReplyOnUpdate;
};
class LLPanelLandGeneral final
:	public LLPanel
{
public:
	LLPanelLandGeneral(LLSafeHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandGeneral();
	void refresh() override;
	void refreshNames();
	void setGroup(const LLUUID& group_id);
	void onClickSetGroup();
	static void onClickBuyLand(void* data);
	static void onClickBuyPass(void* deselect_when_done);
	void onClickDeed();
	void onClickScriptLimits();
	void onClickRelease();
	void onClickReclaim();
	static BOOL enableBuyPass(void*);
	void onCommitAny();
	static void finalizeCommit(void * userdata);
	static void onForSaleChange(LLUICtrl *ctrl, void * userdata);
	static void finalizeSetSellChange(void * userdata);
	static void onSalePriceChange(LLUICtrl *ctrl, void * userdata);
	static bool cbBuyPass(const LLSD& notification, const LLSD& response);
	void onClickSellLand();
	void onClickStopSellLand();
	void onClickSet();
	void onClickClear();
	void onClickShow();
	static void callbackAvatarPick(const std::vector<std::string>& names, const uuid_vec_t& ids, void* data);
	static void finalizeAvatarPick(void* data);
	static void callbackHighlightTransferable(S32 option, void* userdata);
	void onClickStartAuction();
	static void confirmSaleChange(S32 landSize, S32 salePrice, std::string authorizedName, void(*callback)(void*), void* userdata);
	static void callbackConfirmSaleChange(S32 option, void* userdata);
	BOOL postBuild() override;
protected:
	LLLineEditor*	mEditName;
	LLTextEditor*	mEditDesc;
	LLTextBox*		mTextSalePending;
 	LLButton*		mBtnDeedToGroup;
 	LLButton*		mBtnSetGroup;
	LLTextBox*		mTextOwner;
	LLTextBox*		mContentRating;
	LLTextBox*		mLandType;
	LLTextBox*		mTextGroup;
	LLTextBox*		mTextClaimDate;
	LLTextBox*		mTextPriceLabel;
	LLTextBox*		mTextPrice;
	LLCheckBoxCtrl* mCheckDeedToGroup;
	LLCheckBoxCtrl* mCheckContributeWithDeed;
	LLTextBox*		mSaleInfoForSale1;
	LLTextBox*		mSaleInfoForSale2;
	LLTextBox*		mSaleInfoForSaleObjects;
	LLTextBox*		mSaleInfoForSaleNoObjects;
	LLTextBox*		mSaleInfoNotForSale;
	LLButton*		mBtnSellLand;
	LLButton*		mBtnStopSellLand;
	LLTextBox*		mTextDwell;
	LLButton*		mBtnBuyLand;
	LLButton*		mBtnScriptLimits;
	LLButton*		mBtnBuyGroupLand;
	LLButton*		mBtnReleaseLand;
	LLButton*		mBtnReclaimLand;
	LLButton*		mBtnBuyPass;
	LLButton* mBtnStartAuction;
	LLSafeHandle<LLParcelSelection>&	mParcel;
	static LLPointer<LLParcelSelection>	sSelectionForBuyPass;
	static LLHandle<LLFloater> sBuyPassDialogHandle;
};
class LLPanelLandObjects final
:	public LLPanel
{
public:
	LLPanelLandObjects(LLSafeHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandObjects();
	void refresh() override;
	bool callbackReturnOwnerObjects(const LLSD& notification, const LLSD& response);
	bool callbackReturnGroupObjects(const LLSD& notification, const LLSD& response);
	bool callbackReturnOtherObjects(const LLSD& notification, const LLSD& response);
	bool callbackReturnOwnerList(const LLSD& notification, const LLSD& response);
	static void clickShowCore(LLPanelLandObjects* panelp, S32 return_type, uuid_list_t* list = nullptr);
	void onClickShowOwnerObjects();
	void onClickShowGroupObjects();
	void onClickShowOtherObjects();
	void onClickReturnOwnerObjects();
	void onClickReturnGroupObjects();
	void onClickReturnOtherObjects();
	void onClickReturnOwnerList();
	void onClickRefresh();
	void onCommitList();
	static void onLostFocus(LLFocusableElement* caller, void* user_data);
		   void onCommitClean();
	static void processParcelObjectOwnersReply(LLMessageSystem *msg, void **);
	BOOL postBuild() override;
protected:
	LLTextBox		*mParcelObjectBonus;
	LLTextBox		*mSWTotalObjects;
	LLTextBox		*mObjectContribution;
	LLTextBox		*mTotalObjects;
	LLTextBox		*mOwnerObjects;
	LLButton		*mBtnShowOwnerObjects;
	LLButton		*mBtnReturnOwnerObjects;
	LLTextBox		*mGroupObjects;
	LLButton		*mBtnShowGroupObjects;
	LLButton		*mBtnReturnGroupObjects;
	LLTextBox		*mOtherObjects;
	LLButton		*mBtnShowOtherObjects;
	LLButton		*mBtnReturnOtherObjects;
	LLTextBox		*mSelectedObjects;
	LLLineEditor	*mCleanOtherObjectsTime;
	S32				mOtherTime;
	LLButton		*mBtnRefresh;
	LLButton		*mBtnReturnOwnerList;
	LLNameListCtrl	*mOwnerList;
	LLPointer<LLUIImage>	mIconAvatarOnline;
	LLPointer<LLUIImage>	mIconAvatarInSim;
	LLPointer<LLUIImage>	mIconAvatarOffline;
	LLPointer<LLUIImage>	mIconGroup;
	BOOL			mFirstReply;
	uuid_list_t		mSelectedOwners;
	std::string		mSelectedName;
	S32				mSelectedCount;
	BOOL			mSelectedIsGroup;
	LLSafeHandle<LLParcelSelection>&	mParcel;
};
class LLPanelLandOptions final
:	public LLPanel
{
public:
	LLPanelLandOptions(LLSafeHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandOptions();
	BOOL postBuild() override;
	void draw() override;
	void refresh() override;
private:
	void refreshSearch();
	void onCommitAny();
	void onClickSet();
	void onClickClear();
	static void onClickPublishHelp(void*);
private:
	LLCheckBoxCtrl*	mCheckEditObjects;
	LLCheckBoxCtrl*	mCheckEditGroupObjects;
	LLCheckBoxCtrl*	mCheckAllObjectEntry;
	LLCheckBoxCtrl*	mCheckGroupObjectEntry;
	LLCheckBoxCtrl*	mCheckEditLand;
	LLCheckBoxCtrl*	mCheckSafe;
	LLCheckBoxCtrl*	mCheckFly;
	LLCheckBoxCtrl*	mCheckGroupScripts;
	LLCheckBoxCtrl*	mCheckOtherScripts;
	LLCheckBoxCtrl*	mCheckLandmark;
	LLCheckBoxCtrl*	mCheckShowDirectory;
	LLComboBox*		mCategoryCombo;
	LLComboBox*		mLandingTypeCombo;
	LLTextureCtrl*	mSnapshotCtrl;
	LLTextBox*		mLocationText;
	LLButton*		mSetBtn;
	LLButton*		mClearBtn;
	LLCheckBoxCtrl		*mMatureCtrl;
	LLCheckBoxCtrl		*mGamingCtrl;
	LLCheckBoxCtrl		*mPushRestrictionCtrl;
	LLCheckBoxCtrl		*mSeeAvatarsCtrl;
	LLButton			*mPublishHelpButton;
	LLSafeHandle<LLParcelSelection>&	mParcel;
};
class LLPanelLandAccess final
:	public LLPanel
{
public:
	LLPanelLandAccess(LLSafeHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandAccess();
	void refresh() override;
	void refresh_ui();
	void refreshNames();
	void draw() override;
	void onCommitPublicAccess(LLUICtrl* ctrl);
	void onCommitAny();
	void onCommitGroupCheck(LLUICtrl* ctrl);
	void onClickRemoveAccess();
	void onClickRemoveBanned();
	BOOL postBuild() override;
	void onClickAddAccess();
	void onClickAddBanned();
	void callbackAvatarCBBanned(const uuid_vec_t& ids);
	void callbackAvatarCBBanned2(const uuid_vec_t& ids, S32 duration);
	void callbackAvatarCBAccess(const uuid_vec_t& ids);
protected:
	LLNameListCtrl*		mListAccess;
	LLNameListCtrl*		mListBanned;
	LLSafeHandle<LLParcelSelection>&	mParcel;
};
class LLPanelLandCovenant final
:	public LLPanel
{
public:
	LLPanelLandCovenant(LLSafeHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandCovenant();
	BOOL postBuild() override;
	void refresh() override;
	static void updateCovenantText(const std::string& string);
	static void updateEstateName(const std::string& name);
	static void updateLastModified(const std::string& text);
	static void updateEstateOwnerID(const LLUUID& id);
protected:
	LLSafeHandle<LLParcelSelection>&	mParcel;
private:
	LLUUID mLastRegionID;
	F64 mNextUpdateTime;
};
#endif
