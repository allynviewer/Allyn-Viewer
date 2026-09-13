/** 
 * @file llpanelavatar.h
 * @brief LLPanelAvatar and related class definitions
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LL_LLPANELAVATAR_H
#define LL_LLPANELAVATAR_H
#include "llpanel.h"
#include "v3dmath.h"
#include "lluuid.h"
#include "llmediactrl.h"
#include "llavatarpropertiesprocessor.h"
#include "llmutelist.h"
class LLAvatarName;
class LLCheckBoxCtrl;
class LLLineEditor;
class LLPanelAvatar;
class LLTabContainer;
class LLViewerObject;
class LLMediaCtrl;
class LLPanelPick;
enum EOnlineStatus
{
	ONLINE_STATUS_NO      = 0,
	ONLINE_STATUS_YES     = 1
};
class LLPanelAvatarTab : public LLPanel, public LLAvatarPropertiesObserver
{
public:
	LLPanelAvatarTab(const std::string& name, const LLRect &rect,
		LLPanelAvatar* panel_avatar);
	virtual ~LLPanelAvatarTab();
	void draw();
	LLPanelAvatar* getPanelAvatar() const { return mPanelAvatar; }
	void setAvatarID(const LLUUID& avatar_id);
	void setDataRequested(bool requested) { mDataRequested = requested; }
	bool isDataRequested() const		  { return mDataRequested; }
private:
	LLPanelAvatar* mPanelAvatar;
	bool mDataRequested;
protected:
	LLUUID mAvatarID;
};
class LLPanelAvatarFirstLife : public LLPanelAvatarTab
{
public:
	LLPanelAvatarFirstLife(const std::string& name, const LLRect &rect, LLPanelAvatar* panel_avatar);
	BOOL postBuild();
	void processProperties(void* data, EAvatarProcessorType type);
	void enableControls(BOOL own_avatar);
};
class LLPanelAvatarSecondLife
: public LLPanelAvatarTab
, public LLMuteListObserver
{
public:
	LLPanelAvatarSecondLife(const std::string& name, const LLRect &rect, LLPanelAvatar* panel_avatar );
	~LLPanelAvatarSecondLife();
	BOOL postBuild();
	void refresh();
	void processProperties(void* data, EAvatarProcessorType type);
	void onChange() {}
	void onChangeDetailed(const LLMute& mute);
	void onClickFriends();
	void onDoubleClickGroup();
	static bool onClickPartnerHelpLoadURL(const LLSD& notification, const LLSD& response);
	void clearControls();
	void enableControls(BOOL own_avatar);
	void updateOnlineText(BOOL online, BOOL have_calling_card);
private:
	LLUUID				mPartnerID;
};
class LLPanelAvatarWeb :
	public LLPanelAvatarTab
	, public LLViewerMediaObserver
{
public:
	LLPanelAvatarWeb(const std::string& name, const LLRect& rect, LLPanelAvatar* panel_avatar);
	~LLPanelAvatarWeb();
	BOOL postBuild();
	void refresh();
	void processProperties(void* data, EAvatarProcessorType type);
	void enableControls(BOOL own_avatar);
	void setWebURL(std::string url);
	void load(const std::string& url);
	void onCommitLoad(const LLSD& value);
	void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);
private:
	std::string			mHome;
	std::string         mNavigateTo;
	LLMediaCtrl*		mWebBrowser;
};
class LLPanelAvatarAdvanced : public LLPanelAvatarTab
{
public:
	LLPanelAvatarAdvanced(const std::string& name, const LLRect& rect, LLPanelAvatar* panel_avatar);
	BOOL postBuild();
	void processProperties(void* data, EAvatarProcessorType type);
	void enableControls(BOOL own_avatar);
	void setWantSkills(U32 want_to_mask, const std::string& want_to_text,
					   U32 skills_mask, const std::string& skills_text,
					   const std::string& languages_text);
	void getWantSkills(U32* want_to_mask, std::string& want_to_text,
					   U32* skills_mask, std::string& skills_text,
					   std::string& languages_text);
private:
	S32					mWantToCount;
	S32					mSkillsCount;
	LLCheckBoxCtrl		*mWantToCheck[8];
	LLLineEditor		*mWantToEdit;
	LLCheckBoxCtrl		*mSkillsCheck[8];
	LLLineEditor		*mSkillsEdit;
};
class LLPanelAvatarNotes : public LLPanelAvatarTab
{
public:
	LLPanelAvatarNotes(const std::string& name, const LLRect& rect, LLPanelAvatar* panel_avatar);
	BOOL postBuild();
	void refresh();
	void processProperties(void* data, EAvatarProcessorType type){}
	void clearControls();
};
class LLPanelAvatarClassified : public LLPanelAvatarTab
{
public:
	LLPanelAvatarClassified(const std::string& name, const LLRect& rect, LLPanelAvatar* panel_avatar);
	BOOL postBuild();
	void refresh();
	void processProperties(void* data, EAvatarProcessorType type);
	BOOL canClose();
	void apply();
	BOOL titleIsValid();
	void deleteClassifiedPanels();
private:
	void onClickNew();
	void onClickDelete();
	bool callbackDelete(const LLSD& notification, const LLSD& response);
	bool callbackNew(const LLSD& notification, const LLSD& response);
	bool mInDirectory;
};
class LLPanelAvatarPicks : public LLPanelAvatarTab
{
public:
	LLPanelAvatarPicks(const std::string& name, const LLRect& rect, LLPanelAvatar* panel_avatar);
	BOOL	postBuild(void);
	void refresh();
	void processProperties(void* data, EAvatarProcessorType type);
	void deletePickPanels();
private:
	void onClickNew();
	void onClickDelete();
	void onClickImport();
	static void onClickImport_continued(void* self, bool importt);
	void onClickExport();
	bool callbackDelete(const LLSD& notification, const LLSD& response);
	LLPanelPick* mPanelPick;
};
class LLPanelAvatar : public LLPanel, public LLAvatarPropertiesObserver
{
public:
	LLPanelAvatar(const std::string& name, const LLRect &rect, BOOL allow_edit);
	~LLPanelAvatar();
	BOOL	postBuild(void);
	void	processProperties(void* data, EAvatarProcessorType type);
	BOOL canClose();
	void setAvatar(LLViewerObject *avatarp);
	void setAvatarID(const LLUUID &avatar_id);
	void setOnlineStatus(EOnlineStatus online_status);
	const LLUUID& getAvatarID() const { return mAvatarID; }
	void resetGroupList();
	void sendAvatarStatisticsRequest();
	void sendAvatarPropertiesRequest();
	void sendAvatarPropertiesUpdate();
	void sendAvatarNotesRequest();
	void sendAvatarNotesUpdate();
	void sendAvatarPicksRequest();
	void selectTab(S32 tabnum);
	void selectTabByName(std::string tab_name);
	bool haveData() const { return mHaveProperties && mHaveStatistics; }
	bool isEditable() const { return mAllowEdit; }
	void onClickCopy(const LLSD& val);
	void onClickOK();
	void onClickCancel();
private:
	void enableOKIfReady();
	static	void*	createPanelAvatar(void*	data);
	static	void*	createFloaterAvatarInfo(void*	data);
	static	void*	createPanelAvatarSecondLife(void*	data);
	static	void*	createPanelAvatarWeb(void*	data);
	static	void*	createPanelAvatarInterests(void*	data);
	static	void*	createPanelAvatarPicks(void*	data);
	static	void*	createPanelAvatarClassified(void*	data);
	static	void*	createPanelAvatarFirstLife(void*	data);
	static	void*	createPanelAvatarNotes(void*	data);
public:
	LLPanelAvatarSecondLife*	mPanelSecondLife;
	LLPanelAvatarAdvanced*		mPanelAdvanced;
	LLPanelAvatarClassified*	mPanelClassified;
	LLPanelAvatarPicks*			mPanelPicks;
	LLPanelAvatarNotes*			mPanelNotes;
	LLPanelAvatarFirstLife*		mPanelFirstLife;
	LLPanelAvatarWeb*			mPanelWeb;
	std::list<LLPanelAvatarTab*> mAvatarPanelList;
	static BOOL sAllowFirstLife;
private:
	LLUUID						mAvatarID;
	bool						mIsFriend;
	bool						mHaveProperties;
	bool						mHaveStatistics;
	bool						mHaveNotes;
	std::string					mLastNotes;
	LLTabContainer*		mTab;
	bool						mAllowEdit;
	boost::signals2::connection mCacheConnection;
	typedef std::list<LLPanelAvatar*> panel_list_t;
	static panel_list_t sAllPanels;
};
#endif
